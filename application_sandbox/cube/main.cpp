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

// #include "cube_render.h"
#include "render_quad.h"

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "support/entry/entry.h"

struct CubeSampleData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  // CubeRenderData cubeRenderData;
  RenderQuadData renderQuadData;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class CubeSample : public sample_application::Sample<CubeSampleData> {
 public:
  CubeSample(const entry::EntryData* data)
      : data_(data),
        Sample<CubeSampleData>(
            data->allocator(), data, 10, 1024, 10, 1,
            sample_application::SampleOptions().EnableMultisampling()),
            // cube(data),
            quad(data) {}


  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    // CubeVulkanInfo cubeVulkanInfo{};
    // cubeVulkanInfo.num_samples = num_samples();
    // cubeVulkanInfo.format = render_format();
    // cubeVulkanInfo.depthFormat = depth_format();
    // cubeVulkanInfo.scissor = scissor();
    // cubeVulkanInfo.viewport = viewport();
    // cube.InitializeCubeData(app(), data_->allocator(), cubeVulkanInfo, initialization_buffer, num_swapchain_images);

    QuadVulkanInfo quadVulkanInfo{};
    quadVulkanInfo.num_samples = num_samples();
    quadVulkanInfo.colorFormat = render_format();
    quadVulkanInfo.scissor = scissor();
    quadVulkanInfo.viewport = viewport();
    quad.InitializeQuadData(app(), data_->allocator(), quadVulkanInfo, initialization_buffer, num_swapchain_images);
  }

  virtual void InitializeFrameData(CubeSampleData* frame_data, 
    vulkan::VkCommandBuffer* initialization_buffer,
    size_t frame_index) override {

    frame_data->command_buffer_ = 
      containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);

    // cube.InitializeFrameData(
    //     app(), 
    //     &frame_data->cubeRenderData,
    //     data_->allocator(), 
    //     color_view(frame_data),
    //     frame_index);

    quad.InitializeFrameData(
        app(), 
        &frame_data->renderQuadData,
        data_->allocator(), 
        color_view(frame_data),
        frame_index);

    cmdBuffer->vkBeginCommandBuffer(cmdBuffer, &sample_application::kBeginCommandBuffer);
    // cube.RecordRenderCmds(app(), &frame_data->cubeRenderData, cmdBuffer);
    quad.RecordRenderCmds(app(), &frame_data->renderQuadData, cmdBuffer, frame_index);
    cmdBuffer->vkEndCommandBuffer(cmdBuffer);
  }

  virtual void Update(float time_since_last_render) override {
    // cube.Update(time_since_last_render);
    quad.Update(time_since_last_render);
  }
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      CubeSampleData* frame_data) override {
    // cube.UpdateRenderData(queue, frame_index);
    quad.UpdateRenderData(queue, frame_index);
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
    const entry::EntryData* data_;
    // CubeRender cube;
    RenderQuad quad;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  CubeSample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
