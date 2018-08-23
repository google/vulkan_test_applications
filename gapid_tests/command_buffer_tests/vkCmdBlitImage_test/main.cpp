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
#include <tuple>

#include "support/entry/entry.h"
#include "support/log/log.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication application(data->allocator(), data->logger(),
                                        data, {}, {0}, 1024 * 100, 1024 * 100,
                                        1024 * 100);

  VkExtent3D src_image_extent{32, 32, 1};
  VkImageCreateInfo image_create_info{
      /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* imageType = */ VK_IMAGE_TYPE_2D,
      /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
      /* extent = */ src_image_extent,
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

  vulkan::ImagePointer src_image =
      application.CreateAndBindImage(&image_create_info);
  size_t image_data_size = vulkan::GetImageExtentSizeInBytes(
      src_image_extent, VK_FORMAT_R8G8B8A8_UNORM);
  containers::vector<uint8_t> image_data(image_data_size, 0,
                                         data->allocator());
  for (size_t i = 0; i < image_data_size; i++) {
    image_data[i] = i & 0xFF;
  }
  vulkan::ImagePointer dst_image =
      application.CreateAndBindImage(&image_create_info);

  // Create semaphores, one for image data filling, another for layout
  // transitioning.
  VkSemaphoreCreateInfo image_fill_semaphore_create_info{
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};
  ::VkSemaphore image_fill_semaphore;
  application.device()->vkCreateSemaphore(application.device(),
                                          &image_fill_semaphore_create_info,
                                          nullptr, &image_fill_semaphore);
  vulkan::VkSemaphore image_fill_semaphore_wrapper(
      image_fill_semaphore, nullptr, &application.device());
  VkSemaphoreCreateInfo layout_transition_semaphore_create_info{
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};

  ::VkSemaphore src_layout_transition_semaphore;
  application.device()->vkCreateSemaphore(
      application.device(), &layout_transition_semaphore_create_info, nullptr,
      &src_layout_transition_semaphore);
  vulkan::VkSemaphore src_layout_transition_semaphore_wrapper(
      src_layout_transition_semaphore, nullptr, &application.device());
  ::VkSemaphore dst_layout_transition_semaphore;
  application.device()->vkCreateSemaphore(
      application.device(), &layout_transition_semaphore_create_info, nullptr,
      &dst_layout_transition_semaphore);
  vulkan::VkSemaphore dst_layout_transition_semaphore_wrapper(
      dst_layout_transition_semaphore, nullptr, &application.device());
  ::VkSemaphore layout_transition_semaphores[2] = {
      src_layout_transition_semaphore, dst_layout_transition_semaphore};

  std::tuple<bool, vulkan::VkCommandBuffer, vulkan::BufferPointer> fill_result =
      application.FillImageLayersData(
          src_image.get(),
          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},   // subresourcelayer
          {0, 0, 0},                              // offset
          src_image_extent,                       // extent
          VK_IMAGE_LAYOUT_UNDEFINED,              // initial layout
          image_data,                             // data
          {},                                     // wait_semaphores
          {image_fill_semaphore},                 // signal_semaphores
          static_cast<::VkFence>(VK_NULL_HANDLE)  // fence
          );
  bool fill_succeed = std::get<0>(fill_result);
  // Before the all the image filling commands are executed, the command buffer
  // must not be freed.
  vulkan::VkCommandBuffer fill_image_cmd_buf(
      std::move(std::get<1>(fill_result)));

  VkCommandBufferInheritanceInfo cmd_buf_hinfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
      nullptr,
      VK_NULL_HANDLE,
      0,
      VK_NULL_HANDLE,
      VK_FALSE,
      0,
      0};
  VkCommandBufferBeginInfo cmd_buf_begin_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, &cmd_buf_hinfo};
  vulkan::VkCommandBuffer layout_transition_cmd_buf =
      application.GetCommandBuffer();
  {
    // Image layout transition.

    VkImageMemoryBarrier src_image_barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,   // sType
        nullptr,                                  // pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,             // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,              // dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,     // newLayout
        VK_QUEUE_FAMILY_IGNORED,                  // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                  // dstQueueFamilyIndex
        *src_image,                               // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},  // subresourceRange
    };
    VkImageMemoryBarrier dst_image_barrier = src_image_barrier;
    dst_image_barrier.srcAccessMask = 0;
    dst_image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dst_image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    dst_image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    dst_image_barrier.image = *dst_image;

    VkImageMemoryBarrier barriers[2] = {src_image_barrier, dst_image_barrier};
    VkPipelineStageFlags wait_stages[1] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    application.BeginCommandBuffer(&layout_transition_cmd_buf);
    layout_transition_cmd_buf->vkCmdPipelineBarrier(
        layout_transition_cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 2,
        barriers);
    application.EndAndSubmitCommandBuffer(
        &layout_transition_cmd_buf, &application.render_queue(),
        {image_fill_semaphore}, {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT},
        {src_layout_transition_semaphore, dst_layout_transition_semaphore},
        ::VkFence(VK_NULL_HANDLE));
  }

  {
    // 1. Blit to an image with exactly the same image create info as the source
    // image. And the source image has only one layer and one mip level.
    VkImageBlit region{
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {{0, 0, 0},
         {int32_t(src_image_extent.width), int32_t(src_image_extent.height),
          1}},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {{0, 0, 0},
         {int32_t(src_image_extent.width), int32_t(src_image_extent.height),
          1}},
    };
    auto cmd_buf = application.GetCommandBuffer();
    cmd_buf->vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin_info);
    cmd_buf->vkCmdBlitImage(
        cmd_buf, *src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *dst_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);
    cmd_buf->vkEndCommandBuffer(cmd_buf);

    VkCommandBuffer raw_cmd_buf = cmd_buf.get_command_buffer();
    const VkPipelineStageFlags wait_dst_stage_masks[2] = {
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    const ::VkSemaphore wait_semaphores[2] = {src_layout_transition_semaphore,
                                              dst_layout_transition_semaphore};
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO,
                        nullptr,
                        2,
                        wait_semaphores,
                        wait_dst_stage_masks,
                        1,
                        &raw_cmd_buf,
                        0,
                        nullptr};

    application.render_queue()->vkQueueSubmit(
        application.render_queue(), 1, &submit,
        static_cast<VkFence>(VK_NULL_HANDLE));
    application.render_queue()->vkQueueWaitIdle(application.render_queue());

    // Dump the content of the destination image to check the result.
    containers::vector<uint8_t> dump_data(data->allocator());
    application.DumpImageLayersData(
        dst_image.get(),
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},  // subresourcelayer
        {0, 0, 0},                             // offset
        src_image_extent,                      // extent
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // initial layout
        &dump_data,                            // data
        {}                                     // wait_semaphores
        );
    LOG_ASSERT(==, data->logger(), image_data.size(), dump_data.size());
    LOG_ASSERT(
        ==, data->logger(), true,
        std::equal(image_data.begin(), image_data.end(), dump_data.begin()));
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
