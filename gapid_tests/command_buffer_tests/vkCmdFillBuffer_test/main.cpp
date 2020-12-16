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

  vulkan::VulkanApplication application(data->allocator(), data->logger(),
                                        data);
  {
    // Fill a buffer first with data: 0x12345678, size: VK_WHOLE_SIZE and
    // offset: 0, then fill it again with data: 0xabcdabcd, size: 256 and
    // offset: 128.
    const size_t buffer_size = 1024;
    const uint32_t first_fill_uint = 0x12345678;
    VkBufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // createFlags
        buffer_size,                           // size
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
        0,                                     // queueFamilyIndexCount
        nullptr                                // pQueueFamilyIndices
    };
    // Create a buffer as the destination buffer for vkCmdFillBuffer
    vulkan::BufferPointer buffer =
        application.CreateAndBindHostBuffer(&create_info);

    vulkan::VkCommandBuffer cmd_buf = application.GetCommandBuffer();

    // Fill the buffer first time.
    VkCommandBufferBeginInfo begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
    cmd_buf->vkBeginCommandBuffer(cmd_buf, &begin_info);
    cmd_buf->vkCmdFillBuffer(cmd_buf,         // commandBuffer
                             *buffer,         // dstBuffer
                             0,               // dstOffset
                             VK_WHOLE_SIZE,   // size
                             first_fill_uint  // pData
    );

    VkBufferMemoryBarrier dst_to_src_barrier{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // sType
        nullptr,                                  // pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,             // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,              // dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                  // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                  // dstQueueFamilyIndex
        *buffer,                                  // buffer
        0,                                        // offset
        buffer_size,                              // size
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

    // Check the result of the first buffer fill.
    buffer->invalidate();
    uint32_t* data_ptr = reinterpret_cast<uint32_t*>(buffer->base_address());
    std::for_each(data_ptr, data_ptr + buffer_size / sizeof(uint32_t),
                  [&data, first_fill_uint](uint32_t d) {
                    LOG_ASSERT(==, data->logger(), d, first_fill_uint);
                  });

    // Fill the buffer second time
    const size_t fill_offset = 128;
    const size_t fill_size = 256;
    const uint32_t second_fill_uint = 0xabcdabcd;
    cmd_buf->vkBeginCommandBuffer(cmd_buf, &begin_info);
    cmd_buf->vkCmdFillBuffer(cmd_buf,          // commandBuffer
                             *buffer,          // dstBuffer
                             fill_offset,      // dstOffset
                             fill_size,        // size
                             second_fill_uint  // pData
    );
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
    application.render_queue()->vkQueueSubmit(
        application.render_queue(), 1, &submit,
        static_cast<VkFence>(VK_NULL_HANDLE));
    application.render_queue()->vkQueueWaitIdle(application.render_queue());

    // Check the result of the second buffer fill
    buffer->invalidate();
    size_t counter = 0;
    std::for_each(
        data_ptr, data_ptr + buffer_size / sizeof(uint32_t),
        [&data, &counter, first_fill_uint, second_fill_uint, fill_offset,
         fill_size](uint32_t d) {
          if (counter >= fill_offset / sizeof(uint32_t) &&
              counter < (fill_offset + fill_size) / sizeof(uint32_t)) {
            LOG_ASSERT(==, data->logger(), d, second_fill_uint);
          } else {
            LOG_ASSERT(==, data->logger(), d, first_fill_uint);
          }
          counter++;
        });
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
