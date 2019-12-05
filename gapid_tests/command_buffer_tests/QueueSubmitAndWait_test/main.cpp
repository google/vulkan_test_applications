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
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/queue_wrapper.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->allocator(), data->logger());
  vulkan::VkInstance instance(
      vulkan::CreateDefaultInstance(data->allocator(), &wrapper));
  vulkan::VkSurfaceKHR surface(vulkan::CreateDefaultSurface(&instance, data));

  uint32_t queues[2];
  vulkan::VkDevice device(vulkan::CreateDeviceForSwapchain(
      data->allocator(), &instance, &surface, &queues[0], &queues[1], false));

  vulkan::VkQueue queue = vulkan::GetQueue(&device, queues[0]);
  {  // 0 submits
    queue->vkQueueSubmit(queue, 0, nullptr,
                         static_cast<VkFence>(VK_NULL_HANDLE));
    queue->vkQueueWaitIdle(queue);
  }

  {  // 1 submit, 0 command_buffers
    VkSubmitInfo queue_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask
        0,                              // commandBufferCount
        nullptr,                        // pCommandBuffers
        0,                              // signalSemaphoreCount
        nullptr                         // pSignalSemaphores
    };
    queue->vkQueueSubmit(queue, 1, &queue_submit_info,
                         static_cast<VkFence>(VK_NULL_HANDLE));
    queue->vkQueueWaitIdle(queue);
  }

  {  // 1 submit, 1 command_buffer
    vulkan::VkCommandPool pool(
        vulkan::CreateDefaultCommandPool(data->allocator(), device, false));
    vulkan::VkCommandBuffer command_buffer =
        vulkan::CreateDefaultCommandBuffer(&pool, &device);

    VkCommandBufferBeginInfo info{
        /* sType = */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* pInheritanceInfo = */ nullptr,
    };
    command_buffer->vkBeginCommandBuffer(command_buffer, &info);
    command_buffer->vkEndCommandBuffer(command_buffer);

    ::VkCommandBuffer raw_command_buffer = command_buffer;

    VkSubmitInfo queue_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask
        1,                              // commandBufferCount
        &raw_command_buffer,            // pCommandBuffers
        0,                              // signalSemaphoreCount
        nullptr                         // pSignalSemaphores
    };

    queue->vkQueueSubmit(queue, 1, &queue_submit_info,
                         static_cast<VkFence>(VK_NULL_HANDLE));
    queue->vkQueueWaitIdle(queue);
  }

  {  // 1 submit, 2 command_buffers
    vulkan::VkCommandPool pool(
        vulkan::CreateDefaultCommandPool(data->allocator(), device, false));
    vulkan::VkCommandBuffer command_buffer =
        vulkan::CreateDefaultCommandBuffer(&pool, &device);

    VkCommandBufferBeginInfo info{
        /* sType = */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* pInheritanceInfo = */ nullptr,
    };
    command_buffer->vkBeginCommandBuffer(command_buffer, &info);
    command_buffer->vkEndCommandBuffer(command_buffer);

    vulkan::VkCommandBuffer command_buffer2 =
        vulkan::CreateDefaultCommandBuffer(&pool, &device);
    command_buffer->vkBeginCommandBuffer(command_buffer2, &info);
    command_buffer->vkEndCommandBuffer(command_buffer2);

    VkCommandBuffer command_buffers[2] = {command_buffer, command_buffer2};

    VkSubmitInfo queue_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask
        2,                              // commandBufferCount
        command_buffers,                // pCommandBuffers
        0,                              // signalSemaphoreCount
        nullptr                         // pSignalSemaphores
    };

    queue->vkQueueSubmit(queue, 1, &queue_submit_info,
                         static_cast<VkFence>(VK_NULL_HANDLE));
    queue->vkQueueWaitIdle(queue);
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
