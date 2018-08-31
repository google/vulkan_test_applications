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

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->allocator(), data->logger());
  vulkan::VkInstance instance(
      vulkan::CreateDefaultInstance(data->allocator(), &wrapper));
  vulkan::VkSurfaceKHR surface(vulkan::CreateDefaultSurface(&instance, data));

  uint32_t queues[2];
  vulkan::VkDevice device(vulkan::CreateDeviceForSwapchain(
      data->allocator(), &instance, &surface, &queues[0], &queues[1]));
  vulkan::VkSwapchainKHR swapchain(vulkan::CreateDefaultSwapchain(
      &instance, &device, &surface, data->allocator(), queues[0], queues[1],
      data));

  uint32_t num_images;

  LOG_ASSERT(
      ==, data->logger(),
      device->vkGetSwapchainImagesKHR(device, swapchain, &num_images, nullptr),
      VK_SUCCESS);
  containers::vector<VkImage> images(data->allocator());
  images.resize(num_images);
  LOG_EXPECT(==, data->logger(),
             device->vkGetSwapchainImagesKHR(device, swapchain, &num_images,
                                             images.data()),
             VK_SUCCESS);

  if (num_images > 1) {
    num_images -= 1;
    LOG_EXPECT(==, data->logger(),
               device->vkGetSwapchainImagesKHR(device, swapchain, &num_images,
                                               images.data()),
               VK_INCOMPLETE);
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
