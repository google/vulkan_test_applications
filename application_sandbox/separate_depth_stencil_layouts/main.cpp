// Copyright 2020 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "mathfu/matrix.h"
#include "mathfu/vector.h"
#include "support/entry/entry.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

using Mat44 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;

namespace cube_model {
#include "cube.obj.h"
}
const auto& cube_data = cube_model::model;

uint32_t cube_vertex_shader[] =
#include "cube.vert.spv"
    ;

uint32_t cube_fragment_shader[] =
#include "cube.frag.spv"
    ;

namespace torus_model {
#include "torus_knot.obj.h"
}
const auto& torus_data = torus_model::model;

uint32_t torus_vertex_shader[] =
#include "torus.vert.spv"
    ;

uint32_t torus_fragment_shader[] =
#include "torus.frag.spv"
    ;

const VkFormat kDepthStencilFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

struct MixedSamplesFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> torus_framebuffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> cube_framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> torus_descriptor_set_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_descriptor_set_;

  // The sample application assumes the depth format to  VK_FORMAT_D16_UNORM.
  // As we need to use stencil aspect, we declare another depth_stencil image
  // and its view here.
  vulkan::ImagePointer depth_stencil_image_;
  containers::unique_ptr<vulkan::VkImageView> depth_stencil_image_view_;
  containers::unique_ptr<vulkan::VkImageView> depth_image_view_;
  containers::unique_ptr<vulkan::VkImageView> stencil_image_view_;
};

VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR
    separate_depth_stencil_layout_features{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES_KHR,
        nullptr, true};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class MixedSamplesSample
    : public sample_application::Sample<MixedSamplesFrameData> {
 public:
  MixedSamplesSample(const entry::EntryData* data)
      : data_(data),
        Sample<MixedSamplesFrameData>(
            data->allocator(), data, 1, 512, 1, 1,
            sample_application::SampleOptions()
                .EnableDepthBuffer()
                .AddDeviceExtensionStructure(
                    &separate_depth_stencil_layout_features),
            {0}, {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
            {VK_KHR_MULTIVIEW_EXTENSION_NAME,
             VK_KHR_MAINTENANCE2_EXTENSION_NAME,
             VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
             VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME}),
        cube_(data->allocator(), data->logger(), cube_data),
        torus_(data->allocator(), data->logger(), torus_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    cube_.InitializeData(app(), initialization_buffer);
    torus_.InitializeData(app(), initialization_buffer);

    // Initialization for cube and torus rendering. Cube and torus share the
    // same transformation matrix, so they share the same descriptor sets for
    // vertex shader and pipeline layout. However, the fragment shaders are
    // different, so two different pipelines are required.
    torus_descriptor_set_layout_bindings_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    torus_descriptor_set_layout_bindings_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };

    torus_pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(), app()->CreatePipelineLayout(
                                {{torus_descriptor_set_layout_bindings_[0],
                                  torus_descriptor_set_layout_bindings_[1]}}));

    cube_descriptor_set_layout_bindings_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    cube_descriptor_set_layout_bindings_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    cube_descriptor_set_layout_bindings_[2] = {
        2,														    // binding
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,					    // descriptorType
        1,															// descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,  // stage
        nullptr														// pImmutableSamplers
    };

    cube_pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(), app()->CreatePipelineLayout(
                                {{cube_descriptor_set_layout_bindings_[0],
                                  cube_descriptor_set_layout_bindings_[1],
                                  cube_descriptor_set_layout_bindings_[2]}}));

    VkAttachmentReference2KHR depth_stencil_attachment = {
        VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR,            // sType
        nullptr,                                                 // pNext
        0,                                                       // attachment
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,        // layout
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT  // aspectMask
    };
    VkAttachmentReference2KHR color_attachment = {
        VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR,  // sType
        nullptr,                                       // pNext
        0,                                             // attachment
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,      // layout
        VK_IMAGE_ASPECT_COLOR_BIT                      // aspectMask
    };
    VkAttachmentReference2KHR stencil_attachment = {
        VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR,      // sType
        nullptr,                                           // pNext
        1,                                                 // attachment
        VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR,    // layout
        VK_IMAGE_ASPECT_STENCIL_BIT                        // aspectMask
    };
    VkAttachmentReference2KHR depth_read_attachment = {
        VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR,  // sType
        nullptr,                                       // pNext
        2,                                             // attachment
        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR,   // layout
        VK_IMAGE_ASPECT_DEPTH_BIT                      // aspectMask
    };

	VkAttachmentDescription2KHR depth_stencil_attachment_description{
        VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR,    // sType
        nullptr,                                           // pNext
        0,                                                 // flags
        kDepthStencilFormat,                               // format
        num_depth_stencil_samples(),                       // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,                       // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,                      // storeOp
        VK_ATTACHMENT_LOAD_OP_CLEAR,                       // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_STORE,                      // stencilStoreOp
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,  // initialLayout
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL   // finalLayout
    };
    VkAttachmentDescription2KHR cube_color_attachment_description{
        VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR,  // sType
        nullptr,                                         // pNext
        0,                                               // flags`
        render_format(),                                 // format
        num_color_samples(),                             // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,                     // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,                    // storeOp
        VK_ATTACHMENT_LOAD_OP_LOAD,						 // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,                // stencilStoreOp
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,        // initialLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL         // finalLayout
    };
    VkAttachmentDescription2KHR stencil_attachment_description{
        VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR,   // sType
        nullptr,                                          // pNext
        0,                                                // flags
        kDepthStencilFormat,                              // format
        num_depth_stencil_samples(),                      // samples
        VK_ATTACHMENT_LOAD_OP_LOAD,                       // loadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,                 // storeOp
        VK_ATTACHMENT_LOAD_OP_LOAD,                       // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_STORE,	                  // stencilStoreOp
        VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR,   // initialLayout
        VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR    // finalLayout
    };
    VkAttachmentDescriptionStencilLayoutKHR stencil_layout{
        VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT_KHR,  // sType
        nullptr,                                                      // pNext
        VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR,				  // stencilInitialLayout
        VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR				  // stencilFinalLayout
    };
    VkAttachmentDescription2KHR depth_read_attachment_description{
        VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR,  // sType
        &stencil_layout,                                 // pNext
        0,                                               // flags
        kDepthStencilFormat,                             // format
        num_depth_stencil_samples(),                     // samples
        VK_ATTACHMENT_LOAD_OP_LOAD,                      // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,                    // storeOp
        VK_ATTACHMENT_LOAD_OP_LOAD,                      // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,                // stencilStoreOp
        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR,     // initialLayout
        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR      // finalLayout
    };

    torus_render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass2(
            {depth_stencil_attachment_description},			  // AttachmentDescriptions
            {{
                VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR,  // sType
                nullptr,                                      // pNext
                0,                                            // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,			  // pipelineBindPoint
                0,											  // viewMask
                0,											  // inputAttachmentCount
                nullptr,									  // pInputAttachments
                0,											  // colorAttachmentCount
                nullptr,									  // colorAttachment
                nullptr,									  // pResolveAttachments
                &depth_stencil_attachment,					  // pDepthStencilAttachment
                0,                                			  // preserveAttachmentCount
                nullptr                         			  // pPreserveAttachments
            }},                                 			  // SubpassDescriptions
            {}                                  			  // SubpassDependencies
            ));

    cube_render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass2(
            {cube_color_attachment_description, stencil_attachment_description,
             depth_read_attachment_description},			  // AttachmentDescriptions
            {{
                VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR,  // sType
                nullptr,                                      // pNext
                0,                                            // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,			  // pipelineBindPoint
                0,											  // viewMask
                1,											  // inputAttachmentCount
                &depth_read_attachment,						  // pInputAttachments
                1,											  // colorAttachmentCount
                &color_attachment,							  // colorAttachment
                nullptr,									  // pResolveAttachments
                &stencil_attachment,						  // pDepthStencilAttachment
                0,											  // preserveAttachmentCount
                nullptr										  // pPreserveAttachments
            }},												  // SubpassDescriptions
            {}												  // SubpassDependencies
            ));

    // Initialize torus shaders
    torus_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(),
        app()->CreateGraphicsPipeline(torus_pipeline_layout_.get(),
                                      torus_render_pass_.get(), 0));
    torus_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                               torus_vertex_shader);
    torus_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                               torus_fragment_shader);
    torus_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    torus_pipeline_->SetInputStreams(&torus_);
    torus_pipeline_->SetViewport(viewport());
    torus_pipeline_->SetScissor(scissor());
    torus_pipeline_->SetSamples(num_samples());
    torus_pipeline_->AddAttachment();
    // Need to enable the stencil buffer to be written. The reference and
    // write mask will be set later dynamically, the actual value write to
    // stencil buffer will be 'reference & write mask'.
    torus_pipeline_->AddDynamicState(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    torus_pipeline_->AddDynamicState(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
    torus_pipeline_->DepthStencilState().stencilTestEnable = VK_TRUE;
    torus_pipeline_->DepthStencilState().front.compareOp = VK_COMPARE_OP_ALWAYS;
    torus_pipeline_->DepthStencilState().front.passOp = VK_STENCIL_OP_REPLACE;
    torus_pipeline_->Commit();

    // Initialize cube shaders
    cube_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(),
        app()->CreateGraphicsPipeline(cube_pipeline_layout_.get(),
                                      cube_render_pass_.get(), 0));
    cube_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                              cube_vertex_shader);
    cube_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                              cube_fragment_shader);
    cube_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    cube_pipeline_->SetInputStreams(&cube_);
    cube_pipeline_->SetViewport(viewport());
    cube_pipeline_->SetScissor(scissor());
    cube_pipeline_->SetSamples(num_samples());
    cube_pipeline_->AddAttachment();
    // Need to test the stencil buffer. The reference and compare mask are to
    // be set later dynamically. The real value used to compare to the stencil
    // buffer is 'reference & compare mask'.
    cube_pipeline_->AddDynamicState(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    cube_pipeline_->AddDynamicState(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
    cube_pipeline_->DepthStencilState().stencilTestEnable = VK_TRUE;
    cube_pipeline_->DepthStencilState().front.compareOp = VK_COMPARE_OP_EQUAL;
    // Disable depth test, since we don't really care.
    cube_pipeline_->DepthStencilState().depthTestEnable = VK_FALSE;
    cube_pipeline_->DepthStencilState().depthWriteEnable = VK_FALSE;
    cube_pipeline_->Commit();

    // Transformation data for viewing and cube/torus rotation.
    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data_ = containers::make_unique<vulkan::BufferFrameData<ModelData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    camera_data_->data().projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data_->data().transform =
        Mat44::FromTranslationVector(
            mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f}) *
        Mat44::FromRotationMatrix(Mat44::RotationX(3.14f * 0.2f));

    model_data_->data().transform =
        model_data_->data().transform *
        Mat44::FromRotationMatrix(Mat44::RotationY(3.14f * 0.3f));
  }

  virtual void InitializeFrameData(
      MixedSamplesFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    // Initalize the depth stencil image and the image view.
    VkImageCreateInfo depth_stencil_image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,      // sType
        nullptr,								  // pNext
        0,										  // flags
        VK_IMAGE_TYPE_2D,						  // imageType
        kDepthStencilFormat,					  // format
        {
            app()->swapchain().width(),
            app()->swapchain().height(),
            app()->swapchain().depth(),
        },										  // extent
        1,										  // mipLevels
        1,										  // arrayLayers
        num_depth_stencil_samples(),			  // samples
        VK_IMAGE_TILING_OPTIMAL,				  // tiling
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,                // sharingMode
        0,                                        // queueFamilyIndexCount
        nullptr,                                  // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
    };
    frame_data->depth_stencil_image_ =
        app()->CreateAndBindImage(&depth_stencil_image_create_info);
    frame_data->depth_stencil_image_view_ = app()->CreateImageView(
        frame_data->depth_stencil_image_.get(), VK_IMAGE_VIEW_TYPE_2D,
        {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1});
    frame_data->depth_image_view_ = app()->CreateImageView(
        frame_data->depth_stencil_image_.get(), VK_IMAGE_VIEW_TYPE_2D,
        {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});
    frame_data->stencil_image_view_ = app()->CreateImageView(
        frame_data->depth_stencil_image_.get(), VK_IMAGE_VIEW_TYPE_2D,
        {VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1});

    // Initialize the torus descriptor sets
    frame_data->torus_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet(
                {torus_descriptor_set_layout_bindings_[0],
                 torus_descriptor_set_layout_bindings_[1]}));

    VkDescriptorBufferInfo buffer_infos[2] = {
        {
            camera_data_->get_buffer(),                       // buffer
            camera_data_->get_offset_for_frame(frame_index),  // offset
            camera_data_->size(),                             // range
        },
        {
            model_data_->get_buffer(),                       // buffer
            model_data_->get_offset_for_frame(frame_index),  // offset
            model_data_->size(),                             // range
        }};

    VkWriteDescriptorSet torus_write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        *frame_data->torus_descriptor_set_,      // dstSet
        0,                                       // dstbinding
        0,                                       // dstArrayElement
        2,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
        nullptr,                                 // pImageInfo
        buffer_infos,                            // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };

    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &torus_write, 0,
                                            nullptr);

    // Initialize the cube descriptor sets
    frame_data->cube_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(), app()->AllocateDescriptorSet(
                                    {cube_descriptor_set_layout_bindings_[0],
                                     cube_descriptor_set_layout_bindings_[1],
                                     cube_descriptor_set_layout_bindings_[2]}));

    VkDescriptorBufferInfo cube_buffer_infos[2] = {
        {
            camera_data_->get_buffer(),                       // buffer
            camera_data_->get_offset_for_frame(frame_index),  // offset
            camera_data_->size(),                             // range
        },
        {
            model_data_->get_buffer(),                       // buffer
            model_data_->get_offset_for_frame(frame_index),  // offset
            model_data_->size(),                             // range
        }};

    VkWriteDescriptorSet cube_write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        *frame_data->cube_descriptor_set_,       // dstSet
        0,                                       // dstbinding
        0,                                       // dstArrayElement
        2,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
        nullptr,                                 // pImageInfo
        cube_buffer_infos,                       // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };
    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &cube_write, 0,
                                            nullptr);

    VkDescriptorImageInfo image_info = {
        VK_NULL_HANDLE,                              // sampler
        *frame_data->depth_image_view_,              // imageView
        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR  // imageLayout
    };
    cube_write.pBufferInfo = nullptr;
    cube_write.descriptorCount = 1;
    cube_write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    cube_write.pImageInfo = &image_info;
    cube_write.dstBinding = 2;
    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &cube_write, 0,
                                            nullptr);

    ::VkImageView torus_raw_views[3] = {*frame_data->depth_stencil_image_view_};

    // Create framebuffers for each renderpass. Need two separate frame buffers
    // due to different attachment usage in the renderpasses.
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *torus_render_pass_,                        // renderPass
        1,                                          // attachmentCount
        torus_raw_views,                            // pAttachments
        app()->swapchain().width(),                 // width
        app()->swapchain().height(),                // height
        1                                           // layers
    };

    ::VkFramebuffer torus_raw_framebuffer;
    app()->device()->vkCreateFramebuffer(app()->device(),
                                         &framebuffer_create_info, nullptr,
                                         &torus_raw_framebuffer);
    frame_data->torus_framebuffer_ =
        containers::make_unique<vulkan::VkFramebuffer>(
            data_->allocator(),
            vulkan::VkFramebuffer(torus_raw_framebuffer, nullptr,
                                  &app()->device()));

    ::VkImageView cube_raw_views[3] = {color_view(frame_data),
                                       *frame_data->stencil_image_view_,
                                       *frame_data->depth_image_view_};

    framebuffer_create_info.renderPass = *cube_render_pass_;
    framebuffer_create_info.attachmentCount = 3;
    framebuffer_create_info.pAttachments = cube_raw_views;

    ::VkFramebuffer cube_raw_framebuffer;
    app()->device()->vkCreateFramebuffer(app()->device(),
                                         &framebuffer_create_info, nullptr,
                                         &cube_raw_framebuffer);
    frame_data->cube_framebuffer_ =
        containers::make_unique<vulkan::VkFramebuffer>(
            data_->allocator(),
            vulkan::VkFramebuffer(cube_raw_framebuffer, nullptr,
                                  &app()->device()));

    // Populate the render command buffer
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    (*frame_data->command_buffer_)
        ->vkBeginCommandBuffer((*frame_data->command_buffer_),
                               &sample_application::kBeginCommandBuffer);
    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);

    VkClearValue clears[3];
    clears[0].color = {1.0f, 1.0f, 1.0f, 1.0f};
    clears[1].depthStencil = {1.0f, 0};
    clears[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *torus_render_pass_,                       // renderPass
        *frame_data->torus_framebuffer_,           // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},		   // renderArea
        2,										   // clearValueCount
        clears									   // clears
    };

    // Barrier before write to depth/stencil image
    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// sType
        nullptr,											// pNext
        0,													// srcAccessMask
        0,													// dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,							// oldLayout
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,	// newLayout
        VK_QUEUE_FAMILY_IGNORED,							// srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,							// dstQueueFamilyIndex
        *frame_data->depth_stencil_image_,					// image
        {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
		 0, 1, 0, 1}};										// subresourceRange

    cmdBuffer->vkCmdPipelineBarrier(cmdBuffer,
                                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                    nullptr, 0, nullptr, 1, &barrier);

    // Draw the torus
    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*torus_pipeline_layout_), 0, 1,
        &frame_data->torus_descriptor_set_->raw_set(), 0, nullptr);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *torus_pipeline_);
    cmdBuffer->vkCmdSetStencilReference(cmdBuffer, VK_STENCIL_FACE_FRONT_BIT,
                                        0xAB);
    cmdBuffer->vkCmdSetStencilWriteMask(cmdBuffer, VK_STENCIL_FACE_FRONT_BIT,
                                        0x0F);
    torus_.Draw(&cmdBuffer);

    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    // Barriers before read from depth/stencil image
    barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
    cmdBuffer->vkCmdPipelineBarrier(cmdBuffer,
                                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
									VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                    nullptr, 0, nullptr, 1, &barrier);

	barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1};
    cmdBuffer->vkCmdPipelineBarrier(cmdBuffer,
                                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                    nullptr, 0, nullptr, 1, &barrier);

    // Draw the cube
    pass_begin.renderPass = *cube_render_pass_;
    pass_begin.framebuffer = *frame_data->cube_framebuffer_;
    pass_begin.clearValueCount = 3;
    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*cube_pipeline_layout_), 0, 1,
        &frame_data->cube_descriptor_set_->raw_set(), 0, nullptr);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *cube_pipeline_);
    cmdBuffer->vkCmdSetStencilReference(cmdBuffer, VK_STENCIL_FACE_FRONT_BIT,
                                        0xFF);
    cmdBuffer->vkCmdSetStencilCompareMask(cmdBuffer, VK_STENCIL_FACE_FRONT_BIT,
                                          0x0B);
    cube_.Draw(&cmdBuffer);

    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    (*frame_data->command_buffer_)
        ->vkEndCommandBuffer(*frame_data->command_buffer_);
  }

  virtual void Update(float time_since_last_render) override {
    model_data_->data().transform = model_data_->data().transform *
                                    Mat44::FromRotationMatrix(Mat44::RotationY(
                                        3.14f * time_since_last_render * 0.5f));
  }
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      MixedSamplesFrameData* frame_data) override {
    // Update our uniform buffers.
    camera_data_->UpdateBuffer(queue, frame_index);
    model_data_->UpdateBuffer(queue, frame_index);

    VkSubmitInfo init_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,						   // sType
        nullptr,											   // pNext
        0,													   // waitSemaphoreCount
        nullptr,											   // pWaitSemaphores
        nullptr,											   // pWaitDstStageMask,
        1,													   // commandBufferCount
        &(frame_data->command_buffer_->get_command_buffer()),  // pCommandBuffers
        0,													   // signalSemaphoreCount
        nullptr												   // pSignalSemaphores
    };

    app()->render_queue()->vkQueueSubmit(app()->render_queue(), 1,
                                         &init_submit_info,
                                         static_cast<VkFence>(VK_NULL_HANDLE));
  }

 private:
  struct CameraData {
    Mat44 projection_matrix;
  };

  struct ModelData {
    Mat44 transform;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> torus_pipeline_layout_;
  containers::unique_ptr<vulkan::PipelineLayout> cube_pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> torus_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> cube_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> torus_render_pass_;
  containers::unique_ptr<vulkan::VkRenderPass> cube_render_pass_;
  VkDescriptorSetLayoutBinding torus_descriptor_set_layout_bindings_[2];
  VkDescriptorSetLayoutBinding cube_descriptor_set_layout_bindings_[3];
  vulkan::VulkanModel torus_;
  vulkan::VulkanModel cube_;

  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_data_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  MixedSamplesSample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
