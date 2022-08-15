/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "support/entry/entry.h"
#include "support/log/log.h"
#include "vulkan_helpers/vulkan_application.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::VulkanApplication app(data->allocator(), data->logger(), data,
                                vulkan::VulkanApplicationOptions());
  vulkan::VkDevice& device = app.device();
  {
    // 1. Acquire the next image with one semaphore but no fence
    ::VkSemaphore raw_semaphore;
    VkSemaphoreCreateInfo semaphore_create_info{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,  // sType
        NULL,                                     // pNext
        0,                                        // flags
    };
    device->vkCreateSemaphore(device, &semaphore_create_info, nullptr,
                              &raw_semaphore);
    vulkan::VkSemaphore semaphore(raw_semaphore, nullptr, &device);

    uint32_t image_index = 0;
    device->vkAcquireNextImageKHR(device, app.swapchain(), 10u, semaphore,
                                  (::VkFence)VK_NULL_HANDLE, &image_index);
    data->logger()->LogInfo("Next image index: ", image_index);

    // Begin and end an command buffer and submit the queue so that we can
    // safely destroy the semaphore
    vulkan::VkCommandBuffer cmd_buf = app.GetCommandBuffer();
    ::VkCommandBuffer raw_cmd_buf = cmd_buf.get_command_buffer();
    VkCommandBufferBeginInfo info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // sType
        nullptr,                                      // pNext
        0,                                            // flags
        nullptr,                                      // pInheritanceInfo
    };
    cmd_buf->vkBeginCommandBuffer(cmd_buf, &info);
    cmd_buf->vkEndCommandBuffer(cmd_buf);
    VkPipelineStageFlags pipe_stage_flags =
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        NULL,                           // pNext
        1,                              // waitSemaphoreCount
        &raw_semaphore,                 // pWaitSemaphores
        &pipe_stage_flags,              // pWaitDstStageMask
        1,                              // commandBufferCount
        &raw_cmd_buf,                   // pCommandBuffers
        0,                              // signalSemaphoreCount
        NULL,                           // pSignalSemaphores
    };
    app.present_queue()->vkQueueSubmit(app.present_queue(), 1, &submit_info,
                                       (::VkFence)VK_NULL_HANDLE);
    app.present_queue()->vkQueueWaitIdle(app.present_queue());
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
