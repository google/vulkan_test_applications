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
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  auto allocator = data->allocator();
  vulkan::VulkanApplication app(data->allocator(), data->logger(), data,
                                vulkan::VulkanApplicationOptions());
  vulkan::VkDevice& device = app.device();

  {  // 1. Zero writes and zero copies.
    device->vkUpdateDescriptorSets(device, 0, nullptr, 0, nullptr);
  }

  {  // 2. One write and zero copies.
    VkDescriptorSetLayoutBinding binding{
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        2,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr,                            // pImmutableSamplers
    };
    auto set = app.AllocateDescriptorSet({binding});

    auto buffer = app.CreateAndBindDefaultExclusiveDeviceBuffer(
        1024, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    const VkDescriptorBufferInfo bufinfo[2] = {
        {
            /* buffer = */ *buffer,
            /* offset = */ 0,
            /* range = */ 512,
        },
        {
            /* buffer = */ *buffer,
            /* offset = */ 512,
            /* range = */ 512,
        },
    };

    const VkWriteDescriptorSet write = {
        /* sType = */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        /* pNext = */ nullptr,
        /* dstSet = */ set,
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
    VkDescriptorSetLayoutBinding binding{
        0,                             // binding
        VK_DESCRIPTOR_TYPE_SAMPLER,    // descriptorType
        2,                             // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,  // stageFlags
        nullptr,                       // pImmutableSamplers
    };
    auto set = app.AllocateDescriptorSet({binding});

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
            /* dstSet = */ set,
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
            /* dstSet = */ set,
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
    VkDescriptorSetLayoutBinding first_binding{
        0,                                 // binding
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // descriptorType
        1,                                 // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,      // stageFlags
        nullptr,                           // pImmutableSamplers
    };
    auto first_set = app.AllocateDescriptorSet({first_binding});

    VkDescriptorSetLayoutBinding second_binding{
        0,                                 // binding
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // descriptorType
        5,                                 // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,      // stageFlags
        nullptr,                           // pImmutableSamplers
    };
    auto second_set = app.AllocateDescriptorSet({second_binding});

    VkImageCreateInfo image_create_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                        nullptr,
                                        0,
                                        VK_IMAGE_TYPE_2D,
                                        VK_FORMAT_R8G8B8A8_UNORM,
                                        {4, 4, 1},
                                        1,
                                        1,
                                        VK_SAMPLE_COUNT_1_BIT,
                                        VK_IMAGE_TILING_OPTIMAL,
                                        VK_IMAGE_USAGE_STORAGE_BIT,
                                        VK_SHARING_MODE_EXCLUSIVE,
                                        0,
                                        nullptr,
                                        VK_IMAGE_LAYOUT_UNDEFINED};
    auto image = app.CreateAndBindImage(&image_create_info);
    VkImageViewCreateInfo image_view_create_info{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        *image,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_R8G8B8A8_UNORM,
        {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
         VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    ::VkImageView raw_image_view;
    device->vkCreateImageView(device, &image_view_create_info, nullptr,
                              &raw_image_view);
    vulkan::VkImageView image_view(raw_image_view, nullptr, &device);

    const VkDescriptorImageInfo imginfo[3] = {
        // One image info for the first descriptor set
        {
            /* sampler = */ VK_NULL_HANDLE,
            /* imageView = */ image_view,
            /* imageLayout = */ VK_IMAGE_LAYOUT_GENERAL,
        },
        // Two image infos for the second descriptor set
        {
            /* sampler = */ VK_NULL_HANDLE,
            /* imageView = */ image_view,
            /* imageLayout = */ VK_IMAGE_LAYOUT_GENERAL,
        },
        {
            /* sampler = */ VK_NULL_HANDLE,
            /* imageView = */ image_view,
            /* imageLayout = */ VK_IMAGE_LAYOUT_GENERAL,
        },
    };

    // First, we need to write two descriptors to the descriptor set.
    const VkWriteDescriptorSet writes[2] = {
        // Write to first_set binding 0, array element 0.
        {
            /* sType = */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            /* pNext = */ nullptr,
            /* dstSet = */ first_set,
            /* dstBinding = */ 0,
            /* dstArrayElement = */ 0,
            /* descriptorCount = */ 1,
            /* descriptorType = */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* pImageInfo = */ &imginfo[0],
            /* pBufferInfo = */ nullptr,
            /* pTexelBufferView = */ nullptr,
        },
        // Write to second_set binding 0, array element 1 and 2.
        {
            /* sType = */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            /* pNext = */ nullptr,
            /* dstSet = */ second_set,
            /* dstBinding = */ 0,
            /* dstArrayElement = */ 1,
            /* descriptorCount = */ 2,
            /* descriptorType = */ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            /* pImageInfo = */ &imginfo[1],
            /* pBufferInfo = */ nullptr,
            /* pTexelBufferView = */ nullptr,
        },
    };

    device->vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);

    // Then we test copying descriptors in VkUpdateDescriptorSets
    const VkCopyDescriptorSet copies[2] = {
        // Copy the only descriptor from set[0] to set[1].
        {
            /* sType = */ VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
            /* pNext = */ nullptr,
            /* srcSet = */ first_set,
            /* srcBinding = */ 0,
            /* srcArrayElement = */ 0,
            /* dstSet = */ second_set,
            /* dstBinding = */ 0,
            /* dstArrayElement = */ 0,
            /* descriptorCount = */ 1,
        },
        // Copy the 2nd & 3rd descriptors to the 4th & 5th.
        {
            /* sType = */ VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
            /* pNext = */ nullptr,
            /* srcSet = */ second_set,
            /* srcBinding = */ 0,
            /* srcArrayElement = */ 1,
            /* dstSet = */ second_set,
            /* dstBinding = */ 0,
            /* dstArrayElement = */ 3,
            /* descriptorCount = */ 2,
        },
    };

    device->vkUpdateDescriptorSets(device, 0, nullptr, 2, copies);
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
