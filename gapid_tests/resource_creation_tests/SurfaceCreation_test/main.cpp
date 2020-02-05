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
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  auto allocator = data->allocator();
  vulkan::LibraryWrapper wrapper(allocator, data->logger());
  vulkan::VkInstance instance(
      vulkan::CreateDefaultInstance(allocator, &wrapper));
  VkSurfaceKHR surface;

  data->logger()->LogInfo("Instance: ", (VkInstance)instance);

#if defined __ANDROID__
  VkAndroidSurfaceCreateInfoKHR create_info{
      VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, 0, 0,
      data->native_window_handle()};

  instance->vkCreateAndroidSurfaceKHR(instance, &create_info, nullptr,
                                      &surface);
#elif defined __ggp__ // Keep this line before __linux__
  VkStreamDescriptorSurfaceCreateInfoGGP create_info{
      VK_STRUCTURE_TYPE_STREAM_DESCRIPTOR_SURFACE_CREATE_INFO_GGP, nullptr, 1};

  instance->vkCreateStreamDescriptorSurfaceGGP(instance, &create_info, nullptr,
                                      &surface);
#elif defined __linux__
  VkXcbSurfaceCreateInfoKHR create_info{
      VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR, 0, 0,
      data->native_connection(), data->native_window_handle()};

  instance->vkCreateXcbSurfaceKHR(instance, &create_info, nullptr, &surface);
#elif defined _WIN32
  VkWin32SurfaceCreateInfoKHR create_info{
      VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, 0, 0,
      data->native_hinstance(), data->native_window_handle()};

  instance->vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &surface);
#endif

  // TODO(awoloszyn): Other platforms

  instance->vkDestroySurfaceKHR(instance, surface, nullptr);

  instance->vkDestroySurfaceKHR(
      instance, static_cast<VkSurfaceKHR>(VK_NULL_HANDLE), nullptr);
  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
