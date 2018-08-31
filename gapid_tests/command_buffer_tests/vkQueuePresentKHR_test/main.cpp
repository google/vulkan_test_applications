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
#include "vulkan_wrapper/sub_objects.h"

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data);
  vulkan::VkDevice& device = app.device();
  {
    // 1. Present a queue with no semaphore and result array, but one swapchain.
    // Get the index of the next image to present
    // This semaphore is only created for getting the next image index, not for
    // presenting queue.
    ::VkSemaphore raw_semaphore;
    VkSemaphoreCreateInfo semaphore_create_info{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,  // sType
        NULL,                                     // pNext
        0,                                        // flags
    };
    device->vkCreateSemaphore(device, &semaphore_create_info, nullptr,
                              &raw_semaphore);
    vulkan::VkSemaphore semaphore(raw_semaphore, nullptr, &device);

    uint32_t image_index = 0;
    device->vkAcquireNextImageKHR(device, app.swapchain(), 10u, semaphore,
                                  (::VkFence)VK_NULL_HANDLE, &image_index);
    data->logger()->LogInfo("Next image index: ", image_index);
    // Get the image to be presented.
    ::VkImage raw_image_to_present = app.swapchain_images()[image_index];

    // Change the image layout, fill the image with clear color value, prepare
    // to be presented.
    vulkan::VkCommandBuffer cmd_buf = app.GetCommandBuffer();
    ::VkCommandBuffer raw_cmd_buf = cmd_buf.get_command_buffer();
    app.BeginCommandBuffer(&cmd_buf);
    RecordImageLayoutTransition(
        raw_image_to_present, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        VK_IMAGE_LAYOUT_UNDEFINED, static_cast<VkAccessFlags>(0u),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
        &cmd_buf);
    app.EndAndSubmitCommandBufferAndWaitForQueueIdle(&cmd_buf, &app.present_queue());

    VkCommandBufferBeginInfo info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // sType
        nullptr,                                      // pNext
        0,                                            // flags
        nullptr,                                      // pInheritanceInfo
    };
    cmd_buf->vkBeginCommandBuffer(cmd_buf, &info);
    VkClearColorValue clear_color_value;
    clear_color_value.float32[0] = 0.2f;
    clear_color_value.float32[1] = 0.2f;
    clear_color_value.float32[2] = 0.2f;
    clear_color_value.float32[3] = 0.2f;
    VkImageSubresourceRange range{
        VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask;
        0,                          // baseMipLevel
        1,                          // levelCount
        0,                          // baseArrayLayer
        1,                          // layerCount
    };
    cmd_buf->vkCmdClearColorImage(cmd_buf, raw_image_to_present,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  &clear_color_value, 1, &range);
    cmd_buf->vkEndCommandBuffer(cmd_buf);
    VkPipelineStageFlags pipe_stage_flags =
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        NULL,                           // pNext
        1,                              // waitSemaphoreCount
        &raw_semaphore,                 // pWaitSemaphores
        &pipe_stage_flags,              // pWaitDstStageMask
        1,                              // commandBufferCount
        &raw_cmd_buf,                   // pCommandBuffers
        0,                              // signalSemaphoreCount
        NULL,                           // pSignalSemaphores
    };
    app.present_queue()->vkQueueSubmit(app.present_queue(), 1, &submit_info,
                                       (::VkFence)VK_NULL_HANDLE);
    app.present_queue()->vkQueueWaitIdle(app.present_queue());

    // Set the layout of the image to be presented from
    // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    app.BeginCommandBuffer(&cmd_buf);
    RecordImageLayoutTransition(
        raw_image_to_present, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, static_cast<VkAccessFlags>(0u),
        &cmd_buf);
    app.EndAndSubmitCommandBufferAndWaitForQueueIdle(&cmd_buf, &app.present_queue());


    // Call vkQueuePresentKHR()
    ::VkSwapchainKHR raw_swapchain = app.swapchain().get_raw_object();
    VkPresentInfoKHR present_info{
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,  // sType
        nullptr,                             // pNext
        0,                                   // waitSemaphoreCount
        nullptr,                             // pWaitSemaphores
        1,                                   // swapchainCount
        &raw_swapchain,                      // pSwapchains
        &image_index,                        // pImageIndices
        nullptr,                             // pResults
    };
    LOG_ASSERT(==, data->logger(), app.present_queue()->vkQueuePresentKHR(
                                  app.present_queue(), &present_info),
               VK_SUCCESS);
  }

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
