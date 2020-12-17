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

  vulkan::VulkanApplication application(data->allocator(), data->logger(), data,
                                        vulkan::VulkanApplicationOptions()
                                            .SetHostBufferSize(1024 * 100)
                                            .SetDeviceImageSize(1024 * 100)
                                            .SetDeviceBufferSize(1024 * 100));
  VkImageCreateInfo image_create_info{
      /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* imageType = */ VK_IMAGE_TYPE_2D,
      /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
      /* extent = */
      {
          /* width = */ 32,
          /* height = */ 32,
          /* depth = */ 1,
      },
      /* mipLevels = */ 1,
      /* arrayLayers = */ 1,
      /* samples = */ VK_SAMPLE_COUNT_1_BIT,
      /* tiling = */ VK_IMAGE_TILING_OPTIMAL,
      /* usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
      /* queueFamilyIndexCount = */ 0,
      /* pQueueFamilyIndices = */ nullptr,
      /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
  };

  vulkan::ImagePointer image =
      application.CreateAndBindImage(&image_create_info);

  // Create a buffer for transferring to our image.
  VkBufferCreateInfo create_info = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
      nullptr,                               // pNext
      0,                                     // createFlags
      32 * 32 * 4,                           // size
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,      // usage
      VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
      0,                                     // queueFamilyIndexCount
      nullptr                                // pQueueFamilyIndices
  };
  vulkan::BufferPointer src_buffer =
      application.CreateAndBindHostBuffer(&create_info);

  create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  vulkan::BufferPointer dest_buffer =
      application.CreateAndBindHostBuffer(&create_info);

  // Set up the memory for our buffers.
  // Set the soruce_buf
  for (size_t i = 0; i < 32 * 32 * 4; ++i) {
    src_buffer->base_address()[i] = (0xFF & i);
    dest_buffer->base_address()[i] = 0;
  }
  src_buffer->flush();

  vulkan::VkCommandBuffer command_buffer = application.GetCommandBuffer();

  VkCommandBufferBeginInfo begin_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
  command_buffer->vkBeginCommandBuffer(command_buffer, &begin_info);

  // Copy our entire image from/to the buffers in packed format.
  VkBufferImageCopy region{
      0, 0, 0, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {0, 0, 0}, {32, 32, 1}};

  // Insert a pipeline barrier to wait until the host has flushed
  // the buffer.
  {
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
                            VK_ACCESS_HOST_WRITE_BIT,
                            VK_ACCESS_TRANSFER_READ_BIT};

    // The image needs to be in IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    // in order to be copied to, so transition to that now.
    VkImageMemoryBarrier image_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                       nullptr,
                                       0,  // no source bit
                                       VK_ACCESS_TRANSFER_WRITE_BIT,
                                       VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       VK_QUEUE_FAMILY_IGNORED,
                                       VK_QUEUE_FAMILY_IGNORED,
                                       *image,
                                       {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    command_buffer->vkCmdPipelineBarrier(
        command_buffer, VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &barrier, 0, nullptr, 1,
        &image_barrier);
  }

  command_buffer->vkCmdCopyBufferToImage(command_buffer, *src_buffer, *image,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                         1, &region);
  // Transfer the image from DST_OPTIMAL to SRC_OPTIMAL so that we can
  // copy it to the other buffer.
  {
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                 nullptr,
                                 VK_ACCESS_TRANSFER_WRITE_BIT,
                                 VK_ACCESS_TRANSFER_READ_BIT,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                 VK_QUEUE_FAMILY_IGNORED,
                                 VK_QUEUE_FAMILY_IGNORED,
                                 *image,
                                 {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    command_buffer->vkCmdPipelineBarrier(
        command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  }

  command_buffer->vkCmdCopyImageToBuffer(command_buffer, *image,
                                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                         *dest_buffer, 1, &region);

  // Insert a pipeline barrier to make sure that the image has copied correctly.
  {
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_HOST_READ_BIT};

    command_buffer->vkCmdPipelineBarrier(
        command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
  }

  command_buffer->vkEndCommandBuffer(command_buffer);

  VkCommandBuffer buffers[1] = {command_buffer};

  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO,
                      nullptr,
                      0,
                      nullptr,
                      nullptr,
                      1,
                      buffers,
                      0,
                      nullptr};

  application.render_queue()->vkQueueSubmit(
      application.render_queue(), 1, &submit,
      static_cast<VkFence>(VK_NULL_HANDLE));
  // Make sure that the queue has finished all of its operations, including
  // the memory barriers before we continue.
  application.render_queue()->vkQueueWaitIdle(application.render_queue());

  dest_buffer->invalidate();
  for (size_t i = 0; i < 32 * 32 * 4; ++i) {
    LOG_ASSERT(==, data->logger(), (0xFF & i),
               static_cast<unsigned char>(dest_buffer->base_address()[i]));
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
