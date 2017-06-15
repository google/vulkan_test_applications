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
#include "vulkan_helpers/known_device_infos.h"
#include "vulkan_helpers/vulkan_application.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");

  vulkan::VulkanApplication application(data->root_allocator, data->log.get(),
                                        data);
  vulkan::VkDevice& device = application.device();

  VkPhysicalDeviceProperties properties;
  application.instance()->vkGetPhysicalDeviceProperties(
      device.physical_device(), &properties);
  const VkDeviceSize min_alignment =
      properties.limits.minTexelBufferOffsetAlignment;
  const VkDeviceSize buffer_size = min_alignment * 4;
  const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
  const VkDeviceSize element_size =
      (VkDeviceSize)std::get<0>(vulkan::GetElementAndTexelBlockSize(format));

  {
    // 1. Create a buffer view with zero offset and VK_WHOLE_SIZE range for a
    // non-sparse buffer created with VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT.
    VkBufferCreateInfo buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,      // sType
        nullptr,                                   // pNext
        0,                                         // createFlags
        buffer_size,                               // size
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,                 // sharingMode
        0,                                         // queueFamilyIndexCount
        nullptr,                                   // pQueueFamilyIndices
    };
    vulkan::BufferPointer buffer =
        application.CreateAndBindHostBuffer(&buffer_create_info);

    VkBufferViewCreateInfo view_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *buffer,                                    // buffer
        format,                                     // format
        0,                                          // offset
        VK_WHOLE_SIZE,                              // range
    };

    ::VkBufferView buffer_view;
    device->vkCreateBufferView(device, &view_create_info, nullptr,
                               &buffer_view);
    device->vkDestroyBufferView(device, buffer_view, nullptr);

    if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5bce4000)) {
      device->vkDestroyBufferView(device, (VkBufferView)VK_NULL_HANDLE,
                                  nullptr);
    }
  }

  {
    // 2. Create a buffer view with non-zero offset and range value other than
    // VK_WHOLE_SIZE, for a non-sparse buffer created with
    // VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT.
    VkBufferCreateInfo buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,      // sType
        nullptr,                                   // pNext
        0,                                         // createFlags
        buffer_size,                               // size
        VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,                 // sharingMode
        0,                                         // queueFamilyIndexCount
        nullptr,                                   // pQueueFamilyIndices
    };
    vulkan::BufferPointer buffer =
        application.CreateAndBindHostBuffer(&buffer_create_info);

    VkBufferViewCreateInfo view_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *buffer,                                    // buffer
        format,                                     // format
        min_alignment * 1,                          // offset
        min_alignment * 3,                          // range
    };

    ::VkBufferView buffer_view;
    device->vkCreateBufferView(device, &view_create_info, nullptr,
                               &buffer_view);
    device->vkDestroyBufferView(device, buffer_view, nullptr);

    if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5bce4000)) {
      device->vkDestroyBufferView(device, (VkBufferView)VK_NULL_HANDLE,
                                  nullptr);
    }
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
