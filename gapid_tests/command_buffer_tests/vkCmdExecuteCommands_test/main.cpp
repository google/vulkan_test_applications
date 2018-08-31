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

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data);
  vulkan::VkDevice& device = app.device();
  containers::vector<char> pseudo_data(16, 0xba, data->allocator());

  // Create pipeline layout
  // Frist, create descriptor set layout bindings
  VkDescriptorSetLayoutBinding vertex_binding{
      0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT,
      nullptr};
  VkDescriptorSetLayoutBinding fragement_binding{
      1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr};
  // Second, create descriptor set layout
  vulkan::VkDescriptorSetLayout descriptor_set_layout =
      vulkan::CreateDescriptorSetLayout(data->allocator(), &device,
                                        {vertex_binding, fragement_binding});
  // Third, create push constant range
  VkPushConstantRange ranges[2]{
      {VK_SHADER_STAGE_VERTEX_BIT, 0, uint32_t(pseudo_data.size())},
      {VK_SHADER_STAGE_FRAGMENT_BIT, 0, uint32_t(pseudo_data.size())}};
  // Fourth, Create pipeline layout
  VkPipelineLayoutCreateInfo pipeline_create_info{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0,     1,
      &descriptor_set_layout.get_raw_object(),       2,       ranges};
  ::VkPipelineLayout raw_pipeline_layout;
  device->vkCreatePipelineLayout(device, &pipeline_create_info, nullptr,
                                 &raw_pipeline_layout);
  vulkan::VkPipelineLayout pipeline_layout(raw_pipeline_layout, nullptr,
                                           &device);

  {
    // 1. Two secondary command buffers allocated by the same pool as the
    // primary command buffer and call vkCmdExecuteCommands out of a render
    // pass.
    vulkan::VkCommandBuffer primary_cmd_buf = app.GetCommandBuffer();
    vulkan::VkCommandBuffer secondary_cmd_bufs[2] = {
        app.GetCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY),
        app.GetCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY)};
    ::VkCommandBuffer raw_secondar_cmd_bufs[2] = {
        secondary_cmd_bufs[0].get_command_buffer(),
        secondary_cmd_bufs[1].get_command_buffer()};
    VkCommandBufferBeginInfo primary_begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // sType
        nullptr,                                      // pNext
        0,                                            // flags
        nullptr,                                      // pInheritanceInfo
    };
    VkCommandBufferInheritanceInfo secondary_inherit_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,  // sType
        nullptr,                                            // pNext
        VK_NULL_HANDLE,                                     // renderPass
        0,                                                  // subpass
        VK_NULL_HANDLE,                                     // framebuffer
        VK_FALSE,  // occlusionQueryEnable
        0,         // queryFlags
        0,         // pipelineStatistics
    };
    VkCommandBufferBeginInfo secondary_begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // sType
        nullptr,                                      // pNext
        0,                                            // flags
        &secondary_inherit_info,                      // pInheritanceInfo
    };
    // Call vkCmdPushConstants on the secondary command buffers.
    // First secondary command buffer
    secondary_cmd_bufs[0]->vkBeginCommandBuffer(secondary_cmd_bufs[0],
                                                &secondary_begin_info);
    secondary_cmd_bufs[0]->vkCmdPushConstants(
        secondary_cmd_bufs[0], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
        pseudo_data.size(), static_cast<void*>(pseudo_data.data()));
    secondary_cmd_bufs[0]->vkEndCommandBuffer(secondary_cmd_bufs[0]);
    // Second secondary command buffer
    secondary_cmd_bufs[1]->vkBeginCommandBuffer(secondary_cmd_bufs[1],
                                                &secondary_begin_info);
    secondary_cmd_bufs[1]->vkCmdPushConstants(
        secondary_cmd_bufs[1], pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        pseudo_data.size(), static_cast<void*>(pseudo_data.data()));
    secondary_cmd_bufs[1]->vkEndCommandBuffer(secondary_cmd_bufs[1]);
    // Call vkCmdExecuteCommands on the primary command buffer.
    primary_cmd_buf->vkBeginCommandBuffer(primary_cmd_buf, &primary_begin_info);
    primary_cmd_buf->vkCmdExecuteCommands(primary_cmd_buf, 2,
                                          raw_secondar_cmd_bufs);
    primary_cmd_buf->vkEndCommandBuffer(primary_cmd_buf);
    // Submit the primary command buffer.
    ::VkCommandBuffer raw_primary_cmd_buf =
        primary_cmd_buf.get_command_buffer();
    VkSubmitInfo submit{
        VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0,      nullptr, nullptr, 1,
        &raw_primary_cmd_buf,          0,       nullptr};
    LOG_ASSERT(==, data->logger(), app.render_queue()->vkQueueSubmit(
                                  app.render_queue(), 1, &submit,
                                  static_cast<VkFence>(VK_NULL_HANDLE)),
               VK_SUCCESS);
    LOG_ASSERT(==, data->logger(),
               app.render_queue()->vkQueueWaitIdle(app.render_queue()),
               VK_SUCCESS);
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
