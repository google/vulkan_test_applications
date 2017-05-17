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
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");

  auto& allocator = data->root_allocator;
  vulkan::LibraryWrapper wrapper(allocator, data->log.get());
  vulkan::VkInstance instance(vulkan::CreateEmptyInstance(allocator, &wrapper));
  vulkan::VkDevice device(vulkan::CreateDefaultDevice(allocator, instance));

  {  // 1. No create flags, max two sets, one descriptor type
    VkDescriptorPoolSize pool_size{
        /* type = */ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        /* descriptorCount = */ 1,
    };
    VkDescriptorPoolCreateInfo create_info{
        /* sType = */ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* maxSets = */ 1,
        /* poolSizeCount = */ 1,
        /* pPoolSizes = */ &pool_size};

    ::VkDescriptorPool pool;
    device->vkCreateDescriptorPool(device, &create_info, nullptr, &pool);
    data->log->LogInfo("  pool: ", pool);
    device->vkResetDescriptorPool(device, pool, 0);
    device->vkDestroyDescriptorPool(device, pool, nullptr);
  }

  {  // 2. With create flags, max ten sets, more descriptor types
    VkDescriptorPoolSize pool_size[3] = {
        {
            /* type = */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* descriptorCount = */ 42,
        },
        {
            /* type = */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            /* descriptorCount = */ 5,
        },
        {
            /* type = */ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            /* descriptorCount = */ 8,
        },
    };
    VkDescriptorPoolCreateInfo create_info{
        /* sType = */ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        /* maxSets = */ 10,
        /* poolSizeCount = */ 3,
        /* pPoolSizes = */ pool_size};

    ::VkDescriptorPool pool;
    device->vkCreateDescriptorPool(device, &create_info, nullptr, &pool);
    data->log->LogInfo("  pool: ", pool);
    device->vkResetDescriptorPool(device, pool, 0);
    device->vkDestroyDescriptorPool(device, pool, nullptr);
  }

  {  // 3. Destroy null sampler handle.
    device->vkDestroyDescriptorPool(device, (VkDescriptorPool)VK_NULL_HANDLE,
                                    nullptr);
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
