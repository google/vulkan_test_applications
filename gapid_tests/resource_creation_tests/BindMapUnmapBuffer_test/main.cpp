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
#include "vulkan_helpers/structs.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");

  auto& allocator = data->root_allocator;
  vulkan::LibraryWrapper wrapper(allocator, data->log.get());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->root_allocator, &wrapper));
  vulkan::VkDevice device(vulkan::CreateDefaultDevice(allocator, instance));

  {  // First test
    VkBufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // createFlags
        1024,                                  // size
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,      // usage
        VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
        0,                                     // queueFamilyIndexCount
        nullptr                                // pQueueFamilyIndices
    };

    VkBuffer raw_buffer;

    LOG_ASSERT(==, data->log, device->vkCreateBuffer(device, &create_info,
                                                     nullptr, &raw_buffer),
               VK_SUCCESS);
    vulkan::VkBuffer buffer(raw_buffer, nullptr, &device);
    VkMemoryRequirements requirements;
    device->vkGetBufferMemoryRequirements(device, buffer, &requirements);

    uint32_t memory_index = vulkan::GetMemoryIndex(
        &device, data->log.get(), requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    VkMemoryAllocateInfo allocate_info{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // sType
        nullptr,                                 // pNext
        requirements.size,                       // allocationSize
        memory_index};

    VkDeviceMemory device_memory;
    LOG_ASSERT(==, data->log, VK_SUCCESS,
               device->vkAllocateMemory(device, &allocate_info, nullptr,
                                        &device_memory));

    LOG_ASSERT(==, data->log, VK_SUCCESS,
               device->vkBindBufferMemory(device, buffer, device_memory, 0));
    void* pData = nullptr;
    LOG_ASSERT(==, data->log, VK_SUCCESS,
               device->vkMapMemory(device, device_memory, 0, VK_WHOLE_SIZE, 0,
                                   &pData));
    LOG_ASSERT(!=, data->log, pData, static_cast<void*>(nullptr));

    // Make sure that we could write to the new pData pointer.
    for (size_t i = 0; i < 1024; ++i) {
      static_cast<char*>(pData)[i] = i % 256;
    }

    device->vkUnmapMemory(device, device_memory);
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
