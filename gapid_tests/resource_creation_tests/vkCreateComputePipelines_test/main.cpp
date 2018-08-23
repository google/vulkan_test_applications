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
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"

const uint32_t kComputeShader[] =
#include "double_numbers.comp.spv"
    ;

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  auto alloc = data->allocator();

  vulkan::LibraryWrapper wrapper(alloc, data->logger());
  vulkan::VkInstance instance(vulkan::CreateDefaultInstance(alloc, &wrapper));
  vulkan::VkDevice device(vulkan::CreateDefaultDevice(alloc, instance, true));
  const ::VkPhysicalDevice pdev = device.physical_device();
  const uint32_t queue_index =
      GetGraphicsAndComputeQueueFamily(alloc, instance, pdev);
  vulkan::VkQueue queue(GetQueue(&device, queue_index));

  // Query memory properties and allocate memory.

  const uint32_t kNumElements = 512;
  const uint32_t buffer_size = kNumElements * sizeof(uint32_t);
  const ::VkDeviceSize memory_size = buffer_size * 2;

  ::VkPhysicalDeviceMemoryProperties properties;
  instance->vkGetPhysicalDeviceMemoryProperties(pdev, &properties);

  uint32_t memory_type_index = VK_MAX_MEMORY_TYPES;
  for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
    const auto& current_type = properties.memoryTypes[i];
    if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & current_type.propertyFlags) &&
        (memory_size < properties.memoryHeaps[current_type.heapIndex].size)) {
      memory_type_index = i;
      break;
    }
  }
  LOG_ASSERT(!=, data->logger(), VK_MAX_MEMORY_TYPES, memory_type_index);

  vulkan::VkDeviceMemory memory(
      AllocateDeviceMemory(&device, memory_type_index, memory_size));

  // Populate the memory with data.

  void* payload = nullptr;
  LOG_ASSERT(==, data->logger(), VK_SUCCESS,
             device->vkMapMemory(device, memory, 0, memory_size, 0, &payload));
  for (uint32_t i = 0; i < kNumElements; ++i) {
    // Only fill the first half. Zero out the second half. The second half will
    // be used to store the result from the compute shader.
    static_cast<uint32_t*>(payload)[i] = 42;
    static_cast<uint32_t*>(payload)[i + kNumElements] = 0;
  }
  device->vkUnmapMemory(device, memory);

  // Create two consecutive buffers from the memory.

  const ::VkBufferCreateInfo buf_create_info = {
      /* sType = */ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* size = */ buffer_size,
      /* usage = */ VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
      /* queueFamilyIndexCount = */ 1,
      /* pQueueFamilyIndices = */ &queue_index,
  };
  ::VkBuffer raw_in_buffer;
  LOG_ASSERT(==, data->logger(), VK_SUCCESS,
             device->vkCreateBuffer(device, &buf_create_info, nullptr,
                                    &raw_in_buffer));
  vulkan::VkBuffer in_buffer(raw_in_buffer, nullptr, &device);
  LOG_ASSERT(==, data->logger(), VK_SUCCESS,
             device->vkBindBufferMemory(device, raw_in_buffer, memory, 0));
  ::VkBuffer raw_out_buffer;
  LOG_ASSERT(==, data->logger(), VK_SUCCESS,
             device->vkCreateBuffer(device, &buf_create_info, nullptr,
                                    &raw_out_buffer));
  vulkan::VkBuffer out_buffer(raw_out_buffer, nullptr, &device);
  LOG_ASSERT(
      ==, data->logger(), VK_SUCCESS,
      device->vkBindBufferMemory(device, raw_out_buffer, memory, buffer_size));

  // Create descriptor set and pipeline layout.

  vulkan::VkDescriptorSetLayout dset_layout(CreateDescriptorSetLayout(
      alloc, &device, {
                          {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                           VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
                          {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                           VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
                      }));
  ::VkDescriptorSetLayout raw_dset_layout = dset_layout.get_raw_object();

  const ::VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
      /* sType = */ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* setLayoutCount = */ 1,
      /* pSetLayouts */ &raw_dset_layout,
      /* pushConstantRangeCount = */ 0,
      /* pPushConstantRanges = */ nullptr,
  };
  ::VkPipelineLayout raw_pipeline_layout;
  LOG_ASSERT(
      ==, data->logger(), VK_SUCCESS,
      device->vkCreatePipelineLayout(device, &pipeline_layout_create_info,
                                     nullptr, &raw_pipeline_layout));
  vulkan::VkPipelineLayout pipeline_layout(raw_pipeline_layout, nullptr,
                                           &device);

  // Create shader module and compute pipeline.

  vulkan::VkShaderModule shader_module(
      CreateShaderModule(&device, kComputeShader));

  const ::VkComputePipelineCreateInfo compute_pipeline_create_info = {
      /* sType = */ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* stage = */ {
          /* sType = */ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          /* pNext = */ nullptr,
          /* flags = */ 0,
          /* stage = */ VK_SHADER_STAGE_COMPUTE_BIT,
          /* module = */ shader_module.get_raw_object(),
          /* pName = */ "main",
          /* pSpecializationInfo = */ nullptr,
      },
      /* layout = */ raw_pipeline_layout,
      /* basePipelineHandle = */ VK_NULL_HANDLE,
      /* basePipelineIndex = */ 0,
  };
  ::VkPipeline raw_pipeline;
  LOG_ASSERT(==, data->logger(), VK_SUCCESS,
             device->vkCreateComputePipelines(
                 device, static_cast<::VkPipelineCache>(VK_NULL_HANDLE), 1,
                 &compute_pipeline_create_info, nullptr, &raw_pipeline));
  vulkan::VkPipeline pipeline(raw_pipeline, nullptr, &device);

  // Create descriptor pool and allocate and update descriptor set.

  const ::VkDescriptorPoolSize descriptor_pool_size = {
      /* type = */ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      /* descriptorCount = */ 2,
  };
  vulkan::VkDescriptorPool descriptor_pool(
      CreateDescriptorPool(&device, 1, &descriptor_pool_size, 1));
  vulkan::VkDescriptorSet dset(
      AllocateDescriptorSet(&device, descriptor_pool, dset_layout));
  ::VkDescriptorSet raw_dset = dset.get_raw_object();

  const ::VkDescriptorBufferInfo in_buffer_info = {
      /* buffer = */ raw_in_buffer,
      /* offset = */ 0,
      /* range = */ VK_WHOLE_SIZE,
  };
  const ::VkDescriptorBufferInfo out_buffer_info = {
      /* buffer = */ raw_out_buffer,
      /* offset = */ 0,
      /* range = */ VK_WHOLE_SIZE,
  };
  const ::VkWriteDescriptorSet write_descriptor_set[2] = {
      {
          /* sType = */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          /* pNext = */ nullptr,
          /* dstSet = */ raw_dset,
          /* dstBinding = */ 0,
          /* dstArrayElement = */ 0,
          /* descriptorCount = */ 1,
          /* descriptorType = */ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          /* pImageInfo = */ nullptr,
          /* pBufferInfo = */ &in_buffer_info,
          /* pTexelBufferView = */ nullptr,
      },
      {
          /* sType = */ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          /* pNext = */ nullptr,
          /* dstSet = */ raw_dset,
          /* dstBinding = */ 1,
          /* dstArrayElement = */ 0,
          /* descriptorCount = */ 1,
          /* descriptorType = */ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          /* pImageInfo = */ nullptr,
          /* pBufferInfo = */ &out_buffer_info,
          /* pTexelBufferView = */ nullptr,
      },
  };
  device->vkUpdateDescriptorSets(device, 2, write_descriptor_set, 0, nullptr);

  // Create command pool and allocate command buffer.

  const ::VkCommandPoolCreateInfo command_pool_create_info = {
      /* sType = */ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* queueFamilyIndex = */ queue_index,
  };
  ::VkCommandPool raw_command_pool;
  LOG_ASSERT(==, data->logger(), VK_SUCCESS,
             device->vkCreateCommandPool(device, &command_pool_create_info,
                                         nullptr, &raw_command_pool));
  vulkan::VkCommandPool command_pool(raw_command_pool, nullptr, &device);

  vulkan::VkCommandBuffer command_buffer(
      CreateDefaultCommandBuffer(&command_pool, &device));
  ::VkCommandBuffer raw_command_buffer = command_buffer.get_command_buffer();

  // Record command buffer.

  const ::VkCommandBufferBeginInfo cmdbuf_begin_info = {
      /* sType = */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      /* pNext = */ nullptr,
      /* flags = */ VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      /* pInheritanceInfo = */ nullptr,
  };
  LOG_ASSERT(
      ==, data->logger(), VK_SUCCESS,
      command_buffer->vkBeginCommandBuffer(command_buffer, &cmdbuf_begin_info));
  command_buffer->vkCmdBindPipeline(
      command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, raw_pipeline);
  command_buffer->vkCmdBindDescriptorSets(
      command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, raw_pipeline_layout, 0, 1,
      &raw_dset, 0, nullptr);
  command_buffer->vkCmdDispatch(command_buffer, kNumElements, 1, 1);
  LOG_ASSERT(==, data->logger(), VK_SUCCESS,
             command_buffer->vkEndCommandBuffer(command_buffer));

  // Submit.

  const ::VkSubmitInfo submit_info = {
      /* sType = */ VK_STRUCTURE_TYPE_SUBMIT_INFO,
      /* pNext = */ nullptr,
      /* waitSemaphoreCount = */ 0,
      /* pWaitSemaphores = */ nullptr,
      /* pWaitDstStageMask = */ nullptr,
      /* commandBufferCount = */ 1,
      /* pCommandBuffers = */ &raw_command_buffer,
      /* signalSemaphoreCount = */ 0,
      /* pSignalSemaphores = */ nullptr,
  };
  LOG_ASSERT(==, data->logger(), VK_SUCCESS,
             queue->vkQueueSubmit(queue, 1, &submit_info,
                                  static_cast<::VkFence>(VK_NULL_HANDLE)));
  LOG_ASSERT(==, data->logger(), VK_SUCCESS, queue->vkQueueWaitIdle(queue));

  LOG_ASSERT(==, data->logger(), VK_SUCCESS,
             device->vkMapMemory(device, memory, 0, memory_size, 0, &payload));
  for (uint32_t i = 0; i < kNumElements; ++i) {
    LOG_EXPECT(==, data->logger(), 84,
               static_cast<uint32_t*>(payload)[i + kNumElements]);
  }
  device->vkUnmapMemory(device, memory);

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
