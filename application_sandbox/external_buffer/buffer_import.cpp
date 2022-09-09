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

#include <chrono>

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "external_buffer.h"
#include "mathfu/matrix.h"
#include "mathfu/vector.h"
#include "support/entry/entry.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

#ifdef __linux__
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#endif

using Mat44 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;

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

struct CubeFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_descriptor_set_;
  containers::unique_ptr<vulkan::VkFence> free_fence;
  containers::unique_ptr<vulkan::VkFence> ready_fence;
};

class CubeSample : public sample_application::Sample<CubeFrameData> {
 public:
  CubeSample(const entry::EntryData* data)
      : data_(data),
        Sample<CubeFrameData>(
            data->allocator(), data, 1, 512, 1, 1,
            sample_application::SampleOptions().EnableMultisampling(), {0},
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
            }),
        cube_(data->allocator(), data->logger(), cube_data),
        native_handles(data->allocator()) {
  }

  ~CubeSample() {
#ifdef _WIN32
    for (HANDLE handle : native_handles) {
      CloseHandle(handle);
    }
#elif __linux__
    for (int fd : native_handles) {
      close(fd);
    }
#endif
  }
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    cube_.InitializeData(app(), initialization_buffer);
    native_handles.resize(1 + num_swapchain_images * 2);

    cube_descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    cube_descriptor_set_layouts_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout({{cube_descriptor_set_layouts_[0],
                                      cube_descriptor_set_layouts_[1]}}));

    VkAttachmentReference color_attachment = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass(
            {{
                0,                                         // flags
                render_format(),                           // format
                num_samples(),                             // samples
                VK_ATTACHMENT_LOAD_OP_CLEAR,               // loadOp
                VK_ATTACHMENT_STORE_OP_STORE,              // storeOp
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stencilLoadOp
                VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stencilStoreOp
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

    cube_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));
    cube_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                              cube_vertex_shader);
    cube_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                              cube_fragment_shader);
    cube_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    cube_pipeline_->SetInputStreams(&cube_);
    cube_pipeline_->SetViewport(viewport());
    cube_pipeline_->SetScissor(scissor());
    cube_pipeline_->SetSamples(num_samples());
    cube_pipeline_->AddAttachment();
    cube_pipeline_->Commit();

    getNativeHandles();

    model_data_ = containers::make_unique<VkBufferImported<ModelData>>(
        data_->allocator(), app()->device(), app()->GetLogger(),
        num_swapchain_images, native_handles[0]);

    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    camera_data_->data().projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);
  }

  virtual void InitializeFrameData(
      CubeFrameData* frame_data, vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    VkFenceCreateInfo fence_create_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                        nullptr, 0};

    VkFence fence;

    app()->device()->vkCreateFence(app()->device(), &fence_create_info, nullptr,
                                   &fence);
    frame_data->free_fence = containers::make_unique<vulkan::VkFence>(
        data_->allocator(), fence, nullptr, &app()->device());
    app()->device()->vkCreateFence(app()->device(), &fence_create_info, nullptr,
                                   &fence);
    frame_data->ready_fence = containers::make_unique<vulkan::VkFence>(
        data_->allocator(), fence, nullptr, &app()->device());

#ifdef _WIN32
    VkImportFenceWin32HandleInfoKHR fence_import_info{
        VK_STRUCTURE_TYPE_IMPORT_FENCE_WIN32_HANDLE_INFO_KHR,
        nullptr,
        VK_NULL_HANDLE,
        0,
        VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT,
        0,
        nullptr};

    fence_import_info.fence = frame_data->free_fence->get_raw_object();
    fence_import_info.handle = native_handles[1 + frame_index * 2];
    app()->device()->vkImportFenceWin32HandleKHR(app()->device(),
                                                 &fence_import_info);
    fence_import_info.fence = frame_data->ready_fence->get_raw_object();
    fence_import_info.handle = native_handles[2 + frame_index * 2];
    app()->device()->vkImportFenceWin32HandleKHR(app()->device(),
                                                 &fence_import_info);
#elif __linux__
    VkImportFenceFdInfoKHR fence_import_info{
        VK_STRUCTURE_TYPE_IMPORT_FENCE_FD_INFO_KHR,  nullptr, VK_NULL_HANDLE, 0,
        VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT, 0};

    fence_import_info.fence = frame_data->free_fence->get_raw_object();
    fence_import_info.fd = native_handles[1 + frame_index * 2];
    app()->device()->vkImportFenceFdKHR(app()->device(), &fence_import_info);
    fence_import_info.fence = frame_data->ready_fence->get_raw_object();
    fence_import_info.fd = native_handles[2 + frame_index * 2];
    app()->device()->vkImportFenceFdKHR(app()->device(), &fence_import_info);
#endif

    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    frame_data->cube_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet({cube_descriptor_set_layouts_[0],
                                          cube_descriptor_set_layouts_[1]}));

    VkDescriptorBufferInfo buffer_infos[2] = {
        {
            camera_data_->get_buffer(),                       // buffer
            camera_data_->get_offset_for_frame(frame_index),  // offset
            camera_data_->size(),                             // range
        },
        {
            model_data_->get_buffer(),                       // buffer
            model_data_->get_offset_for_frame(frame_index),  // offset
            model_data_->size(),                             // range
        }};

    VkWriteDescriptorSet write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        *frame_data->cube_descriptor_set_,       // dstSet
        0,                                       // dstbinding
        0,                                       // dstArrayElement
        2,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
        nullptr,                                 // pImageInfo
        buffer_infos,                            // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };

    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                            nullptr);

    ::VkImageView raw_view = color_view(frame_data);

    // Create a framebuffer with depth and image attachments
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_pass_,                              // renderPass
        1,                                          // attachmentCount
        &raw_view,                                  // attachments
        app()->swapchain().width(),                 // width
        app()->swapchain().height(),                // height
        1                                           // layers
    };

    ::VkFramebuffer raw_framebuffer;
    app()->device()->vkCreateFramebuffer(
        app()->device(), &framebuffer_create_info, nullptr, &raw_framebuffer);
    frame_data->framebuffer_ = containers::make_unique<vulkan::VkFramebuffer>(
        data_->allocator(),
        vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app()->device()));

    (*frame_data->command_buffer_)
        ->vkBeginCommandBuffer((*frame_data->command_buffer_),
                               &sample_application::kBeginCommandBuffer);
    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);

    VkClearValue clear;
    vulkan::MemoryClear(&clear);

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *frame_data->framebuffer_,                 // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        1,                                // clearValueCount
        &clear                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *cube_pipeline_);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &frame_data->cube_descriptor_set_->raw_set(), 0, nullptr);
    cube_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    (*frame_data->command_buffer_)
        ->vkEndCommandBuffer(*frame_data->command_buffer_);
  }

#ifdef _WIN32
  void getNativeHandles() {
    HANDLE pipe_handle;
    do {
      pipe_handle = CreateFile(
          TEXT("\\\\.\\pipe\\LOCAL\\vulkan_external_buffer_example"),
          GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
      if (pipe_handle == INVALID_HANDLE_VALUE) Sleep(1000);
    } while (pipe_handle == INVALID_HANDLE_VALUE);
    DWORD bytes_read;
    ReadFile(pipe_handle, native_handles.data(),
             static_cast<DWORD>(sizeof(HANDLE) * native_handles.size()),
             &bytes_read, NULL);

    CloseHandle(pipe_handle);
  }
#elif __linux__

  void getNativeHandles() {
    struct sockaddr_un addr;
    int sock;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(&addr.sun_path[1], "vulkan_external_buffer_example");
    while (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      sleep(1);
    }

    struct msghdr msg;
    struct iovec iov[1];
    struct cmsghdr* cmsg = NULL;
    containers::vector<char> ctrl_buf(app()->GetAllocator());
    ctrl_buf.resize(CMSG_SPACE(sizeof(int) * native_handles.size()), 0);
    char data[1];

    memset(&msg, 0, sizeof(struct msghdr));

    iov[0].iov_base = data;
    iov[0].iov_len = sizeof(data);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_control = ctrl_buf.data();
    msg.msg_controllen = CMSG_SPACE(sizeof(int) * native_handles.size());
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    recvmsg(sock, &msg, 0);

    cmsg = CMSG_FIRSTHDR(&msg);

    memcpy(native_handles.data(), CMSG_DATA(cmsg),
           sizeof(int) * native_handles.size());
  }
#endif

  virtual void Update(float time_since_last_render) override {}

  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      CubeFrameData* frame_data) override {
    app()->device()->vkWaitForFences(app()->device(), 1,
                                     &frame_data->ready_fence->get_raw_object(),
                                     VK_TRUE, UINT64_MAX);
    app()->device()->vkResetFences(app()->device(), 1,
                                   &frame_data->ready_fence->get_raw_object());

    // Update our uniform buffers.
    camera_data_->UpdateBuffer(queue, frame_index);

    VkSubmitInfo init_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask,
        1,                              // commandBufferCount
        &(frame_data->command_buffer_->get_command_buffer()),
        0,       // signalSemaphoreCount
        nullptr  // pSignalSemaphores
    };

    app()->render_queue()->vkQueueSubmit(
        app()->render_queue(), 1, &init_submit_info,
        frame_data->free_fence->get_raw_object());
  }

 private:
  struct CameraData {
    Mat44 projection_matrix;
  };

  struct ModelData {
    Mat44 transform;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> cube_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  VkDescriptorSetLayoutBinding cube_descriptor_set_layouts_[2];
  vulkan::VulkanModel cube_;

  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
  containers::unique_ptr<VkBufferImported<ModelData>> model_data_;

#ifdef _WIN32
  containers::vector<HANDLE> native_handles;
#elif __linux__
  containers::vector<int> native_handles;
#endif
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  CubeSample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
