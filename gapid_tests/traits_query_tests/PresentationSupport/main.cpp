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
      vulkan::CreateDefaultInstance(data->allocator(), &wrapper));
  containers::vector<VkPhysicalDevice> physical_devices(
      vulkan::GetPhysicalDevices(data->allocator(), instance));

  {
    for (const auto& device : physical_devices) {
      data->logger()->LogInfo("  Phyiscal Device: ", device);
      uint32_t queue_count = 0;
      VkBool32 result = VK_FALSE;
      instance->vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count,
                                                         nullptr);
      if (queue_count == 0) {
        continue;
      }
#if defined __ANDROID__
      data->logger()->LogInfo(
          "According to Vulkan Spec, all physical devices and queue families "
          "on Android must be capable of presentation with any native window. "
          "So there is no Android-specific query for presentation support");
      break;
#elif defined __ggp__
      break;
#elif defined __linux__
      data->logger()->LogInfo(
          "API: vkGetPhysicalDeviceXcbPresentationSupportKHR");
      result = instance->vkGetPhysicalDeviceXcbPresentationSupportKHR(
          device, 0, data->native_connection(), data->native_window_handle());
#elif defined _WIN32
      data->logger()->LogInfo(
          "API: vkGetPhysicalDeviceWin32PresentationSupportKHR");
      result =
          instance->vkGetPhysicalDeviceWin32PresentationSupportKHR(device, 0);
#else
      data->logger()->LogInfo(
          "Presentation Support test not available on target OS, test "
          "skipped.");
      break;
#endif
      data->logger()->LogInfo("  Physical Device: ", device);
      data->logger()->LogInfo("    Return result: ", BoolString(result));
    }
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
