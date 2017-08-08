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
#include "vulkan_helpers/shader_collection.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

#include <chrono>

namespace {

std::vector<uint32_t> glslc_glsl_vertex_shader =
#include "passthrough.glsl.vert.spv"
    ;

std::vector<uint32_t> glslc_glsl_fragment_shader =
#include "passthrough.glsl.frag.spv"
    ;

std::vector<uint32_t> glslc_hlsl_vertex_shader =
#include "passthrough.glslc.hlsl.vert.spv"
    ;

std::vector<uint32_t> glslc_hlsl_fragment_shader =
#include "passthrough.glslc.hlsl.frag.spv"
    ;

std::vector<uint32_t> dxc_hlsl_vertex_shader =
#include "passthrough.dxc.hlsl.vert.spv"
    ;

std::vector<uint32_t> dxc_hlsl_fragment_shader =
#include "passthrough.dxc.hlsl.frag.spv"
    ;

// Geometry data of the triangle to be drawn
const float kVertices[] = {
    0.0,  -0.5, 0.0, 1.0,  // point 1
    -0.5, 0.5,  0.0, 1.0,  // point 2
    0.5,  0.5,  0.0, 1.0   // point 3
};
const vulkan::VulkanGraphicsPipeline::InputStream kVerticesStream{
    0,                              // binding
    VK_FORMAT_R32G32B32A32_SFLOAT,  // format
    0,                              // offset
};

const float kUV[] = {
    1.0, 0.0, 0.0, 1.0,  // point 1
    0.0, 1.0, 0.0, 1.0,  // point 2
    0.0, 0.0, 1.0, 1.0   // point 3
};
const vulkan::VulkanGraphicsPipeline::InputStream kUVStream{
    1,                           // binding
    VK_FORMAT_R32G32B32_SFLOAT,  // format
    0,                           // offset
};

const VkCommandBufferBeginInfo kCommandBufferBeginInfo{
    /* sType = */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    /* pNext = */ nullptr,
    /* flags = */ VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    /* pInheritanceInfo = */ nullptr,
};

// A helper function that returns a default buffer create info struct with
// given size and usage
VkBufferCreateInfo GetBufferCreateInfo(VkDeviceSize size,
                                       VkBufferUsageFlags usage) {
  return VkBufferCreateInfo{
      /* sType = */ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* size = */ size,
      /* usage = */ usage,
      /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
      /* queueFamilyIndexCount = */ 0,
      /* pQueueFamilyIndices = */ nullptr,
  };
}

// A helper function that flushes the given data to the device memory for the
// given host visible buffer. If a command buffer is given, add a buffer memory
// barrier to the command buffer.
void FlushHostVisibleBuffer(vulkan::VulkanApplication::Buffer* buf, size_t size,
                            const char* data, vulkan::VkCommandBuffer* cmd_buf,
                            VkPipelineStageFlags dst_stage_mask,
                            VkAccessFlags dst_access_flags) {
  char* p = reinterpret_cast<char*>(buf->base_address());
  for (size_t i = 0; i < buf->size() && i < size; i++) {
    p[i] = data[i];
  }
  buf->flush();

  if (cmd_buf) {
    VkBufferMemoryBarrier buf_barrier{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
        dst_access_flags,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        *buf,
        0,
        VK_WHOLE_SIZE};

    (*cmd_buf)->vkCmdPipelineBarrier(
        *cmd_buf, VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
        dst_stage_mask, 0, 0, nullptr, 1, &buf_barrier, 0, nullptr);
  }
}

}  // namespace

struct PassthroughFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class PassthroughSample
    : public sample_application::Sample<PassthroughFrameData> {
 public:
  PassthroughSample(const entry::entry_data* data)
      : data_(data),
        Sample<PassthroughFrameData>(
            data->root_allocator, data, 1, 512, 1, 1,
            sample_application::SampleOptions().EnableMultisampling()) {}

  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->root_allocator, app()->CreatePipelineLayout({{}}));

    VkAttachmentReference color_attachment = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->root_allocator,
        app()->CreateRenderPass(
            {{
                0,                                         // flags
                render_format(),                           // format
                num_samples(),                             // samples
                VK_ATTACHMENT_LOAD_OP_CLEAR,               // loadOp
                VK_ATTACHMENT_STORE_OP_STORE,              // storeOp
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stenilLoadOp
                VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stenilStoreOp
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // initialLayout
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL   // finalLayout
            }},  // AttachmentDescriptions
            {{
                0,                                // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                0,                                // inputAttachmentCount
                nullptr,                          // pInputAttachments
                1,                                // colorAttachmentCount
                &color_attachment,                // colorAttachment
                nullptr,                          // pResolveAttachments
                nullptr,                          // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    passthrough_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->root_allocator,
            app()->CreateGraphicsPipeline(pipeline_layout_.get(),
                                          render_pass_.get(), 0));
    vulkan::ShaderCollection shaders(
        data_->log.get(), data_->options.shader_compiler,
        glslc_glsl_vertex_shader, glslc_glsl_fragment_shader,
        glslc_hlsl_vertex_shader, glslc_hlsl_fragment_shader,
        dxc_hlsl_vertex_shader, dxc_hlsl_fragment_shader);
    passthrough_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                     shaders.vertexShader(),
                                     shaders.vertexShaderWordCount());
    passthrough_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                     shaders.fragmentShader(),
                                     shaders.fragmentShaderWordCount());
    passthrough_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    passthrough_pipeline_->AddInputStream(
        sizeof(float) * 4, VK_VERTEX_INPUT_RATE_VERTEX, {kVerticesStream});
    passthrough_pipeline_->AddInputStream(
        sizeof(float) * 4, VK_VERTEX_INPUT_RATE_VERTEX, {kUVStream});

    passthrough_pipeline_->SetViewport(viewport());
    passthrough_pipeline_->SetScissor(scissor());
    passthrough_pipeline_->SetSamples(num_samples());
    passthrough_pipeline_->AddAttachment();
    passthrough_pipeline_->Commit();
  }

  virtual void InitializeFrameData(
      PassthroughFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->root_allocator, app()->GetCommandBuffer());

    ::VkImageView raw_view = color_view(frame_data);

    // Create a framebuffer with depth and image attachments
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_pass_,                              // renderPass
        1,                                          // attachmentCount
        &raw_view,                                  // attachments
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

    (*frame_data->command_buffer_)
        ->vkBeginCommandBuffer((*frame_data->command_buffer_),
                               &sample_application::kBeginCommandBuffer);
    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);

    VkClearValue clear;
    vulkan::MemoryClear(&clear);

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *frame_data->framebuffer_,                 // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        1,                                // clearValueCount
        &clear                            // clears
    };

    // Create vertex buffers and index buffer
    const auto vertices_buf_create_info = GetBufferCreateInfo(
        sizeof(kVertices),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    const auto uv_buf_create_info = GetBufferCreateInfo(
        sizeof(kUV),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    auto vertices_buf =
        app()->CreateAndBindHostBuffer(&vertices_buf_create_info);
    auto uv_buf = app()->CreateAndBindHostBuffer(&uv_buf_create_info);
    const ::VkBuffer vertex_buffers[2] = {*vertices_buf, *uv_buf};
    const ::VkDeviceSize vertex_buffer_offsets[2] = {0, 0};

    FlushHostVisibleBuffer(&*vertices_buf, sizeof(kVertices),
                           reinterpret_cast<const char*>(kVertices), &cmdBuffer,
                           VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                           VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
    FlushHostVisibleBuffer(&*uv_buf, sizeof(kUV),
                           reinterpret_cast<const char*>(kUV), &cmdBuffer,
                           VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                           VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *passthrough_pipeline_);
    cmdBuffer->vkCmdBindVertexBuffers(cmdBuffer, 0, 2, vertex_buffers,
                                      vertex_buffer_offsets);
    cmdBuffer->vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);
    cmdBuffer->vkEndCommandBuffer(cmdBuffer);
  }

  virtual void Update(float time_since_last_render) override {
    // Do Nothing!
  }
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      PassthroughFrameData* frame_data) override {
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
  const entry::entry_data* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> passthrough_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
};

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  PassthroughSample sample(data);
  sample.Initialize();

  while (!sample.should_exit()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->log->LogInfo("Application Shutdown");
  return 0;
}
