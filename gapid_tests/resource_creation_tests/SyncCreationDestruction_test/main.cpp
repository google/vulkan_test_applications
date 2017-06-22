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
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");

  auto& allocator = data->root_allocator;
  vulkan::LibraryWrapper wrapper(allocator, data->log.get());
  vulkan::VkInstance instance(vulkan::CreateEmptyInstance(allocator, &wrapper));
  vulkan::VkDevice device(vulkan::CreateDefaultDevice(allocator, instance));

  VkSemaphoreCreateInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 0, 0};
  VkSemaphore semaphore;
  device->vkCreateSemaphore(device, &info, nullptr, &semaphore);

  device->vkDestroySemaphore(device, semaphore, nullptr);

  data->log->LogInfo("Application Shutdown");
  return 0;
}
