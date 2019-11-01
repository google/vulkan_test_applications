// Copyright 2019 Google Inc.
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

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "external_buffer.h"
#include "mathfu/matrix.h"
#include "support/entry/entry.h"
#include "vulkan_helpers/vulkan_application.h"

#ifdef __linux__
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#endif

using Mat44 = mathfu::Matrix<float, 4, 4>;

struct ModelData {
  Mat44 transform;
};

struct FrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer;
  containers::unique_ptr<vulkan::VkFence> free_fence;
  containers::unique_ptr<vulkan::VkFence> ready_fence;
};

int main_entry(const entry::EntryData* data) {
  logging::Logger* log = data->logger();
  log->LogInfo("Application Startup");

  vulkan::VulkanApplication app(
      data->allocator(), data->logger(), data,
      {VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
       VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME},
      {VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
       VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME,
#ifdef _WIN32
       VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
       VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME
#elif __linux__
       VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
       VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME
#endif
      });

  vulkan::VkDevice& device = app.device();

  size_t num_images = app.swapchain_images().size();

  containers::unique_ptr<VkBufferExported<ModelData>> model_data =
      containers::make_unique<VkBufferExported<ModelData>>(
          data->allocator(), device, data->logger(), num_images);

  VkBufferCreateInfo create_info = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,          // sType
      nullptr,                                       // pNext
      0,                                             // flags
      model_data->aligned_data_size() * num_images,  // size
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,              // usage
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr};

  containers::unique_ptr<vulkan::VulkanApplication::Buffer> host_buffer =
      app.CreateAndBindHostBuffer(&create_info);

  containers::vector<FrameData> frame_data(data->allocator());
  frame_data.resize(num_images);

  for (int i = 0; i < num_images; i++) {
    FrameData& frame_data_i = frame_data[i];

    frame_data_i.command_buffer =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data->allocator(), app.GetCommandBuffer());

    vulkan::VkCommandBuffer& cmdBuffer = *frame_data_i.command_buffer;
    cmdBuffer->vkBeginCommandBuffer(cmdBuffer,
                                    &sample_application::kBeginCommandBuffer);
    VkBufferCopy region{model_data->get_offset_for_frame(i),
                        model_data->get_offset_for_frame(i),
                        model_data->size()};
    cmdBuffer->vkCmdCopyBuffer(cmdBuffer, *host_buffer,
                               model_data->get_buffer(), 1, &region);
    cmdBuffer->vkEndCommandBuffer(cmdBuffer);

    VkFence fence;
    VkExportFenceCreateInfo fence_export_info{
        VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO, nullptr,
#ifdef _WIN32
        VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT
#elif __linux__
        VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT
#endif
    };
    VkFenceCreateInfo fence_create_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                        &fence_export_info,
                                        VK_FENCE_CREATE_SIGNALED_BIT};

    LOG_ASSERT(
        ==, log, VK_SUCCESS,
        device->vkCreateFence(device, &fence_create_info, nullptr, &fence));
    frame_data_i.free_fence = containers::make_unique<vulkan::VkFence>(
        data->allocator(), fence, nullptr, &device);

    fence_create_info.flags = 0;
    LOG_ASSERT(
        ==, log, VK_SUCCESS,
        device->vkCreateFence(device, &fence_create_info, nullptr, &fence));
    frame_data_i.ready_fence = containers::make_unique<vulkan::VkFence>(
        data->allocator(), fence, nullptr, &device);
  }

  VkSubmitInfo submit_info{
      VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
      nullptr,                        // pNext
      0,                              // waitSemaphoreCount
      nullptr,                        // pWaitSemaphores
      nullptr,                        // pWaitDstStageMask,
      1,                              // commandBufferCount
      nullptr,                        //
      0,                              // signalSemaphoreCount
      nullptr                         // pSignalSemaphores
  };

#ifdef _WIN32
  HANDLE pipe_handle;

  HANDLE model_data_handle = model_data->getMemoryWin32Handle();

  pipe_handle = CreateNamedPipe(
      TEXT("\\\\.\\pipe\\LOCAL\\vulkan_external_buffer_example"),
      PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1,
      1024 * 16, 1024 * 16, NMPWAIT_USE_DEFAULT_WAIT, NULL);
  if (ConnectNamedPipe(pipe_handle, NULL) != FALSE) {
    ULONG pid;
    GetNamedPipeClientProcessId(pipe_handle, &pid);

    HANDLE client_process = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid);
    HANDLE current_process = GetCurrentProcess();
    containers::vector<HANDLE> client_data_handles(data->allocator());
    client_data_handles.resize(1 + num_images * 2);

    DuplicateHandle(current_process, model_data_handle, client_process,
                    &client_data_handles[0], 0, FALSE,
                    DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    for (int i = 0; i < num_images; i++) {
      FrameData& frame_data_i = frame_data[i];
      VkFenceGetWin32HandleInfoKHR export_info{
          VK_STRUCTURE_TYPE_FENCE_GET_WIN32_HANDLE_INFO_KHR, nullptr,
          frame_data_i.free_fence->get_raw_object(),
          VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT};
      HANDLE fence_handle;

      device->vkGetFenceWin32HandleKHR(device, &export_info, &fence_handle);

      DuplicateHandle(current_process, fence_handle, client_process,
                      &client_data_handles[1 + i * 2], 0, FALSE,
                      DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

      export_info.fence = frame_data_i.ready_fence->get_raw_object();
      device->vkGetFenceWin32HandleKHR(device, &export_info, &fence_handle);
      DuplicateHandle(current_process, fence_handle, client_process,
                      &client_data_handles[2 + i * 2], 0, FALSE,
                      DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    }

    DWORD bytes_writen;
    WriteFile(pipe_handle, client_data_handles.data(),
              sizeof(HANDLE) * client_data_handles.size(), &bytes_writen, NULL);
    FlushFileBuffers(pipe_handle);

    CloseHandle(current_process);
    CloseHandle(client_process);
  }
  DisconnectNamedPipe(pipe_handle);
#elif __linux__
  containers::vector<int> file_descriptors(data->allocator());
  file_descriptors.resize(1 + num_images * 2);

  file_descriptors[0] = model_data->getMemoryFd();
  for (int i = 0; i < num_images; i++) {
    FrameData& frame_data_i = frame_data[i];
    VkFenceGetFdInfoKHR export_info{
        VK_STRUCTURE_TYPE_FENCE_GET_FD_INFO_KHR, nullptr,
        frame_data_i.free_fence->get_raw_object(),
        VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT};

    device->vkGetFenceFdKHR(device, &export_info, &file_descriptors[1 + i * 2]);

    export_info.fence = frame_data_i.ready_fence->get_raw_object();
    device->vkGetFenceFdKHR(device, &export_info, &file_descriptors[2 + i * 2]);
  }

  int sock = socket(AF_UNIX, SOCK_STREAM, 0);

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(&addr.sun_path[1], "vulkan_external_buffer_example");
  bind(sock, (struct sockaddr*)&addr, sizeof(addr));

  listen(sock, 1);
  int conn = accept(sock, NULL, 0);

  struct msghdr msg;
  struct iovec iov[1];
  containers::vector<char> ctrl_buf(data->allocator());
  ctrl_buf.resize(CMSG_SPACE(sizeof(int) * file_descriptors.size()), 0);

  memset(&msg, 0, sizeof(struct msghdr));

  char sock_data[1] = {' '};
  iov[0].iov_base = sock_data;
  iov[0].iov_len = sizeof(sock_data);

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_controllen = CMSG_SPACE(sizeof(int) * file_descriptors.size());
  msg.msg_control = ctrl_buf.data();

  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(int) * file_descriptors.size());

  memcpy(CMSG_DATA(cmsg), file_descriptors.data(),
         sizeof(int) * file_descriptors.size());

  sendmsg(conn, &msg, 0);

  close(conn);
  close(sock);

  for (int fd : file_descriptors) {
    close(fd);
  }
#endif

  float speed = 0.00001f;
  ModelData model{Mat44::FromTranslationVector(
      mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f})};

  int i = 0;

  while (true) {
    FrameData& frame_data_i = frame_data[i];
    device->vkWaitForFences(device, 1,
                            &frame_data_i.free_fence->get_raw_object(), VK_TRUE,
                            UINT64_MAX);
    device->vkResetFences(device, 1,
                          &frame_data_i.free_fence->get_raw_object());
    model.transform =
        model.transform *
        Mat44::FromRotationMatrix(Mat44::RotationX(3.14f * speed) *
                                  Mat44::RotationY(3.14f * speed * 0.5f));

    memcpy(host_buffer->base_address() + model_data->aligned_data_size() * i++,
           &model, sizeof(model));
    i %= num_images;

    submit_info.pCommandBuffers =
        &frame_data_i.command_buffer->get_command_buffer();

    LOG_ASSERT(==, log, VK_SUCCESS,
               app.render_queue()->vkQueueSubmit(
                   app.render_queue(), 1, &submit_info,
                   frame_data_i.ready_fence->get_raw_object()));
  }

  log->LogInfo("Application Shutdown");
  return 0;
}
