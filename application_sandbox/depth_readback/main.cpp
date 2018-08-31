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

namespace plane_model {
#include "fullscreen_quad.obj.h"
}
const auto& plane_data = plane_model::model;

uint32_t cube_vertex_shader[] =
#include "cube.vert.spv"
    ;

uint32_t cube_fragment_shader[] =
#include "cube.frag.spv"
    ;

uint32_t plane_vertex_shader[] =
#include "plane.vert.spv"
    ;

uint32_t plane_fragment_shader[] =
#include "plane.frag.spv"
    ;

struct DepthFrameData {
  // All of the commands to render a single frame
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  // The framebuffer for a single frame
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  // The descriptor_set used for rendering the cube
  containers::unique_ptr<vulkan::DescriptorSet> render_cube_descriptor_set_;
  // The descriptor_set used for reading the multi-sampled depth
  containers::unique_ptr<vulkan::DescriptorSet> read_depth_descriptor_set_;
};

class DepthReadbackSample : public sample_application::Sample<DepthFrameData> {
 public:
  DepthReadbackSample(const entry::entry_data* data)
      : data_(data),
        Sample<DepthFrameData>(data->root_allocator, data, 1, 512, 1, 1,
                               sample_application::SampleOptions()
                                   .EnableMultisampling()
                                   .EnableDepthBuffer()),
        cube_(data->root_allocator, data->log.get(), cube_data),
        plane_(data->root_allocator, data->log.get(), plane_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    const uint32_t width = app()->swapchain().width();
    const uint32_t height = app()->swapchain().height();

    cube_.InitializeData(app(), initialization_buffer);
    plane_.InitializeData(app(), initialization_buffer);

    render_cube_descriptor_set_layout_bindings_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    render_cube_descriptor_set_layout_bindings_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };

    render_cube_pipeline_layout_ =
        containers::make_unique<vulkan::PipelineLayout>(
            data_->root_allocator,
            app()->CreatePipelineLayout(
                {{render_cube_descriptor_set_layout_bindings_[0],
                  render_cube_descriptor_set_layout_bindings_[1]}}));

    VkAttachmentReference color_attachment = {
        1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_attachment = {
        0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_read_attachment = {
        0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};

    render_cube_render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->root_allocator,
        app()->CreateRenderPass(
            {{
                 0,                                 // flags
                 depth_format(),                    // format
                 num_samples(),                     // samples
                 VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp
                 VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
                 VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stenilLoadOp
                 VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stenilStoreOp
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,  // initialLayout
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL  // finalLayout
             },
             {
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
                &depth_attachment,                // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    depth_read_render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->root_allocator,
        app()->CreateRenderPass(
            {{
                 0,                                 // flags
                 depth_format(),                    // format
                 num_samples(),                     // samples
                 VK_ATTACHMENT_LOAD_OP_LOAD,        // loadOp
                 VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
                 VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stenilLoadOp
                 VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stenilStoreOp
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,  // initialLayout
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // finalLayout
             },
             {
                 0,                                         // flags
                 render_format(),                           // format
                 num_samples(),                             // samples
                 VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // loadOp
                 VK_ATTACHMENT_STORE_OP_STORE,              // storeOp
                 VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stenilLoadOp
                 VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stenilStoreOp
                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // initialLayout
                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL   // finalLayout
             }},  // AttachmentDescriptions
            {{
                0,                                // flags
                VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                1,                                // inputAttachmentCount
                &depth_read_attachment,           // pInputAttachments
                1,                                // colorAttachmentCount
                &color_attachment,                // colorAttachment
                nullptr,                          // pResolveAttachments
                nullptr,                          // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    render_cube_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->root_allocator,
            app()->CreateGraphicsPipeline(render_cube_pipeline_layout_.get(),
                                          render_cube_render_pass_.get(), 0));
    render_cube_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                     cube_vertex_shader);
    render_cube_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                     cube_fragment_shader);
    render_cube_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    render_cube_pipeline_->SetInputStreams(&cube_);
    render_cube_pipeline_->SetViewport(viewport());
    render_cube_pipeline_->SetScissor(scissor());
    render_cube_pipeline_->SetSamples(num_samples());
    render_cube_pipeline_->AddAttachment();
    render_cube_pipeline_->Commit();

    depth_read_pipeline_layout_bindings_ = {
        0,                                    // binding
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,  // descriptorType
        1,                                    // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,         // stage
        nullptr                               // pImmutableSamplers
    };

    depth_read_pipeline_layout_ =
        containers::make_unique<vulkan::PipelineLayout>(
            data_->root_allocator,
            app()->CreatePipelineLayout(
                {{depth_read_pipeline_layout_bindings_}}));

    depth_read_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->root_allocator,
            app()->CreateGraphicsPipeline(depth_read_pipeline_layout_.get(),
                                          depth_read_render_pass_.get(), 0));
    depth_read_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                    plane_vertex_shader);
    depth_read_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                    plane_fragment_shader);
    depth_read_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    depth_read_pipeline_->SetViewport(viewport());
    depth_read_pipeline_->SetScissor(scissor());
    depth_read_pipeline_->SetInputStreams(&plane_);
    depth_read_pipeline_->SetSamples(num_samples());
    depth_read_pipeline_->AddAttachment();
    depth_read_pipeline_->Commit();

    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        data_->root_allocator, app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data_ = containers::make_unique<vulkan::BufferFrameData<ModelData>>(
        data_->root_allocator, app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect = (float)width / (float)height;
    camera_data_->data().projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data_->data().transform =
        Mat44::FromTranslationVector(mathfu::Vector<float, 3>{2.0, 2.0, -3.0});
  }

  virtual void InitializeFrameData(
      DepthFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    const uint32_t width = app()->swapchain().width();
    const uint32_t height = app()->swapchain().height();
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->root_allocator, app()->GetCommandBuffer());

    frame_data->render_cube_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->root_allocator,
            app()->AllocateDescriptorSet(
                {render_cube_descriptor_set_layout_bindings_[0],
                 render_cube_descriptor_set_layout_bindings_[1]}));

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
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
        nullptr,                                   // pNext
        *frame_data->render_cube_descriptor_set_,  // dstSet
        0,                                         // dstbinding
        0,                                         // dstArrayElement
        2,                                         // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         // descriptorType
        nullptr,                                   // pImageInfo
        buffer_infos,                              // pBufferInfo
        nullptr,                                   // pTexelBufferView
    };

    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                            nullptr);

    frame_data->read_depth_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->root_allocator,
            app()->AllocateDescriptorSet(
                {depth_read_pipeline_layout_bindings_}));
    VkDescriptorImageInfo image_info = {
        VK_NULL_HANDLE,                                  // sampler
        depth_view(frame_data),                          // imageView
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL  // imageLayout
    };
    write.dstSet = *frame_data->read_depth_descriptor_set_;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    write.pBufferInfo = nullptr;
    write.pImageInfo = &image_info;
    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                            nullptr);

    VkImageView raw_views[2] = {depth_view(frame_data), color_view(frame_data)};

    // Create a framebuffer with depth and image attachments
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_cube_render_pass_,                  // renderPass
        2,                                          // attachmentCount
        raw_views,                                  // attachments
        width,                                      // width
        height,                                     // height
        1                                           // layers
    };

    ::VkFramebuffer raw_framebuffer;
    app()->device()->vkCreateFramebuffer(
        app()->device(), &framebuffer_create_info, nullptr, &raw_framebuffer);
    frame_data->framebuffer_ = containers::make_unique<vulkan::VkFramebuffer>(
        data_->root_allocator,
        vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app()->device()));

    (*frame_data->command_buffer_)
        ->vkBeginCommandBuffer((*frame_data->command_buffer_),
                               &sample_application::kBeginCommandBuffer);
    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);

    VkClearValue clears[2];
    clears[0].depthStencil.depth = 1.0f;
    vulkan::MemoryClear(&clears[1]);
    clears[1].color.float32[0] = 1.0f;

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_cube_render_pass_,                 // renderPass
        *frame_data->framebuffer_,                 // framebuffer
        {{0, 0}, {width, height}},                 // renderArea
        2,                                         // clearValueCount
        clears                                     // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *render_cube_pipeline_);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*render_cube_pipeline_layout_), 0, 1,
        &frame_data->render_cube_descriptor_set_->raw_set(), 0, nullptr);
    cube_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    pass_begin.renderPass = *depth_read_render_pass_;
    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*depth_read_pipeline_layout_), 0, 1,
        &frame_data->read_depth_descriptor_set_->raw_set(), 0, nullptr);
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *depth_read_pipeline_);
    plane_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    (*frame_data->command_buffer_)
        ->vkEndCommandBuffer(*frame_data->command_buffer_);
  }

  virtual void Update(float time_since_last_render) override {
    model_data_->data().transform =
        model_data_->data().transform *
        Mat44::FromRotationMatrix(
            Mat44::RotationX(3.14f * time_since_last_render) *
            Mat44::RotationY(3.14f * time_since_last_render * 0.5f));
  }
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      DepthFrameData* frame_data) override {
    // Update our uniform buffers.
    camera_data_->UpdateBuffer(queue, frame_index);
    model_data_->UpdateBuffer(queue, frame_index);

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
  }

 private:
  struct CameraData {
    bool operator==(const CameraData& other) {
      for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
          if (projection_matrix(i, j) != other.projection_matrix(i, j)) {
            return false;
          }
        }
      }
      return true;
    }
    Mat44 projection_matrix;
  };

  struct ModelData {
    bool operator==(const ModelData& other) {
      for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
          if (transform(i, j) != other.transform(i, j)) {
            return false;
          }
        }
      }
      return true;
    }
    Mat44 transform;
  };

  const entry::entry_data* data_;
  containers::unique_ptr<vulkan::PipelineLayout> render_cube_pipeline_layout_;
  containers::unique_ptr<vulkan::PipelineLayout> depth_read_pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> render_cube_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> depth_read_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_cube_render_pass_;
  containers::unique_ptr<vulkan::VkRenderPass> depth_read_render_pass_;
  VkDescriptorSetLayoutBinding render_cube_descriptor_set_layout_bindings_[2] =
      {};
  VkDescriptorSetLayoutBinding depth_read_pipeline_layout_bindings_ = {};
  vulkan::VulkanModel cube_;
  vulkan::VulkanModel plane_;

  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_data_;
};

int main_entry(const entry::entry_data* data) {
  data->log->LogInfo("Application Startup");
  DepthReadbackSample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->should_exit()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->log->LogInfo("Application Shutdown");
  return 0;
}
