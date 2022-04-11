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

struct FrameData {
  // All of the commands to render a single frame
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  // The framebuffer for a single frame
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  // The descriptor_set used for rendering the cube
  containers::unique_ptr<vulkan::DescriptorSet> render_cube_vk_descriptor_set_;
  // The descriptor_set used for rendering the cube
  containers::unique_ptr<vulkan::DescriptorSet> render_cube_gl_descriptor_set_;
  // The descriptor_set used for reading the multi-sampled depth
  containers::unique_ptr<vulkan::DescriptorSet> read_depth_descriptor_set_;
};

VkPhysicalDeviceDepthClipControlFeaturesEXT kDepthClipControlFeature = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT,
    nullptr,
    VK_TRUE,
};

class DepthClipControlSample : public sample_application::Sample<FrameData> {
 public:
  DepthClipControlSample(const entry::EntryData* data)
      : data_(data),
        Sample<FrameData>(
            data->allocator(), data, 1, 512, 1, 1,
            sample_application::SampleOptions()
                .EnableMultisampling()
                .EnableDepthBuffer()
                .SetVulkanApiVersion(VK_API_VERSION_1_1)
                .AddDeviceExtensionStructure(&kDepthClipControlFeature),
            {}, {}, {VK_EXT_DEPTH_CLIP_CONTROL_EXTENSION_NAME}),
        cube_(data->allocator(), data->logger(), cube_data),
        plane_(data->allocator(), data->logger(), plane_data) {}

  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    const uint32_t width = app()->swapchain().width();
    const uint32_t height = app()->swapchain().height();

    cube_.InitializeData(app(), initialization_buffer);
    plane_.InitializeData(app(), initialization_buffer);

    VkAttachmentReference color_attachment = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_attachment = {
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_read_attachment = {
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};

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
             },
             {
                 0,                                 // flags
                 depth_format(),                    // format
                 num_samples(),                     // samples
                 VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp
                 VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
                 VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stencilLoadOp
                 VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stencilStoreOp
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,  // initialLayout
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // finalLayout
             }},  // AttachmentDescriptions
            {
                {
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
                },
                {
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
                },
            },  // SubpassDescriptions
            {
                {
                    0,                                          // srcSubpass
                    1,                                          // dstSubpass
                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,  // srcStageMask
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,      // dstStageMask
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,  // srcAccessMask
                    VK_ACCESS_SHADER_READ_BIT,  // dstAccessMask
                    0,                          // flags
                },
            }  // SubpassDependencies
            ));

    render_cube_vk_descriptor_set_layout_bindings_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    render_cube_vk_descriptor_set_layout_bindings_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };

    render_cube_pipeline_layout_ =
        containers::make_unique<vulkan::PipelineLayout>(
            data_->allocator(),
            app()->CreatePipelineLayout(
                {{render_cube_vk_descriptor_set_layout_bindings_[0],
                  render_cube_vk_descriptor_set_layout_bindings_[1]}}));

    render_cube_vk_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->allocator(),
            app()->CreateGraphicsPipeline(render_cube_pipeline_layout_.get(),
                                          render_pass_.get(), 0));
    render_cube_vk_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                        cube_vertex_shader);
    render_cube_vk_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                        cube_fragment_shader);
    render_cube_vk_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    render_cube_vk_pipeline_->SetInputStreams(&cube_);
    render_cube_vk_pipeline_->SetViewport(viewport());
    render_cube_vk_pipeline_->SetScissor(scissor());
    render_cube_vk_pipeline_->SetSamples(num_samples());
    render_cube_vk_pipeline_->AddAttachment();
    render_cube_vk_pipeline_->Commit();

    render_cube_gl_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->allocator(),
            app()->CreateGraphicsPipeline(render_cube_pipeline_layout_.get(),
                                          render_pass_.get(), 0));
    render_cube_gl_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                        cube_vertex_shader);
    render_cube_gl_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                        cube_fragment_shader);
    render_cube_gl_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    render_cube_gl_pipeline_->SetInputStreams(&cube_);
    render_cube_gl_pipeline_->SetViewport(viewport());
    render_cube_gl_pipeline_->SetScissor(scissor());
    render_cube_gl_pipeline_->SetSamples(num_samples());
    render_cube_gl_pipeline_->AddAttachment();
    VkPipelineViewportDepthClipControlCreateInfoEXT clip_control{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_DEPTH_CLIP_CONTROL_CREATE_INFO_EXT,
        nullptr,
        VK_TRUE,
    };
    render_cube_gl_pipeline_->SetViewportExtensions(&clip_control);
    render_cube_gl_pipeline_->Commit();

    depth_read_pipeline_layout_bindings_ = {
        0,                                    // binding
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,  // descriptorType
        1,                                    // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,         // stage
        nullptr                               // pImmutableSamplers
    };

    depth_read_pipeline_layout_ =
        containers::make_unique<vulkan::PipelineLayout>(
            data_->allocator(), app()->CreatePipelineLayout(
                                    {{depth_read_pipeline_layout_bindings_}}));

    depth_read_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->allocator(),
            app()->CreateGraphicsPipeline(depth_read_pipeline_layout_.get(),
                                          render_pass_.get(), 1));
    depth_read_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                    plane_vertex_shader);
    depth_read_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                    plane_fragment_shader);
    depth_read_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    auto depth_viewport = viewport();
    depth_viewport.width /= 2;
    depth_read_pipeline_->SetViewport(depth_viewport);
    depth_read_pipeline_->SetScissor(scissor());
    depth_read_pipeline_->SetInputStreams(&plane_);
    depth_read_pipeline_->SetSamples(num_samples());
    depth_read_pipeline_->AddAttachment();
    depth_read_pipeline_->Commit();

    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_vk_data_ =
        containers::make_unique<vulkan::BufferFrameData<ModelData>>(
            data_->allocator(), app(), num_swapchain_images,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_gl_data_ =
        containers::make_unique<vulkan::BufferFrameData<ModelData>>(
            data_->allocator(), app(), num_swapchain_images,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    // Both models share the same camera, the depth range is [2.5, 4], the model
    // is placed at z = -3.0 and is +/- 0.5 units wide so the front corner
    // should be clipped by the front plane as it spins.
    float aspect = (float)width / (float)height;
    camera_data_->data().projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 1.25f, 2.0f);

    model_vk_data_->data().transform =
        Mat44::FromTranslationVector(mathfu::Vector<float, 3>{0.0, 1.0, -3.0}) *
        Mat44::FromRotationMatrix(Mat44::RotationX(-3.14f / 12.0f));

    model_gl_data_->data().transform =
        Mat44::FromTranslationVector(
            mathfu::Vector<float, 3>{0.0, -1.0, -3.0}) *
        Mat44::FromRotationMatrix(Mat44::RotationX(3.14f / 12.0f));
  }

  virtual void InitializeFrameData(
      FrameData* frame_data, vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    const uint32_t width = app()->swapchain().width();
    const uint32_t height = app()->swapchain().height();

    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    frame_data->render_cube_vk_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet(
                {render_cube_vk_descriptor_set_layout_bindings_[0],
                 render_cube_vk_descriptor_set_layout_bindings_[1]}));

    {
      VkDescriptorBufferInfo buffer_infos[2] = {
          {
              camera_data_->get_buffer(),                       // buffer
              camera_data_->get_offset_for_frame(frame_index),  // offset
              camera_data_->size(),                             // range
          },
          {
              model_vk_data_->get_buffer(),                       // buffer
              model_vk_data_->get_offset_for_frame(frame_index),  // offset
              model_vk_data_->size(),                             // range
          }};

      VkWriteDescriptorSet write{
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,       // sType
          nullptr,                                      // pNext
          *frame_data->render_cube_vk_descriptor_set_,  // dstSet
          0,                                            // dstbinding
          0,                                            // dstArrayElement
          2,                                            // descriptorCount
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,            // descriptorType
          nullptr,                                      // pImageInfo
          buffer_infos,                                 // pBufferInfo
          nullptr,                                      // pTexelBufferView
      };

      app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                              nullptr);
    }

    frame_data->render_cube_gl_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet(
                {render_cube_vk_descriptor_set_layout_bindings_[0],
                 render_cube_vk_descriptor_set_layout_bindings_[1]}));

    {
      VkDescriptorBufferInfo buffer_infos[2] = {
          {
              camera_data_->get_buffer(),                       // buffer
              camera_data_->get_offset_for_frame(frame_index),  // offset
              camera_data_->size(),                             // range
          },
          {
              model_gl_data_->get_buffer(),                       // buffer
              model_gl_data_->get_offset_for_frame(frame_index),  // offset
              model_gl_data_->size(),                             // range
          }};

      VkWriteDescriptorSet write{
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,       // sType
          nullptr,                                      // pNext
          *frame_data->render_cube_gl_descriptor_set_,  // dstSet
          0,                                            // dstbinding
          0,                                            // dstArrayElement
          2,                                            // descriptorCount
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,            // descriptorType
          nullptr,                                      // pImageInfo
          buffer_infos,                                 // pBufferInfo
          nullptr,                                      // pTexelBufferView
      };

      app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                              nullptr);
    }

    frame_data->read_depth_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(), app()->AllocateDescriptorSet(
                                    {depth_read_pipeline_layout_bindings_}));

    {
      VkDescriptorImageInfo image_info = {
          VK_NULL_HANDLE,                                  // sampler
          depth_view(frame_data),                          // imageView
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL  // imageLayout
      };

      VkWriteDescriptorSet write{
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,   // sType
          nullptr,                                  // pNext
          *frame_data->read_depth_descriptor_set_,  // dstSet
          0,                                        // dstbinding
          0,                                        // dstArrayElement
          1,                                        // descriptorCount
          VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,      // descriptorType
          &image_info,                              // pImageInfo
          nullptr,                                  // pBufferInfo
          nullptr,                                  // pTexelBufferView
      };

      app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                              nullptr);
    }

    VkImageView raw_views[2] = {color_view(frame_data), depth_view(frame_data)};

    // Create a framebuffer with depth and image attachments
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_pass_,                              // renderPass
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
        data_->allocator(),
        vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app()->device()));

    (*frame_data->command_buffer_)
        ->vkBeginCommandBuffer((*frame_data->command_buffer_),
                               &sample_application::kBeginCommandBuffer);
    vulkan::VkCommandBuffer& cmdBuffer = (*frame_data->command_buffer_);

    VkClearValue clears[2];
    clears[0].depthStencil = {1.0f, 0};
    clears[1].color = {1.0f, 0.0f, 0.0f, 0.0f};

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *frame_data->framebuffer_,                 // framebuffer
        {{0, 0}, {width, height}},                 // renderArea
        2,                                         // clearValueCount
        clears                                     // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*render_cube_pipeline_layout_), 0, 1,
        &frame_data->render_cube_vk_descriptor_set_->raw_set(), 0, nullptr);
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *render_cube_vk_pipeline_);
    cube_.Draw(&cmdBuffer);

    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*render_cube_pipeline_layout_), 0, 1,
        &frame_data->render_cube_gl_descriptor_set_->raw_set(), 0, nullptr);
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *render_cube_gl_pipeline_);
    cube_.Draw(&cmdBuffer);

    cmdBuffer->vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*depth_read_pipeline_layout_), 0, 1,
        &frame_data->read_depth_descriptor_set_->raw_set(), 0, nullptr);
    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *depth_read_pipeline_);
    plane_.Draw(&cmdBuffer);

    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    cmdBuffer->vkEndCommandBuffer(*frame_data->command_buffer_);
  }

  virtual void Update(float time_since_last_render) override {
    model_vk_data_->data().transform =
        model_vk_data_->data().transform *
        Mat44::FromRotationMatrix(
            Mat44::RotationY(3.14f * time_since_last_render / 3.0f));

    model_gl_data_->data().transform =
        model_gl_data_->data().transform *
        Mat44::FromRotationMatrix(
            Mat44::RotationY(3.14f * time_since_last_render / 3.0f));
  }

  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      FrameData* frame_data) override {
    // Update our uniform buffers.
    camera_data_->UpdateBuffer(queue, frame_index);
    model_vk_data_->UpdateBuffer(queue, frame_index);
    model_gl_data_->UpdateBuffer(queue, frame_index);

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
    Mat44 projection_matrix;
  };

  struct ModelData {
    Mat44 transform;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> render_cube_pipeline_layout_;
  containers::unique_ptr<vulkan::PipelineLayout> depth_read_pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline>
      render_cube_vk_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline>
      render_cube_gl_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> depth_read_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  VkDescriptorSetLayoutBinding
      render_cube_vk_descriptor_set_layout_bindings_[2] = {};
  VkDescriptorSetLayoutBinding depth_read_pipeline_layout_bindings_ = {};
  vulkan::VulkanModel cube_;
  vulkan::VulkanModel plane_;

  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_vk_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_gl_data_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  DepthClipControlSample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
