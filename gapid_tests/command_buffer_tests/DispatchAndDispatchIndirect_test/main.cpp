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
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"

#include <algorithm>

uint32_t compute_shader[] =
#include "double_numbers.comp.spv"
    ;

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data);
  vulkan::VkDevice& device = app.device();

  // Both input and output buffers have 512 32-bit integers.
  const uint32_t kNumElements = 512;
  const uint32_t in_out_buffer_size = kNumElements * sizeof(uint32_t);

  VkBufferUsageFlags in_out_buf_usages = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                         VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  auto in_buffer = app.CreateAndBindDefaultExclusiveHostBuffer(
      in_out_buffer_size, in_out_buf_usages);
  auto out_buffer = app.CreateAndBindDefaultExclusiveHostBuffer(
      in_out_buffer_size, in_out_buf_usages);

  // Create descriptor set
  VkDescriptorSetLayoutBinding in_binding{
      0,                                  // binding
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
      1,                                  // descriptorCount
      VK_SHADER_STAGE_COMPUTE_BIT,        // stageFlags
      nullptr,                            // pImmutableSamplers
  };
  VkDescriptorSetLayoutBinding out_binding{
      1,                                  // binding
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
      1,                                  // descriptorCount
      VK_SHADER_STAGE_COMPUTE_BIT,        // stageFlags
      nullptr,                            // pImmutableSamplers
  };
  auto compute_descriptor_set = containers::make_unique<vulkan::DescriptorSet>(
      data->allocator(),
      app.AllocateDescriptorSet({in_binding, out_binding}));

  const VkDescriptorBufferInfo buffer_infos[2] = {
      {*in_buffer, 0, VK_WHOLE_SIZE}, {*out_buffer, 0, VK_WHOLE_SIZE}};

  const VkWriteDescriptorSet write_descriptor_set{
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
      nullptr,                                 // pNext
      *compute_descriptor_set,                 // dstSet
      0,                                       // dstBinding
      0,                                       // dstArrayElement
      2,                                       // descriptorCount
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,       // descriptorType
      nullptr,                                 // pImageInfo
      buffer_infos,                            // pBufferInfo
      nullptr,                                 // pTexelBufferView
  };
  device->vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);

  // Create pipeline
  auto compute_pipeline_layout =
      containers::make_unique<vulkan::PipelineLayout>(
          data->allocator(),
          app.CreatePipelineLayout({{in_binding, out_binding}}));
  auto compute_pipeline =
      containers::make_unique<vulkan::VulkanComputePipeline>(
          data->allocator(),
          app.CreateComputePipeline(
              compute_pipeline_layout.get(),
              VkShaderModuleCreateInfo{
                  VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0,
                  sizeof(compute_shader), compute_shader},
              "main"));
  {
    // 1. vkCmdDispatch
    auto cmd_buf = app.GetCommandBuffer();
    app.BeginCommandBuffer(&cmd_buf);

    // Set inital values for the in-buffer and clear the out-buffer
    containers::vector<uint32_t> initial_in_buffer_value(kNumElements, 1,
                                                         data->allocator());
    app.FillHostVisibleBuffer(
        &*in_buffer,
        reinterpret_cast<const char*>(initial_in_buffer_value.data()),
        initial_in_buffer_value.size() * sizeof(uint32_t), 0, &cmd_buf,
        VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    containers::vector<uint32_t> initial_out_buffer_value(kNumElements, 0,
                                                          data->allocator());
    app.FillHostVisibleBuffer(
        &*out_buffer,
        reinterpret_cast<const char*>(initial_out_buffer_value.data()),
        initial_out_buffer_value.size() * sizeof(uint32_t), 0, &cmd_buf,
        VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    // Call dispatch
    cmd_buf->vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                               *compute_pipeline);
    cmd_buf->vkCmdBindDescriptorSets(
        cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, *compute_pipeline_layout, 0, 1,
        &compute_descriptor_set->raw_set(), 0, nullptr);
    cmd_buf->vkCmdDispatch(cmd_buf, kNumElements, 1, 1);
    LOG_ASSERT(==, data->logger(), VK_SUCCESS,
               app.EndAndSubmitCommandBufferAndWaitForQueueIdle(
                   &cmd_buf, &app.render_queue()));

    // Check the output values
    containers::vector<uint32_t> output =
        vulkan::GetHostVisibleBufferData(data->allocator(), &*out_buffer);
    std::for_each(output.begin(), output.end(),
                  [data](uint32_t w) { LOG_EXPECT(==, data->logger(), 2, w); });
  }

  {
    // 2. vkCmdDispatch Indirect
    // Prepare the indirect buffer
    auto indirect_buffer = app.CreateAndBindDefaultExclusiveHostBuffer(
        sizeof(VkDispatchIndirectCommand),
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    auto cmd_buf = app.GetCommandBuffer();
    app.BeginCommandBuffer(&cmd_buf);

    // Set inital values for the in-buffer and clear the out-buffer
    containers::vector<uint32_t> initial_in_buffer_value(kNumElements, 1,
                                                         data->allocator());
    app.FillHostVisibleBuffer(
        &*in_buffer,
        reinterpret_cast<const char*>(initial_in_buffer_value.data()),
        initial_in_buffer_value.size() * sizeof(uint32_t), 0, &cmd_buf,
        VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    containers::vector<uint32_t> initial_out_buffer_value(kNumElements, 0,
                                                          data->allocator());
    app.FillHostVisibleBuffer(
        &*out_buffer,
        reinterpret_cast<const char*>(initial_out_buffer_value.data()),
        initial_out_buffer_value.size() * sizeof(uint32_t), 0, &cmd_buf,
        VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    // Set the values for the indirect buffer
    VkDispatchIndirectCommand indirect_command{kNumElements, 1, 1};
    app.FillHostVisibleBuffer(&*indirect_buffer,
                              reinterpret_cast<const char*>(&indirect_command),
                              sizeof(indirect_command), 0, &cmd_buf,
                              VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                              VK_ACCESS_INDIRECT_COMMAND_READ_BIT);

    // Call dispatch indirect
    cmd_buf->vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                               *compute_pipeline);
    cmd_buf->vkCmdBindDescriptorSets(
        cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, *compute_pipeline_layout, 0, 1,
        &compute_descriptor_set->raw_set(), 0, nullptr);
    cmd_buf->vkCmdDispatchIndirect(cmd_buf, *indirect_buffer, 0);
    LOG_ASSERT(==, data->logger(), VK_SUCCESS,
               app.EndAndSubmitCommandBufferAndWaitForQueueIdle(
                   &cmd_buf, &app.render_queue()));

    // Check the output values
    containers::vector<uint32_t> output =
        vulkan::GetHostVisibleBufferData(data->allocator(), &*out_buffer);
    std::for_each(output.begin(), output.end(),
                  [data](uint32_t w) { LOG_EXPECT(==, data->logger(), 2, w); });
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
