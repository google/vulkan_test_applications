// Copyright 2022 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <chrono>

#include "mathfu/matrix.h"
#include "mathfu/vector.h"
#include "shared_data.h"
#include "support/entry/entry.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

template <typename T>
struct range_internal {
  struct range_end_iterator {
    T val;
  };
  struct range_iterator {
    T current;
    T step;
    range_iterator& operator++() {
      current += step;
      return *this;
    }
    bool operator!=(const range_end_iterator& e) const {
      if (step > 0) {
        return current < e.val;
      }
      return current > e.val;
    }
    const T& operator*() const { return current; }
  };
  explicit range_internal(T len) : first{0, 1}, last{len} {}
  range_internal(T begin, T end, T step = 1) : first{begin, step}, last{end} {}
  range_iterator begin() const { return first; }
  range_end_iterator end() const { return last; }

  range_iterator first;
  range_end_iterator last;
};

template <typename T>
typename range_internal<T> range(T len) {
  return range_internal<T>(len);
}

template <typename T>
typename range_internal<T> range(T begin, T end, T step = 1) {
  return range_internal<T>(begin, end, step);
}

static const size_t k_buffering_count = 2;

// This sample will create N storage buffers, and bind them to successive
// locations in a compute shader.
int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  HANDLE hMapFile;
  char* mapped_buffer;
  HANDLE file = CreateFile("E:\\test.txt", GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, 0, 0);
  hMapFile =
      CreateFileMappingA(file, NULL, PAGE_READWRITE, 0, k_file_mapping_size, 0);
  mapped_buffer = reinterpret_cast<char*>(
      MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, k_file_mapping_size));
  image_mapping_header* header =
      reinterpret_cast<image_mapping_header*>(mapped_buffer);

  vulkan::VulkanApplication app(
      data->allocator(), data->logger(), data,
      vulkan::VulkanApplicationOptions()
          .SetCoherentBufferSize(1024 * 1024 * 64)
          .SetMinSwapchainImageCount(3)
          //.SetPreferredPresentMode(VK_PRESENT_MODE_MAILBOX_KHR));
          .SetPreferredPresentMode(VK_PRESENT_MODE_FIFO_KHR));
  vulkan::VkDevice& device = app.device();
  const uint32_t width = app.swapchain().width();
  const uint32_t height = app.swapchain().height();

  containers::vector<vulkan::VkFence> fences(data->allocator());
  containers::vector<vulkan::VkSemaphore> image_acquired_semaphores(
      data->allocator());
  containers::vector<vulkan::VkSemaphore> semaphores(data->allocator());
  containers::vector<vulkan::VkCommandBuffer> command_buffers(
      data->allocator());
  containers::vector<vulkan::BufferPointer> src_buffers(data->allocator());

  for (size_t i : range(k_buffering_count)) {
    VkBufferCreateInfo create_info{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // flags
        width * height * 4,                    // size
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,      // usage
        VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
        0,
        nullptr};

    src_buffers.push_back(app.CreateAndBindCoherentBuffer(&create_info));

    {
      VkFenceCreateInfo create_info = {
          VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,  // sType
          nullptr,                              // pNext
          VK_FENCE_CREATE_SIGNALED_BIT          // flags
      };
      ::VkFence raw_fence;
      device->vkCreateFence(device, &create_info, nullptr, &raw_fence);
      fences.push_back(vulkan::VkFence(raw_fence, nullptr, &device));
    }
    {
      VkSemaphoreCreateInfo create_info = {
          VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,  // sType
          nullptr,                                  // pNext,
          0,                                        // flags
      };
      ::VkSemaphore raw_semaphore;
      device->vkCreateSemaphore(device, &create_info, nullptr, &raw_semaphore);
      image_acquired_semaphores.push_back(
          vulkan::VkSemaphore(raw_semaphore, nullptr, &device));

      device->vkCreateSemaphore(device, &create_info, nullptr, &raw_semaphore);
      semaphores.push_back(
          vulkan::VkSemaphore(raw_semaphore, nullptr, &device));
    }

    command_buffers.push_back(app.GetCommandBuffer(app.render_queue().index()));
  }

  auto last_frame_time = std::chrono::high_resolution_clock::now();
  while (header->image_to_read == ~static_cast<uint64_t>(0)) {
    MemoryBarrier();
  }
  size_t frame_num = 0;
  size_t x = 0;
  size_t total_frame_num = 0;
  auto last_reported_time = last_frame_time;
  size_t last_reported_frame = 0;
  uint64_t last_frame_processed = ~static_cast<uint64_t>(0);
  uint64_t dropped_frames = 0;
  while (true) {
    auto current_time = std::chrono::high_resolution_clock::now();
    if (current_time - last_reported_time > std::chrono::seconds(1)) {
      data->logger()->LogInfo("Number of frames processed in the last second: ",
                              total_frame_num - last_reported_frame);
      data->logger()->LogInfo("Number of Dropped frames in the last second: ",
                              dropped_frames);
      last_reported_time = current_time;
      last_reported_frame = total_frame_num;
      dropped_frames = 0;
    }
    total_frame_num++;
    // The spec defines std::chrono::duration<float> to be time in seconds.
    auto time_diff = std::chrono::duration_cast<std::chrono::duration<float>>(
        current_time - last_frame_time);
    last_frame_time = current_time;

    // Step 1 wait until the resoures from the PREVIOUS render are done
    device->vkWaitForFences(device, 1, &fences[frame_num].get_raw_object(),
                            VK_TRUE, 0xFFFFFFFFFFFFFFFF);
    device->vkResetFences(device, 1, &fences[frame_num].get_raw_object());
    uint32_t framebuffer_index;
    device->vkAcquireNextImageKHR(device, app.swapchain(), 0xFFFFFFFFFFFFFFFF,
                                  image_acquired_semaphores[frame_num],
                                  VK_NULL_HANDLE, &framebuffer_index);

    auto& cb = command_buffers[frame_num];
    cb->vkResetCommandBuffer(cb, 0);
    app.BeginCommandBuffer(&cb);

    // We always transition from undefined because we just don't care about the
    // previous contents.
    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
        nullptr,                                 // pNext
        0,                                       // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,            // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,               // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,    // newLayout
        VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                 // dstQueueFamilyIndex
        app.swapchain_images()[framebuffer_index],
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    cb->vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

    VkBufferImageCopy copy = {
        0,  // bufferOffset
        0,  // bufferRowLength
        0,  // bufferImageHeight
        VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        VkOffset3D{0, 0, 0},
        VkExtent3D{width, height, 1}};

    cb->vkCmdCopyBufferToImage(cb, *src_buffers[frame_num],
                               app.swapchain_images()[framebuffer_index],
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = 0;
    cb->vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);
    while (header->frame_num == last_frame_processed) {
      MemoryBarrier();
    }
    while (InterlockedCompareExchange64(&header->header_lock, 1, 0) != 0) {
    }
    dropped_frames += (header->frame_num - last_frame_processed) - 1;
    last_frame_processed = header->frame_num;
    uint64_t image = header->image_to_read;
    header->image_being_read = image;
    InterlockedExchange64(&header->header_lock, 0);

    memcpy(src_buffers[frame_num]->base_address(),
           mapped_buffer + header->image_offsets[image],
           header->width * header->height * 4);
    header->image_being_read = ~static_cast<uint64_t>(0);
    app.EndAndSubmitCommandBuffer(
        &cb, &app.render_queue(), {image_acquired_semaphores[frame_num]},
        {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        {semaphores[frame_num]}, fences[frame_num]);

    VkPresentInfoKHR present_info = {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,       // sType
        nullptr,                                  // pNext
        1,                                        // waitSemaphoreCount
        &semaphores[frame_num].get_raw_object(),  // pWaitSemaphores
        1,                                        // swapchainCount
        &app.swapchain().get_raw_object(),        // pSwapchains
        &framebuffer_index,                       // pImageIndices
        nullptr,                                  // pResults
    };

    app.render_queue()->vkQueuePresentKHR(app.render_queue(), &present_info);

    frame_num = (frame_num + 1) % k_buffering_count;
    // Nothing goes after this statement in the loop
  }
}