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

uint32_t test_shader[] =
#include "test.vert.spv"
    ;

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->allocator(), data->logger());
  vulkan::VkInstance instance(
      vulkan::CreateDefaultInstance(data->allocator(), &wrapper));
  vulkan::VkDevice device(
      vulkan::CreateDefaultDevice(data->allocator(), instance, true));

  {  // Valid usage

    VkShaderModuleCreateInfo create_info{
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,  // sType
        nullptr,                                      // pNext
        0,                                            // flags
        sizeof(test_shader),                          // codeSize
        test_shader                                   // pCode
    };

    VkShaderModule shader_module;
    LOG_ASSERT(==, data->logger(),
               device->vkCreateShaderModule(device, &create_info, nullptr,
                                            &shader_module),
               VK_SUCCESS);
    device->vkDestroyShaderModule(device, shader_module, nullptr);
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
