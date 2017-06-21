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
  vulkan::LibraryWrapper wrapper(data->root_allocator, data->log.get());
  vulkan::VkInstance instance(
      vulkan::CreateDefaultInstance(data->root_allocator, &wrapper));
  vulkan::VkSurfaceKHR surface(vulkan::CreateDefaultSurface(&instance, data));

  uint32_t queues[2];
  vulkan::VkDevice device(vulkan::CreateDeviceForSwapchain(
      data->root_allocator, &instance, &surface, &queues[0], &queues[1]));
  bool has_multiple_queues = queues[0] != queues[1];

  VkSurfaceCapabilitiesKHR surface_caps;
  LOG_ASSERT(==, data->log,
             instance->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                 device.physical_device(), surface, &surface_caps),
             VK_SUCCESS);

  uint32_t num_formats = 0;
  LOG_ASSERT(==, data->log,
             instance->vkGetPhysicalDeviceSurfaceFormatsKHR(
                 device.physical_device(), surface, &num_formats, nullptr),
             VK_SUCCESS);

  containers::vector<VkSurfaceFormatKHR> surface_formats(data->root_allocator);
  surface_formats.resize(num_formats);
  LOG_ASSERT(==, data->log,
             instance->vkGetPhysicalDeviceSurfaceFormatsKHR(
                 device.physical_device(), surface, &num_formats,
                 surface_formats.data()),
             VK_SUCCESS);

  uint32_t num_present_modes = 0;

  LOG_ASSERT(
      ==, data->log,
      instance->vkGetPhysicalDeviceSurfacePresentModesKHR(
          device.physical_device(), surface, &num_present_modes, nullptr),
      VK_SUCCESS);
  containers::vector<VkPresentModeKHR> present_modes(data->root_allocator);
  present_modes.resize(num_present_modes);
  LOG_ASSERT(==, data->log,
             instance->vkGetPhysicalDeviceSurfacePresentModesKHR(
                 device.physical_device(), surface, &num_present_modes,
                 present_modes.data()),
             VK_SUCCESS);

  data->log->LogInfo("Created device for rendering to a swapchain");
  data->log->LogInfo("   Graphics Queue: ", queues[0]);
  data->log->LogInfo("   Present Queue: ", queues[1]);

  uint32_t chosenAlpha =
      static_cast<uint32_t>(surface_caps.supportedCompositeAlpha);

  chosenAlpha = vulkan::GetLSB(chosenAlpha);

  VkExtent2D image_extent(surface_caps.currentExtent);
  if (image_extent.width == 0xFFFFFFFF) {
    image_extent = VkExtent2D{100, 100};
  }

  VkSwapchainCreateInfoKHR swapchainCreateInfo{
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,  // sType
      nullptr,                                      // pNext
      0,                                            // flags
      surface,                                      // surface
      surface_caps.minImageCount,                   // minImageCount
      surface_formats[0].format,                    // surfaceFormat
      surface_formats[0].colorSpace,                // colorSpace
      image_extent,                                 // imageExtent
      1,                                            // imageArrayLayers
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,          // imageUsage
      has_multiple_queues ? VK_SHARING_MODE_CONCURRENT
                          : VK_SHARING_MODE_EXCLUSIVE,  // sharingMode
      has_multiple_queues ? 2u : 0u,
      has_multiple_queues ? queues : nullptr,  // pQueueFamilyIndices
      surface_caps.currentTransform,           // preTransform,
      static_cast<VkCompositeAlphaFlagBitsKHR>(chosenAlpha),  // compositeAlpha
      present_modes.front(),                                  // presentModes
      false,                                                  // clipped
      VK_NULL_HANDLE                                          // oldSwapchain
  };

  VkSwapchainKHR swapchain;

  LOG_ASSERT(==, data->log,
             device->vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr,
                                          &swapchain),
             VK_SUCCESS);

  device->vkDestroySwapchainKHR(device, swapchain, nullptr);

  data->log->LogInfo("Device ID: ", device.device_id());
  data->log->LogInfo("Vendor ID: ", device.vendor_id());
  data->log->LogInfo("driver version: ", device.driver_version());

  data->log->LogInfo("Application Shutdown");
  return 0;
}
