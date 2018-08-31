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

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  auto allocator = data->allocator();
  vulkan::LibraryWrapper wrapper(allocator, data->logger());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->allocator(), &wrapper));
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

    VkBuffer buffer;
    LOG_ASSERT(==, data->logger(),
               device->vkCreateBuffer(device, &create_info, nullptr, &buffer),
               VK_SUCCESS);

    VkMemoryRequirements requirements;
    device->vkGetBufferMemoryRequirements(device, buffer, &requirements);

    device->vkDestroyBuffer(device, buffer, nullptr);
    data->logger()->LogInfo("Memory requirements for 1024 byte buffer:");
    data->logger()->LogInfo("   Size      :", requirements.size);
    data->logger()->LogInfo("   Alignment :", requirements.alignment);
    data->logger()->LogInfo("   TypeBits  :", requirements.memoryTypeBits);
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
