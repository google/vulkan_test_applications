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

#include "cube_render.h"

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "support/entry/entry.h"

struct CubeSampleData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  CubeRenderData cubeRenderData;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class CubeSample : public sample_application::Sample<CubeSampleData> {
 public:
  CubeSample(const entry::EntryData* data)
      : data_(data),
        Sample<CubeSampleData>(
            data->allocator(), data, 1, 512, 1, 1,
            sample_application::SampleOptions().EnableMultisampling()),
            cube(data) {}


  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    CubeVulkanInfo vulkanInfo;
    vulkanInfo.num_samples = num_samples();
    vulkanInfo.format = render_format();
    vulkanInfo.scissor = scissor();
    vulkanInfo.viewport = viewport();
    cube.InitializeCubeData(app(), data_->allocator(), vulkanInfo, initialization_buffer, num_swapchain_images);
  }

  virtual void InitializeFrameData(CubeSampleData* frame_data, 
    vulkan::VkCommandBuffer* initialization_buffer,
    size_t frame_index) override {

    frame_data->command_buffer_ = 
      containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);

    data_->logger()->LogInfo("Melih InitializeFrameData Begin");

    cube.InitializeFrameData(
        app(), 
        &frame_data->cubeRenderData,
        data_->allocator(), 
        color_view(frame_data),
        frame_index);
    data_->logger()->LogInfo("Melih InitializeFrameData End");

    cmdBuffer->vkBeginCommandBuffer(cmdBuffer, &sample_application::kBeginCommandBuffer);
    
    data_->logger()->LogInfo("Melih Record Render Begin");
    cube.RecordRenderCmds(app(), &frame_data->cubeRenderData, cmdBuffer);
    data_->logger()->LogInfo("Melih Record Render End");

    cmdBuffer->vkEndCommandBuffer(cmdBuffer);
  }

  virtual void Update(float time_since_last_render) override {
    cube.Update(time_since_last_render);
  }
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      CubeSampleData* frame_data) override {
    cube.Render(queue, frame_index);
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
    CubeRender cube;
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
