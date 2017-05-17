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

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->root_allocator, data->log.get());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->root_allocator, &wrapper));
  containers::vector<VkPhysicalDevice> physical_devices(
      vulkan::GetPhysicalDevices(data->root_allocator, instance));

  {
    data->log->LogInfo("API: vkGetPhysicalDeviceFeatures");
    VkPhysicalDeviceFeatures features;
    for (const auto& device : physical_devices) {
      data->log->LogInfo("  Phyiscal Device: ", device);
      instance->vkGetPhysicalDeviceFeatures(device, &features);
      data->log->LogInfo("    shaderInt16: ", BoolString(features.shaderInt16));
      data->log->LogInfo("    shaderInt64: ", BoolString(features.shaderInt64));
      data->log->LogInfo("    logicOp: ", BoolString(features.logicOp));

      // Test helper function: vulkan::SupportRequestPhysicalDeviceFeatures().
      // All the supported features returned before should still be considered
      // as "supported" by SupportRequestPhysicalDeviceFeatures().
      LOG_EXPECT(==, data->log, true,
                 vulkan::SupportRequestPhysicalDeviceFeatures(&instance, device,
                                                              features));
    }
  }

  {
    data->log->LogInfo("API: vkGetPhysicalDeviceMemoryProperties");
    VkPhysicalDeviceMemoryProperties properties;
    for (const auto& device : physical_devices) {
      data->log->LogInfo("  Phyiscal Device: ", device);
      instance->vkGetPhysicalDeviceMemoryProperties(device, &properties);
      data->log->LogInfo("    # memory types: ", properties.memoryTypeCount);
      data->log->LogInfo("    # memory heaps: ", properties.memoryHeapCount);
    }
  }

  {
    data->log->LogInfo("API: vkGetPhysicalDeviceProperties");
    VkPhysicalDeviceProperties properties;
    for (const auto& device : physical_devices) {
      data->log->LogInfo("  Phyiscal Device: ", device);
      instance->vkGetPhysicalDeviceProperties(device, &properties);
      data->log->LogInfo("    apiVersion: ", properties.apiVersion);
      data->log->LogInfo("    driverVersion: ", properties.driverVersion);
      data->log->LogInfo("    vendorID: ", properties.vendorID);
      data->log->LogInfo("    deviceID: ", properties.deviceID);
      data->log->LogInfo("    deviceName: ", properties.deviceName);
    }
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
