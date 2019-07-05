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

  const VkExtent3D sample_image_extent{32, 32, 1};
  const VkFormat sample_format = VK_FORMAT_R8G8B8A8_UNORM;
  const VkImageCreateInfo sample_image_create_info{
      /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* imageType = */ VK_IMAGE_TYPE_2D,
      /* format = */ sample_format,
      /* extent = */ sample_image_extent,
      /* mipLevels = */ 1,
      /* arrayLayers = */ 1,
      /* samples = */ VK_SAMPLE_COUNT_1_BIT,
      /* tiling = */ VK_IMAGE_TILING_OPTIMAL,
      /* usage = */
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_IMAGE_USAGE_SAMPLED_BIT,
      /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
      /* queueFamilyIndexCount = */ 0,
      /* pQueueFamilyIndices = */ nullptr,
      /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
  };
  size_t sample_image_data_size =
      vulkan::GetImageExtentSizeInBytes(sample_image_extent, sample_format);
  containers::vector<uint8_t> sample_image_data(sample_image_data_size, 0,
                                                data->allocator());
  for (size_t i = 0; i < sample_image_data_size; i++) {
    sample_image_data[i] = i & 0xFF;
  }

  {
    // 1. Copy from an uncompressed 2D color image, with only 1 layer, 1
    // miplevel and 0 offsets in all dimensions to another 2D image created
    // with same create info.
    vulkan::VulkanApplication application(data->allocator(), data->logger(),
                                          data, {}, {}, {0}, 1024 * 100,
                                          1024 * 100, 1024 * 100);
    vulkan::VkDevice& device = application.device();

    vulkan::ImagePointer src_image =
        application.CreateAndBindImage(&sample_image_create_info);
    vulkan::ImagePointer dst_image =
        application.CreateAndBindImage(&sample_image_create_info);

    // Create semaphores, one for image data filling, another for layout
    // transitioning.
    VkSemaphoreCreateInfo image_fill_semaphore_create_info{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};
    ::VkSemaphore image_fill_semaphore;
    device->vkCreateSemaphore(device, &image_fill_semaphore_create_info,
                              nullptr, &image_fill_semaphore);
    vulkan::VkSemaphore image_fill_semaphore_wrapper(image_fill_semaphore,
                                                     nullptr, &device);

    VkSemaphoreCreateInfo layout_transition_semaphore_create_info{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};
    ::VkSemaphore layout_transition_semaphore;
    device->vkCreateSemaphore(device, &layout_transition_semaphore_create_info,
                              nullptr, &layout_transition_semaphore);
    vulkan::VkSemaphore layout_transition_semaphore_wrapper(
        layout_transition_semaphore, nullptr, &device);

    // Fill initial data in the source image
    std::tuple<bool, vulkan::VkCommandBuffer, vulkan::BufferPointer>
        fill_result = application.FillImageLayersData(
            src_image.get(),
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},   // subresourcelayer
            {0, 0, 0},                              // offset
            sample_image_extent,                    // extent
            VK_IMAGE_LAYOUT_UNDEFINED,              // initial layout
            sample_image_data,                      // data
            {},                                     // wait_semaphores
            {image_fill_semaphore},                 // signal_semaphores
            static_cast<::VkFence>(VK_NULL_HANDLE)  // fence
        );
    bool fill_succeeded = std::get<0>(fill_result);
    // Before the all the image filling commands are executed, the command
    // buffer must not be freed.
    LOG_ASSERT(==, data->logger(), true, fill_succeeded);
    vulkan::VkCommandBuffer fill_image_cmd_buf(
        std::move(std::get<1>(fill_result)));

    // Change the layout of the source and destination images
    auto layout_transition_cmd_buf = application.GetCommandBuffer();
    application.BeginCommandBuffer(&layout_transition_cmd_buf);
    vulkan::RecordImageLayoutTransition(
        *src_image.get(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT,
        &layout_transition_cmd_buf);
    vulkan::RecordImageLayoutTransition(
        *dst_image.get(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT, &layout_transition_cmd_buf);
    application.EndAndSubmitCommandBuffer(
        &layout_transition_cmd_buf, &application.render_queue(),
        {image_fill_semaphore}, {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT},
        {layout_transition_semaphore}, ::VkFence(VK_NULL_HANDLE));

    // Copy image
    auto copy_image_cmd_buf = application.GetCommandBuffer();
    ::VkCommandBuffer raw_copy_image_cmd_buf =
        copy_image_cmd_buf.get_command_buffer();
    VkCommandBufferBeginInfo cmd_buf_begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
    copy_image_cmd_buf->vkBeginCommandBuffer(copy_image_cmd_buf,
                                             &cmd_buf_begin_info);
    const VkImageCopy region{
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {0, 0, 0},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {0, 0, 0},
        sample_image_extent,
    };
    copy_image_cmd_buf->vkCmdCopyImage(
        copy_image_cmd_buf,
        *src_image,                            // srcImage
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  // srcImageLayout
        *dst_image,                            // dstImage
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // dstImageLayout
        1,                                     // regionCount
        &region                                // pRegions
    );
    copy_image_cmd_buf->vkEndCommandBuffer(copy_image_cmd_buf);

    // Submit the commands
    const VkPipelineStageFlags wait_dst_stage_masks[1] = {
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO,
                             nullptr,
                             1,
                             &layout_transition_semaphore,
                             wait_dst_stage_masks,
                             1,
                             &raw_copy_image_cmd_buf,
                             0,
                             nullptr};
    application.render_queue()->vkQueueSubmit(
        application.render_queue(), 1, &submit_info,
        static_cast<VkFence>(VK_NULL_HANDLE));
    application.render_queue()->vkQueueWaitIdle(application.render_queue());

    // Dump the data from the destination image
    containers::vector<uint8_t> dump_data(data->allocator());
    application.DumpImageLayersData(
        dst_image.get(),
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},  // subresourcelayer
        {0, 0, 0},                             // offset
        sample_image_extent,                   // extent
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // initial layout
        &dump_data,                            // data
        {}                                     // wait_semaphores
    );
    LOG_ASSERT(==, data->logger(), sample_image_data.size(), dump_data.size());
    LOG_ASSERT(==, data->logger(), true,
               std::equal(sample_image_data.begin(), sample_image_data.end(),
                          dump_data.begin()));
  }

  {
    // 2. Copy a region of an BC2 2D color image, with only 1 layer, 1
    // miplevel, to a different region of another 2D image created with same
    // dimensions but in BC3 format.
    VkPhysicalDeviceFeatures requested_features = {0};
    requested_features.textureCompressionBC = VK_TRUE;
    vulkan::VulkanApplication application(data->allocator(), data->logger(),
                                          data, {}, {}, requested_features,
                                          1024 * 100, 1024 * 100, 1024 * 100);
    vulkan::VkDevice& device = application.device();
    if (device.is_valid()) {
      VkImageCreateInfo src_image_create_info = sample_image_create_info;
      src_image_create_info.format = VK_FORMAT_BC2_UNORM_BLOCK;
      vulkan::ImagePointer src_image =
          application.CreateAndBindImage(&src_image_create_info);
      VkImageCreateInfo dst_image_create_info = sample_image_create_info;
      dst_image_create_info.format = VK_FORMAT_BC3_UNORM_BLOCK;
      vulkan::ImagePointer dst_image =
          application.CreateAndBindImage(&dst_image_create_info);

      // Create semaphores, one for image data filling, another for layout
      // transitioning.
      VkSemaphoreCreateInfo image_fill_semaphore_create_info{
          VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};
      ::VkSemaphore image_fill_semaphore;
      device->vkCreateSemaphore(device, &image_fill_semaphore_create_info,
                                nullptr, &image_fill_semaphore);
      vulkan::VkSemaphore image_fill_semaphore_wrapper(image_fill_semaphore,
                                                       nullptr, &device);

      VkSemaphoreCreateInfo layout_transition_semaphore_create_info{
          VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};
      ::VkSemaphore layout_transition_semaphore;
      device->vkCreateSemaphore(device,
                                &layout_transition_semaphore_create_info,
                                nullptr, &layout_transition_semaphore);
      vulkan::VkSemaphore layout_transition_semaphore_wrapper(
          layout_transition_semaphore, nullptr, &device);

      // Fill initial data in the source image, only fill the region to be
      // copied
      // to the destination image.
      const VkExtent3D copy_extent{16, 12, 1};
      size_t copy_image_data_size = vulkan::GetImageExtentSizeInBytes(
          copy_extent, src_image_create_info.format);
      containers::vector<uint8_t> copy_image_data(copy_image_data_size, 0,
                                                  data->allocator());
      for (size_t i = 0; i < copy_image_data_size; i++) {
        copy_image_data[i] = i & 0xFF;
      }

      std::tuple<bool, vulkan::VkCommandBuffer, vulkan::BufferPointer>
          fill_result = application.FillImageLayersData(
              src_image.get(),
              {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},   // subresourcelayer
              {8, 12, 0},                             // offset
              copy_extent,                            // extent
              VK_IMAGE_LAYOUT_UNDEFINED,              // initial layout
              copy_image_data,                        // data
              {},                                     // wait_semaphores
              {image_fill_semaphore},                 // signal_semaphores
              static_cast<::VkFence>(VK_NULL_HANDLE)  // fence
          );
      bool fill_succeeded = std::get<0>(fill_result);
      // Before the all the image filling commands are executed, the command
      // buffer must not be freed.
      LOG_ASSERT(==, data->logger(), true, fill_succeeded);
      vulkan::VkCommandBuffer fill_image_cmd_buf(
          std::move(std::get<1>(fill_result)));

      // Change the layout of the source and destination image
      auto layout_transition_cmd_buf = application.GetCommandBuffer();

      application.BeginCommandBuffer(&layout_transition_cmd_buf);
      vulkan::RecordImageLayoutTransition(
          *src_image.get(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT,
          &layout_transition_cmd_buf);
      vulkan::RecordImageLayoutTransition(
          *dst_image.get(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
          VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          VK_ACCESS_TRANSFER_WRITE_BIT, &layout_transition_cmd_buf);
      application.EndAndSubmitCommandBuffer(
          &layout_transition_cmd_buf, &application.render_queue(),
          {image_fill_semaphore}, {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT},
          {layout_transition_semaphore}, ::VkFence(VK_NULL_HANDLE));

      // Copy image
      auto copy_image_cmd_buf = application.GetCommandBuffer();
      ::VkCommandBuffer raw_copy_image_cmd_buf =
          copy_image_cmd_buf.get_command_buffer();
      VkCommandBufferBeginInfo cmd_buf_begin_info{
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
      copy_image_cmd_buf->vkBeginCommandBuffer(copy_image_cmd_buf,
                                               &cmd_buf_begin_info);
      const VkImageCopy region{
          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
          {8, 12, 0},
          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
          {16, 16, 0},
          copy_extent,
      };
      copy_image_cmd_buf->vkCmdCopyImage(
          copy_image_cmd_buf,
          *src_image,                            // srcImage
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  // srcImageLayout
          *dst_image,                            // dstImage
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // dstImageLayout
          1,                                     // regionCount
          &region                                // pRegions
      );
      copy_image_cmd_buf->vkEndCommandBuffer(copy_image_cmd_buf);

      // Submit the commands
      const VkPipelineStageFlags wait_dst_stage_masks[1] = {
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
      VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO,
                               nullptr,
                               1,
                               &layout_transition_semaphore,
                               wait_dst_stage_masks,
                               1,
                               &raw_copy_image_cmd_buf,
                               0,
                               nullptr};
      application.render_queue()->vkQueueSubmit(
          application.render_queue(), 1, &submit_info,
          static_cast<VkFence>(VK_NULL_HANDLE));
      application.render_queue()->vkQueueWaitIdle(application.render_queue());

      // Dump the data from the destination image
      containers::vector<uint8_t> dump_data(data->allocator());
      application.DumpImageLayersData(
          dst_image.get(),
          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},  // subresourcelayer
          {16, 16, 0},                           // offset
          copy_extent,                           // extent
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,  // initial layout
          &dump_data,                            // data
          {}                                     // wait_semaphores
      );
      LOG_ASSERT(==, data->logger(), copy_image_data.size(), dump_data.size());
      LOG_ASSERT(==, data->logger(), true,
                 std::equal(copy_image_data.begin(), copy_image_data.end(),
                            dump_data.begin()));
    } else {
      data->logger()->LogInfo(
          "Disable test due to missing physical device feature: "
          "textureCompressionBC");
    }
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
