// Copyright 2017 Google Inc.
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
#include <thread>

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "mathfu/matrix.h"
#include "mathfu/vector.h"
#include "support/entry/entry.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

using Mat44 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;

namespace cube_model {
#include "cube.obj.h"
}
const auto& cube_data = cube_model::model;

uint32_t cube_vertex_shader[] =
#include "set_event.vert.spv"
    ;

uint32_t cube_fragment_shader[] =
#include "set_event.frag.spv"
    ;

struct CubeFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_descriptor_set_;
  containers::unique_ptr<vulkan::VkBufferView> color_data_buffer_view_;
  vulkan::BufferPointer color_data_buffer_;
  containers::unique_ptr<vulkan::VkEvent> color_data_update_event_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class SetEventSample : public sample_application::Sample<CubeFrameData> {
 public:
  SetEventSample(const entry::EntryData* data)
      : data_(data),
        Sample<CubeFrameData>(data->allocator(), data, 1, 512, 1, 1,
                              sample_application::SampleOptions()),
        cube_(data->allocator(), data->logger(), cube_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    cube_.InitializeData(app(), initialization_buffer);

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
    cube_descriptor_set_layouts_[2] = {
        2,                                        // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,  // descriptorType
        1,                                        // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,             // stageFlags
        nullptr                                   // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(), app()->CreatePipelineLayout({{
                                cube_descriptor_set_layouts_[0],
                                cube_descriptor_set_layouts_[1],
                                cube_descriptor_set_layouts_[2],
                            }}));

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

    camera_data =
        containers::make_unique<vulkan::BufferFrameData<camera_data_>>(
            data_->allocator(), app(), num_swapchain_images,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data = containers::make_unique<vulkan::BufferFrameData<model_data_>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    camera_data->data().projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data->data().transform = Mat44::FromTranslationVector(
        mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f});
  }

  virtual void InitializeFrameData(
      CubeFrameData* frame_data, vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    // Initialize the coherent buffer which is to be used as texel uniform
    // buffer, and the event the control the produce and consume of the data
    frame_data->color_data_buffer_ =
        app()->CreateAndBindDefaultExclusiveCoherentBuffer(
            sizeof(AlphaData), VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
    AlphaData* init_color_data = reinterpret_cast<AlphaData*>(
        frame_data->color_data_buffer_->base_address());
    init_color_data->r = 0.33f;
    init_color_data->g = 0.67f;
    init_color_data->b = 1.0f;
    init_color_data->a = 0.0f;
    frame_data->color_data_update_event_ =
        containers::make_unique<vulkan::VkEvent>(
            data_->allocator(), vulkan::CreateEvent(&app()->device()));

    // The buffer memory barrier for the color data buffer, transition from
    // host write to read in fragment shader
    VkBufferMemoryBarrier color_data_buffer_barrier{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_HOST_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        *frame_data->color_data_buffer_,
        0,
        sizeof(AlphaData)};

    VkBufferViewCreateInfo color_data_buffer_view_create_info{
        VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *frame_data->color_data_buffer_,            // buffer
        VK_FORMAT_R32G32B32A32_SFLOAT,              // format
        0,                                          // offset
        VK_WHOLE_SIZE,                              // range
    };
    ::VkBufferView raw_buf_view;
    LOG_ASSERT(==, data_->logger(), VK_SUCCESS,
               app()->device()->vkCreateBufferView(
                   app()->device(), &color_data_buffer_view_create_info,
                   nullptr, &raw_buf_view));
    frame_data->color_data_buffer_view_ =
        containers::make_unique<vulkan::VkBufferView>(
            data_->allocator(),
            vulkan::VkBufferView(raw_buf_view, nullptr, &app()->device()));

    frame_data->cube_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(), app()->AllocateDescriptorSet({
                                    cube_descriptor_set_layouts_[0],
                                    cube_descriptor_set_layouts_[1],
                                    cube_descriptor_set_layouts_[2],
                                }));

    VkDescriptorBufferInfo buffer_infos[2] = {
        {
            camera_data->get_buffer(),                       // buffer
            camera_data->get_offset_for_frame(frame_index),  // offset
            camera_data->size(),                             // range
        },
        {
            model_data->get_buffer(),                       // buffer
            model_data->get_offset_for_frame(frame_index),  // offset
            model_data->size(),                             // range
        }};

    VkWriteDescriptorSet write[2] = {
        {
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
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,   // sType
            nullptr,                                  // pNext
            *frame_data->cube_descriptor_set_,        // dstSet
            2,                                        // dstbinding
            0,                                        // dstArrayElement
            1,                                        // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,  // descriptorType
            nullptr,                                  // pImageInfo
            nullptr,                                  // pBufferInfo
            &frame_data->color_data_buffer_view_
                 ->get_raw_object()  // pTexelBufferView
        },
    };

    app()->device()->vkUpdateDescriptorSets(app()->device(), 2, write, 0,
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

    cmdBuffer->vkCmdWaitEvents(
        cmdBuffer, 1, &frame_data->color_data_update_event_->get_raw_object(),
        VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        nullptr, 1, &color_data_buffer_barrier, 0, nullptr);
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

  virtual void Update(float time_since_last_render) override {
    model_data->data().transform =
        model_data->data().transform *
        Mat44::FromRotationMatrix(
            Mat44::RotationX(3.14f * time_since_last_render) *
            Mat44::RotationY(3.14f * time_since_last_render * 0.5f));
  }
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      CubeFrameData* frame_data) override {
    // Update our uniform buffers.
    camera_data->UpdateBuffer(queue, frame_index);
    model_data->UpdateBuffer(queue, frame_index);
    app()->device()->vkResetEvent(
        app()->device(),
        frame_data->color_data_update_event_->get_raw_object());

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

    app()->render_queue()->vkQueueSubmit(app()->render_queue(), 1,
                                         &init_submit_info,
                                         static_cast<VkFence>(VK_NULL_HANDLE));
    std::thread wait_idle([&]() {
      app()->render_queue()->vkQueueWaitIdle(app()->render_queue());
    });

    AlphaData* color_data = reinterpret_cast<AlphaData*>(
        frame_data->color_data_buffer_->base_address());
    auto wave_func = [](float* d, float a) { *d = *d > 2.0f ? 0.0f : *d + a; };
    wave_func(&color_data->r, 0.02f);
    wave_func(&color_data->g, 0.04f);
    wave_func(&color_data->b, 0.08f);
    wave_func(&color_data->a, 0.1f);
    app()->device()->vkSetEvent(
        app()->device(),
        frame_data->color_data_update_event_->get_raw_object());

    wait_idle.join();
  }

 private:
  struct camera_data_ {
    Mat44 projection_matrix;
  };

  struct model_data_ {
    Mat44 transform;
  };

  struct AlphaData {
    float r;
    float g;
    float b;
    float a;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> cube_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  VkDescriptorSetLayoutBinding cube_descriptor_set_layouts_[3];
  vulkan::VulkanModel cube_;

  containers::unique_ptr<vulkan::BufferFrameData<camera_data_>> camera_data;
  containers::unique_ptr<vulkan::BufferFrameData<model_data_>> model_data;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  SetEventSample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
