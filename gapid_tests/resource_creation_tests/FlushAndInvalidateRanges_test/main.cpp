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
#include "vulkan_wrapper/command_buffer_wrapper.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::VulkanApplication app(data->allocator(), data->logger(), data, vulkan::VulkanApplicationOptions());
  vulkan::VkDevice& device = app.device();
  vulkan::VkCommandBuffer cmd_buf = app.GetCommandBuffer();
  ::VkCommandBuffer raw_cmd_buf = cmd_buf.get_command_buffer();
  vulkan::VkQueue& queue = app.render_queue();

  // Create a tiny buffer to determine the memory flags
  VkBufferCreateInfo buffer_create_info{
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
      nullptr,                               // pNext
      0,                                     // flags
      1,                                     // size
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,  // usage
      VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
      0,                                     // queueFamilyIndexCount
      nullptr,                               //  pQueueFamilyIndices
  };
  ::VkBuffer tiny_buffer;
  device->vkCreateBuffer(device, &buffer_create_info, nullptr, &tiny_buffer);
  VkMemoryRequirements requirements;
  device->vkGetBufferMemoryRequirements(device, tiny_buffer, &requirements);
  device->vkDestroyBuffer(device, tiny_buffer, nullptr);
  uint32_t memory_type_index =
      GetMemoryIndex(&device, data->logger(), requirements.memoryTypeBits,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  // Allocate memory space for 2048 bytes
  // Mapped memories must be aligned to kNonCoherentAtomSize and all the sizes
  // used for memory mapping, flushing and invalidating must be a multiple of
  // kNonCoherentAtomSize
  const uint64_t kNonCoherentAtomSize = 256;
  VkMemoryAllocateInfo memory_allocate_info{
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // sType
      nullptr,                                 // pNext
      kNonCoherentAtomSize * 8,                // allocationSize
      memory_type_index,                       // memoryTypeIndex
  };
  ::VkDeviceMemory raw_device_memory;
  device->vkAllocateMemory(device, &memory_allocate_info, nullptr,
                           &raw_device_memory);
  vulkan::VkDeviceMemory device_memory(raw_device_memory, nullptr, &device);

  // Create 512-byte src and dst buffer for copying.
  const VkDeviceSize kBufferSize = kNonCoherentAtomSize * 2;
  buffer_create_info.size = kBufferSize;

  {
    // Create src and dst buffer, flush data to a src buffer, copy the buffer
    // content to dst buffer and invalidate the dst buffer to fetch the same
    // data again.
    ::VkBuffer raw_src_buffer;
    ::VkBuffer raw_dst_buffer;
    device->vkCreateBuffer(device, &buffer_create_info, nullptr,
                           &raw_src_buffer);
    device->vkCreateBuffer(device, &buffer_create_info, nullptr,
                           &raw_dst_buffer);
    vulkan::VkBuffer src_buffer(raw_src_buffer, nullptr, &device);
    vulkan::VkBuffer dst_buffer(raw_dst_buffer, nullptr, &device);

    {
      // 1. Flush a mapped memory range to the src buffer. Only flush the
      // second 256-byte block, i.e. the flush offset is not equal to the
      // mapped offset, and flush size is not VK_WHOLE_SIZE.

      // Bind the src buffer to offset 512
      const VkDeviceSize kSrcBufferOffset = kNonCoherentAtomSize * 2;
      device->vkBindBufferMemory(device, src_buffer, device_memory,
                                 kSrcBufferOffset);
      const VkDeviceSize map_offset = kSrcBufferOffset;
      const VkDeviceSize flush_offset = kSrcBufferOffset + kNonCoherentAtomSize;
      const VkDeviceSize map_size = kBufferSize;
      const VkDeviceSize flush_size = kNonCoherentAtomSize;
      char* buf_data;
      device->vkMapMemory(device,                              // device
                          device_memory,                       // memory
                          map_offset,                          // offset
                          map_size,                            // size
                          0,                                   // flags
                          reinterpret_cast<void**>(&buf_data)  // ppData
      );
      for (size_t i = 0; i < map_size; i++) {
        buf_data[i] = i & 0xFF;
      }
      VkMappedMemoryRange flush_range{
          VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,  // sType
          nullptr,                                // pNext
          device_memory,                          // memory
          flush_offset,                           // offset
          flush_size,                             // size
      };
      device->vkFlushMappedMemoryRanges(device, 1, &flush_range);
      device->vkUnmapMemory(device, device_memory);
    }

    {
      // 2. Copy the content from the src buffer to a dst buffer which starts
      // at offset 0. Then map the memory corresponding to the dst buffer and
      // invalidate the whole buffer with size of value VK_WHOLE_SIZE.

      // Bind the dst buffer to offset 0
      const VkDeviceSize kDstBufferOffset = 0;
      device->vkBindBufferMemory(device, dst_buffer, device_memory,
                                 kDstBufferOffset);

      // Copy data from src buffer to dst buffer
      VkCommandBufferBeginInfo cmd_begin_info{
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          nullptr,
          0,
          nullptr,
      };
      cmd_buf->vkBeginCommandBuffer(cmd_buf, &cmd_begin_info);
      VkMemoryBarrier flush_barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
                                    VK_ACCESS_HOST_WRITE_BIT,
                                    VK_ACCESS_TRANSFER_READ_BIT};
      cmd_buf->vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_HOST_BIT,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1,
                                    &flush_barrier, 0, nullptr, 0, nullptr);
      VkBufferCopy copy_region{0, 0, kBufferSize};
      cmd_buf->vkCmdCopyBuffer(cmd_buf, src_buffer, dst_buffer, 1,
                               &copy_region);
      VkMemoryBarrier copy_barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
                                   VK_ACCESS_TRANSFER_READ_BIT,
                                   VK_ACCESS_HOST_READ_BIT};
      cmd_buf->vkEndCommandBuffer(cmd_buf);
      VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO,
                               nullptr,
                               0,
                               nullptr,
                               nullptr,
                               1,
                               &raw_cmd_buf,
                               0,
                               nullptr};
      queue->vkQueueSubmit(queue, 1, &submit_info,
                           static_cast<VkFence>(VK_NULL_HANDLE));
      queue->vkQueueWaitIdle(queue);

      // Invalidate the memory range of the dst buffer. The second 256-byte
      // block
      // of the dst buffer should have the data copied from src buffer's second
      // 256-byte block.
      const VkDeviceSize map_offset = kDstBufferOffset;
      const VkDeviceSize invalidate_offset = kDstBufferOffset;
      const VkDeviceSize map_size = kBufferSize;
      const VkDeviceSize invalidate_size = VK_WHOLE_SIZE;
      char* buf_data;
      device->vkMapMemory(device,                              // device
                          device_memory,                       // memory
                          map_offset,                          // offset
                          map_size,                            // size
                          0,                                   // flags
                          reinterpret_cast<void**>(&buf_data)  // ppData
      );
      VkMappedMemoryRange invalidate_range{
          VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,  // sType
          nullptr,                                // pNext
          device_memory,                          // memory
          invalidate_offset,                      // offset
          invalidate_size,                        // size
      };
      device->vkInvalidateMappedMemoryRanges(device, 1, &invalidate_range);
      // Only the second 256-byte block of the dst buffer has the data flushed
      // before.
      for (size_t i = kNonCoherentAtomSize; i < map_size; i++) {
        LOG_ASSERT(==, data->logger(), (i & 0xFF), (unsigned char)buf_data[i]);
      }
      device->vkUnmapMemory(device, device_memory);
    }
  }

  {
    // Create another src and dst buffer, flush and invalidate buffer again.
    ::VkBuffer raw_src_buffer;
    ::VkBuffer raw_dst_buffer;
    device->vkCreateBuffer(device, &buffer_create_info, nullptr,
                           &raw_src_buffer);
    device->vkCreateBuffer(device, &buffer_create_info, nullptr,
                           &raw_dst_buffer);
    vulkan::VkBuffer src_buffer(raw_src_buffer, nullptr, &device);
    vulkan::VkBuffer dst_buffer(raw_dst_buffer, nullptr, &device);

    {
      // 3. Flush the whole src buffer which starts at offset 0, i.e. the flush
      // offset is equal to the mapped offset, and flush size is VK_WHOLE_SIZE.

      // Bind the src buffer to offset 0
      const VkDeviceSize kSrcBufferOffset = 0;
      device->vkBindBufferMemory(device, src_buffer, device_memory,
                                 kSrcBufferOffset);
      const VkDeviceSize map_offset = kSrcBufferOffset;
      const VkDeviceSize flush_offset = kSrcBufferOffset;
      const VkDeviceSize map_size = kBufferSize;
      const VkDeviceSize flush_size = VK_WHOLE_SIZE;
      char* buf_data;
      device->vkMapMemory(device,                              // device
                          device_memory,                       // memory
                          map_offset,                          // offset
                          map_size,                            // size
                          0,                                   // flags
                          reinterpret_cast<void**>(&buf_data)  // ppData
      );
      for (size_t i = 0; i < map_size; i++) {
        if (i < 512 - i) {
          buf_data[i] = i & 0xFF;
        } else {
          buf_data[i] = (512 - i) & 0xFF;
        }
      }
      VkMappedMemoryRange flush_range{
          VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,  // sType
          nullptr,                                // pNext
          device_memory,                          // memory
          flush_offset,                           // offset
          flush_size,                             // size
      };
      device->vkFlushMappedMemoryRanges(device, 1, &flush_range);
      device->vkUnmapMemory(device, device_memory);
    }

    {
      // 4. Copy the content from the src buffer to the dst buffer. Then map
      // the memory corresponding to the dst buffer and invalidate the second
      // half of the buffer, i.e. invalidate offset is not equal to the mapped
      // offset and the size is not equal to VK_WHOLE_SIZE.

      // Bind the dst buffer to offset 1024
      const VkDeviceSize kDstBufferOffset = kBufferSize * 2;
      device->vkBindBufferMemory(device, dst_buffer, device_memory,
                                 kDstBufferOffset);

      // Copy data from src buffer to dst buffer
      VkCommandBufferBeginInfo cmd_begin_info{
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          nullptr,
          0,
          nullptr,
      };
      cmd_buf->vkBeginCommandBuffer(cmd_buf, &cmd_begin_info);
      VkMemoryBarrier flush_barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
                                    VK_ACCESS_HOST_WRITE_BIT,
                                    VK_ACCESS_TRANSFER_READ_BIT};
      cmd_buf->vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_HOST_BIT,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1,
                                    &flush_barrier, 0, nullptr, 0, nullptr);
      VkBufferCopy copy_region{0, 0, kBufferSize};
      cmd_buf->vkCmdCopyBuffer(cmd_buf, src_buffer, dst_buffer, 1,
                               &copy_region);
      VkMemoryBarrier copy_barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
                                   VK_ACCESS_TRANSFER_READ_BIT,
                                   VK_ACCESS_HOST_READ_BIT};
      cmd_buf->vkEndCommandBuffer(cmd_buf);
      VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO,
                               nullptr,
                               0,
                               nullptr,
                               nullptr,
                               1,
                               &raw_cmd_buf,
                               0,
                               nullptr};
      queue->vkQueueSubmit(queue, 1, &submit_info,
                           static_cast<VkFence>(VK_NULL_HANDLE));
      queue->vkQueueWaitIdle(queue);

      // Invalidate the memory range of the second half of dst buffer.
      const VkDeviceSize map_offset = kDstBufferOffset;
      const VkDeviceSize invalidate_offset =
          kDstBufferOffset + kNonCoherentAtomSize;
      const VkDeviceSize map_size = kBufferSize;
      const VkDeviceSize invalidate_size = kNonCoherentAtomSize;
      char* buf_data;
      device->vkMapMemory(device,                              // device
                          device_memory,                       // memory
                          map_offset,                          // offset
                          map_size,                            // size
                          0,                                   // flags
                          reinterpret_cast<void**>(&buf_data)  // ppData
      );
      VkMappedMemoryRange invalidate_range{
          VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,  // sType
          nullptr,                                // pNext
          device_memory,                          // memory
          invalidate_offset,                      // offset
          invalidate_size,                        // size
      };
      device->vkInvalidateMappedMemoryRanges(device, 1, &invalidate_range);
      for (size_t i = kNonCoherentAtomSize; i < map_size; i++) {
        LOG_ASSERT(==, data->logger(), ((512 - i) & 0xFF),
                   (unsigned char)buf_data[i]);
      }
      device->vkUnmapMemory(device, device_memory);
    }
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
