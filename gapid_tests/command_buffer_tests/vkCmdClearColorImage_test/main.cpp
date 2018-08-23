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
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data);
  vulkan::VkDevice& device = app.device();
  {
    // 1. Clear a 2D single layer, single mip level color image
    const VkExtent3D image_extent{32, 32, 1};
    const VkImageCreateInfo image_create_info{
        /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_2D,
        /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
        /* extent = */ image_extent,
        /* mipLevels = */ 1,
        /* arrayLayers = */ 1,
        /* samples = */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling = */ VK_IMAGE_TILING_OPTIMAL,
        /* usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
    };
    vulkan::ImagePointer image_ptr = app.CreateAndBindImage(&image_create_info);

    // Clear value and range
    VkClearColorValue clear_color{
        {0.2, 0.2, 0.2, 0.2}  // float32[4]
    };
    VkImageSubresourceRange clear_range{
        VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
        0,                          // baseMipLevel
        1,                          // levelCount
        0,                          // baseArrayLayer
        1,                          // layerCount
    };

    // Get command buffer and call the clear command
    vulkan::VkCommandBuffer cmd_buf = app.GetCommandBuffer();
    ::VkCommandBuffer raw_cmd_buf = cmd_buf.get_command_buffer();
    VkCommandBufferBeginInfo cmd_buf_begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
    cmd_buf->vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin_info);

    VkImageMemoryBarrier image_barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,   // sType
        nullptr,                                  // pNext
        0,                                        // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,             // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     // newLayout
        VK_QUEUE_FAMILY_IGNORED,                  // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                  // dstQueueFamilyIndex
        *image_ptr,                               // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},  // subresourceRange
    };

    cmd_buf->vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0,
                                  nullptr, 0, nullptr, 1, &image_barrier);

    cmd_buf->vkCmdClearColorImage(cmd_buf, *image_ptr,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  &clear_color, 1, &clear_range);
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
    app.render_queue()->vkQueueSubmit(app.render_queue(), 1, &submit_info,
                                      static_cast<VkFence>(VK_NULL_HANDLE));
    app.render_queue()->vkQueueWaitIdle(app.render_queue());

    // Dump the data in the cleared image
    containers::vector<uint8_t> dump_data(data->allocator());
    app.DumpImageLayersData(
        image_ptr.get(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {0, 0, 0},
        image_extent, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &dump_data, {});

    // Check the dump data
    containers::vector<uint8_t> expected_data(32 * 32 * 4, 0.2 * 255,
                                              data->allocator());
    LOG_ASSERT(==, data->logger(), expected_data.size(), dump_data.size());
    LOG_ASSERT(==, data->logger(), true,
               std::equal(expected_data.begin(), expected_data.end(),
                          dump_data.begin()));
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
