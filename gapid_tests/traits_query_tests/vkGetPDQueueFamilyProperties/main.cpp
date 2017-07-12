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

#include "support/containers/vector.h"
#include "support/entry/entry.h"
#include "support/log/log.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->root_allocator, data->log.get());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->root_allocator, &wrapper));
  containers::vector<VkPhysicalDevice> physical_devices(
      vulkan::GetPhysicalDevices(data->root_allocator, instance));
  const uint32_t physical_device_count = physical_devices.size();

  data->log->LogInfo("API: vkGetPhysicalDeviceQueueFamilyProperties");

  containers::vector<uint32_t> driver_counts(physical_device_count,
                                             data->root_allocator);
  {
    data->log->LogInfo("  Case: pQueueFamilyProperties == nullptr");
    for (uint32_t i = 0; i < physical_device_count; ++i) {
      const auto device = physical_devices[i];
      data->log->LogInfo("    Phyiscal Device: ", device);
      instance->vkGetPhysicalDeviceQueueFamilyProperties(
          device, &driver_counts[i], nullptr);
      data->log->LogInfo("      # queue family properties: ", driver_counts[i]);
    }
  }

  {
    data->log->LogInfo("  Case: *pQueueFamilyPropertyCount == 0");
    for (uint32_t i = 0; i < physical_device_count; ++i) {
      const auto device = physical_devices[i];
      data->log->LogInfo("    Phyiscal Device: ", device);
      uint32_t count = 0;
      VkQueueFamilyProperties properties = {};
      instance->vkGetPhysicalDeviceQueueFamilyProperties(device, &count,
                                                         &properties);
      LOG_EXPECT(==, data->log, count, 0u);
      LOG_EXPECT(==, data->log, properties.queueCount, 0u);
      LOG_EXPECT(==, data->log, properties.timestampValidBits, 0u);
    }
  }

  {
    data->log->LogInfo("  Case: *pQueueFamilyPropertyCount < capacity");
    for (uint32_t i = 0; i < physical_device_count; ++i) {
      if (driver_counts[i] <= 1) continue;

      const auto device = physical_devices[i];
      data->log->LogInfo("    Phyiscal Device: ", device);
      uint32_t count = driver_counts[i] - 1;
      containers::vector<VkQueueFamilyProperties> properties(
          count, data->root_allocator);
      instance->vkGetPhysicalDeviceQueueFamilyProperties(device, &count,
                                                         properties.data());
      LOG_EXPECT(==, data->log, count, driver_counts[i] - 1);
      for (uint32_t j = 0; j < count; ++j) {
        data->log->LogInfo("      queueCount: ", properties[j].queueCount);
      }
    }
  }

  {
    data->log->LogInfo("  Case: *pQueueFamilyPropertyCount == capacity");
    for (uint32_t i = 0; i < physical_device_count; ++i) {
      const auto device = physical_devices[i];
      data->log->LogInfo("    Phyiscal Device: ", device);
      auto count = driver_counts[i];
      containers::vector<VkQueueFamilyProperties> properties(
          count, data->root_allocator);
      instance->vkGetPhysicalDeviceQueueFamilyProperties(device, &count,
                                                         properties.data());
      LOG_EXPECT(==, data->log, properties.size(), driver_counts[i]);
      for (const auto& p : properties) {
        data->log->LogInfo("      queueCount: ", p.queueCount);
      }
    }
  }

  {
    data->log->LogInfo("  Case: *pQueueFamilyPropertyCount > capacity");
    for (uint32_t i = 0; i < physical_device_count; ++i) {
      const auto device = physical_devices[i];
      data->log->LogInfo("    Phyiscal Device: ", device);
      uint32_t count = driver_counts[i] + 3;
      containers::vector<VkQueueFamilyProperties> properties(
          count, data->root_allocator);
      instance->vkGetPhysicalDeviceQueueFamilyProperties(device, &count,
                                                         properties.data());
      LOG_EXPECT(==, data->log, count, driver_counts[i]);
      for (uint32_t j = 0; j < count; ++j) {
        data->log->LogInfo("      queueCount: ", properties[j].queueCount);
      }
    }
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
