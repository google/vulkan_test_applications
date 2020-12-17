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

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data, vulkan::VulkanApplicationOptions());
  vulkan::VkDevice& device = app.device();
  VkDescriptorSetLayoutBinding binding{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                                       VK_SHADER_STAGE_VERTEX_BIT, nullptr};
  vulkan::VkDescriptorSetLayout descriptor_set_layout =
      vulkan::CreateDescriptorSetLayout(data->allocator(), &device, {binding});

  // The size of the data must be a multiple of 4
  containers::vector<char> constants(100, 0xab, data->allocator());

  VkPushConstantRange range{VK_SHADER_STAGE_VERTEX_BIT, 0,
                            uint32_t(constants.size())};
  VkPipelineLayoutCreateInfo pipeline_create_info{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0,     1,
      &descriptor_set_layout.get_raw_object(),       1,       &range};
  ::VkPipelineLayout raw_pipeline_layout;
  device->vkCreatePipelineLayout(device, &pipeline_create_info, nullptr,
                                 &raw_pipeline_layout);
  vulkan::VkPipelineLayout pipeline_layout(raw_pipeline_layout, nullptr,
                                           &device);
  {
    // 1. Offset of value 0, push to vertex stage.
    vulkan::VkCommandBuffer cmd_buf = app.GetCommandBuffer();
    ::VkCommandBuffer raw_cmd_buf = cmd_buf.get_command_buffer();
    ::VkPipelineLayout raw_pipeline_layout =
        static_cast<::VkPipelineLayout>(pipeline_layout);
    VkCommandBufferBeginInfo command_buffer_begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // sType
        nullptr,                                      // pNext
        0,                                            // flags
        nullptr,                                      // pInheritanceInfo
    };
    cmd_buf->vkBeginCommandBuffer(cmd_buf, &command_buffer_begin_info);
    cmd_buf->vkCmdPushConstants(cmd_buf, raw_pipeline_layout,
                                VK_SHADER_STAGE_VERTEX_BIT, 0, constants.size(),
                                static_cast<void*>(constants.data()));
    cmd_buf->vkEndCommandBuffer(cmd_buf);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO,
                        nullptr,
                        0,
                        nullptr,
                        nullptr,
                        1,
                        &raw_cmd_buf,
                        0,
                        nullptr};

    app.render_queue()->vkQueueSubmit(app.render_queue(), 1, &submit,
                                      static_cast<VkFence>(VK_NULL_HANDLE));
    app.render_queue()->vkQueueWaitIdle(app.render_queue());
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
