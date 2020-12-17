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
      vulkan::CreateEmptyInstance(data->allocator(), &wrapper));
  containers::vector<VkPhysicalDevice> physical_devices(
      vulkan::GetPhysicalDevices(data->allocator(), instance));
  const uint32_t physical_device_count = physical_devices.size();

  data->logger()->LogInfo("API: vkGetPhysicalDeviceFormatProperties");

  auto output_properties = [data](const VkFormatProperties& properties) {
    data->logger()->LogInfo("      linearTilingFeatures: ",
                            properties.linearTilingFeatures);
    data->logger()->LogInfo("      optimalTilingFeatures: ",
                            properties.optimalTilingFeatures);
    data->logger()->LogInfo("      bufferFeatures: ",
                            properties.bufferFeatures);
  };

  {
    for (const auto& device : physical_devices) {
      data->logger()->LogInfo("  Phyiscal Device: ", device);

      VkFormatProperties properties;
      for (auto format : vulkan::AllVkFormats(data->allocator())) {
        data->logger()->LogInfo("    VkFormat: ", format);
        instance->vkGetPhysicalDeviceFormatProperties(device, format,
                                                      &properties);
        output_properties(properties);
      }
    }
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
