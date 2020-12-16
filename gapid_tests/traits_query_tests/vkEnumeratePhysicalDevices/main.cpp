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

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->allocator(), data->logger());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->allocator(), &wrapper));

  uint32_t device_count = 0;

  LOG_EXPECT(
      ==, data->logger(),
      instance->vkEnumeratePhysicalDevices(instance, &device_count, nullptr),
      VK_SUCCESS);

  LOG_ASSERT(>, data->logger(), device_count, 0u);
  data->logger()->LogInfo("Device Count is ", device_count);

  containers::vector<VkPhysicalDevice> physical_devices(data->allocator());
  physical_devices.resize(device_count);
  LOG_ASSERT(==, data->logger(),
             instance->vkEnumeratePhysicalDevices(instance, &device_count,
                                                  physical_devices.data()),
             VK_SUCCESS);

  for (size_t i = 0; i < device_count; ++i) {
    LOG_ASSERT(!=, data->logger(), physical_devices[i],
               VkPhysicalDevice(nullptr));
  }

  device_count -= 1;
  LOG_EXPECT(==, data->logger(),
             instance->vkEnumeratePhysicalDevices(instance, &device_count,
                                                  physical_devices.data()),
             VK_INCOMPLETE);

  device_count = 0;
  LOG_EXPECT(==, data->logger(),
             instance->vkEnumeratePhysicalDevices(instance, &device_count,
                                                  physical_devices.data()),
             VK_INCOMPLETE);

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
