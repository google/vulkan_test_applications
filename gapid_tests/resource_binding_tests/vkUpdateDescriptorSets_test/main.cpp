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

  {  // 1. Zero writes and zero copies.
    device->vkUpdateDescriptorSets(device, 0, nullptr, 0, nullptr);
  }

  {  // 2. One write and zero copies.
    VkDescriptorPoolSize pool_size = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2};
    vulkan::VkDescriptorPool pool =
        vulkan::CreateDescriptorPool(&device, 1, &pool_size, 1);
    ::VkDescriptorPool raw_pool = pool.get_raw_object();

    vulkan::VkDescriptorSetLayout layout = vulkan::CreateDescriptorSetLayout(
        &device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2);
    ::VkDescriptorSetLayout raw_layout = layout.get_raw_object();

    vulkan::VkDescriptorSet set =
        vulkan::AllocateDescriptorSet(&device, raw_pool, raw_layout);
    ::VkDescriptorSet raw_set = set.get_raw_object();

    const VkBufferCreateInfo info = {
        /* sType = */ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* size = */ 1024,
        /* usage = */ VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
    };

    ::VkBuffer raw_buffer;
    device->vkCreateBuffer(device, &info, nullptr, &raw_buffer);
    vulkan::VkBuffer(raw_buffer, nullptr, &device);

    const VkDescriptorBufferInfo bufinfo[2] = {
        {
            /* buffer = */ raw_buffer,
            /* offset = */ 0,
            /* range = */ 512,
        },
        {
            /* buffer = */ raw_buffer,
            /* offset = */ 512,
            /* range = */ 512,
        },
    };

    const VkWriteDescriptorSet write = {
        /* sType = */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        /* pNext = */ nullptr,
        /* dstSet = */ raw_set,
        /* dstBinding = */ 0,
        /* dstArrayElement = */ 0,
        /* descriptorCount = */ 2,
        /* descriptorType = */ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        /* pImageInfo = */ nullptr,
        /* pBufferInfo = */ bufinfo,
        /* pTexelBufferView = */ nullptr,
    };

    device->vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
  }

  {  // 3. Two writes and zero copies.
    VkDescriptorPoolSize pool_size = {VK_DESCRIPTOR_TYPE_SAMPLER, 2};
    vulkan::VkDescriptorPool pool =
        vulkan::CreateDescriptorPool(&device, 1, &pool_size, 1);
    ::VkDescriptorPool raw_pool = pool.get_raw_object();

    vulkan::VkDescriptorSetLayout layout = vulkan::CreateDescriptorSetLayout(
        &device, VK_DESCRIPTOR_TYPE_SAMPLER, 2);
    ::VkDescriptorSetLayout raw_layout = layout.get_raw_object();

    vulkan::VkDescriptorSet set =
        vulkan::AllocateDescriptorSet(&device, raw_pool, raw_layout);
    ::VkDescriptorSet raw_set = set.get_raw_object();

    vulkan::VkSampler sampler = vulkan::CreateDefaultSampler(&device);
    ::VkSampler raw_sampler = sampler.get_raw_object();

    const VkDescriptorImageInfo imginfo[2] = {
        {
            /* sampler = */ raw_sampler,
            /* imageView = */ VK_NULL_HANDLE,             // ignored
            /* imageLayout = */ VK_IMAGE_LAYOUT_GENERAL,  // ignored
        },
        {
            /* sampler = */ raw_sampler,
            /* imageView = */ VK_NULL_HANDLE,
            /* imageLayout = */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        },
    };

    const VkWriteDescriptorSet writes[2] = {
        {
            /* sType = */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            /* pNext = */ nullptr,
            /* dstSet = */ raw_set,
            /* dstBinding = */ 0,
            /* dstArrayElement = */ 0,
            /* descriptorCount = */ 1,
            /* descriptorType = */ VK_DESCRIPTOR_TYPE_SAMPLER,
            /* pImageInfo = */ &imginfo[0],
            /* pBufferInfo = */ nullptr,
            /* pTexelBufferView = */ nullptr,
        },
        {
            /* sType = */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            /* pNext = */ nullptr,
            /* dstSet = */ raw_set,
            /* dstBinding = */ 0,
            /* dstArrayElement = */ 1,
            /* descriptorCount = */ 1,
            /* descriptorType = */ VK_DESCRIPTOR_TYPE_SAMPLER,
            /* pImageInfo = */ &imginfo[1],
            /* pBufferInfo = */ nullptr,
            /* pTexelBufferView = */ nullptr,
        },
    };

    device->vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
  }

  {  // 4. Zero writes and two copies.
    VkDescriptorPoolSize pool_size = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6};
    vulkan::VkDescriptorPool pool =
        vulkan::CreateDescriptorPool(&device, 1, &pool_size, 2);
    ::VkDescriptorPool raw_pool = pool.get_raw_object();

    vulkan::VkDescriptorSetLayout layout[2] = {
        // One descriptors bound to bind number 0.
        vulkan::CreateDescriptorSetLayout(&device,
                                          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1),
        // Five descriptors bound to bind number 0.
        vulkan::CreateDescriptorSetLayout(&device,
                                          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 5),
    };

    ::VkDescriptorSetLayout raw_layout[2] = {
        layout[0].get_raw_object(), layout[1].get_raw_object(),
    };

    vulkan::VkDescriptorSet set[2] = {
        vulkan::AllocateDescriptorSet(&device, raw_pool, raw_layout[0]),
        vulkan::AllocateDescriptorSet(&device, raw_pool, raw_layout[1]),
    };
    ::VkDescriptorSet raw_set[2] = {
        set[0].get_raw_object(), set[1].get_raw_object(),
    };

    const VkCopyDescriptorSet copies[2] = {
        // Copy the only descriptor from set[0] to set[1].
        {
            /* sType = */ VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
            /* pNext = */ nullptr,
            /* srcSet = */ raw_set[0],
            /* srcBinding = */ 0,
            /* srcArrayElement = */ 0,
            /* dstSet = */ raw_set[1],
            /* dstBinding = */ 0,
            /* dstArrayElement = */ 0,
            /* descriptorCount = */ 1,
        },
        // Copy the 2nd & 3rd descriptors to the 4th & 5th.
        {
            /* sType = */ VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
            /* pNext = */ nullptr,
            /* srcSet = */ raw_set[1],
            /* srcBinding = */ 0,
            /* srcArrayElement = */ 1,
            /* dstSet = */ raw_set[1],
            /* dstBinding = */ 0,
            /* dstArrayElement = */ 3,
            /* descriptorCount = */ 2,
        },
    };

    device->vkUpdateDescriptorSets(device, 0, nullptr, 2, copies);
  }

  data->log->LogInfo("Application Shutdown");
  return 0;
}
