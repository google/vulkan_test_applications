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

  {  // 1. One descriptor set.
    VkDescriptorPoolSize pool_size = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3};
    vulkan::VkDescriptorPool pool =
        CreateDescriptorPool(&device, 1, &pool_size, 1);
    ::VkDescriptorPool raw_pool = pool.get_raw_object();

    vulkan::VkDescriptorSetLayout layout =
        CreateDescriptorSetLayout(&device, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3);
    ::VkDescriptorSetLayout raw_layout = layout.get_raw_object();

    VkDescriptorSetAllocateInfo alloc_info{
        /* sType = */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        /* pNext = */ nullptr,
        /* descriptorPool = */ raw_pool,
        /* descriptorSetCount = */ 1,
        /* pSetLayouts = */ &raw_layout,
    };
    ::VkDescriptorSet set;
    device->vkAllocateDescriptorSets(device, &alloc_info, &set);
    data->log->LogInfo("  descriptor set: ", set);
    device->vkFreeDescriptorSets(device, raw_pool, 1, &set);
  }

  {  // 2. Three descriptor sets.
    const uint32_t kNumSets = 3;

    VkDescriptorPoolSize pool_size = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                      kNumSets};
    vulkan::VkDescriptorPool pool =
        CreateDescriptorPool(&device, 1, &pool_size, kNumSets);
    ::VkDescriptorPool raw_pool = pool.get_raw_object();

    vulkan::VkDescriptorSetLayout layout = CreateDescriptorSetLayout(
        &device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
    ::VkDescriptorSetLayout raw_layout = layout.get_raw_object();
    ::VkDescriptorSetLayout raw_layouts[kNumSets] = {raw_layout, raw_layout,
                                                     raw_layout};

    VkDescriptorSetAllocateInfo alloc_info{
        /* sType = */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        /* pNext = */ nullptr,
        /* descriptorPool = */ raw_pool,
        /* descriptorSetCount = */ kNumSets,
        /* pSetLayouts = */ raw_layouts,
    };
    ::VkDescriptorSet sets[kNumSets];
    device->vkAllocateDescriptorSets(device, &alloc_info,
                                     (::VkDescriptorSet*)sets);
    for (int i = 0; i < kNumSets; ++i) {
      data->log->LogInfo("  descriptor set: ", sets[i]);
    }
    device->vkFreeDescriptorSets(device, raw_pool, kNumSets, sets);
  }

  {  // 3. Destroy null descriptor set handles.
    VkDescriptorPoolSize pool_size = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1};
    vulkan::VkDescriptorPool pool =
        CreateDescriptorPool(&device, 1, &pool_size, 1);
    ::VkDescriptorPool raw_pool = pool.get_raw_object();

    ::VkDescriptorSet sets[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    device->vkFreeDescriptorSets(device, raw_pool, 2, sets);
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
