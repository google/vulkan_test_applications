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

uint32_t fragment_shader[] =
#include "simple_fragment.frag.spv"
    ;

uint32_t vertex_shader[] =
#include "simple_vertex.vert.spv"
    ;

// Geometry data of a triangle to be drawn
const float kVertices[] = {
    0.0,  -1.0, 1.0,  // point 1
    -0.5, 1.0,  0.0,  // point 2
    0.5,  1.0,  0.0,  // point 3
};

const vulkan::VulkanGraphicsPipeline::InputStream kVerticesStream{
    0,                           // binding
    VK_FORMAT_R32G32B32_SFLOAT,  // format
    0,                           // offset
};

const float kUV[] = {
    0.0, 0.0,  // point 1
    1.0, 0.0,  // point 2
    0.0, 1.0,  // point 3
};

const vulkan::VulkanGraphicsPipeline::InputStream kUVStream{
    1,                        // binding
    VK_FORMAT_R32G32_SFLOAT,  // format
    0,                        // offset
};

struct DepthBoundsFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> render_command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;

  vulkan::BufferPointer vertices_buf_;
  vulkan::BufferPointer uv_buf_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class DepthBoundsSample
    : public sample_application::Sample<DepthBoundsFrameData> {
 public:
  DepthBoundsSample(const entry::entry_data* data,
                    const VkPhysicalDeviceFeatures& requested_features)
      : data_(data),
        Sample<DepthBoundsFrameData>(
            data->root_allocator, data, 1, 512, 1, 1,
            sample_application::SampleOptions().EnableDepthBuffer(),
            requested_features) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->root_allocator, app()->CreatePipelineLayout({{}}));

    VkAttachmentReference depth_attachment = {
        0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentReference color_attachment = {
        1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    first_render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->root_allocator,
        app()->CreateRenderPass(
            {
                {
                    0,                                 // flags
                    depth_format(),                    // format
                    num_samples(),                     // samples
                    VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stenilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stenilStoreOp
                    VK_IMAGE_LAYOUT_UNDEFINED,         // initialLayout
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // finalLayout
                },  // Depth Attachment
                {
                    0,                                        // flags
                    render_format(),                          // format
                    num_samples(),                            // samples
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // loadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,         // storeOp
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
                &depth_attachment,                // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    second_render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
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
                &depth_attachment,                // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->root_allocator,
        app()->CreateGraphicsPipeline(pipeline_layout_.get(),
                                      first_render_pass_.get(), 0));
    pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main", vertex_shader);
    pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragment_shader);
    pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline_->AddInputStream(3 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX,
                              {kVerticesStream});
    pipeline_->AddInputStream(2 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX,
                              {kUVStream});
    pipeline_->SetScissor(scissor());
    pipeline_->SetViewport(viewport());
    pipeline_->SetSamples(num_samples());
    pipeline_->AddAttachment();
    pipeline_->AddDynamicState(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
    pipeline_->DepthStencilState().depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipeline_->DepthStencilState().depthBoundsTestEnable = VK_TRUE;
    pipeline_->DepthStencilState().minDepthBounds = 0.0f;
    pipeline_->DepthStencilState().maxDepthBounds = 1.0f;
    pipeline_->Commit();

    // Initialize depth bounds
    depthBoundsCenter_ = 0.0f;
    depthBoundsCenterDiff_ = 0.1f;
  }

  void BuildCommandBuffer(DepthBoundsFrameData* frame_data,
                          float minDepthBounds, float maxDepthBounds) {
    const ::VkBuffer vertex_buffers[2] = {*frame_data->vertices_buf_,
                                          *frame_data->uv_buf_};
    const ::VkDeviceSize vertex_buffer_offsets[2] = {0, 0};

    frame_data->render_command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->root_allocator, app()->GetCommandBuffer());

    // Create a framebuffer for rendering
    VkImageView views[2] = {depth_view(frame_data), color_view(frame_data)};
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *first_render_pass_,                        // renderPass
        2,                                          // attachmentCount
        views,                                      // attachments
        app()->swapchain().width(),                 // width
        app()->swapchain().height(),                // height
        1                                           // layers
    };

    ::VkFramebuffer raw_framebuffer;
    app()->device()->vkCreateFramebuffer(
        app()->device(), &framebuffer_create_info, nullptr, &raw_framebuffer);
    frame_data->framebuffer_ = containers::make_unique<vulkan::VkFramebuffer>(
        data_->root_allocator,
        vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app()->device()));

    VkClearValue clears[2] = {{1.0f, 1}, {0.0f, 0.0f, 0.0f, 0.0f}};

    // First renderpass renders the depth buffer, which will be used in the
    // depth bound test.
    (*frame_data->render_command_buffer_)
        ->vkBeginCommandBuffer(*frame_data->render_command_buffer_,
                               &sample_application::kBeginCommandBuffer);
    VkRenderPassBeginInfo begin_first_render_pass = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *first_render_pass_,                       // renderPass
        *frame_data->framebuffer_,                 // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        1,                                // clearValueCount
        clears                            // clears
    };
    (*frame_data->render_command_buffer_)
        ->vkCmdSetDepthBounds(*frame_data->render_command_buffer_, 0.0f, 1.0f);

    (*frame_data->render_command_buffer_)
        ->vkCmdBeginRenderPass(*frame_data->render_command_buffer_,
                               &begin_first_render_pass,
                               VK_SUBPASS_CONTENTS_INLINE);
    (*frame_data->render_command_buffer_)
        ->vkCmdBindPipeline(*frame_data->render_command_buffer_,
                            VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_);
    (*frame_data->render_command_buffer_)
        ->vkCmdBindVertexBuffers(*frame_data->render_command_buffer_, 0, 2,
                                 vertex_buffers, vertex_buffer_offsets);
    (*frame_data->render_command_buffer_)
        ->vkCmdDraw(*frame_data->render_command_buffer_, 3, 1, 0, 0);
    (*frame_data->render_command_buffer_)
        ->vkCmdEndRenderPass(*frame_data->render_command_buffer_);

    // Depth Bound test
    (*frame_data->render_command_buffer_)
        ->vkCmdSetDepthBounds(*frame_data->render_command_buffer_,
                              minDepthBounds, maxDepthBounds);

    (*frame_data->render_command_buffer_)
        ->vkCmdPipelineBarrier(*frame_data->render_command_buffer_,
                               VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                               VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                               nullptr, 0, nullptr, 0, nullptr);

    // The second renderpass renders the color buffer, only for the regin that
    // passes the depth bound test.
    VkRenderPassBeginInfo begin_second_render_pass = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *second_render_pass_,                      // renderPass
        *frame_data->framebuffer_,                 // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        2,                                // clearValueCount
        clears                            // clears
    };
    (*frame_data->render_command_buffer_)
        ->vkCmdBeginRenderPass(*frame_data->render_command_buffer_,
                               &begin_second_render_pass,
                               VK_SUBPASS_CONTENTS_INLINE);
    (*frame_data->render_command_buffer_)
        ->vkCmdBindPipeline(*frame_data->render_command_buffer_,
                            VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_);
    (*frame_data->render_command_buffer_)
        ->vkCmdBindVertexBuffers(*frame_data->render_command_buffer_, 0, 2,
                                 vertex_buffers, vertex_buffer_offsets);
    (*frame_data->render_command_buffer_)
        ->vkCmdDraw(*frame_data->render_command_buffer_, 3, 1, 0, 0);
    (*frame_data->render_command_buffer_)
        ->vkCmdEndRenderPass(*frame_data->render_command_buffer_);
    (*frame_data->render_command_buffer_)
        ->vkEndCommandBuffer(*frame_data->render_command_buffer_);
  }

  virtual void InitializeFrameData(
      DepthBoundsFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    frame_data->vertices_buf_ = app()->CreateAndBindDefaultExclusiveHostBuffer(
        sizeof(kVertices),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    app()->FillHostVisibleBuffer(&*frame_data->vertices_buf_,
                                 reinterpret_cast<const char*>(kVertices),
                                 sizeof(kVertices), 0, initialization_buffer,
                                 VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                 VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
    frame_data->uv_buf_ = app()->CreateAndBindDefaultExclusiveHostBuffer(
        sizeof(kUV),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    app()->FillHostVisibleBuffer(
        &*frame_data->uv_buf_, reinterpret_cast<const char*>(kUV), sizeof(kUV),
        0, initialization_buffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
    BuildCommandBuffer(frame_data, 0.0, 0.0);
  }

  virtual void Update(float time_since_last_render) override {
    if (depthBoundsCenter_ > 0.9f) {
      depthBoundsCenterDiff_ = -0.01f;
    } else if (depthBoundsCenter_ < 0.1f) {
      depthBoundsCenterDiff_ = 0.01f;
    }
    depthBoundsCenter_ += depthBoundsCenterDiff_;
  }

  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      DepthBoundsFrameData* frame_data) override {
    float minBound =
        depthBoundsCenter_ - 0.05f > 0.0f ? depthBoundsCenter_ - 0.05f : 0.0f;
    float maxBound =
        depthBoundsCenter_ + 0.05f < 1.0f ? depthBoundsCenter_ + 0.05f : 1.0f;
    BuildCommandBuffer(frame_data, minBound, maxBound);

    VkSubmitInfo init_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask,
        1,                              // commandBufferCount
        &frame_data->render_command_buffer_->get_command_buffer(),
        0,       // signalSemaphoreCount
        nullptr  // pSignalSemaphores
    };

    app()->render_queue()->vkQueueSubmit(app()->render_queue(), 1,
                                         &init_submit_info,
                                         static_cast<VkFence>(VK_NULL_HANDLE));
  }

 private:
  const entry::entry_data* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> first_render_pass_;
  containers::unique_ptr<vulkan::VkRenderPass> second_render_pass_;

  float depthBoundsCenter_;
  float depthBoundsCenterDiff_;
};

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  VkPhysicalDeviceFeatures requested_features = {0};
  requested_features.depthBounds = VK_TRUE;
  DepthBoundsSample sample(data, requested_features);
  sample.Initialize();

  while (!sample.should_exit()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->log->LogInfo("Application Shutdown");
  return 0;
}
