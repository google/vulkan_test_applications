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
#include "vulkan_helpers/vulkan_application.h"
int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data, vulkan::VulkanApplicationOptions());
  vulkan::VkDevice& device = app.device();

  vulkan::VkCommandBuffer command_buffer = app.GetCommandBuffer();

  VkCommandBufferBeginInfo begin_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
  command_buffer->vkBeginCommandBuffer(command_buffer, &begin_info);

  VkDescriptorSetLayoutBinding binding{
      /* binding = */ 2,
      /* descriptorType = */ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      /* descriptorCount = */ 1,
      /* stageFlags = */ VK_SHADER_STAGE_ALL_GRAPHICS,
      /* pImmutableSamplers = */ nullptr,
  };
  vulkan::PipelineLayout pipeline_layout =
      app.CreatePipelineLayout({{binding}});
  vulkan::DescriptorSet descriptor_set = app.AllocateDescriptorSet({binding});
  ::VkDescriptorSet sets[1] = {descriptor_set};

  command_buffer->vkCmdBindDescriptorSets(
      /* commandBuffer = */ command_buffer,
      /* pipelineBindPoint = */ VK_PIPELINE_BIND_POINT_GRAPHICS,
      /* layout = */ ::VkPipelineLayout(pipeline_layout),
      /* firstSet = */ 0,
      /* descriptorSetCount = */ 1,
      /* pDescriptorSets = */ sets,
      /* dynamicOffsetCount = */ 0,
      /* pDynamicOffsets = */ nullptr);

  command_buffer->vkEndCommandBuffer(command_buffer);

  VkCommandBuffer buffers[1] = {command_buffer};

  VkSubmitInfo submit_info{
      /* sType = */ VK_STRUCTURE_TYPE_SUBMIT_INFO,
      /* pNext = */ nullptr,
      /* waitSemaphoreCount = */ 0,
      /* pWaitSemaphores = */ nullptr,
      /* pWaitDstStageMask = */ nullptr,
      /* commandBufferCount = */ 1,
      /* pCommandBuffers = */ buffers,
      /* signalSemaphoreCount = */ 0,
      /* pSignalSemaphores = */ nullptr,
  };

  app.render_queue()->vkQueueSubmit(app.render_queue(), 1, &submit_info,
                                    static_cast<VkFence>(VK_NULL_HANDLE));
  app.render_queue()->vkQueueWaitIdle(app.render_queue());

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
