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
#include "mathfu/matrix.h"
#include "support/entry/entry.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

#ifdef __linux__
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#endif

using Mat44 = mathfu::Matrix<float, 4, 4>;

struct camera_data_ {
  Mat44 projection_matrix;
};

struct model_data_ {
  Mat44 transform;
};

namespace cube_model {
#include "cube.obj.h"
}
const auto& cube_data = cube_model::model;

uint32_t cube_vertex_shader[] =
#include "cube.vert.spv"
    ;

uint32_t cube_fragment_shader[] =
#include "cube.frag.spv"
    ;

class VkImagesExported {
 public:
  VkImagesExported(vulkan::VkDevice& device, logging::Logger* log,
                   containers::Allocator* allocator, size_t num_images,
                   const VkImageCreateInfo* create_info)
      : device_(device),
        log_(log),
        device_memory_(VK_NULL_HANDLE, nullptr, &device),
        images_(allocator) {
    images_.resize(num_images);

    for (int i = 0; i < num_images; i++) {
      ::VkImage image;
      LOG_ASSERT(==, log_,
                 device_->vkCreateImage(device_, create_info, nullptr, &image),
                 VK_SUCCESS);
      images_[i] = containers::make_unique<vulkan::VkImage>(allocator, image,
                                                            nullptr, &device_);
    }

    ::VkImage image = *images_[0];

    VkMemoryRequirements requirements;
    device_->vkGetImageMemoryRequirements(device_, image, &requirements);

    aligned_data_size_ =
        vulkan::RoundUp(requirements.size, requirements.alignment);

    uint32_t memory_index =
        vulkan::GetMemoryIndex(&device, log, requirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkExportMemoryAllocateInfo export_allocate_info{
        VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO, nullptr,
#ifdef _WIN32
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
#elif __linux__
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
#endif
    };

    VkMemoryAllocateInfo allocate_info{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // sType
        &export_allocate_info,                   // pNext
        aligned_data_size_ * num_images,         // allocationSize
        memory_index};

    VkDeviceMemory device_memory;

    LOG_ASSERT(==, log, VK_SUCCESS,
               device->vkAllocateMemory(device, &allocate_info, nullptr,
                                        &device_memory));
    device_memory_.initialize(device_memory);

    for (int i = 0; i < num_images; i++) {
      device_->vkBindImageMemory(device_, *images_[i], device_memory,
                                 aligned_data_size_ * i);
    }
  }

  ::VkImage get_image(size_t i) const { return *images_[i]; }

  size_t size() const { return aligned_data_size_; }

#ifdef _WIN32
  HANDLE getMemoryWin32Handle(void) {
    HANDLE handle;
    VkMemoryGetWin32HandleInfoKHR get_handle_info{
        VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,  // sType
        nullptr,                                             // pNext
        device_memory_, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT};
    LOG_ASSERT(
        ==, log_, VK_SUCCESS,
        device_->vkGetMemoryWin32HandleKHR(device_, &get_handle_info, &handle));
    return handle;
  }
#elif __linux__
  int getMemoryFd(void) {
    int file_descriptor;
    VkMemoryGetFdInfoKHR get_fd_info{
        VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR, nullptr, device_memory_,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT};
    LOG_ASSERT(
        ==, log_, VK_SUCCESS,
        device_->vkGetMemoryFdKHR(device_, &get_fd_info, &file_descriptor));
    return file_descriptor;
  }
#endif

 private:
  vulkan::VkDevice& device_;
  logging::Logger* log_;
  vulkan::VkDeviceMemory device_memory_;
  containers::vector<containers::unique_ptr<vulkan::VkImage>> images_;
  size_t aligned_data_size_;
};

struct FrameData {
  containers::unique_ptr<vulkan::VkFence> free_fence;
  containers::unique_ptr<vulkan::VkFence> ready_fence;
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_descriptor_set_;
  containers::unique_ptr<vulkan::VkImageView> render_img_view_;
};

int main_entry(const entry::EntryData* data) {
  logging::Logger* log = data->logger();
  log->LogInfo("Application Startup");

  vulkan::VulkanApplication app(
      data->allocator(), data->logger(), data,
      vulkan::VulkanApplicationOptions(),
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

  size_t num_swapchain_images = app.swapchain_images().size();

  vulkan::VkCommandBuffer initialization_command_buffer =
      app.GetCommandBuffer();
  initialization_command_buffer->vkBeginCommandBuffer(
      initialization_command_buffer, &sample_application::kBeginCommandBuffer);

  vulkan::VulkanModel cube(data->allocator(), data->logger(), cube_data);
  cube.InitializeData(&app, &initialization_command_buffer);

  initialization_command_buffer->vkEndCommandBuffer(
      initialization_command_buffer);

  VkSubmitInfo init_submit_info = sample_application::kEmptySubmitInfo;
  init_submit_info.commandBufferCount = 1;
  init_submit_info.pCommandBuffers =
      &(initialization_command_buffer.get_command_buffer());

  vulkan::VkFence init_fence = vulkan::CreateFence(&device);

  app.render_queue()->vkQueueSubmit(app.render_queue(), 1, &init_submit_info,
                                    init_fence.get_raw_object());
  app.device()->vkWaitForFences(device, 1, &init_fence.get_raw_object(), false,
                                0xFFFFFFFFFFFFFFFF);

  VkDescriptorSetLayoutBinding cube_descriptor_set_layouts_[2] = {
      {
          0,                                  // binding
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
          1,                                  // descriptorCount
          VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
          nullptr                             // pImmutableSamplers
      },
      {
          1,                                  // binding
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
          1,                                  // descriptorCount
          VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
          nullptr                             // pImmutableSamplers
      }};

  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout =
      containers::make_unique<vulkan::PipelineLayout>(
          data->allocator(),
          app.CreatePipelineLayout({{cube_descriptor_set_layouts_[0],
                                     cube_descriptor_set_layouts_[1]}}));

  VkAttachmentReference color_attachment = {
      0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkFormat render_target_format = app.swapchain().format();

  uint32_t image_resolution = 1024;

  VkSampleCountFlagBits num_samples = VK_SAMPLE_COUNT_1_BIT;
  VkViewport viewport = {0.0f,
                         0.0f,
                         static_cast<float>(image_resolution),
                         static_cast<float>(image_resolution),
                         0.0f,
                         1.0f};

  VkRect2D scissor = {{0, 0}, {image_resolution, image_resolution}};

  containers::unique_ptr<vulkan::VkRenderPass> render_pass =
      containers::make_unique<vulkan::VkRenderPass>(
          data->allocator(),
          app.CreateRenderPass(
              {{
                  0,                                         // flags
                  render_target_format,                      // format
                  num_samples,                               // samples
                  VK_ATTACHMENT_LOAD_OP_CLEAR,               // loadOp
                  VK_ATTACHMENT_STORE_OP_STORE,              // storeOp
                  VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stenilLoadOp
                  VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stenilStoreOp
                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // initialLayout
                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL   // finalLayout
              }},  // AttachmentDescriptions
              {{
                  0,                                // flags
                  VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                  0,                                // inputAttachmentCount
                  nullptr,                          // pInputAttachments
                  1,                                // colorAttachmentCount
                  &color_attachment,                // colorAttachment
                  nullptr,                          // pResolveAttachments
                  nullptr,                          // pDepthStencilAttachment
                  0,                                // preserveAttachmentCount
                  nullptr                           // pPreserveAttachments
              }},                                   // SubpassDescriptions
              {}                                    // SubpassDependencies
              ));

  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> cube_pipeline =
      containers::make_unique<vulkan::VulkanGraphicsPipeline>(
          data->allocator(), app.CreateGraphicsPipeline(pipeline_layout.get(),
                                                        render_pass.get(), 0));
  cube_pipeline->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                           cube_vertex_shader);
  cube_pipeline->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                           cube_fragment_shader);
  cube_pipeline->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  cube_pipeline->SetInputStreams(&cube);
  cube_pipeline->SetViewport(viewport);
  cube_pipeline->SetScissor(scissor);
  cube_pipeline->SetSamples(num_samples);
  cube_pipeline->AddAttachment();
  cube_pipeline->Commit();

  containers::unique_ptr<vulkan::BufferFrameData<camera_data_>> camera_data =
      containers::make_unique<vulkan::BufferFrameData<camera_data_>>(
          data->allocator(), &app, num_swapchain_images,
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

  containers::unique_ptr<vulkan::BufferFrameData<model_data_>> model_data =
      containers::make_unique<vulkan::BufferFrameData<model_data_>>(
          data->allocator(), &app, num_swapchain_images,
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

  camera_data->data().projection_matrix =
      Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
      Mat44::Perspective(1.5708f, 1.0f, 0.1f, 100.0f);

  model_data->data().transform =
      Mat44::FromTranslationVector(mathfu::Vector<float, 3>{0.0f, 0.0f, -2.0f});

  // Creates the render image and its ImageView.
  VkImageCreateInfo render_img_create_info{
      /* sType = */
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* imageType = */ VK_IMAGE_TYPE_2D,
      /* format = */ render_target_format,
      /* extent = */
      {
          /* width = */ image_resolution,
          /* height = */ image_resolution,
          /* depth = */ 1u,
      },
      /* mipLevels = */ 1,
      /* arrayLayers = */ 1,
      /* samples = */ VK_SAMPLE_COUNT_1_BIT,
      /* tiling = */
      VK_IMAGE_TILING_OPTIMAL,
      /* usage = */
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      /* sharingMode = */
      VK_SHARING_MODE_EXCLUSIVE,
      /* queueFamilyIndexCount = */ 0,
      /* pQueueFamilyIndices = */ nullptr,
      /* initialLayout = */
      VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VkImagesExported render_images(app.device(), app.GetLogger(),
                                 data->allocator(), num_swapchain_images,
                                 &render_img_create_info);

  containers::vector<FrameData> frame_data(data->allocator());
  frame_data.resize(num_swapchain_images);

  for (int i = 0; i < num_swapchain_images; i++) {
    FrameData& frame_data_i = frame_data[i];

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

    VkImageViewCreateInfo render_img_view_create_info{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        render_images.get_image(i),                // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        render_target_format,                      // format
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
         VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    ::VkImageView raw_view;
    LOG_ASSERT(==, data->logger(), VK_SUCCESS,
               device->vkCreateImageView(device, &render_img_view_create_info,
                                         nullptr, &raw_view));
    frame_data_i.render_img_view_ =
        containers::make_unique<vulkan::VkImageView>(
            data->allocator(), vulkan::VkImageView(raw_view, nullptr, &device));

    frame_data_i.command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data->allocator(), app.GetCommandBuffer());

    frame_data_i.cube_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data->allocator(),
            app.AllocateDescriptorSet({cube_descriptor_set_layouts_[0],
                                       cube_descriptor_set_layouts_[1]}));

    VkDescriptorBufferInfo buffer_infos[2] = {
        {
            camera_data->get_buffer(),             // buffer
            camera_data->get_offset_for_frame(i),  // offset
            camera_data->size(),                   // range
        },
        {
            model_data->get_buffer(),             // buffer
            model_data->get_offset_for_frame(i),  // offset
            model_data->size(),                   // range
        }};

    VkWriteDescriptorSet write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        *frame_data_i.cube_descriptor_set_,      // dstSet
        0,                                       // dstbinding
        0,                                       // dstArrayElement
        2,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
        nullptr,                                 // pImageInfo
        buffer_infos,                            // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };

    device->vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    // Create a framebuffer with the render image as the color attachment
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,         // sType
        nullptr,                                           // pNext
        0,                                                 // flags
        *render_pass,                                      // renderPass
        1,                                                 // attachmentCount
        &frame_data_i.render_img_view_->get_raw_object(),  // attachments
        image_resolution,                                  // width
        image_resolution,                                  // height
        1                                                  // layers
    };

    ::VkFramebuffer raw_framebuffer;
    device->vkCreateFramebuffer(device, &framebuffer_create_info, nullptr,
                                &raw_framebuffer);
    frame_data_i.framebuffer_ = containers::make_unique<vulkan::VkFramebuffer>(
        data->allocator(),
        vulkan::VkFramebuffer(raw_framebuffer, nullptr, &device));

    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data_i.command_buffer_);
    cmdBuffer->vkBeginCommandBuffer(cmdBuffer,
                                    &sample_application::kBeginCommandBuffer);

    VkClearValue clear;
    // Make the clear color white.
    clear.color = {1.0, 1.0, 1.0, 1.0};

    // Image barriers for the render image, UNDEFINED ->
    // COLOR_ATTACHMENT_OPTIMAL and COLOR_ATTACHMENT_OPTIMAL ->
    // TRANSFER_SRC_OPTIMAL
    VkImageMemoryBarrier undef_to_attach{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // sType
        nullptr,                                   // pNext
        0,                                         // srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,      // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                 // oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // newLayout
        VK_QUEUE_FAMILY_IGNORED,                   // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                   // dstQueueFamilyIndex
        render_images.get_image(i),                // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    VkImageMemoryBarrier attach_to_shader{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // sType
        nullptr,                                   // pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,      // srcAccessMask
        0,                                         // dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // oldLayout
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // newLayout
        app.render_queue().index(),                // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_EXTERNAL,                  // dstQueueFamilyIndex
        render_images.get_image(i),                // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    // Change the layout of render image to COLOR_ATTACHMENT_OPTIMAL
    cmdBuffer->vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,           // srcStageMask
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // dstStageMask
        0,                                              // dependencyFlags
        0, nullptr, 0, nullptr, 1, &undef_to_attach);

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,        // sType
        nullptr,                                         // pNext
        *render_pass,                                    // renderPass
        *frame_data_i.framebuffer_,                      // framebuffer
        {{0, 0}, {image_resolution, image_resolution}},  // renderArea
        1,                                               // clearValueCount
        &clear                                           // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *cube_pipeline);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout), 0, 1,
        &frame_data_i.cube_descriptor_set_->raw_set(), 0, nullptr);
    cube.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    cmdBuffer->vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,              // dstStageMask
        0,                                              // dependencyFlags
        0, nullptr, 0, nullptr, 1, &attach_to_shader);

    (*frame_data_i.command_buffer_)
        ->vkEndCommandBuffer(*frame_data_i.command_buffer_);
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

  HANDLE render_images_handle = render_images.getMemoryWin32Handle();

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
    client_data_handles.resize(1 + num_swapchain_images * 2);

    DuplicateHandle(current_process, render_images_handle, client_process,
                    &client_data_handles[0], 0, FALSE,
                    DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    for (int i = 0; i < num_swapchain_images; i++) {
      FrameData& frame_data_i = frame_data[i];
      VkFenceGetWin32HandleInfoKHR export_info{
          VK_STRUCTURE_TYPE_FENCE_GET_WIN32_HANDLE_INFO_KHR, nullptr,
          frame_data_i.free_fence->get_raw_object(),
          VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT};
      HANDLE fence_handle;

      device->vkGetFenceWin32HandleKHR(device, &export_info, &fence_handle);

      DuplicateHandle(current_process, fence_handle, client_process,
                      &client_data_handles[1 + i * 2], 0, FALSE,
                      DUPLICATE_SAME_ACCESS);

      export_info.fence = frame_data_i.ready_fence->get_raw_object();
      device->vkGetFenceWin32HandleKHR(device, &export_info, &fence_handle);
      DuplicateHandle(current_process, fence_handle, client_process,
                      &client_data_handles[2 + i * 2], 0, FALSE,
                      DUPLICATE_SAME_ACCESS);
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
  file_descriptors.resize(1 + num_swapchain_images * 2);

  file_descriptors[0] = render_images.getMemoryFd();
  for (int i = 0; i < num_swapchain_images; i++) {
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

  float speed = 0.0001f;
  int i = 0;

  while (true) {
    FrameData& frame_data_i = frame_data[i];
    device->vkWaitForFences(device, 1,
                            &frame_data_i.free_fence->get_raw_object(), VK_TRUE,
                            UINT64_MAX);
    device->vkResetFences(device, 1,
                          &frame_data_i.free_fence->get_raw_object());

    camera_data->UpdateBuffer(&app.render_queue(), i);
    model_data->UpdateBuffer(&app.render_queue(), i++);

    i %= num_swapchain_images;

    model_data->data().transform =
        model_data->data().transform *
        Mat44::FromRotationMatrix(Mat44::RotationX(3.14f * speed) *
                                  Mat44::RotationY(3.14f * speed * 0.5f));
    submit_info.pCommandBuffers =
        &frame_data_i.command_buffer_->get_command_buffer();

    LOG_ASSERT(==, log, VK_SUCCESS,
               app.render_queue()->vkQueueSubmit(
                   app.render_queue(), 1, &submit_info,
                   frame_data_i.ready_fence->get_raw_object()));
  }

  log->LogInfo("Application Shutdown");
  return 0;
}
