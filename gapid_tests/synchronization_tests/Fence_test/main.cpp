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
#include "vulkan_helpers/known_device_infos.h"
#include "vulkan_helpers/structs.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data,
                                        vulkan::VulkanApplicationOptions());
  // So we don't have to type app.device every time.
  vulkan::VkDevice& device = app.device();
  vulkan::VkQueue& render_queue = app.render_queue();

  {  // Create/Destroy/Wait
    VkFenceCreateInfo create_info{
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,  // sType
        nullptr,                              // pNext
        0                                     // flags
    };
    VkFence fence;
    LOG_ASSERT(==, data->logger(), VK_SUCCESS,
               device->vkCreateFence(device, &create_info, nullptr, &fence));
    render_queue->vkQueueSubmit(render_queue, 0, nullptr, fence);

    LOG_ASSERT(==, data->logger(), VK_SUCCESS,
               device->vkWaitForFences(device, 1, &fence, VK_FALSE, 100000000));
    LOG_ASSERT(==, data->logger(), VK_SUCCESS,
               device->vkResetFences(device, 1, &fence));

    device->vkDestroyFence(device, fence, nullptr);
  }

  {  // Get fence status
    vulkan::VkFence fence = vulkan::CreateFence(&device, false);
    LOG_ASSERT(==, data->logger(), VK_NOT_READY,
               device->vkGetFenceStatus(device, fence));
    render_queue->vkQueueSubmit(render_queue, 0, nullptr, fence);
    render_queue->vkQueueWaitIdle(render_queue);
    LOG_ASSERT(==, data->logger(), VK_SUCCESS,
               device->vkGetFenceStatus(device, fence));
    device->vkResetFences(device, 1, &fence.get_raw_object());
    LOG_ASSERT(==, data->logger(), VK_NOT_READY,
               device->vkGetFenceStatus(device, fence));

    vulkan::VkFence fence_signaled = vulkan::CreateFence(&device, true);
    LOG_ASSERT(==, data->logger(), VK_SUCCESS,
               device->vkGetFenceStatus(device, fence_signaled));
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
