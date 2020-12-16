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

inline const char* BoolString(bool value) { return value ? "true" : "false"; }
inline const char* BoolString(VkBool32 value) {
  return value != 0 ? "true" : "false";
}

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->allocator(), data->logger());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->allocator(), &wrapper));
  containers::vector<VkPhysicalDevice> physical_devices(
      vulkan::GetPhysicalDevices(data->allocator(), instance));

  {
    data->logger()->LogInfo("API: vkGetPhysicalDeviceFeatures");
    VkPhysicalDeviceFeatures features;
    for (const auto& device : physical_devices) {
      data->logger()->LogInfo("  Phyiscal Device: ", device);
      instance->vkGetPhysicalDeviceFeatures(device, &features);
      data->logger()->LogInfo("    shaderInt16: ",
                              BoolString(features.shaderInt16));
      data->logger()->LogInfo("    shaderInt64: ",
                              BoolString(features.shaderInt64));
      data->logger()->LogInfo("    logicOp: ", BoolString(features.logicOp));

      // Test helper function: vulkan::SupportRequestPhysicalDeviceFeatures().
      // All the supported features returned before should still be considered
      // as "supported" by SupportRequestPhysicalDeviceFeatures().
      LOG_EXPECT(==, data->logger(), true,
                 vulkan::SupportRequestPhysicalDeviceFeatures(&instance, device,
                                                              features));
    }
  }

  {
    data->logger()->LogInfo("API: vkGetPhysicalDeviceMemoryProperties");
    VkPhysicalDeviceMemoryProperties properties;
    for (const auto& device : physical_devices) {
      data->logger()->LogInfo("  Phyiscal Device: ", device);
      instance->vkGetPhysicalDeviceMemoryProperties(device, &properties);
      data->logger()->LogInfo("    # memory types: ",
                              properties.memoryTypeCount);
      data->logger()->LogInfo("    # memory heaps: ",
                              properties.memoryHeapCount);
    }
  }

  {
    data->logger()->LogInfo("API: vkGetPhysicalDeviceProperties");
    VkPhysicalDeviceProperties properties;
    for (const auto& device : physical_devices) {
      data->logger()->LogInfo("  Phyiscal Device: ", device);
      instance->vkGetPhysicalDeviceProperties(device, &properties);
      data->logger()->LogInfo("    apiVersion: ", properties.apiVersion);
      data->logger()->LogInfo("    driverVersion: ", properties.driverVersion);
      data->logger()->LogInfo("    vendorID: ", properties.vendorID);
      data->logger()->LogInfo("    deviceID: ", properties.deviceID);
      data->logger()->LogInfo("    deviceName: ", properties.deviceName);
    }
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
