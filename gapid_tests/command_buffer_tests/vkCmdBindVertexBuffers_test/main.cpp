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
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/queue_wrapper.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data);
  // So we don't have to type app.device every time.
  vulkan::VkDevice& device = app.device();

  {
    //
    VkBufferCreateInfo vertex_buffer_create_info{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // flags
        1024,                                  // size
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
        0,                                     // queueFamilyIndexCount
        nullptr                                // queueFamilyIndices
    };

    auto vertex_buffer =
        app.CreateAndBindDeviceBuffer(&vertex_buffer_create_info);

    VkBufferCreateInfo transfer_buffer_create_info{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // flags
        1024,                                  // size
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,      // usage
        VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
        0,                                     // queueFamilyIndexCount
        nullptr                                // queueFamilyIndices
    };

    auto transfer_buffer =
        app.CreateAndBindHostBuffer(&transfer_buffer_create_info);

    for (size_t i = 0; i < 1024; ++i) {
      *(transfer_buffer->base_address()) = i & 0xFF;
    }
    transfer_buffer->flush();

    vulkan::VkCommandBuffer command_buffer = app.GetCommandBuffer();
    VkCommandBufferBeginInfo begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // sType
        nullptr,                                      // pNext
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,  // flags
        nullptr                                       // pInheritanceInfo
    };

    command_buffer->vkBeginCommandBuffer(command_buffer, &begin_info);
    VkMemoryBarrier upload_barrier{
        VK_STRUCTURE_TYPE_MEMORY_BARRIER,  // sType
        nullptr,                           // pNext
        VK_ACCESS_HOST_WRITE_BIT,          // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT        // dstAccessMask
    };
    command_buffer->vkCmdPipelineBarrier(
        command_buffer, VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &upload_barrier, 0, nullptr, 0,
        nullptr);

    VkBufferCopy copy{
        0,    // srcOffset
        0,    // dstOffset
        1024  // size
    };
    command_buffer->vkCmdCopyBuffer(command_buffer, *transfer_buffer,
                                    *vertex_buffer, 1, &copy);
    VkMemoryBarrier transfer_barrier{
        VK_STRUCTURE_TYPE_MEMORY_BARRIER,    // sType
        nullptr,                             // pNext
        VK_ACCESS_TRANSFER_READ_BIT,         // srcAccessMask
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT  // dstAccessMask
    };
    command_buffer->vkCmdPipelineBarrier(
        command_buffer, VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &transfer_barrier, 0, nullptr, 0,
        nullptr);

    VkBuffer raw_buffer = *vertex_buffer;
    VkDeviceSize offset = 0;

    command_buffer->vkCmdBindVertexBuffers(command_buffer, 0, 1, &raw_buffer,
                                           &offset);

    command_buffer->vkEndCommandBuffer(command_buffer);
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
