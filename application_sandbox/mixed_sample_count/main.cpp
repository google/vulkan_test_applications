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

namespace floor_model {
#include "floor.obj.h"
}
const auto& floor_data = floor_model::model;

uint32_t floor_vertex_shader[] =
#include "floor.vert.spv"
    ;

uint32_t floor_fragment_shader[] =
#include "floor.frag.spv"
    ;

uint32_t mirror_vertex_shader[] =
#include "mirror.vert.spv"
    ;

const VkFormat kDepthStencilFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

struct MixedSamplesFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_descriptor_set_;

  // The sample application assumes the depth format to  VK_FORMAT_D16_UNORM.
  // As we need to use stencil aspect, we declare another depth_stencil image
  // and its view here.
  vulkan::ImagePointer depth_stencil_image_;
  containers::unique_ptr<vulkan::VkImageView> depth_stencil_image_view_;
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
            sample_application::SampleOptions()
                .EnableMultisampling()
                .EnableMixedMultisampling(),
            {0}, {}, {VK_AMD_MIXED_ATTACHMENT_SAMPLES_EXTENSION_NAME}),
        cube_(data->allocator(), data->logger(), cube_data),
        floor_(data->allocator(), data->logger(), floor_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    cube_.InitializeData(app(), initialization_buffer);
    floor_.InitializeData(app(), initialization_buffer);

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

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout(
            {{descriptor_set_layouts_[0], descriptor_set_layouts_[1]}}));

    VkAttachmentReference color_attachment = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_attachment = {
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkAttachmentDescription color_attachment_description{
        0,                                         // flags
        render_format(),                           // format
        num_color_samples(),                       // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,               // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,              // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stenilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stenilStoreOp
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // initialLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL   // finalLayout
    };
    VkAttachmentDescription depth_stencil_attachment_description{
        0,                                                // flags
        kDepthStencilFormat,                              // format
        num_depth_stencil_samples(),                      // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,                      // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,                     // storeOp
        VK_ATTACHMENT_LOAD_OP_CLEAR,                      // stenilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,                 // stenilStoreOp
        VK_IMAGE_LAYOUT_UNDEFINED,                        // initialLayout
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // finalLayout
    };

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass(
            {color_attachment_description,
             depth_stencil_attachment_description},  // AttachmentDescriptions
            {{
                0,                                // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                0,                                // inputAttachmentCount
                nullptr,                          // pInputAttachments
                1,                                // colorAttachmentCount
                &color_attachment,                // colorAttachment
                nullptr,                          // pResolveAttachments
                &depth_attachment,                // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    // Initialize cube shaders
    cube_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));
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
    cube_pipeline_->Commit();

    // Initialize floor shaders
    floor_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));
    floor_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                               floor_vertex_shader);
    floor_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                               floor_fragment_shader);
    floor_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    floor_pipeline_->SetInputStreams(&floor_);
    floor_pipeline_->SetViewport(viewport());
    floor_pipeline_->SetScissor(scissor());
    floor_pipeline_->SetSamples(num_samples());
    floor_pipeline_->AddAttachment();
    // Need to enable the stencil buffer to be written. The reference and
    // write mask will be set later dynamically, the actual value write to
    // stencil buffer will be 'reference & write mask'.
    floor_pipeline_->AddDynamicState(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    floor_pipeline_->AddDynamicState(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
    floor_pipeline_->DepthStencilState().stencilTestEnable = VK_TRUE;
    floor_pipeline_->DepthStencilState().front.compareOp = VK_COMPARE_OP_ALWAYS;
    floor_pipeline_->DepthStencilState().front.passOp = VK_STENCIL_OP_REPLACE;

    floor_pipeline_->Commit();

    // Initialize mirror pipeline
    mirror_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));
    mirror_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                mirror_vertex_shader);
    mirror_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                cube_fragment_shader);
    mirror_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    mirror_pipeline_->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
    mirror_pipeline_->SetInputStreams(&cube_);
    mirror_pipeline_->SetViewport(viewport());
    mirror_pipeline_->SetScissor(scissor());
    mirror_pipeline_->SetSamples(num_samples());
    // Enable color blend
    mirror_pipeline_->AddDynamicState(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
    mirror_pipeline_->AddAttachment(VkPipelineColorBlendAttachmentState{
        VK_TRUE,                         // blendEnable
        VK_BLEND_FACTOR_CONSTANT_COLOR,  // srcColorBlendFactor
        VK_BLEND_FACTOR_ONE,             // dstColorBlendFactor
        VK_BLEND_OP_ADD,                 // colorBlendOp
        VK_BLEND_FACTOR_ONE,             // srcAlphaBlendFactor
        VK_BLEND_FACTOR_ONE,             // dstAlphaBlendFactor
        VK_BLEND_OP_MAX,                 // alphaBlendOp
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        // colorWriteMask
    });
    // Need to test the stencil buffer. The reference and compare mask are to
    // be set later dynamically. The real value used to compare to the stencil
    // buffer is 'reference & compare mask'.
    mirror_pipeline_->AddDynamicState(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    mirror_pipeline_->AddDynamicState(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
    mirror_pipeline_->DepthStencilState().stencilTestEnable = VK_TRUE;
    mirror_pipeline_->DepthStencilState().front.compareOp = VK_COMPARE_OP_EQUAL;
    // Disable depth test, so the reflection can be shown on the floor.
    mirror_pipeline_->DepthStencilState().depthTestEnable = VK_FALSE;

    mirror_pipeline_->Commit();

    // Transformation data for viewing and cube/floor rotation.
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
        {
            app()->swapchain().width(),
            app()->swapchain().height(),
            app()->swapchain().depth(),
        },                                            // extent
        1,                                            // mipLevels
        1,                                            // arrayLayers
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
        {VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1});

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

    VkWriteDescriptorSet write{
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

    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                            nullptr);

    ::VkImageView raw_views[2] = {color_view(frame_data),
                                  *frame_data->depth_stencil_image_view_};

    // Create a framebuffer with depth and image attachments
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_pass_,                              // renderPass
        2,                                          // attachmentCount
        raw_views,                                  // attachments
        app()->swapchain().width(),                 // width
        app()->swapchain().height(),                // height
        1                                           // layers
    };

    ::VkFramebuffer raw_framebuffer;
    app()->device()->vkCreateFramebuffer(
        app()->device(), &framebuffer_create_info, nullptr, &raw_framebuffer);
    frame_data->framebuffer_ = containers::make_unique<vulkan::VkFramebuffer>(
        data_->allocator(),
        vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app()->device()));

    // Populate the render command buffer
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    (*frame_data->command_buffer_)
        ->vkBeginCommandBuffer((*frame_data->command_buffer_),
                               &sample_application::kBeginCommandBuffer);
    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);

    VkClearValue clears[2];
    clears[0].color = {1.0f, 1.0f, 1.0f, 1.0f};
    clears[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *frame_data->framebuffer_,                 // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        2,                                // clearValueCount
        clears                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &frame_data->cube_descriptor_set_->raw_set(), 0, nullptr);

    // Draw the cube
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *cube_pipeline_);
    cube_.Draw(&cmdBuffer);

    // Draw the floor
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *floor_pipeline_);
    cmdBuffer->vkCmdSetStencilReference(cmdBuffer, VK_STENCIL_FACE_FRONT_BIT,
                                        0xAB);
    cmdBuffer->vkCmdSetStencilWriteMask(cmdBuffer, VK_STENCIL_FACE_FRONT_BIT,
                                        0x0F);
    floor_.Draw(&cmdBuffer);

    // Draw the reflection
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *mirror_pipeline_);
    float blend_constants[4] = {0.5f, 0.5f, 0.5f, 0.5f};
    cmdBuffer->vkCmdSetBlendConstants(cmdBuffer, blend_constants);
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
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> cube_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> floor_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> mirror_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  VkDescriptorSetLayoutBinding descriptor_set_layouts_[2];
  vulkan::VulkanModel cube_;
  vulkan::VulkanModel floor_;

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
