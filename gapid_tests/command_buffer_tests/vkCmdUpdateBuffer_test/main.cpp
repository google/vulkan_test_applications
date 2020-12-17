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

#include <algorithm>

#include "support/entry/entry.h"
#include "support/log/log.h"
#include "vulkan_helpers/vulkan_application.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication application(data->allocator(), data->logger(), data,
                                        vulkan::VulkanApplicationOptions()
                                            .SetHostBufferSize(1024 * 100)
                                            .SetDeviceImageSize(1024 * 100)
                                            .SetDeviceBufferSize(1024 * 100));
  {
    // Update a buffer with size 65536 bytes and 0 offset.
    size_t update_size = 65536;
    uint8_t update_byte = 0xab;
    VkBufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // createFlags
        update_size,                           // size
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
        0,                                     // queueFamilyIndexCount
        nullptr                                // pQueueFamilyIndices
    };
    // Create an update buffer as the destination buffer for vkCmdUpdateBuffer
    vulkan::BufferPointer update_buffer =
        application.CreateAndBindHostBuffer(&create_info);

    containers::vector<uint8_t> buffer_data(update_size, update_byte,
                                            data->allocator());
    vulkan::VkCommandBuffer cmd_buf = application.GetCommandBuffer();
    VkCommandBufferBeginInfo begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
    cmd_buf->vkBeginCommandBuffer(cmd_buf, &begin_info);
    cmd_buf->vkCmdUpdateBuffer(cmd_buf,            // commandBuffer
                               *update_buffer,     // dstBuffer
                               0,                  // dstOffset
                               update_size,        // dataSize
                               buffer_data.data()  // pData
    );

    VkBufferMemoryBarrier dst_to_src_barrier{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // sType
        nullptr,                                  // pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,             // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,              // dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                  // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                  // dstQueueFamilyIndex
        *update_buffer,                           // buffer
        0,                                        // offset
        update_size,                              // size
    };
    cmd_buf->vkCmdPipelineBarrier(
        cmd_buf,                         // commandBuffer
        VK_PIPELINE_STAGE_HOST_BIT,      // srcStageMask
        VK_PIPELINE_STAGE_TRANSFER_BIT,  // dstStageMask
        0,                               // dependencyFlags
        0,                               // memoryBarrierCount
        nullptr,                         // pMemoryBarriers
        1,                               // bufferMemoryBarrierCount
        &dst_to_src_barrier,             // pBufferMemoryBarriers
        0,                               // imageMemoryBarrierCount
        nullptr                          // pImageMemoryBarriers
    );

    cmd_buf->vkEndCommandBuffer(cmd_buf);
    VkCommandBuffer raw_cmd_buf = cmd_buf.get_command_buffer();
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO,
                        nullptr,
                        0,
                        nullptr,
                        nullptr,
                        1,
                        &raw_cmd_buf,
                        0,
                        nullptr};
    application.render_queue()->vkQueueSubmit(
        application.render_queue(), 1, &submit,
        static_cast<VkFence>(VK_NULL_HANDLE));
    application.render_queue()->vkQueueWaitIdle(application.render_queue());

    update_buffer->invalidate();
    std::for_each(update_buffer->base_address(),
                  update_buffer->base_address() + update_size,
                  [&data, update_byte](uint8_t c) {
                    LOG_ASSERT(==, data->logger(), c, update_byte);
                  });
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
