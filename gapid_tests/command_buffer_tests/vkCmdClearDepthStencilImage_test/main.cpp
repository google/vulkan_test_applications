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

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data, vulkan::VulkanApplicationOptions());
  vulkan::VkDevice& device = app.device();
  {
    // 1. Clear a 2D single layer, single mip level depth/stencil image
    const VkExtent3D image_extent{32, 32, 1};
    const VkImageCreateInfo image_create_info{
        /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        /* pNext = */ nullptr,
        /* flags = */ 0,
        /* imageType = */ VK_IMAGE_TYPE_2D,
        /* format = */ VK_FORMAT_D16_UNORM,
        /* extent = */ image_extent,
        /* mipLevels = */ 1,
        /* arrayLayers = */ 1,
        /* samples = */ VK_SAMPLE_COUNT_1_BIT,
        /* tiling = */ VK_IMAGE_TILING_OPTIMAL,
        /* usage = */ VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
        /* queueFamilyIndexCount = */ 0,
        /* pQueueFamilyIndices = */ nullptr,
        /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
    };
    vulkan::ImagePointer image_ptr = app.CreateAndBindImage(&image_create_info);

    // Clear value and range
    const float depth_clear_float = 0.2;
    unsigned short depth_clear_unorm = depth_clear_float * 0xffff;
    VkClearDepthStencilValue clear_depth_stencil{
        depth_clear_float,  // depth
        1,  // stencil, not used here as the format does not contain stencil
            // data.
    };
    VkImageSubresourceRange clear_range{
        VK_IMAGE_ASPECT_DEPTH_BIT,  // aspectMask
        0,                          // baseMipLevel
        1,                          // levelCount
        0,                          // baseArrayLayer
        1,                          // layerCount
    };

    // Get command buffer and call the vkCmdClearDepthStencilImage command
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
        {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},  // subresourceRange
    };
    cmd_buf->vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0,
                                  nullptr, 0, nullptr, 1, &image_barrier);

    cmd_buf->vkCmdClearDepthStencilImage(
        cmd_buf,                               // commandBuffer
        *image_ptr,                            // image
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // imageLayout
        &clear_depth_stencil,                  // pDepthStencil
        1,                                     // rangeCount
        &clear_range                           // pRagnes
    );
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
        image_ptr.get(), {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1}, {0, 0, 0},
        image_extent, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &dump_data, {});

    // Check the dump data
    containers::vector<unsigned short> expected_data(
        vulkan::GetImageExtentSizeInBytes(image_extent, VK_FORMAT_D16_UNORM) /
            sizeof(unsigned short),
        depth_clear_unorm, data->allocator());
    LOG_ASSERT(==, data->logger(), expected_data.size(),
               dump_data.size() / sizeof(unsigned short));
    unsigned short* dump_data_ptr =
        reinterpret_cast<unsigned short*>(dump_data.data());
    std::for_each(expected_data.begin(), expected_data.end(),
                  [&data, &dump_data_ptr](unsigned short d) {
                    LOG_ASSERT(==, data->logger(), d, *dump_data_ptr++);
                  });
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
