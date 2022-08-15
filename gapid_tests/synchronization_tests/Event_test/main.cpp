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

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

#include "support/entry/entry.h"
#include "support/log/log.h"
#include "vulkan_helpers/known_device_infos.h"
#include "vulkan_helpers/structs.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"
#include "vulkan_wrapper/sub_objects.h"

namespace {
// Records a vkCmdWaitEvents command without any memory barrier in the command
// buffer.
void AddCmdWaitEvents(containers::Allocator* allocator,
                      vulkan::VkCommandBuffer* cmd_buf_ptr,
                      std::initializer_list<::VkEvent> wait_events,
                      VkPipelineStageFlags src_stages,
                      VkPipelineStageFlags dst_stages) {
  if (!cmd_buf_ptr) {
    return;
  }
  auto& cmd_buf = *cmd_buf_ptr;
  containers::vector<::VkEvent> events(wait_events, allocator);
  cmd_buf->vkCmdWaitEvents(cmd_buf,
                           events.size(),  // eventCount
                           events.data(),  // pEvents
                           src_stages,     // srcStageMask
                           dst_stages,     // dstStageMask
                           0,              // memoryBarrierCount
                           nullptr,        // pMemoryBarriers
                           0,              // bufferMemoryBarrierCount
                           nullptr,        // pBufferMemoryBarriers
                           0,              // imageMemoryBarrierCount
                           nullptr         // pImageMemoryBarriers
  );
}

// Records a vkCmdCopyBuffer to the command buffer, which copies from the src
// buffer to the dst buffer. The copy offsets are 0 for both src and dst
// buffer, the copy size is the min(src_buffer_size, dst_buffer_size)
void AddCmdCopyBuffer(vulkan::VkCommandBuffer* cmd_buf_ptr,
                      const vulkan::VulkanApplication::Buffer& src_buf,
                      const vulkan::VulkanApplication::Buffer& dst_buf) {
  if (!cmd_buf_ptr) {
    return;
  }
  auto& cmd_buf = *cmd_buf_ptr;
  VkDeviceSize size =
      src_buf.size() < dst_buf.size() ? src_buf.size() : dst_buf.size();
  VkBufferCopy copy_region{0, 0, size};
  cmd_buf->vkCmdCopyBuffer(cmd_buf, src_buf, dst_buf, 1, &copy_region);
}

void RunInTwoThreads(std::function<void(std::function<void()>)> run_first,
                     std::function<void()> run_second) {
  std::mutex m;
  std::condition_variable cv;
  bool second_thread_can_run = false;
  auto start_second_thread = [&]() {
    {
      std::lock_guard<std::mutex> lk(m);
      second_thread_can_run = true;
    }
    cv.notify_all();
  };
  std::thread first_thread([&]() { run_first(start_second_thread); });
  std::unique_lock<std::mutex> lk(m);
  cv.wait(lk, [&] { return second_thread_can_run; });
  run_second();
  first_thread.join();
}

struct CommonHandles {
  CommonHandles(const entry::EntryData* data, vulkan::VulkanApplication* app)
      : device(app->device()),
        queue(app->render_queue()),
        cmd_buf(app->GetCommandBuffer()),
        src_buf(app->CreateAndBindDefaultExclusiveCoherentBuffer(
            sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT)),
        dst_buf(app->CreateAndBindDefaultExclusiveDeviceBuffer(
            sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT)) {}

  vulkan::VkDevice& device;
  vulkan::VkQueue& queue;
  vulkan::VkCommandBuffer cmd_buf;
  vulkan::BufferPointer src_buf;
  vulkan::BufferPointer dst_buf;
};
}  // anonymous namespace

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  vulkan::VulkanApplication app(data->allocator(), data->logger(), data,
                                vulkan::VulkanApplicationOptions());
  // Basic test of vkSetEvent, vkResetEvent and vkGetEventStatus
  {
    // Use "TAG" in the trace to figure out where we are supposed to be
    app.device().getProcAddrFunction()(app.device(), "TAG");
    CommonHandles t(data, &app);

    auto event = vulkan::CreateEvent(&t.device);
    LOG_EXPECT(==, data->logger(), VK_EVENT_RESET,
               t.device->vkGetEventStatus(t.device, event));
    LOG_EXPECT(==, data->logger(), VK_SUCCESS,
               t.device->vkSetEvent(t.device, event));
    LOG_EXPECT(==, data->logger(), VK_EVENT_SET,
               t.device->vkGetEventStatus(t.device, event));
    LOG_EXPECT(==, data->logger(), VK_SUCCESS,
               t.device->vkResetEvent(t.device, event));
    LOG_EXPECT(==, data->logger(), VK_EVENT_RESET,
               t.device->vkGetEventStatus(t.device, event));
    // vkDestroyEvent will be called when 'event' is out of scope.
  }

  // Single thread
  {
    app.device().getProcAddrFunction()(app.device(), "TAG");
    CommonHandles t(data, &app);

    auto event_x = vulkan::CreateEvent(&t.device);
    auto event_y = vulkan::CreateEvent(&t.device);

    // submit -> update -> set -> wait idle
    *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x00000000;
    app.BeginCommandBuffer(&t.cmd_buf);
    AddCmdWaitEvents(data->allocator(), &t.cmd_buf, {event_x},
                     VK_PIPELINE_STAGE_HOST_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT);
    AddCmdCopyBuffer(&t.cmd_buf, *t.src_buf, *t.dst_buf);
    app.EndAndSubmitCommandBuffer(&t.cmd_buf, &t.queue, {}, {}, {},
                                  VkFence(VK_NULL_HANDLE));
    *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x11111111;
    t.device->vkSetEvent(t.device, event_x);
    t.queue->vkQueueWaitIdle(t.queue);
    t.device->vkResetEvent(t.device, event_x);

    // update -> set -> submit -> wait idle
    *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x22222222;
    app.BeginCommandBuffer(&t.cmd_buf);
    t.device->vkSetEvent(t.device, event_x);
    AddCmdWaitEvents(data->allocator(), &t.cmd_buf, {event_x},
                     VK_PIPELINE_STAGE_HOST_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT);
    AddCmdCopyBuffer(&t.cmd_buf, *t.src_buf, *t.dst_buf);
    app.EndAndSubmitCommandBuffer(&t.cmd_buf, &t.queue, {}, {}, {},
                                  VkFence(VK_NULL_HANDLE));
    t.queue->vkQueueWaitIdle(t.queue);
    t.device->vkResetEvent(t.device, event_x);

    // update -> submit:[cmdSetEvent (multiple),  ... , cmdWaitEvents]
    app.BeginCommandBuffer(&t.cmd_buf);
    t.cmd_buf->vkCmdSetEvent(t.cmd_buf, event_x,
                             VK_PIPELINE_STAGE_TRANSFER_BIT);
    t.cmd_buf->vkCmdSetEvent(t.cmd_buf, event_y,
                             VK_PIPELINE_STAGE_TRANSFER_BIT);
    AddCmdWaitEvents(data->allocator(), &t.cmd_buf, {event_x, event_y},
                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT);
    AddCmdCopyBuffer(&t.cmd_buf, *t.src_buf, *t.dst_buf);
    *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x33333333;
    app.EndAndSubmitCommandBufferAndWaitForQueueIdle(&t.cmd_buf, &t.queue);
    t.device->vkResetEvent(t.device, event_x);
    t.device->vkResetEvent(t.device, event_y);

    // update -> submit:[cmdSetEvent] -> submit:[cmdWaitEvents, ...]
    app.BeginCommandBuffer(&t.cmd_buf);
    t.cmd_buf->vkCmdSetEvent(t.cmd_buf, event_x,
                             VK_PIPELINE_STAGE_TRANSFER_BIT);
    app.EndAndSubmitCommandBuffer(&t.cmd_buf, &t.queue, {}, {}, {},
                                  ::VkFence(VK_NULL_HANDLE));
    auto another_cmd_buf = app.GetCommandBuffer();
    app.BeginCommandBuffer(&another_cmd_buf);
    AddCmdWaitEvents(data->allocator(), &another_cmd_buf, {event_x},
                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT);
    AddCmdCopyBuffer(&another_cmd_buf, *t.src_buf, *t.dst_buf);
    *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x44444444;
    app.EndAndSubmitCommandBufferAndWaitForQueueIdle(&another_cmd_buf,
                                                     &t.queue);
    t.device->vkResetEvent(t.device, event_x);
  }

  // Multiple thread, host sends signal to the event waiting in a t.queue
  {
    app.device().getProcAddrFunction()(app.device(), "TAG");
    CommonHandles t(data, &app);
    auto event_x = vulkan::CreateEvent(&t.device);
    auto event_y = vulkan::CreateEvent(&t.device);
    *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x00000000;

    // Thread 1: submit [vkCmdWaitEvents] ->        -> t.queue wait idle
    // Thread 2:                            setEvent
    app.BeginCommandBuffer(&t.cmd_buf);
    AddCmdWaitEvents(data->allocator(), &t.cmd_buf, {event_x},
                     VK_PIPELINE_STAGE_HOST_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT);
    AddCmdCopyBuffer(&t.cmd_buf, *t.src_buf, *t.dst_buf);
    std::function<void(std::function<void()>)> run_first =
        [&](std::function<void()> start_second_thread) {
          app.EndAndSubmitCommandBuffer(&t.cmd_buf, &t.queue, {}, {}, {},
                                        VkFence(VK_NULL_HANDLE));
          start_second_thread();
          t.queue->vkQueueWaitIdle(t.queue);
        };
    std::function<void()> run_second = [&]() {
      *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x11111111;
      t.device->vkSetEvent(t.device, event_x);
    };
    RunInTwoThreads(run_first, run_second);
    t.device->vkResetEvent(t.device, event_x);

    // Thread 1: submit [vkCmdWaitEvents (multiple events)] ->    -> t.queue
    // idle
    // Thread 2:                                           setEvent(s)
    app.BeginCommandBuffer(&t.cmd_buf);
    AddCmdWaitEvents(data->allocator(), &t.cmd_buf, {event_x, event_y},
                     VK_PIPELINE_STAGE_HOST_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT);
    AddCmdCopyBuffer(&t.cmd_buf, *t.src_buf, *t.dst_buf);
    run_second = [&]() {
      *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x22222222;
      t.device->vkSetEvent(t.device, event_x);
      *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x33333333;
      t.device->vkSetEvent(t.device, event_y);
    };
    RunInTwoThreads(run_first, run_second);
    t.device->vkResetEvent(t.device, event_x);
    t.device->vkResetEvent(t.device, event_y);

    // Thread 1: submit [wait, reset, semaphore, fence]-> submit [wait]-> idle
    // Thread 2: setEvent -> setEvent
    vulkan::VkFence fence = vulkan::CreateFence(&t.device, false);
    run_first = [&](std::function<void()> start_second_thread) {
      vulkan::VkSemaphore semaphore = vulkan::CreateSemaphore(&t.device);
      VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      app.BeginCommandBuffer(&t.cmd_buf);
      AddCmdWaitEvents(data->allocator(), &t.cmd_buf, {event_x},
                       VK_PIPELINE_STAGE_HOST_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT);
      AddCmdCopyBuffer(&t.cmd_buf, *t.src_buf, *t.dst_buf);
      t.cmd_buf->vkCmdResetEvent(t.cmd_buf, event_x,
                                 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
      app.EndAndSubmitCommandBuffer(&t.cmd_buf, &t.queue, {}, {},
                                    {semaphore.get_raw_object()}, fence);
      auto another_cmd_buf = app.GetCommandBuffer();
      app.BeginCommandBuffer(&another_cmd_buf);
      AddCmdWaitEvents(data->allocator(), &another_cmd_buf, {event_x},
                       VK_PIPELINE_STAGE_HOST_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT);
      AddCmdCopyBuffer(&another_cmd_buf, *t.src_buf, *t.dst_buf);
      app.EndAndSubmitCommandBuffer(&another_cmd_buf, &t.queue,
                                    {semaphore.get_raw_object()}, {wait_stage},
                                    {}, ::VkFence(VK_NULL_HANDLE));
      start_second_thread();
      t.queue->vkQueueWaitIdle(t.queue);
    };
    run_second = [&]() {
      *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x44444444;
      t.device->vkSetEvent(t.device, event_x);
      t.device->vkWaitForFences(t.device, 1, &fence.get_raw_object(), true,
                                uint64_t(0) - 1);
      *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x55555555;
      t.device->vkSetEvent(t.device, event_x);
    };
    RunInTwoThreads(run_first, run_second);
    t.device->vkResetEvent(t.device, event_x);

    // Thread 1: submit [wait x, wait y, copy]          -> idle
    // Thread 2:                         set y -> set x
    app.BeginCommandBuffer(&t.cmd_buf);
    AddCmdWaitEvents(data->allocator(), &t.cmd_buf, {event_x},
                     VK_PIPELINE_STAGE_HOST_BIT,
                     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    AddCmdWaitEvents(data->allocator(), &t.cmd_buf, {event_y},
                     VK_PIPELINE_STAGE_HOST_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT);
    AddCmdCopyBuffer(&t.cmd_buf, *t.src_buf, *t.dst_buf);
    run_first = [&](std::function<void()> start_second_thread) {
      app.EndAndSubmitCommandBuffer(&t.cmd_buf, &t.queue, {}, {}, {},
                                    ::VkFence(VK_NULL_HANDLE));
      start_second_thread();
      t.queue->vkQueueWaitIdle(t.queue);
    };
    run_second = [&]() {
      *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x66666666;
      t.device->vkSetEvent(t.device, event_y);
      *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x77777777;
      t.device->vkSetEvent(t.device, event_x);
    };
    RunInTwoThreads(run_first, run_second);
    t.device->vkResetEvent(t.device, event_x);
    t.device->vkResetEvent(t.device, event_y);
  }

  // Test for memory barriers carried with vkCmdWaitEvents
  {
    app.device().getProcAddrFunction()(app.device(), "TAG");
    CommonHandles t(data, &app);
    vulkan::VkImage img = vulkan::CreateDefault2DColorImage(
        &t.device, app.swapchain().width(), app.swapchain().height());
    auto event_x = vulkan::CreateEvent(&t.device);
    *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0x00000000;

    // barriers
    VkMemoryBarrier memory_barrier{
        VK_STRUCTURE_TYPE_MEMORY_BARRIER,  // sType
        nullptr,                           // pNext
        VK_ACCESS_HOST_WRITE_BIT,          // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,  // dstAccessMask
    };
    VkBufferMemoryBarrier buffer_barriers[2] = {
        {
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // stype
            nullptr,                                  // pNext
            0,                                        // srcAccessMask
            VK_ACCESS_TRANSFER_WRITE_BIT,             // dstAccessMask
            VK_QUEUE_FAMILY_IGNORED,                  // srcQueueFamilyIndex
            VK_QUEUE_FAMILY_IGNORED,                  // dstQueueFamilyIndex
            *t.dst_buf,                               // buffer
            0,                                        // offset
            sizeof(uint32_t)                          // size
        },
        {
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // stype
            nullptr,                                  // pNext
            VK_ACCESS_HOST_WRITE_BIT,                 // srcAccessMask
            VK_ACCESS_TRANSFER_READ_BIT,              // dstAccessMask
            VK_QUEUE_FAMILY_IGNORED,                  // srcQueueFamilyIndex
            VK_QUEUE_FAMILY_IGNORED,                  // dstQueueFamilyIndex
            *t.src_buf,                               // buffer
            0,                                        // offset
            sizeof(uint32_t)                          // size
        }};
    VkImageSubresourceRange rng{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkImageMemoryBarrier image_barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
        nullptr,                                 // pNext
        0,                                       // srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,    // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,               // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,    // newLayout
        VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                 // dstQueueFamilyIndex
        img,                                     // image
        rng,                                     // subresourceRange
    };

    app.BeginCommandBuffer(&t.cmd_buf);
    t.cmd_buf->vkCmdWaitEvents(
        t.cmd_buf, 1, &event_x.get_raw_object(), VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 1, &memory_barrier, 2, buffer_barriers,
        1, &image_barrier);
    AddCmdCopyBuffer(&t.cmd_buf, *t.src_buf, *t.dst_buf);
    auto run_first = [&](std::function<void()> start_second_thread) {
      app.EndAndSubmitCommandBuffer(&t.cmd_buf, &t.queue, {}, {}, {},
                                    ::VkFence(0));
      start_second_thread();
      t.queue->vkQueueWaitIdle(t.queue);
    };
    auto run_second = [&]() {
      *reinterpret_cast<uint32_t*>(t.src_buf->base_address()) = 0xFFFFFFFF;
      t.device->vkSetEvent(t.device, event_x);
    };
    RunInTwoThreads(run_first, run_second);
  }
  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
