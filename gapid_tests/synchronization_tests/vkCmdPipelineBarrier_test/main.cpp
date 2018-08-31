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
#include "vulkan_helpers/structs.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->allocator(), data->logger());
  vulkan::VkInstance instance(
      vulkan::CreateDefaultInstance(data->allocator(), &wrapper));
  vulkan::VkSurfaceKHR surface(vulkan::CreateDefaultSurface(&instance, data));

  uint32_t queues[2];
  vulkan::VkDevice device(vulkan::CreateDeviceForSwapchain(
      data->allocator(), &instance, &surface, &queues[0], &queues[1]));
  vulkan::VkSwapchainKHR swapchain(vulkan::CreateDefaultSwapchain(
      &instance, &device, &surface, data->allocator(), queues[0], queues[1],
      data));

  containers::vector<VkImage> images(data->allocator());
  vulkan::LoadContainer(data->logger(), device->vkGetSwapchainImagesKHR,
                        &images, device, swapchain);

  vulkan::VkCommandPool command_pool =
      vulkan::CreateDefaultCommandPool(data->allocator(), device);
  vulkan::VkCommandBuffer command_buffer =
      vulkan::CreateDefaultCommandBuffer(&command_pool, &device);

  VkImageMemoryBarrier image_memory_barrier{
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // sType
      nullptr,                                   // pNext
      VK_ACCESS_MEMORY_READ_BIT,                 // srcAccessMask
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,      // dstAccessMask
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,           // oldLayout
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // newLayout
      queues[0],                                 // srcQueueFamilyIndex
      queues[1],                                 // dstQueueFamilyIndex
      images[0],                                 // image
      {
          VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
          0,                          // baseMipLevel
          1,                          // mipCount
          0,                          // baseArrayLayer
          1,                          // layerCount
      }};

  VkCommandBufferBeginInfo begin_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr};

  command_buffer->vkBeginCommandBuffer(command_buffer, &begin_info);

  command_buffer->vkCmdPipelineBarrier(
      command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 0, nullptr);

  command_buffer->vkCmdPipelineBarrier(
      command_buffer,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

  command_buffer->vkEndCommandBuffer(command_buffer);

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
