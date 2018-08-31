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

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "support/entry/entry.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

#include <chrono>
#include "mathfu/matrix.h"
#include "mathfu/vector.h"

using Mat44 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;

namespace cube_model {
#include "cube.obj.h"
}
const auto& cube_data = cube_model::model;

namespace plane_model {
#include "fullscreen_quad.obj.h"
}
const auto& plane_data = plane_model::model;

uint32_t cube_render_vertex_shader[] =
#include "cube.vert.spv"
    ;

uint32_t cube_render_fragment_shader[] =
#include "cube.frag.spv"
    ;

uint32_t depth_render_vertex_shader[] =
#include "depth.vert.spv"
    ;

uint32_t depth_render_fragment_shader[] =
#include "depth.frag.spv"
    ;

struct CubeDepthFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> cube_render_framebuffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> depth_render_framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_render_descriptor_set_;
  containers::unique_ptr<vulkan::DescriptorSet> depth_render_descriptor_set_;
  vulkan::ImagePointer cube_render_color_image_;
  containers::unique_ptr<vulkan::VkImageView> cube_render_color_image_view_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class ClearDepthImageSample
    : public sample_application::Sample<CubeDepthFrameData> {
 public:
  ClearDepthImageSample(const entry::entry_data* data)
      : data_(data),
        Sample<CubeDepthFrameData>(
            data->root_allocator, data, 1, 512, 1, 1,
            sample_application::SampleOptions().EnableDepthBuffer()),
        cube_(data->root_allocator, data->log.get(), cube_data),
        plane_(data->root_allocator, data->log.get(), plane_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    cube_.InitializeData(app(), initialization_buffer);
    plane_.InitializeData(app(), initialization_buffer);

    cube_render_descriptor_set_layout_bindings_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    cube_render_descriptor_set_layout_bindings_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    depth_render_descriptor_set_layout_binding_ = {
        0,                                    // binding
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,  // descriptorType
        1,                                    // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,         // stageFlags
        nullptr                               // pImmutableSamplers
    };

    cube_render_pipeline_layout_ =
        containers::make_unique<vulkan::PipelineLayout>(
            data_->root_allocator,
            app()->CreatePipelineLayout(
                {{cube_render_descriptor_set_layout_bindings_[0],
                  cube_render_descriptor_set_layout_bindings_[1]}}));
    depth_render_pipeline_layout_ =
        containers::make_unique<vulkan::PipelineLayout>(
            data_->root_allocator,
            app()->CreatePipelineLayout(
                {{depth_render_descriptor_set_layout_binding_}}));

    VkAttachmentReference depth_write_attachment = {
        0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentReference color_attachment = {
        1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_render_attachment = {
        0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};

    cube_render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->root_allocator,
        app()->CreateRenderPass(
            {
                {
                    0,                                 // flags
                    depth_format(),                    // format
                    num_samples(),                     // samples
                    VK_ATTACHMENT_LOAD_OP_LOAD,        // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stenilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stenilStoreOp
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,  // initialLayout
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // finalLayout
                },  // Depth Attachment
                {
                    0,                                        // flags
                    render_format(),                          // format
                    num_samples(),                            // samples
                    VK_ATTACHMENT_LOAD_OP_CLEAR,              // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stenilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stenilStoreOp
                    VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL  // finalLayout
                },  // Color Attachment
            },      // AttachmentDescriptions
            {{
                0,                                // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                0,                                // inputAttachmentCount
                nullptr,                          // pInputAttachments
                1,                                // colorAttachmentCount
                &color_attachment,                // colorAttachment
                nullptr,                          // pResolveAttachments
                &depth_write_attachment,          // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    depth_render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->root_allocator,
        app()->CreateRenderPass(
            {
                {
                    0,                                 // flags
                    depth_format(),                    // format
                    num_samples(),                     // samples
                    VK_ATTACHMENT_LOAD_OP_LOAD,        // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stenilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stenilStoreOp
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,  // initialLayout
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL  // finalLayout
                },  // Depth Attachment
                {
                    0,                                         // flags
                    render_format(),                           // format
                    num_samples(),                             // samples
                    VK_ATTACHMENT_LOAD_OP_CLEAR,               // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,              // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stenilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stenilStoreOp
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // initialLayout
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL   // finalLayout
                }  // Color Attachment
            },     // AttachmentDescriptions
            {{
                0,                                // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                1,                                // inputAttachmentCount
                &depth_render_attachment,         // pInputAttachments
                1,                                // colorAttachmentCount
                &color_attachment,                // colorAttachment
                nullptr,                          // pResolveAttachments
                nullptr,                          // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    cube_render_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->root_allocator,
            app()->CreateGraphicsPipeline(cube_render_pipeline_layout_.get(),
                                          cube_render_pass_.get(), 0));
    cube_render_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                     cube_render_vertex_shader);
    cube_render_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                     cube_render_fragment_shader);
    cube_render_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    cube_render_pipeline_->SetInputStreams(&cube_);
    cube_render_pipeline_->SetViewport(viewport());
    cube_render_pipeline_->SetScissor(scissor());
    cube_render_pipeline_->SetSamples(num_samples());
    cube_render_pipeline_->AddAttachment();
    cube_render_pipeline_->Commit();

    depth_render_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->root_allocator,
            app()->CreateGraphicsPipeline(depth_render_pipeline_layout_.get(),
                                          depth_render_pass_.get(), 0));
    depth_render_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                      depth_render_vertex_shader);
    depth_render_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                      depth_render_fragment_shader);
    depth_render_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    depth_render_pipeline_->SetInputStreams(&plane_);
    depth_render_pipeline_->SetViewport(viewport());
    depth_render_pipeline_->SetScissor(scissor());
    depth_render_pipeline_->SetSamples(num_samples());
    depth_render_pipeline_->AddAttachment();
    depth_render_pipeline_->Commit();

    camera_data =
        containers::make_unique<vulkan::BufferFrameData<camera_data_>>(
            data_->root_allocator, app(), num_swapchain_images,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data = containers::make_unique<vulkan::BufferFrameData<model_data_>>(
        data_->root_allocator, app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    camera_data->data().projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data->data().transform = Mat44::FromTranslationVector(
        mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f});
  }

  virtual void InitializeFrameData(
      CubeDepthFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    VkImageCreateInfo cube_render_color_image_create_info{
        /* sType = */
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_2D,
        /* format = */ render_format(),
        /* extent = */
        {
            /* width = */ app()->swapchain().width(),
            /* height = */ app()->swapchain().height(),
            /* depth = */ app()->swapchain().depth(),
        },
        /* mipLevels = */ 1,
        /* arrayLayers = */ 1,
        /* samples = */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling = */
        VK_IMAGE_TILING_OPTIMAL,
        /* usage = */
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        /* sharingMode = */
        VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */
        VK_IMAGE_LAYOUT_UNDEFINED,
    };
    frame_data->cube_render_color_image_ =
        app()->CreateAndBindImage(&cube_render_color_image_create_info);
    VkImageViewCreateInfo cube_render_color_image_view_create_info{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        *frame_data->cube_render_color_image_,     // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        render_format(),                           // format
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
         VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    ::VkImageView raw_view;
    LOG_ASSERT(==, data_->log.get(), VK_SUCCESS,
               app()->device()->vkCreateImageView(
                   app()->device(), &cube_render_color_image_view_create_info,
                   nullptr, &raw_view));
    frame_data->cube_render_color_image_view_ =
        containers::make_unique<vulkan::VkImageView>(
            data_->root_allocator,
            vulkan::VkImageView(raw_view, nullptr, &app()->device()));

    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->root_allocator, app()->GetCommandBuffer());

    // Initialize cube rendering descriptor set.
    frame_data->cube_render_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->root_allocator,
            app()->AllocateDescriptorSet(
                {cube_render_descriptor_set_layout_bindings_[0],
                 cube_render_descriptor_set_layout_bindings_[1]}));

    VkDescriptorBufferInfo buffer_infos[2] = {
        {
            camera_data->get_buffer(),                       // buffer
            camera_data->get_offset_for_frame(frame_index),  // offset
            camera_data->size(),                             // range
        },
        {
            model_data->get_buffer(),                       // buffer
            model_data->get_offset_for_frame(frame_index),  // offset
            model_data->size(),                             // range
        }};

    VkWriteDescriptorSet write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
        nullptr,                                   // pNext
        *frame_data->cube_render_descriptor_set_,  // dstSet
        0,                                         // dstbinding
        0,                                         // dstArrayElement
        2,                                         // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         // descriptorType
        nullptr,                                   // pImageInfo
        buffer_infos,                              // pBufferInfo
        nullptr,                                   // pTexelBufferView
    };

    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                            nullptr);

    // Initiailize depth rendering descriptor set
    frame_data->depth_render_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->root_allocator,
            app()->AllocateDescriptorSet(
                {depth_render_descriptor_set_layout_binding_}));
    VkDescriptorImageInfo depth_input_image_info{
        VK_NULL_HANDLE,                                  // sampler
        depth_view(frame_data),                          // imageView,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL  // imageLayout
    };
    write.dstSet = *frame_data->depth_render_descriptor_set_;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    write.pBufferInfo = nullptr;
    write.pImageInfo = &depth_input_image_info;
    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                            nullptr);

    // Create a framebuffer for cube rendering
    VkImageView raw_views[2] = {depth_view(frame_data),
                                *frame_data->cube_render_color_image_view_};
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *cube_render_pass_,                         // renderPass
        2,                                          // attachmentCount
        raw_views,                                  // attachments
        app()->swapchain().width(),                 // width
        app()->swapchain().height(),                // height
        1                                           // layers
    };

    ::VkFramebuffer raw_framebuffer;
    app()->device()->vkCreateFramebuffer(
        app()->device(), &framebuffer_create_info, nullptr, &raw_framebuffer);
    frame_data->cube_render_framebuffer_ =
        containers::make_unique<vulkan::VkFramebuffer>(
            data_->root_allocator,
            vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app()->device()));

    // Create a framebuffer for depth rendering
    framebuffer_create_info.renderPass = *depth_render_pass_;
    raw_views[1] = color_view(frame_data);
    framebuffer_create_info.pAttachments = raw_views;
    app()->device()->vkCreateFramebuffer(
        app()->device(), &framebuffer_create_info, nullptr, &raw_framebuffer);
    frame_data->depth_render_framebuffer_ =
        containers::make_unique<vulkan::VkFramebuffer>(
            data_->root_allocator,
            vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app()->device()));

    // Populate the render command buffer
    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);
    cmdBuffer->vkBeginCommandBuffer(cmdBuffer,
                                    &sample_application::kBeginCommandBuffer);

    // // Call vkCmdClearDepthStencilImage to clear the depth image
    vulkan::RecordImageLayoutTransition(
        depth_image(frame_data), {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
        &cmdBuffer);
    VkClearDepthStencilValue clear_depth{
        0.93f,  // depth
        1,      // stencil
    };
    VkImageSubresourceRange clear_range{
        VK_IMAGE_ASPECT_DEPTH_BIT,  // aspectMask
        0,                          // baseMipLevel
        1,                          // levelCount
        0,                          // baseArrayLayer
        1,                          // layerCount
    };
    cmdBuffer->vkCmdClearDepthStencilImage(cmdBuffer, depth_image(frame_data),
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           &clear_depth, 1, &clear_range);
    vulkan::RecordImageLayoutTransition(
        depth_image(frame_data), {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, &cmdBuffer);

    // Render the cube
    VkClearValue clears[2];
    vulkan::MemoryClear(&clears[1]);  // clear the color attachment

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *cube_render_pass_,                        // renderPass
        *frame_data->cube_render_framebuffer_,     // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        2,                                // clearValueCount
        clears                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *cube_render_pipeline_);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*cube_render_pipeline_layout_), 0, 1,
        &frame_data->cube_render_descriptor_set_->raw_set(), 0, nullptr);

    cube_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    // Render the depth buffer
    vulkan::RecordImageLayoutTransition(
        depth_image(frame_data), {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, &cmdBuffer);

    pass_begin.renderPass = *depth_render_pass_;
    pass_begin.framebuffer = *frame_data->depth_render_framebuffer_;
    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *depth_render_pipeline_);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*depth_render_pipeline_layout_), 0, 1,
        &frame_data->depth_render_descriptor_set_->raw_set(), 0, nullptr);
    plane_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    vulkan::RecordImageLayoutTransition(
        depth_image(frame_data), {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, &cmdBuffer);

    (*frame_data->command_buffer_)
        ->vkEndCommandBuffer(*frame_data->command_buffer_);
  }

  virtual void Update(float time_since_last_render) override {
    model_data->data().transform =
        model_data->data().transform *
        Mat44::FromRotationMatrix(
            Mat44::RotationX(3.14f * time_since_last_render) *
            Mat44::RotationY(3.14f * time_since_last_render * 0.5f));
  }
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      CubeDepthFrameData* frame_data) override {
    // Update our uniform buffers.
    camera_data->UpdateBuffer(queue, frame_index);
    model_data->UpdateBuffer(queue, frame_index);

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
  struct camera_data_ {
    Mat44 projection_matrix;
  };

  struct model_data_ {
    Mat44 transform;
  };

  const entry::entry_data* data_;
  containers::unique_ptr<vulkan::PipelineLayout> cube_render_pipeline_layout_;
  containers::unique_ptr<vulkan::PipelineLayout> depth_render_pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> cube_render_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> depth_render_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> cube_render_pass_;
  containers::unique_ptr<vulkan::VkRenderPass> depth_render_pass_;
  VkDescriptorSetLayoutBinding cube_render_descriptor_set_layout_bindings_[2];
  VkDescriptorSetLayoutBinding depth_render_descriptor_set_layout_binding_;
  vulkan::VulkanModel cube_;
  vulkan::VulkanModel plane_;

  containers::unique_ptr<vulkan::BufferFrameData<camera_data_>> camera_data;
  containers::unique_ptr<vulkan::BufferFrameData<model_data_>> model_data;
};

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  ClearDepthImageSample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->should_exit()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->log->LogInfo("Application Shutdown");
  return 0;
}
