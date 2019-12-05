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
#include "vulkan_wrapper/command_buffer_wrapper.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::LibraryWrapper wrapper(data->allocator(), data->logger());
  vulkan::VkInstance instance(
      vulkan::CreateEmptyInstance(data->allocator(), &wrapper));
  vulkan::VkDevice device(
      vulkan::CreateDefaultDevice(data->allocator(), instance));
  vulkan::VkCommandPool pool(
      vulkan::CreateDefaultCommandPool(data->allocator(), device, false));
  vulkan::VkCommandBuffer command_buffer =
      vulkan::CreateDefaultCommandBuffer(&pool, &device);

  {
    // Test a nullptr pInheritanceInfo
    VkCommandBufferBeginInfo info{
        /* sType = */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* pInheritanceInfo = */ nullptr,
    };
    command_buffer->vkBeginCommandBuffer(command_buffer, &info);
    command_buffer->vkEndCommandBuffer(command_buffer);
  }

  {
    // Test a non-nullptr pInheritanceInfo
    VkCommandBufferInheritanceInfo hinfo{
        /* sType = */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        /* pNext = */ nullptr,
        /* renderPass = */ VK_NULL_HANDLE,
        /* subpass = */ 0,
        /* framebuffer = */ VK_NULL_HANDLE,
        /* occlusionQueryEnable = */ VK_FALSE,
        /* queryFlags = */ 0,
        /* pipelineStatistics = */ 0,
    };
    VkCommandBufferBeginInfo info{
        /* sType = */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* pInheritanceInfo = */ &hinfo,
    };
    command_buffer->vkBeginCommandBuffer(command_buffer, &info);
    command_buffer->vkEndCommandBuffer(command_buffer);
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
