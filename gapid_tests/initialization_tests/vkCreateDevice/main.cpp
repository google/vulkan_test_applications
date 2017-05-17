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
#include "vulkan_wrapper/device_wrapper.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->root_allocator, data->log.get());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->root_allocator, &wrapper));
  containers::vector<VkPhysicalDevice> devices(
      vulkan::GetPhysicalDevices(data->root_allocator, instance),
      data->root_allocator);

  float priority = 1.f;
  VkDeviceQueueCreateInfo queue_info{
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, 0, 1, &priority};

  VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                          nullptr,
                          0,
                          0,        // Start with 0 queues
                          nullptr,  // we will update this in the next test.
                          0,
                          nullptr,
                          0,
                          nullptr,
                          nullptr};

  {
    ::VkDevice raw_device;
    LOG_EXPECT(==, data->log, (*instance).vkCreateDevice(devices[0], &info,
                                                         nullptr, &raw_device),
               VK_SUCCESS);

    vulkan::VkDevice device(data->root_allocator, raw_device, nullptr,
                            &instance);
  }

  {
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queue_info;
    ::VkDevice raw_device;
    LOG_EXPECT(==, data->log, instance->vkCreateDevice(devices[0], &info,
                                                       nullptr, &raw_device),
               VK_SUCCESS);

    vulkan::VkDevice device(data->root_allocator, raw_device, nullptr,
                            &instance);
  }
  data->log->LogInfo("Application Shutdown");
  return 0;
}
