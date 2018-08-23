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

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "support/entry/entry.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

#include <chrono>
#include "mathfu/matrix.h"
#include "mathfu/vector.h"

using Mat44 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;

namespace cube_model {
#include "cube.obj.h"
}
const auto& cube_data = cube_model::model;

uint32_t cube_vertex_shader[] =
#include "execute_commands.vert.spv"
    ;

uint32_t cube_fragment_shader[] =
#include "execute_commands.frag.spv"
    ;

uint32_t compute_shader[] =
#include "execute_commands.comp.spv"
    ;

struct CubeFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> primary_command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> render_descriptor_set_;
  containers::unique_ptr<vulkan::DescriptorSet> compute_descriptor_set_;
  containers::unique_ptr<vulkan::VkBufferView> dispatch_data_buffer_view_;

  // Secondary command buffers
  containers::unique_ptr<vulkan::VkCommandBuffer> compute_command_buffer_;
  containers::unique_ptr<vulkan::VkCommandBuffer> graphics_command_buffer_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class ExecuteCommandsSample : public sample_application::Sample<CubeFrameData> {
 public:
  ExecuteCommandsSample(const entry::EntryData* data)
      : data_(data),
        Sample<CubeFrameData>(data->allocator(), data, 1, 512, 2, 1,
                              sample_application::SampleOptions()),
        cube_(data->allocator(), data->logger(), cube_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    cube_.InitializeData(app(), initialization_buffer);

    render_descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    render_descriptor_set_layouts_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    render_descriptor_set_layouts_[2] = {
        2,                                        // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,  // descriptorType
        1,                                        // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,             // stageFlags
        nullptr                                   // pImmutableSamplers
    };

    render_pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout({{
            render_descriptor_set_layouts_[0],
            render_descriptor_set_layouts_[1],
            render_descriptor_set_layouts_[2],
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

    render_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(),
        app()->CreateGraphicsPipeline(render_pipeline_layout_.get(),
                                      render_pass_.get(), 0));
    render_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                cube_vertex_shader);
    render_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                cube_fragment_shader);
    render_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    render_pipeline_->SetInputStreams(&cube_);
    render_pipeline_->SetViewport(viewport());
    render_pipeline_->SetScissor(scissor());
    render_pipeline_->SetSamples(num_samples());
    render_pipeline_->AddAttachment();
    render_pipeline_->Commit();

    camera_data =
        containers::make_unique<vulkan::BufferFrameData<camera_data_>>(
            data_->allocator(), app(), num_swapchain_images,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data = containers::make_unique<vulkan::BufferFrameData<model_data_>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    dispatch_data_ =
        containers::make_unique<vulkan::BufferFrameData<DispatchData>>(
            data_->allocator(), app(), num_swapchain_images,
            VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    camera_data->data().projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data->data().transform = Mat44::FromTranslationVector(
        mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f});

    // initialize the compute pipeline
    compute_descriptor_set_layout_ = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_COMPUTE_BIT,        // stageFlags
        nullptr                             // pImmutableSamplers
    };
    compute_pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout({{compute_descriptor_set_layout_}}));
    compute_pipeline_ = containers::make_unique<vulkan::VulkanComputePipeline>(
        data_->allocator(),
        app()->CreateComputePipeline(
            compute_pipeline_layout_.get(),
            VkShaderModuleCreateInfo{
                VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0,
                sizeof(compute_shader), compute_shader},
            "main"));

    // initialize dispatch data value
    dispatch_data_->data().value = 0.0;
  }

  virtual void InitializeFrameData(
      CubeFrameData* frame_data, vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    // Create the primary command buffer
    frame_data->primary_command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    // Create the two secondary command buffers.
    frame_data->compute_command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(),
            app()->GetCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));
    frame_data->graphics_command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(),
            app()->GetCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));

    // Allocate the descriptors for the render pass
    VkBufferViewCreateInfo dispatch_data_buffer_view_create_info{
        VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,          // sType
        nullptr,                                            // pNext
        0,                                                  // flags
        dispatch_data_->get_buffer(),                       // buffer
        VK_FORMAT_R32_SFLOAT,                               // format
        dispatch_data_->get_offset_for_frame(frame_index),  // offset
        dispatch_data_->aligned_data_size(),                // range
    };
    ::VkBufferView raw_buf_view;
    LOG_ASSERT(==, data_->logger(), VK_SUCCESS,
               app()->device()->vkCreateBufferView(
                   app()->device(), &dispatch_data_buffer_view_create_info,
                   nullptr, &raw_buf_view));
    frame_data->dispatch_data_buffer_view_ =
        containers::make_unique<vulkan::VkBufferView>(
            data_->allocator(),
            vulkan::VkBufferView(raw_buf_view, nullptr, &app()->device()));

    frame_data->render_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet({
                render_descriptor_set_layouts_[0],
                render_descriptor_set_layouts_[1],
                render_descriptor_set_layouts_[2],
            }));

    // Allocate the descriptors for the compute pipeline
    frame_data->compute_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet({compute_descriptor_set_layout_}));

    // Update the descriptor sets.
    VkDescriptorBufferInfo buffer_infos[3] = {
        // Model matrix
        {
            camera_data->get_buffer(),                       // buffer
            camera_data->get_offset_for_frame(frame_index),  // offset
            camera_data->size(),                             // range
        },
        // Project matrix
        {
            model_data->get_buffer(),                       // buffer
            model_data->get_offset_for_frame(frame_index),  // offset
            model_data->size(),                             // range
        },
        // Compute pipeline in-out buffer
        {
            dispatch_data_->get_buffer(),                       // buffer
            dispatch_data_->get_offset_for_frame(frame_index),  // offset
            dispatch_data_->size(),                             // range
        },
    };

    VkWriteDescriptorSet write[3] = {
        // Render pass: Model and Projection matrix
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->render_descriptor_set_,     // dstSet
            0,                                       // dstbinding
            0,                                       // dstArrayElement
            2,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
            nullptr,                                 // pImageInfo
            buffer_infos,                            // pBufferInfo
            nullptr,                                 // pTexelBufferView
        },
        // Render pass: Dispatch buffer
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,   // sType
            nullptr,                                  // pNext
            *frame_data->render_descriptor_set_,      // dstSet
            2,                                        // dstbinding
            0,                                        // dstArrayElement
            1,                                        // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,  // descriptorType
            nullptr,                                  // pImageInfo
            nullptr,                                  // pBufferInfo
            &frame_data->dispatch_data_buffer_view_
                 ->get_raw_object()  // pTexelBufferView
        },
        // Compute pass: Dispatch buffer
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->compute_descriptor_set_,    // dstSet
            0,                                       // dstbinding
            0,                                       // dstArrayElement
            1,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,       // descriptorType
            nullptr,                                 // pImageInfo
            &buffer_infos[2],                        // pBufferInfo
            nullptr                                  // pTexelBufferView
        },
    };

    app()->device()->vkUpdateDescriptorSets(app()->device(), 3, write, 0,
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

    // Prepare the buffer memory barriers of the dispatch/texel buffer
    VkBufferMemoryBarrier to_use_in_comp{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        dispatch_data_->get_buffer(),
        dispatch_data_->get_offset_for_frame(frame_index),
        dispatch_data_->aligned_data_size()};
    VkBufferMemoryBarrier to_use_in_frag{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_SHADER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        dispatch_data_->get_buffer(),
        dispatch_data_->get_offset_for_frame(frame_index),
        dispatch_data_->aligned_data_size()};
    VkBufferMemoryBarrier to_use_in_host{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_SHADER_READ_BIT,
        VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        dispatch_data_->get_buffer(),
        dispatch_data_->get_offset_for_frame(frame_index),
        dispatch_data_->aligned_data_size()};

    // Start the compute pass in the compute command buffer
    VkCommandBufferBeginInfo comp_buf_begin_info =
        sample_application::kBeginCommandBuffer;
    comp_buf_begin_info.pInheritanceInfo =
        &sample_application::kInheritanceCommandBuffer;
    vulkan::VkCommandBuffer& comp_buf = (*frame_data->compute_command_buffer_);
    comp_buf->vkBeginCommandBuffer(comp_buf, &comp_buf_begin_info);
    comp_buf->vkCmdBindPipeline(comp_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                *compute_pipeline_);
    comp_buf->vkCmdBindDescriptorSets(
        comp_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
        ::VkPipelineLayout(*compute_pipeline_layout_), 0, 1,
        &frame_data->compute_descriptor_set_->raw_set(), 0, nullptr);
    comp_buf->vkCmdDispatch(comp_buf, 1, 1, 1);
    comp_buf->vkEndCommandBuffer(comp_buf);

    // Start the render pass in the graphics command buffer
    VkCommandBufferBeginInfo graphics_buf_begin_info =
        sample_application::kBeginCommandBuffer;
    VkCommandBufferInheritanceInfo graphics_buf_inheritance_info =
        sample_application::kInheritanceCommandBuffer;
    graphics_buf_inheritance_info.renderPass = *render_pass_;
    graphics_buf_inheritance_info.framebuffer = *frame_data->framebuffer_;
    graphics_buf_begin_info.pInheritanceInfo = &graphics_buf_inheritance_info;
    graphics_buf_begin_info.flags =
        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    vulkan::VkCommandBuffer& graphics_buf =
        (*frame_data->graphics_command_buffer_);
    graphics_buf->vkBeginCommandBuffer(graphics_buf, &graphics_buf_begin_info);
    graphics_buf->vkCmdBindPipeline(
        graphics_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, *render_pipeline_);
    graphics_buf->vkCmdBindDescriptorSets(
        graphics_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*render_pipeline_layout_), 0, 1,
        &frame_data->render_descriptor_set_->raw_set(), 0, nullptr);
    cube_.Draw(&graphics_buf);
    graphics_buf->vkEndCommandBuffer(graphics_buf);

    // Executes commands on the primary command buffer
    ::VkCommandBuffer raw_secondary_buffers[2] = {comp_buf, graphics_buf};
    vulkan::VkCommandBuffer& prim_buf = (*frame_data->primary_command_buffer_);
    prim_buf->vkBeginCommandBuffer(prim_buf,
                                   &sample_application::kBeginCommandBuffer);
    prim_buf->vkCmdPipelineBarrier(
        prim_buf, VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        0, 0, nullptr, 1, &to_use_in_comp, 0, nullptr);
    prim_buf->vkCmdExecuteCommands(prim_buf, 1, &raw_secondary_buffers[0]);
    prim_buf->vkCmdPipelineBarrier(prim_buf,
                                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                   nullptr, 1, &to_use_in_frag, 0, nullptr);
    prim_buf->vkCmdBeginRenderPass(
        prim_buf, &pass_begin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    prim_buf->vkCmdExecuteCommands(prim_buf, 1, &raw_secondary_buffers[1]);
    prim_buf->vkCmdEndRenderPass(prim_buf);
    prim_buf->vkCmdPipelineBarrier(
        prim_buf, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT |
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 1, &to_use_in_host, 0, nullptr);
    prim_buf->vkEndCommandBuffer(prim_buf);
  }

  virtual void Update(float time_since_last_render) override {
    model_data->data().transform =
        model_data->data().transform *
        Mat44::FromRotationMatrix(
            Mat44::RotationX(3.14f * time_since_last_render) *
            Mat44::RotationY(3.14f * time_since_last_render * 0.5f));
    dispatch_data_->data().value += 1.0;
  }

  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      CubeFrameData* frame_data) override {
    // Update our uniform buffers.
    camera_data->UpdateBuffer(queue, frame_index);
    model_data->UpdateBuffer(queue, frame_index);
    dispatch_data_->UpdateBuffer(queue, frame_index);

    VkSubmitInfo init_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask,
        1,                              // commandBufferCount
        &(frame_data->primary_command_buffer_->get_command_buffer()),
        0,       // signalSemaphoreCount
        nullptr  // pSignalSemaphores
    };

    app()->render_queue()->vkQueueSubmit(app()->render_queue(), 1,
                                         &init_submit_info,
                                         static_cast<VkFence>(VK_NULL_HANDLE));
  }

 private:
  struct camera_data_ {
    Mat44 projection_matrix;
  };

  struct model_data_ {
    Mat44 transform;
  };

  struct DispatchData {
    float value;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> render_pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> render_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  containers::unique_ptr<vulkan::PipelineLayout> compute_pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanComputePipeline> compute_pipeline_;
  VkDescriptorSetLayoutBinding render_descriptor_set_layouts_[3];
  VkDescriptorSetLayoutBinding compute_descriptor_set_layout_;
  vulkan::VulkanModel cube_;

  containers::unique_ptr<vulkan::BufferFrameData<camera_data_>> camera_data;
  containers::unique_ptr<vulkan::BufferFrameData<model_data_>> model_data;
  containers::unique_ptr<vulkan::BufferFrameData<DispatchData>> dispatch_data_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  ExecuteCommandsSample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->ShouldExit()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
