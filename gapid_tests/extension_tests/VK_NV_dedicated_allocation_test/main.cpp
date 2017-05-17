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
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->root_allocator, data->log.get(), data,
                                {VK_NV_DEDICATED_ALLOCATION_EXTENSION_NAME});
  vulkan::VkDevice& device = app.device();
  if (device.is_valid()) {
    data->log->LogInfo(VK_NV_DEDICATED_ALLOCATION_EXTENSION_NAME, " found.");

    {  // 1. Image Test
      VkDedicatedAllocationImageCreateInfoNV dedicated_image_info = {
          VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV,  // sType
          nullptr,                                                      // pNext
          VK_TRUE  // dedicatedAllocation
      };

      VkImageCreateInfo image_create_info = {
          VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
          &dedicated_image_info,                // pNext
          0,                                    // flags
          VK_IMAGE_TYPE_2D,                     // 2d image
          VK_FORMAT_R8G8B8A8_UNORM,             // format
          {128, 128, 1},                        // extent
          1,                                    // mipLevels
          1,                                    // arrayLayers
          VK_SAMPLE_COUNT_1_BIT,                // samples
          VK_IMAGE_TILING_OPTIMAL,              // tiling
          VK_IMAGE_USAGE_TRANSFER_DST_BIT,      // usage
          VK_SHARING_MODE_EXCLUSIVE,            // sharingMode
          0,                                    // queueFamilyIndexCount
          nullptr,                              // queueFamilyIndices
          VK_IMAGE_LAYOUT_UNDEFINED             // initialLayout
      };
      ::VkImage raw_image;
      LOG_ASSERT(==, data->log, VK_SUCCESS,
                 device->vkCreateImage(device, &image_create_info, nullptr,
                                       &raw_image));
      vulkan::VkImage image(raw_image, nullptr, &device);

      VkMemoryRequirements memory_requirements;
      device->vkGetImageMemoryRequirements(device, image, &memory_requirements);

      VkDedicatedAllocationMemoryAllocateInfoNV dedicated_allocate_info = {
          VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV,
          nullptr, image, VK_NULL_HANDLE};

      VkMemoryAllocateInfo memoryAllocateInfo = {
          VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // sType
          &dedicated_allocate_info,                // pNext
          memory_requirements.size,                // size
          GetMemoryIndex(&device, data->log.get(),
                         memory_requirements.memoryTypeBits, 0)};
      ::VkDeviceMemory raw_memory;
      device->vkAllocateMemory(device, &memoryAllocateInfo, nullptr,
                               &raw_memory);
      vulkan::VkDeviceMemory memory(raw_memory, nullptr, &device);

      device->vkBindImageMemory(device, image, memory, 0);
    }
    {  // 2. Buffer Test
      VkDedicatedAllocationBufferCreateInfoNV dedicated_buffer_info = {
          VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV,  // sType
          nullptr,  // pNext
          VK_TRUE   // dedicatedAllocation
      };

      VkBufferCreateInfo buffer_create_info = {
          VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
          &dedicated_buffer_info,                // pNext
          0,                                     // flags
          1024,                                  // size
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
              VK_BUFFER_USAGE_TRANSFER_DST_BIT,  // usage
          VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
          0,                                     // queueFamilyIndexCount
          nullptr                                // pQueueFamilyIndices
      };
      ::VkBuffer raw_buffer;
      LOG_ASSERT(==, data->log, VK_SUCCESS,
                 device->vkCreateBuffer(device, &buffer_create_info, nullptr,
                                        &raw_buffer));
      vulkan::VkBuffer buffer(raw_buffer, nullptr, &device);

      VkMemoryRequirements memory_requirements;
      device->vkGetBufferMemoryRequirements(device, buffer,
                                            &memory_requirements);

      VkDedicatedAllocationMemoryAllocateInfoNV dedicated_allocate_info = {
          VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV,
          nullptr, VK_NULL_HANDLE, buffer};

      VkMemoryAllocateInfo memoryAllocateInfo = {
          VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // sType
          &dedicated_allocate_info,                // pNext
          memory_requirements.size,                // size
          GetMemoryIndex(&device, data->log.get(),
                         memory_requirements.memoryTypeBits, 0)};

      ::VkDeviceMemory raw_memory;
      device->vkAllocateMemory(device, &memoryAllocateInfo, nullptr,
                               &raw_memory);
      vulkan::VkDeviceMemory memory(raw_memory, nullptr, &device);

      device->vkBindBufferMemory(device, buffer, memory, 0);
    }
  } else {
    data->log->LogInfo("Disabled test due to missing ",
                       VK_NV_DEDICATED_ALLOCATION_EXTENSION_NAME);
  }
  data->log->LogInfo("Application Shutdown");
  return 0;
}
