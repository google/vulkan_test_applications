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

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  auto allocator = data->allocator();
  vulkan::LibraryWrapper wrapper(allocator, data->logger());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->allocator(), &wrapper));
  vulkan::VkDevice device(vulkan::CreateDefaultDevice(allocator, instance));
  {
    VkImageCreateInfo image_create_info{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
        nullptr,                              // pNext
        0,                                    // flags
        VK_IMAGE_TYPE_2D,                     // imageType
        VK_FORMAT_R8G8B8A8_UNORM,             // format
        {
            32,  //     extent.width
            32,  //     extent.height
            1,   //     depth
        },
        1,                                    // mipLevels
        1,                                    // arrayLayers
        VK_SAMPLE_COUNT_1_BIT,                // samples
        VK_IMAGE_TILING_OPTIMAL,              // tiling
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,            // sharingMode
        0,                                    // queueFamilyIndexCount
        nullptr,                              // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,            // initialLayout
    };
    ::VkImage raw_image;
    device->vkCreateImage(device, &image_create_info, nullptr, &raw_image);
    vulkan::VkImage image(raw_image, nullptr, &device);

    VkMemoryRequirements image_memory_requirements = {};
    device->vkGetImageMemoryRequirements(device, image,
                                         &image_memory_requirements);
    data->logger()->LogInfo("Memory Requirements: ");
    data->logger()->LogInfo("    size : ", image_memory_requirements.size);
    data->logger()->LogInfo("    alignment : ",
                            image_memory_requirements.alignment);
    data->logger()->LogInfo("    memoryTypeBits : ",
                            image_memory_requirements.memoryTypeBits);

    uint32_t memory_index = vulkan::GetMemoryIndex(
        &device, data->logger(), image_memory_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    data->logger()->LogInfo("Using memory index: ", memory_index);

    VkMemoryAllocateInfo allocate_info{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // sType
        nullptr,                                 // pNext
        image_memory_requirements.size,          // allocationSize
        memory_index};

    VkDeviceMemory device_memory;
    LOG_ASSERT(==, data->logger(), VK_SUCCESS,
               device->vkAllocateMemory(device, &allocate_info, nullptr,
                                        &device_memory));

    LOG_ASSERT(==, data->logger(), VK_SUCCESS,
               device->vkBindImageMemory(device, image, device_memory, 0));

    device->vkFreeMemory(device, device_memory, nullptr);
  }
  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
