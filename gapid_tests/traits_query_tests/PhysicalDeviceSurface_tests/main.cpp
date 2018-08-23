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
#include "vulkan_helpers/structs.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->allocator(), data->logger());
  vulkan::VkInstance instance(
      vulkan::CreateDefaultInstance(data->allocator(), &wrapper));

  vulkan::VkSurfaceKHR surface(vulkan::CreateDefaultSurface(&instance, data));
  containers::vector<VkPhysicalDevice> devices =
      vulkan::GetPhysicalDevices(data->allocator(), instance);

  for (auto device : devices) {
    VkPhysicalDeviceProperties device_properties;
    instance->vkGetPhysicalDeviceProperties(device, &device_properties);

    data->logger()->LogInfo("Physical Device Surfaces for ",
                       ::VkPhysicalDevice(device));
    auto properties =
        GetQueueFamilyProperties(data->allocator(), instance, device);
    bool device_supports = false;

    for (size_t i = 0; i < properties.size(); ++i) {
      VkBool32 supported = false;
      LOG_EXPECT(==, data->logger(), instance->vkGetPhysicalDeviceSurfaceSupportKHR(
                                    device, i, surface, &supported),
                 VK_SUCCESS);
      if (supported) {
        data->logger()->LogInfo("  Supports surfaces on queue ", i);
      } else {
        data->logger()->LogInfo("  Does not support surfaces on queue ", i);
      }
      device_supports |= (supported != 0);
    }
    if (device_supports) {
      VkSurfaceCapabilitiesKHR surface_caps;
      LOG_ASSERT(==, data->logger(),
                 instance->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                     device, surface, &surface_caps),
                 VK_SUCCESS);
      data->logger()->LogInfo("  Capabilities: ");
      data->logger()->LogInfo("    minImageCount: ", surface_caps.minImageCount);
      data->logger()->LogInfo("    maxImageCount: ", surface_caps.maxImageCount);
      data->logger()->LogInfo("    currentExtent: [",
                         surface_caps.currentExtent.width, ",",
                         surface_caps.currentExtent.height, "]");

      uint32_t num_formats = 0;
      LOG_EXPECT(==, data->logger(), instance->vkGetPhysicalDeviceSurfaceFormatsKHR(
                                    device, surface, &num_formats, nullptr),
                 VK_SUCCESS);

      LOG_EXPECT(>, data->logger(), num_formats, 0u);

      containers::vector<VkSurfaceFormatKHR> surface_formats(
          data->allocator());
      surface_formats.resize(num_formats);

      LOG_EXPECT(==, data->logger(),
                 instance->vkGetPhysicalDeviceSurfaceFormatsKHR(
                     device, surface, &num_formats, surface_formats.data()),
                 VK_SUCCESS);
      data->logger()->LogInfo("    Formats[", num_formats, "]:");
      for (auto format : surface_formats) {
        data->logger()->LogInfo("      ", format.format, ":", format.colorSpace);
      }
      if (num_formats > 1) {
        num_formats -= 1;
        const uint32_t expected_num_formats = num_formats;
        LOG_EXPECT(==, data->logger(),
                   instance->vkGetPhysicalDeviceSurfaceFormatsKHR(
                       device, surface, &num_formats, surface_formats.data()),
                   VK_INCOMPLETE);
        LOG_EXPECT(==, data->logger(), expected_num_formats, num_formats);
        LOG_EXPECT(!=, data->logger(), (size_t)surface_formats.data(),
                   (size_t) nullptr);
      }

      uint32_t num_present_modes = 0;
      LOG_EXPECT(==, data->logger(),
                 instance->vkGetPhysicalDeviceSurfacePresentModesKHR(
                     device, surface, &num_present_modes, nullptr),
                 VK_SUCCESS);

      containers::vector<VkPresentModeKHR> present_modes(data->allocator());
      present_modes.resize(num_present_modes);

      LOG_EXPECT(==, data->logger(),
                 instance->vkGetPhysicalDeviceSurfacePresentModesKHR(
                     device, surface, &num_present_modes, present_modes.data()),
                 VK_SUCCESS);

      if (num_present_modes > 1) {
        num_present_modes -= 1;
        uint32_t expected_num_present_modes = num_present_modes;
        LOG_EXPECT(
            ==, data->logger(),
            instance->vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, surface, &num_present_modes, present_modes.data()),
            VK_INCOMPLETE);
        LOG_EXPECT(==, data->logger(), expected_num_present_modes,
                   num_present_modes);
      }
    }
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
