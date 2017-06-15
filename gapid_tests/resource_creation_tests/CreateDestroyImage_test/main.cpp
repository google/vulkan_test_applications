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
    // 1. Test a normal color attachment image
    VkImageCreateInfo image_create_info{
        /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_2D,
        /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
        /* extent = */
        {
            /* width = */ 32,
            /* height = */ 32,
            /* depth = */ 1,
        },
        /* mipLevels = */ 1,
        /* arrayLayers = */ 1,
        /* samples = */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling = */ VK_IMAGE_TILING_OPTIMAL,
        /* usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
    };
    ::VkImage image;
    device->vkCreateImage(device, &image_create_info, nullptr, &image);
    device->vkDestroyImage(device, image, nullptr);

    if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5DD08000)) {
      device->vkDestroyImage(device, (VkImage)VK_NULL_HANDLE, nullptr);
    }
  }

  {
    // 2. Test a normal depth image
    VkImageCreateInfo image_create_info{
        /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_2D,
        /* format = */ VK_FORMAT_D16_UNORM,
        /* extent = */
        {
            /* width = */ 32,
            /* height = */ 32,
            /* depth = */ 1,
        },
        /* mipLevels = */ 1,
        /* arrayLayers = */ 1,
        /* samples = */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling = */ VK_IMAGE_TILING_OPTIMAL,
        /* usage = */ VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
    };
    ::VkImage image;
    device->vkCreateImage(device, &image_create_info, nullptr, &image);
    device->vkDestroyImage(device, image, nullptr);

    if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5DD08000)) {
      device->vkDestroyImage(device, (VkImage)VK_NULL_HANDLE, nullptr);
    }
  }

  {
    // 3. Test a cube compatible image with mutable format support
    VkImageCreateInfo image_create_info{
        /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT |
            VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
        /* imageType = */ VK_IMAGE_TYPE_2D,
        /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
        /* extent = */
        {
            /* width = */ 32,
            /* height = */ 32,
            /* depth = */ 1,
        },
        /* mipLevels = */ 1,
        /* arrayLayers = */ 6,
        /* samples = */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling = */ VK_IMAGE_TILING_OPTIMAL,
        /* usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
    };
    ::VkImage image;
    device->vkCreateImage(device, &image_create_info, nullptr, &image);
    device->vkDestroyImage(device, image, nullptr);
    if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5DD08000)) {
      device->vkDestroyImage(device, (VkImage)VK_NULL_HANDLE, nullptr);
    }
  }
  {
    // 4. Test creating an image with linear tiling that can be used as the
    // source and destination of a transfer command.
    VkImageCreateInfo image_create_info{
        /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_2D,
        /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
        /* extent = */
        {
            /* width = */ 32,
            /* height = */ 32,
            /* depth = */ 1,
        },
        /* mipLevels = */ 1,
        /* arrayLayers = */ 1,
        /* samples = */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling = */ VK_IMAGE_TILING_LINEAR,
        /* usage = */ VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
    };
    ::VkImage image;
    device->vkCreateImage(device, &image_create_info, nullptr, &image);
    device->vkDestroyImage(device, image, nullptr);
    if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5DD08000)) {
      device->vkDestroyImage(device, (VkImage)VK_NULL_HANDLE, nullptr);
    }
  }
  {
    // 5. Test an image of 3D type with different extent values and
    // mip levels
    VkImageCreateInfo image_create_info{
        /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_3D,
        /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
        /* extent = */
        {
            /* width = */ 8,
            /* height = */ 8,
            /* depth = */ 16,
        },
        /* mipLevels = */ 5,
        /* arrayLayers = */ 1,
        /* samples = */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling = */ VK_IMAGE_TILING_OPTIMAL,
        /* usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
    };
    ::VkImage image;
    device->vkCreateImage(device, &image_create_info, nullptr, &image);
    device->vkDestroyImage(device, image, nullptr);
    if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5DD08000)) {
      device->vkDestroyImage(device, (VkImage)VK_NULL_HANDLE, nullptr);
    }
  }
  {
    // 6. Test creating an preinitialized image with multi-sample
    VkImageCreateInfo image_create_info{
        /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_2D,
        /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
        /* extent = */
        {
            /* width = */ 32,
            /* height = */ 32,
            /* depth = */ 1,
        },
        /* mipLevels = */ 1,
        /* arrayLayers = */ 1,
        /* samples = */ VK_SAMPLE_COUNT_4_BIT,
        /* tiling = */ VK_IMAGE_TILING_OPTIMAL,
        /* usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */ VK_IMAGE_LAYOUT_PREINITIALIZED,
    };
    ::VkImage image;
    device->vkCreateImage(device, &image_create_info, nullptr, &image);
    device->vkDestroyImage(device, image, nullptr);
    if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5DD08000)) {
      device->vkDestroyImage(device, (VkImage)VK_NULL_HANDLE, nullptr);
    }
  }
  {
    // 7. Test a 1D image
    VkImageCreateInfo image_create_info{
        /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_1D,
        /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
        /* extent = */
        {
            /* width = */ 32,
            /* height = */ 1,
            /* depth = */ 1,
        },
        /* mipLevels = */ 1,
        /* arrayLayers = */ 1,
        /* samples = */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling = */ VK_IMAGE_TILING_OPTIMAL,
        /* usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
    };
    ::VkImage image;
    device->vkCreateImage(device, &image_create_info, nullptr, &image);
    device->vkDestroyImage(device, image, nullptr);
    if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5DD08000)) {
      device->vkDestroyImage(device, (VkImage)VK_NULL_HANDLE, nullptr);
    }
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
