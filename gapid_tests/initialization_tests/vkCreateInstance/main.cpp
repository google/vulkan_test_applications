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
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->root_allocator, data->log.get());

  {
    // Test a nullptr pApplicationInfo
    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                              nullptr,
                              0,
                              nullptr,
                              0,
                              nullptr,
                              0,
                              nullptr};

    VkInstance raw_instance;
    wrapper.vkCreateInstance(&info, nullptr, &raw_instance);
    // vulkan::VkInstance will handle destroying the instance
    vulkan::VkInstance instance(data->root_allocator, raw_instance, nullptr,
                                &wrapper);
  }

  {
    // Test a non-nullptr pApplicationInfo
    VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO,
                               nullptr,
                               "Application",
                               1,
                               "Engine",
                               0,
                               VK_MAKE_VERSION(1, 0, 0)};

    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                              nullptr,
                              0,
                              &app_info,
                              0,
                              nullptr,
                              0,
                              nullptr};

    VkInstance raw_instance;
    wrapper.vkCreateInstance(&info, nullptr, &raw_instance);
    // vulkan::VkInstance will handle destroying the instance
    vulkan::VkInstance instance(data->root_allocator, raw_instance, nullptr,
                                &wrapper);
  }
  data->log->LogInfo("Application Shutdown");
  return 0;
}
