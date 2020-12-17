/* Copyright 2020 Google Inc.
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
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->allocator(), data->logger());
  vulkan::VkInstance instance(
      // vkTrimCommandPool should only be called with VK version 1.1 and greater
      vulkan::CreateEmptyInstance(data->allocator(), &wrapper,
                                  VK_MAKE_VERSION(1, 1, 0)));
  vulkan::VkDevice device(
      vulkan::CreateDefaultDevice(data->allocator(), instance));
  vulkan::VkCommandPool pool(
      vulkan::CreateDefaultCommandPool(data->allocator(), device, false));

  { device->vkTrimCommandPool(device, pool, 0); }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
