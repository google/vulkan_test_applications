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

  {  // 1. Zero bindings.
    VkDescriptorSetLayoutCreateInfo create_info{
        /* sType = */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* bindingCount = */ 0,
        /* pBindings = */ nullptr,
    };

    ::VkDescriptorSetLayout layout;
    device->vkCreateDescriptorSetLayout(device, &create_info, nullptr, &layout);
    data->log->LogInfo("  layout: ", layout);
    device->vkDestroyDescriptorSetLayout(device, layout, nullptr);
  }

  {  // 2. Three bindings.
    VkDescriptorSetLayoutBinding bindings[3] = {
        {
            /* binding = */ 0,
            /* descriptorType = */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* descriptorCount = */ 6,
            /* stageFlags = */
            (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
            /* pImmutableSamplers = */ nullptr,
        },
        {
            /* binding = */ 2,
            /* descriptorType = */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            /* descriptorCount = */ 1,
            /* stageFlags = */ VK_SHADER_STAGE_VERTEX_BIT,
            /* pImmutableSamplers = */ nullptr,
        },
        {
            /* binding = */ 5,
            /* descriptorType = */ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            /* descriptorCount = */ 0,
            /* stageFlags = */ 0xdeadbeef,
            /* pImmutableSamplers = */ nullptr,
        },
    };

    VkDescriptorSetLayoutCreateInfo create_info{
        /* sType = */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* bindingCount = */ 3,
        /* pBindings = */ bindings,
    };

    ::VkDescriptorSetLayout layout;
    device->vkCreateDescriptorSetLayout(device, &create_info, nullptr, &layout);
    data->log->LogInfo("  layout: ", layout);
    device->vkDestroyDescriptorSetLayout(device, layout, nullptr);
  }

  {  // 3. Two bindings with samplers.
    containers::vector<vulkan::VkSampler> samplers(allocator);
    ::VkSampler raw_samplers[3];
    samplers.reserve(3);
    for (int i = 0; i < 3; ++i) {
      samplers.emplace_back(CreateDefaultSampler(&device));
      raw_samplers[i] = samplers.back().get_raw_object();
    }

    VkDescriptorSetLayoutBinding bindings[2] = {
        {
            /* binding = */ 2,
            /* descriptorType = */ VK_DESCRIPTOR_TYPE_SAMPLER,
            /* descriptorCount = */ 3,
            /* stageFlags = */ VK_SHADER_STAGE_ALL,
            /* pImmutableSamplers = */ nullptr,
        },
        {
            /* binding = */ 7,
            /* descriptorType = */ VK_DESCRIPTOR_TYPE_SAMPLER,
            /* descriptorCount = */ 3,
            /* stageFlags = */ VK_SHADER_STAGE_ALL,
            /* pImmutableSamplers = */ raw_samplers,
        },
    };

    VkDescriptorSetLayoutCreateInfo create_info{
        /* sType = */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* bindingCount = */ 2,
        /* pBindings = */ bindings,
    };

    ::VkDescriptorSetLayout layout;
    device->vkCreateDescriptorSetLayout(device, &create_info, nullptr, &layout);
    data->log->LogInfo("  layout: ", layout);
    device->vkDestroyDescriptorSetLayout(device, layout, nullptr);
  }

  {  // 4. Destroy null descriptor set layout handle.
    if (NOT_DEVICE(data->log.get(), device, vulkan::NvidiaK2200, 0x5bce4000)) {
      device->vkDestroyDescriptorSetLayout(
          device, (VkDescriptorSetLayout)VK_NULL_HANDLE, nullptr);
    }
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
