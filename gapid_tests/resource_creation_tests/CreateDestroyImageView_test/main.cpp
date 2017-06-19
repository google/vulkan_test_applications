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
      vulkan::CreateDefaultInstance(data->root_allocator, &wrapper));

  vulkan::VkSurfaceKHR surface(vulkan::CreateDefaultSurface(&instance, data));

  {
    // 1. Test creating a image view for a swapchain image
    // prepare swapchain
    uint32_t queues[2];
    vulkan::VkDevice device(vulkan::CreateDeviceForSwapchain(
        data->root_allocator, &instance, &surface, &queues[0], &queues[1]));
    vulkan::VkSwapchainKHR swapchain(vulkan::CreateDefaultSwapchain(
        &instance, &device, &surface, allocator, queues[0], queues[1], data));

    // get images
    uint32_t num_images;

    LOG_ASSERT(==, data->log,
               device->vkGetSwapchainImagesKHR(device, swapchain, &num_images,
                                               nullptr),
               VK_SUCCESS);
    containers::vector<::VkImage> images(allocator);
    images.resize(num_images);
    LOG_EXPECT(==, data->log,
               device->vkGetSwapchainImagesKHR(device, swapchain, &num_images,
                                               images.data()),
               VK_SUCCESS);
    LOG_ASSERT(!=, data->log, images.size(), 0);

    // prepare image view create info
    VkImageViewCreateInfo image_view_create_info{
        /* sType = */ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* image = */ images.front(),  // uses the first image
        /* viewType = */ VK_IMAGE_VIEW_TYPE_2D,
        /* format = */ swapchain.format(),
        /* components = */
        {
            /* r = */ VK_COMPONENT_SWIZZLE_IDENTITY,
            /* g = */ VK_COMPONENT_SWIZZLE_IDENTITY,
            /* b = */ VK_COMPONENT_SWIZZLE_IDENTITY,
            /* a = */ VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        /* subresourceRange = */
        {
            /* aspectMask = */ VK_IMAGE_ASPECT_COLOR_BIT,
            /* baseMipLevel = */ 0,
            /* levelCount = */ 1,
            /* baseArrayLayer = */ 0,
            /* layerCount = */ 1,
        },
    };
    ::VkImageView image_view;
    LOG_EXPECT(==, data->log,
               device->vkCreateImageView(device, &image_view_create_info,
                                         nullptr, &image_view),
               VK_SUCCESS);

    device->vkDestroyImageView(device, image_view, nullptr);

    if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5bce4000)) {
      device->vkDestroyImageView(device, (VkImageView)VK_NULL_HANDLE, nullptr);
    }
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
