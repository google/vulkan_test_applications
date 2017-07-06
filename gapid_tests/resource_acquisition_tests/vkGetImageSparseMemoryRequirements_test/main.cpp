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

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");

  auto& allocator = data->root_allocator;
  vulkan::LibraryWrapper wrapper(allocator, data->log.get());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->root_allocator, &wrapper));
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

    uint32_t count = 0;
    device->vkGetImageSparseMemoryRequirements(device, image, &count, nullptr);
    data->log->LogInfo("  SparseMemoryRequirementCount: ", count);
    containers::vector<VkSparseImageMemoryRequirements> requirements(
        count, {}, data->root_allocator);
    device->vkGetImageSparseMemoryRequirements(device, image, &count,
                                               requirements.data());
    for (uint32_t i = 0; i < count; i++) {
      data->log->LogInfo("  Memory Requirement: ", i);
      data->log->LogInfo(
          "  formatProperties.imageGranularity.width: ",
          requirements[i].formatProperties.imageGranularity.width);
      data->log->LogInfo(
          "  formatProperties.imageGranularity.height: ",
          requirements[i].formatProperties.imageGranularity.height);
      data->log->LogInfo(
          "  formatProperties.imageGranularity.depth: ",
          requirements[i].formatProperties.imageGranularity.depth);
      data->log->LogInfo("    imageMipTailFirstLod: ", requirements[i].imageMipTailFirstLod);
      data->log->LogInfo("        imageMipTailSize: ", requirements[i].imageMipTailSize);
      data->log->LogInfo("      imageMipTailOffset: ", requirements[i].imageMipTailOffset);
      data->log->LogInfo("      imageMipTailStride: ", requirements[i].imageMipTailStride);
    }
  }
  data->log->LogInfo("Application Shutdown");
  return 0;
}
