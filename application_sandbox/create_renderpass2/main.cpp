// Copyright 2017 Google Inc.
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

namespace plane_model {
#include "fullscreen_quad.obj.h"
}
const auto& plane_data = plane_model::model;

uint32_t final_fragment_shader[] =
#include "final.frag.spv"
    ;

uint32_t passthrough_vertex_shader[] =
#include "passthrough.vert.spv"
    ;

const VkFormat kDepthStencilFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

const uint32_t kMultiviewCount = 2;

static VkPhysicalDeviceMultiviewFeatures kMultiviewFeatures{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES,
    nullptr,  // pNext
    true,     // multiview
    true,     // multiviewGeometryShader
    true      // multiviewTessellationShader
};

struct MixedSamplesFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> multiview_framebuffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> presentation_framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_descriptor_set_;
  containers::unique_ptr<vulkan::DescriptorSet> plane_descriptor_set_;

  // The sample application assumes the depth format to  VK_FORMAT_D16_UNORM.
  // As we need to use stencil aspect, we declare another depth_stencil image
  // and its view here.
  vulkan::ImagePointer depth_stencil_image_;
  vulkan::ImagePointer multiview_image_;
  containers::unique_ptr<vulkan::VkImageView> depth_stencil_image_view_;
  containers::unique_ptr<vulkan::VkImageView> multiview_image_view_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class MixedSamplesSample
    : public sample_application::Sample<MixedSamplesFrameData> {
 public:
  MixedSamplesSample(const entry::EntryData* data)
      : data_(data),
        Sample<MixedSamplesFrameData>(
            data->allocator(), data, 1, 512, 1, 1,
            sample_application::SampleOptions().AddDeviceExtensionStructure(
                (void*)&kMultiviewFeatures),
            {0}, {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
            {VK_KHR_MULTIVIEW_EXTENSION_NAME,
             VK_KHR_MAINTENANCE2_EXTENSION_NAME,
             VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME}),
        cube_(data->allocator(), data->logger(), cube_data),
        plane_(data->allocator(), data->logger(), plane_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    cube_.InitializeData(app(), initialization_buffer);
    plane_.InitializeData(app(), initialization_buffer);

    // Initialization for cube and floor rendering. Cube and floor shares the
    // same transformation matrix, so they shares the same descriptor sets for
    // vertex shader and pipeline layout. However, the fragment shaders are
    // different, so two different pipelines are required.
    descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    descriptor_set_layouts_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    descriptor_set_layouts_[2] = {
        2,                             // binding
        VK_DESCRIPTOR_TYPE_SAMPLER,    // descriptorType
        1,                             // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,  // stageFlags
        nullptr                        // pImmutableSamplers
    };
    descriptor_set_layouts_[3] = {
        3,                                 // binding
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  // descriptorType
        1,                                 // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,      // stageFlags
        nullptr                            // pImmutableSamplers
    };

    sampler_ = containers::make_unique<vulkan::VkSampler>(
        data_->allocator(),
        vulkan::CreateSampler(&app()->device(), VK_FILTER_LINEAR,
                              VK_FILTER_LINEAR));

    cube_pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout(
            {{descriptor_set_layouts_[0], descriptor_set_layouts_[1]}}));

    plane_pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout(
            {{descriptor_set_layouts_[2], descriptor_set_layouts_[3]}}));

    VkAttachmentReference2KHR color_attachment = {
        VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR,  // sType
        nullptr,                                       // pNext
        0,                                             // attachment
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,      // layout
        VK_IMAGE_ASPECT_COLOR_BIT                      // aspectMask
    };
    VkAttachmentReference2KHR depth_attachment = {
        VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR,            // sType
        nullptr,                                                 // pNext
        1,                                                       // attachment
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,        // layout
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT  // aspectMask
    };

    VkAttachmentDescription2KHR color_attachment_description{
        VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR,  // sType
        nullptr,                                         // pNext
        0,                                               // flags
        render_format(),                                 // format
        num_color_samples(),                             // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,                     // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,                    // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,                 // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,                // stencilStoreOp
        VK_IMAGE_LAYOUT_UNDEFINED,                       // initialLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL         // finalLayout
    };

    VkAttachmentDescription2KHR depth_stencil_attachment_description{
        VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR,   // sType
        nullptr,                                          // pNext
        0,                                                // flags
        kDepthStencilFormat,                              // format
        num_depth_stencil_samples(),                      // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,                      // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,                     // storeOp
        VK_ATTACHMENT_LOAD_OP_CLEAR,                      // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,                 // stencilStoreOp
        VK_IMAGE_LAYOUT_UNDEFINED,                        // initialLayout
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // finalLayout
    };

    const uint32_t view_mask = 0b00000011;
    const uint32_t correlation_mask = 0b00000011;

    VkViewport multiview_viewport = viewport();
    multiview_viewport.width = multiview_viewport.width / 2;
    VkRect2D multiview_scissor = scissor();
    multiview_scissor.extent.width = multiview_scissor.extent.width / 2;

    multiview_render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass2(
            {color_attachment_description,
             depth_stencil_attachment_description},  // AttachmentDescriptions
            {{
                VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR,  // sType
                nullptr,                                      // pNext
                0,                                            // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                view_mask,                        // viewMask
                0,                                // inputAttachmentCount
                nullptr,                          // pInputAttachments
                1,                                // colorAttachmentCount
                &color_attachment,                // colorAttachment
                nullptr,                          // pResolveAttachments
                &depth_attachment,                // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {},                                   // SubpassDependencies
            1,                                    // correlatedViewMaskCount
            &correlation_mask                     // pCorrelatedViewMasks
            ));

    presentation_render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass2(
            {color_attachment_description},  // AttachmentDescriptions
            {{
                VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR,  // sType
                nullptr,                                      // pNext
                0,                                            // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                0,                                // viewMask
                0,                                // inputAttachmentCount
                nullptr,                          // pInputAttachments
                1,                                // colorAttachmentCount
                &color_attachment,                // colorAttachment
                nullptr,                          // pResolveAttachments
                nullptr,                          // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {},                                   // SubpassDependencies
            0,                                    // correlatedViewMaskCount
            nullptr                               // pCorrelatedViewMasks
            ));

    for (uint32_t i = 0; i < kMultiviewCount; ++i) {
      multiview_viewport.x = multiview_viewport.width * i;
      multiview_scissor.offset.x = multiview_scissor.extent.width * i;

      // Initialize cube shaders
      cube_pipelines_[i] =
          containers::make_unique<vulkan::VulkanGraphicsPipeline>(
              data_->allocator(),
              app()->CreateGraphicsPipeline(cube_pipeline_layout_.get(),
                                            multiview_render_pass_.get(), 0));
      cube_pipelines_[i]->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                    cube_vertex_shader);
      cube_pipelines_[i]->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                    cube_fragment_shader);
      cube_pipelines_[i]->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
      cube_pipelines_[i]->SetInputStreams(&cube_);
      cube_pipelines_[i]->SetViewport(multiview_viewport);
      cube_pipelines_[i]->SetScissor(multiview_scissor);
      cube_pipelines_[i]->SetSamples(num_samples());
      cube_pipelines_[i]->AddAttachment();
      cube_pipelines_[i]->Commit();
    }

    plane_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(),
        app()->CreateGraphicsPipeline(plane_pipeline_layout_.get(),
                                      presentation_render_pass_.get(), 0));
    plane_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                               passthrough_vertex_shader);
    plane_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                               final_fragment_shader);
    plane_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    plane_pipeline_->SetInputStreams(&cube_);
    plane_pipeline_->SetViewport(viewport());
    plane_pipeline_->SetScissor(scissor());
    plane_pipeline_->SetSamples(num_samples());
    plane_pipeline_->AddAttachment();
    plane_pipeline_->Commit();

    // Transformation data for viewing and cube/floor rotation.
    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data_ = containers::make_unique<vulkan::BufferFrameData<ModelData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect = ((float)app()->swapchain().width() / 2.0f) /
                   (float)app()->swapchain().height();
    camera_data_->data().projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data_->data().transform =
        Mat44::FromTranslationVector(
            mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f}) *
        Mat44::FromRotationMatrix(Mat44::RotationX(3.14f * 0.2f));
  }

  virtual void InitializeFrameData(
      MixedSamplesFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    // Initalize the depth stencil image and the image view.
    VkImageCreateInfo depth_stencil_image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
        nullptr,                              // pNext
        0,                                    // flags
        VK_IMAGE_TYPE_2D,                     // imageType
        kDepthStencilFormat,                  // format
        {app()->swapchain().width(), app()->swapchain().height(),
         app()->swapchain().depth()},                 // extent
        1,                                            // mipLevels
        kMultiviewCount,                              // arrayLayers
        num_depth_stencil_samples(),                  // samples
        VK_IMAGE_TILING_OPTIMAL,                      // tiling
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,                    // sharingMode
        0,                                            // queueFamilyIndexCount
        nullptr,                                      // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,                    // initialLayout
    };
    frame_data->depth_stencil_image_ =
        app()->CreateAndBindImage(&depth_stencil_image_create_info);
    frame_data->depth_stencil_image_view_ = app()->CreateImageView(
        frame_data->depth_stencil_image_.get(), VK_IMAGE_VIEW_TYPE_2D,
        {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0,
         kMultiviewCount});

    // Initialize the descriptor sets
    frame_data->cube_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet(
                {descriptor_set_layouts_[0], descriptor_set_layouts_[1]}));

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

    VkWriteDescriptorSet cube_write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        *frame_data->cube_descriptor_set_,       // dstSet
        0,                                       // dstbinding
        0,                                       // dstArrayElement
        2,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
        nullptr,                                 // pImageInfo
        buffer_infos,                            // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };

    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &cube_write, 0,
                                            nullptr);

    VkImageCreateInfo swapchain_image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
        nullptr,                              // pNext
        0,                                    // flags
        VK_IMAGE_TYPE_2D,                     // imageType
        app()->swapchain().format(),          // format
        {app()->swapchain().width(), app()->swapchain().height(),
         app()->swapchain().depth()},  // extent
        1,                             // mipLevels
        kMultiviewCount,               // arrayLayers
        num_color_samples(),           // samples
        VK_IMAGE_TILING_OPTIMAL,       // tiling
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,       // sharingMode
        0,                               // queueFamilyIndexCount
        nullptr,                         // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,       // initialLayout
    };
    frame_data->multiview_image_ =
        app()->CreateAndBindImage(&swapchain_image_create_info);

    frame_data->multiview_image_view_ = app()->CreateImageView(
        frame_data->multiview_image_.get(), VK_IMAGE_VIEW_TYPE_2D,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, kMultiviewCount});

    ::VkImageView multiview_raw_views[2] = {
        *frame_data->multiview_image_view_,
        *frame_data->depth_stencil_image_view_};

    // Create a framebuffer with depth and image attachments
    VkFramebufferCreateInfo multiview_framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *multiview_render_pass_,                    // renderPass
        2,                                          // attachmentCount
        multiview_raw_views,                        // attachments
        app()->swapchain().width(),                 // width
        app()->swapchain().height(),                // height
        kMultiviewCount                             // layers
    };

    ::VkFramebuffer multiview_raw_framebuffer;
    app()->device()->vkCreateFramebuffer(app()->device(),
                                         &multiview_framebuffer_create_info,
                                         nullptr, &multiview_raw_framebuffer);
    frame_data->multiview_framebuffer_ =
        containers::make_unique<vulkan::VkFramebuffer>(
            data_->allocator(),
            vulkan::VkFramebuffer(multiview_raw_framebuffer, nullptr,
                                  &app()->device()));

    frame_data->plane_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet(
                {descriptor_set_layouts_[2], descriptor_set_layouts_[3]}));
    VkDescriptorImageInfo sampler_info = {
        *sampler_,                 // sampler
        VK_NULL_HANDLE,            // imageView
        VK_IMAGE_LAYOUT_UNDEFINED  //  imageLayout
    };
    VkDescriptorImageInfo texture_info = {
        VK_NULL_HANDLE,                            // sampler
        *frame_data->multiview_image_view_.get(),  // imageView
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // imageLayout
    };
    VkWriteDescriptorSet plane_writes[2]{
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->plane_descriptor_set_,      // dstSet
            2,                                       // dstbinding
            0,                                       // dstArrayElement
            1,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_SAMPLER,              // descriptorType
            &sampler_info,                           // pImageInfo
            nullptr,                                 // pBufferInfo
            nullptr,                                 // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->plane_descriptor_set_,      // dstSet
            3,                                       // dstbinding
            0,                                       // dstArrayElement
            1,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,        // descriptorType
            &texture_info,                           // pImageInfo
            nullptr,                                 // pBufferInfo
            nullptr,                                 // pTexelBufferView
        }};

    app()->device()->vkUpdateDescriptorSets(app()->device(), 2, plane_writes, 0,
                                            nullptr);

    ::VkImageView presentation_raw_view = color_view(frame_data);

    // Create a framebuffer with depth and image attachments
    VkFramebufferCreateInfo presentation_framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *presentation_render_pass_,                 // renderPass
        1,                                          // attachmentCount
        &presentation_raw_view,                     // attachments
        app()->swapchain().width(),                 // width
        app()->swapchain().height(),                // height
        1                                           // layers
    };

    ::VkFramebuffer presentation_raw_framebuffer;
    app()->device()->vkCreateFramebuffer(
        app()->device(), &presentation_framebuffer_create_info, nullptr,
        &presentation_raw_framebuffer);

    frame_data->presentation_framebuffer_ =
        containers::make_unique<vulkan::VkFramebuffer>(
            data_->allocator(),
            vulkan::VkFramebuffer(presentation_raw_framebuffer, nullptr,
                                  &app()->device()));

    // Populate the render command buffer
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    (*frame_data->command_buffer_)
        ->vkBeginCommandBuffer((*frame_data->command_buffer_),
                               &sample_application::kBeginCommandBuffer);
    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);

    VkClearValue clears[2];
    clears[0].color = {0.0f, 0.0f, 0.0f, 0.0f};
    clears[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo multiview_pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *multiview_render_pass_,                   // renderPass
        *frame_data->multiview_framebuffer_,       // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        2,                                // clearValueCount
        clears                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &multiview_pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*cube_pipeline_layout_), 0, 1,
        &frame_data->cube_descriptor_set_->raw_set(), 0, nullptr);

    for (uint32_t i = 0; i < kMultiviewCount; ++i) {
      // Draw the cube
      cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   *cube_pipelines_[i]);
      cube_.Draw(&cmdBuffer);
    }

    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    VkImageMemoryBarrier barrier1{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // sType
        nullptr,                                   // pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,      // srcAccessMask
        VK_ACCESS_SHADER_READ_BIT,                 // dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // oldLayout
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // newLayout
        VK_QUEUE_FAMILY_IGNORED,                   // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                   // dstQueueFamilyIndex
        *frame_data->multiview_image_,             // image
        {
            VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask;
            0,                          // baseMipLevel;
            1,                          // levelCount;
            0,                          // baseArrayLayer;
            kMultiviewCount             // layerCount;
        }                               // subresourceRange;
    };

    cmdBuffer->vkCmdPipelineBarrier(
        cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
        &barrier1);

    VkRenderPassBeginInfo presentation_pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *presentation_render_pass_,                // renderPass
        *frame_data->presentation_framebuffer_,    // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        1,                                // clearValueCount
        clears                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &presentation_pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*plane_pipeline_layout_), 0, 1,
        &frame_data->plane_descriptor_set_->raw_set(), 0, nullptr);

    // Draw the plane
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *plane_pipeline_);
    plane_.Draw(&cmdBuffer);

    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    VkImageMemoryBarrier barrier2{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // sType
        nullptr,                                   // pNext
        VK_ACCESS_SHADER_READ_BIT,                 // srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,      // dstAccessMask
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // newLayout
        VK_QUEUE_FAMILY_IGNORED,                   // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                   // dstQueueFamilyIndex
        *frame_data->multiview_image_,             // image
        {
            VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask;
            0,                          // baseMipLevel;
            1,                          // levelCount;
            0,                          // baseArrayLayer;
            kMultiviewCount             // layerCount;
        }                               // subresourceRange;
    };

    cmdBuffer->vkCmdPipelineBarrier(
        cmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
        nullptr, 1, &barrier2);

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
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask,
        1,                              // commandBufferCount
        &(frame_data->command_buffer_->get_command_buffer()),
        0,       // signalSemaphoreCount
        nullptr  // pSignalSemaphores
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
  containers::unique_ptr<vulkan::PipelineLayout> cube_pipeline_layout_;
  containers::unique_ptr<vulkan::PipelineLayout> plane_pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline>
      cube_pipelines_[kMultiviewCount];
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> plane_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> multiview_render_pass_;
  containers::unique_ptr<vulkan::VkRenderPass> presentation_render_pass_;
  VkDescriptorSetLayoutBinding descriptor_set_layouts_[4];
  vulkan::VulkanModel cube_;
  vulkan::VulkanModel plane_;
  containers::unique_ptr<vulkan::VkSampler> sampler_;

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
