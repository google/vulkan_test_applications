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

struct FrameData {
  containers::unique_ptr<vulkan::VkFence> rendered_fence;
  containers::unique_ptr<vulkan::VkSemaphore> swapchain_sema;
  containers::unique_ptr<vulkan::VkSemaphore> present_ready_sema;
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_descriptor_set_;
  containers::unique_ptr<vulkan::VkImageView> render_img_view_;
};

int main_entry(const entry::EntryData* data) {
  logging::Logger* log = data->logger();
  log->LogInfo("Application Startup");

  VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timeline_semaphore_features{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR,
      nullptr,
      VK_TRUE,
  };

  vulkan::VulkanApplication app(
      data->allocator(), data->logger(), data,
      {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
      {VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME}, {}, 131072, 131072, 131072,
      131072, false, false, false, 0, false, false,
      VK_COLORSPACE_SRGB_NONLINEAR_KHR, false, false, nullptr, true, false,
      &timeline_semaphore_features);

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

  VkSampleCountFlagBits num_samples = VK_SAMPLE_COUNT_1_BIT;
  VkViewport viewport = {0.0f,
                         0.0f,
                         static_cast<float>(app.swapchain().width()),
                         static_cast<float>(app.swapchain().height()),
                         0.0f,
                         1.0f};

  VkRect2D scissor = {{0, 0},
                      {app.swapchain().width(), app.swapchain().height()}};

  containers::unique_ptr<vulkan::VkRenderPass> render_pass =
      containers::make_unique<vulkan::VkRenderPass>(
          data->allocator(),
          app.CreateRenderPass(
              {{
                  0,                                 // flags
                  render_target_format,              // format
                  num_samples,                       // samples
                  VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp
                  VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
                  VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stenilLoadOp
                  VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stenilStoreOp
                  VK_IMAGE_LAYOUT_UNDEFINED,         // initialLayout
                  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR    // finalLayout
              }},                                    // AttachmentDescriptions
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

  containers::vector<FrameData> frame_data(data->allocator());
  frame_data.resize(num_swapchain_images);

  for (int i = 0; i < num_swapchain_images; i++) {
    FrameData& frame_data_i = frame_data[i];

    frame_data_i.rendered_fence = containers::make_unique<vulkan::VkFence>(
        data->allocator(), vulkan::CreateFence(&device, true));
    frame_data_i.swapchain_sema = containers::make_unique<vulkan::VkSemaphore>(
        data->allocator(), vulkan::CreateSemaphore(&device));
    frame_data_i.present_ready_sema =
        containers::make_unique<vulkan::VkSemaphore>(
            data->allocator(), vulkan::CreateSemaphore(&device));

    VkImageViewCreateInfo render_img_view_create_info{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        app.swapchain_images()[i],                 // image
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
        app.swapchain().width(),                           // width
        app.swapchain().height(),                          // height
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
    clear.color = {0.0, 0.0, 0.0, 1.0};

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass,                              // renderPass
        *frame_data_i.framebuffer_,                // framebuffer
        {{0, 0},
         {app.swapchain().width(), app.swapchain().height()}},  // renderArea
        1,      // clearValueCount
        &clear  // clears
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

    (*frame_data_i.command_buffer_)
        ->vkEndCommandBuffer(*frame_data_i.command_buffer_);
  }

  uint64_t zero = 0;
  uint64_t signal_from_swap = 0;
  uint64_t signal_to_swap = 0;
  VkPipelineStageFlags waitStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkTimelineSemaphoreSubmitInfoKHR timelineSubmitInfos[] = {
      VkTimelineSemaphoreSubmitInfoKHR{
          VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR,  // sType
          nullptr,                                               // pNext
          1,                 // waitSemaphoreValueCount
          &zero,             // waitSemaphoreValues,
          1,                 // signalSemaphoreValueCount
          &signal_from_swap  // signalSemaphoreValues
      },
      VkTimelineSemaphoreSubmitInfoKHR{
          VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR,  // sType
          nullptr,                                               // pNext
          1,                  // waitSemaphoreValueCount
          &signal_from_swap,  // waitSemaphoreValues,
          1,                  // signalSemaphoreValueCount
          &signal_to_swap     // signalSemaphoreValues
      },
      VkTimelineSemaphoreSubmitInfoKHR{
          VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR,  // sType
          nullptr,                                               // pNext
          1,                // waitSemaphoreValueCount
          &signal_to_swap,  // waitSemaphoreValues,
          1,                // signalSemaphoreValueCount
          &zero             // signalSemaphoreValues
      },
  };

  VkSubmitInfo submit_infos[] = {VkSubmitInfo{
                                     VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
                                     &timelineSubmitInfos[0],        // pNext
                                     1,               // waitSemaphoreCount
                                     nullptr,         // pWaitSemaphores
                                     &waitStageMask,  // pWaitDstStageMask,
                                     0,               // commandBufferCount
                                     nullptr,         // pCommandBuffers
                                     1,               // signalSemaphoreCount
                                     nullptr          // pSignalSemaphores
                                 },
                                 VkSubmitInfo{
                                     VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
                                     &timelineSubmitInfos[1],        // pNext
                                     1,               // waitSemaphoreCount
                                     nullptr,         // pWaitSemaphores
                                     &waitStageMask,  // pWaitDstStageMask,
                                     1,               // commandBufferCount
                                     nullptr,         //
                                     1,               // signalSemaphoreCount
                                     nullptr          // pSignalSemaphores
                                 },
                                 VkSubmitInfo{
                                     VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
                                     &timelineSubmitInfos[2],        // pNext
                                     1,               // waitSemaphoreCount
                                     nullptr,         // pWaitSemaphores
                                     &waitStageMask,  // pWaitDstStageMask,
                                     0,               // commandBufferCount
                                     nullptr,         // pCommandBuffers
                                     1,               // signalSemaphoreCount
                                     nullptr          // pSignalSemaphores
                                 }};

  // TimelineSemaphore
  auto timelineSemaphore = vulkan::CreateTimelineSemaphore(&device, 0);

  uint32_t i = 0;
  auto last_frame_time = std::chrono::high_resolution_clock::now();
  while (true) {
    auto current_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed_time = current_time - last_frame_time;
    last_frame_time = current_time;
    float fdt = data->fixed_timestep() ? 0.1f : elapsed_time.count();
    float speed = fdt;
    uint32_t image_i;

    device->vkAcquireNextImageKHR(
        device, app.swapchain().get_raw_object(), UINT64_MAX,
        frame_data[i].swapchain_sema->get_raw_object(),
        static_cast<::VkFence>(VK_NULL_HANDLE), &image_i);

    FrameData& frame_data_i = frame_data[image_i];

    device->vkWaitForFences(device, 1,
                            &frame_data_i.rendered_fence->get_raw_object(),
                            VK_TRUE, UINT64_MAX);
    device->vkResetFences(device, 1,
                          &frame_data_i.rendered_fence->get_raw_object());

    camera_data->UpdateBuffer(&app.render_queue(), image_i);
    model_data->UpdateBuffer(&app.render_queue(), image_i);

    model_data->data().transform =
        model_data->data().transform *
        Mat44::FromRotationMatrix(Mat44::RotationX(3.14f * speed) *
                                  Mat44::RotationY(3.14f * speed * 0.5f));

    signal_from_swap = signal_to_swap + 1;
    signal_to_swap = signal_from_swap + 1;

    submit_infos[0].pWaitSemaphores =
        &frame_data[i].swapchain_sema->get_raw_object();
    submit_infos[0].pSignalSemaphores = &timelineSemaphore.get_raw_object();

    submit_infos[1].pCommandBuffers =
        &frame_data_i.command_buffer_->get_command_buffer();
    submit_infos[1].pWaitSemaphores = &timelineSemaphore.get_raw_object();
    submit_infos[1].pSignalSemaphores = &timelineSemaphore.get_raw_object();

    submit_infos[2].pWaitSemaphores = &timelineSemaphore.get_raw_object();
    submit_infos[2].pSignalSemaphores =
        &frame_data[i].present_ready_sema->get_raw_object();

    LOG_ASSERT(==, log, VK_SUCCESS,
               app.render_queue()->vkQueueSubmit(
                   app.render_queue(), 3, &submit_infos[0],
                   frame_data_i.rendered_fence->get_raw_object()));

    VkPresentInfoKHR present_info{
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,  // sType
        nullptr,                             // pNext
        1,                                   // waitSemaphoreCount
        &frame_data[i++]
             .present_ready_sema->get_raw_object(),  // pWaitSemaphores
        1,                                           // swapchainCount
        &app.swapchain().get_raw_object(),           // pSwapchains
        &image_i,                                    // pImageIndices
        nullptr,                                     // pResults
    };
    i %= num_swapchain_images;
    LOG_ASSERT(==, log,
               app.present_queue()->vkQueuePresentKHR(app.present_queue(),
                                                      &present_info),
               VK_SUCCESS);
  }

  log->LogInfo("Application Shutdown");
  return 0;
}
