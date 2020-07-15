// Copyright 2020 Google Inc.
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

uint32_t cube_vertex_shader[] =
#include "cube.vert.spv"
    ;

uint32_t cube_fragment_shader[] =
#include "cube.frag.spv"
    ;

struct DepthClipEnableFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_red_descriptor_set_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_green_descriptor_set_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_blue_descriptor_set_;
};

VkPhysicalDeviceDepthClipEnableFeaturesEXT kDepthClipEnableFeature = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT, nullptr,
    VK_TRUE};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class DepthClipEnableSample
    : public sample_application::Sample<DepthClipEnableFrameData> {
 public:
  DepthClipEnableSample(const entry::EntryData* data,
                        const VkPhysicalDeviceFeatures& requested_features)
      : data_(data),
        Sample<DepthClipEnableFrameData>(
            data->allocator(), data, 1, 512, 1, 1,
            sample_application::SampleOptions()
                .EnableDepthBufferFloat()
                .EnableMultisampling()
                .AddDeviceExtensionStructure(&kDepthClipEnableFeature),
            requested_features, {}, {VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME}),
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
        2,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,       // stageFlags
        nullptr                             // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout(
            {{cube_descriptor_set_layouts_[0], cube_descriptor_set_layouts_[1],
              cube_descriptor_set_layouts_[2]}}));

    VkAttachmentReference color_attachment = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_attachment = {
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

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
             },
             {
                 0,                                 // flags
                 depth_format(),                    // format
                 num_samples(),                     // samples
                 VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp
                 VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
                 VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stenilLoadOp
                 VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stenilStoreOp
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,  // initialLayout
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // finalLayout
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

    // The red_pipeline_ is for the red cube. This is rendering with depth
    // clipping enabled and depth clamping disabled -- This would be stock
    // Vulkan. Additionally, the geometry is rendered with a "-2" depth bias to
    // effectively put it in front of everything
    red_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));
    red_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                             cube_vertex_shader);
    red_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                             cube_fragment_shader);
    red_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    red_pipeline_->SetInputStreams(&cube_);
    red_pipeline_->SetViewport(viewport());
    red_pipeline_->SetScissor(scissor());
    red_pipeline_->SetSamples(num_samples());
    red_pipeline_->AddAttachment();
    red_pipeline_->SetDepthClampEnable(VK_FALSE);
    red_pipeline_->EnableDepthBias(-2, -2, 0);

    VkPipelineRasterizationDepthClipStateCreateInfoEXT red_depth_clip_state{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT,
        nullptr, 0, VK_TRUE};
    red_pipeline_->SetRasterizationExtension(&red_depth_clip_state);

    red_pipeline_->Commit();

    // The green_pipeline_ is for the green cube. This is rendering with depth
    // clipping disabled and depth clamping enabled -- This would be default
    // Vulkan with depthClamp enabled (i.e with clamping you disable clipping).
    // This is rendered with no bias putting it in the "middle"
    green_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));
    green_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                               cube_vertex_shader);
    green_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                               cube_fragment_shader);
    green_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    green_pipeline_->SetInputStreams(&cube_);
    green_pipeline_->SetViewport(viewport());
    green_pipeline_->SetScissor(scissor());
    green_pipeline_->SetSamples(num_samples());
    green_pipeline_->AddAttachment();
    green_pipeline_->SetDepthClampEnable(VK_TRUE);

    VkPipelineRasterizationDepthClipStateCreateInfoEXT green_depth_clip_state{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT,
        nullptr, 0, VK_FALSE};
    green_pipeline_->SetRasterizationExtension(&green_depth_clip_state);

    green_pipeline_->Commit();

    // The blue_pipeline_ is for the blue cube. This is rendering with depth
    // clipping disabled and depth clamping dsiable -- This behaviour that
    // only comes with the VK_EXT_depth_clip_enable feature. This geometry
    // is rendered with a "+2" depth bias to put it in the "back".
    blue_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(), app()->CreateGraphicsPipeline(
                                pipeline_layout_.get(), render_pass_.get(), 0));
    blue_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                              cube_vertex_shader);
    blue_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                              cube_fragment_shader);
    blue_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    blue_pipeline_->SetInputStreams(&cube_);
    blue_pipeline_->SetViewport(viewport());
    blue_pipeline_->SetScissor(scissor());
    blue_pipeline_->SetSamples(num_samples());
    blue_pipeline_->AddAttachment();
    blue_pipeline_->SetDepthClampEnable(VK_FALSE);
    blue_pipeline_->EnableDepthBias(2, 2, 0);

    VkPipelineRasterizationDepthClipStateCreateInfoEXT blue_depth_clip_state{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT,
        nullptr, 0, VK_FALSE};
    blue_pipeline_->SetRasterizationExtension(&blue_depth_clip_state);

    blue_pipeline_->Commit();

    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data_ = containers::make_unique<vulkan::BufferFrameData<ModelData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    color_red_data_ =
        containers::make_unique<vulkan::BufferFrameData<ColorData>>(
            data_->allocator(), app(), num_swapchain_images,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    color_green_data_ =
        containers::make_unique<vulkan::BufferFrameData<ColorData>>(
            data_->allocator(), app(), num_swapchain_images,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    color_blue_data_ =
        containers::make_unique<vulkan::BufferFrameData<ColorData>>(
            data_->allocator(), app(), num_swapchain_images,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    camera_data_->data().projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(3.14f / 4, aspect, 1.2f, 1.5f);

    model_data_->data().transform =
        Mat44::FromTranslationVector(
            mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f}) *
        Mat44::FromRotationMatrix(Mat44::RotationX(3.14f / 8.0f));

    color_red_data_->data().color = Vector4(1.0, 0.0, 0.0, 0.0);
    color_green_data_->data().color = Vector4(0.0, 1.0, 0.0, 0.0);
    color_blue_data_->data().color = Vector4(0.0, 0.0, 1.0, 0.0);
  }

  virtual void InitializeFrameData(
      DepthClipEnableFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    frame_data->cube_red_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet({cube_descriptor_set_layouts_[0],
                                          cube_descriptor_set_layouts_[1],
                                          cube_descriptor_set_layouts_[2]}));
    frame_data->cube_green_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet({cube_descriptor_set_layouts_[0],
                                          cube_descriptor_set_layouts_[1],
                                          cube_descriptor_set_layouts_[2]}));
    frame_data->cube_blue_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet({cube_descriptor_set_layouts_[0],
                                          cube_descriptor_set_layouts_[1],
                                          cube_descriptor_set_layouts_[2]}));

    VkDescriptorBufferInfo buffer_infos_common[2] = {
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
    VkDescriptorBufferInfo buffer_infos_red[1] = {{
        color_red_data_->get_buffer(),                       // buffer
        color_red_data_->get_offset_for_frame(frame_index),  // offset
        color_red_data_->size(),                             // range
    }};
    VkDescriptorBufferInfo buffer_infos_green[1] = {{
        color_green_data_->get_buffer(),                       // buffer
        color_green_data_->get_offset_for_frame(frame_index),  // offset
        color_green_data_->size(),                             // range
    }};
    VkDescriptorBufferInfo buffer_infos_blue[1] = {{
        color_blue_data_->get_buffer(),                       // buffer
        color_blue_data_->get_offset_for_frame(frame_index),  // offset
        color_blue_data_->size(),                             // range
    }};

    VkWriteDescriptorSet writes_vertex[3] = {
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->cube_red_descriptor_set_,   // dstSet
            0,                                       // dstbinding
            0,                                       // dstArrayElement
            2,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
            nullptr,                                 // pImageInfo
            buffer_infos_common,                     // pBufferInfo
            nullptr,                                 // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,   // sType
            nullptr,                                  // pNext
            *frame_data->cube_green_descriptor_set_,  // dstSet
            0,                                        // dstbinding
            0,                                        // dstArrayElement
            2,                                        // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,        // descriptorType
            nullptr,                                  // pImageInfo
            buffer_infos_common,                      // pBufferInfo
            nullptr,                                  // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->cube_blue_descriptor_set_,  // dstSet
            0,                                       // dstbinding
            0,                                       // dstArrayElement
            2,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
            nullptr,                                 // pImageInfo
            buffer_infos_common,                     // pBufferInfo
            nullptr,                                 // pTexelBufferView
        }};

    app()->device()->vkUpdateDescriptorSets(app()->device(), 3, writes_vertex,
                                            0, nullptr);

    VkWriteDescriptorSet writes_fragment[3] = {
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->cube_red_descriptor_set_,   // dstSet
            2,                                       // dstbinding
            0,                                       // dstArrayElement
            1,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
            nullptr,                                 // pImageInfo
            buffer_infos_red,                        // pBufferInfo
            nullptr,                                 // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,   // sType
            nullptr,                                  // pNext
            *frame_data->cube_green_descriptor_set_,  // dstSet
            2,                                        // dstbinding
            0,                                        // dstArrayElement
            1,                                        // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,        // descriptorType
            nullptr,                                  // pImageInfo
            buffer_infos_green,                       // pBufferInfo
            nullptr,                                  // pTexelBufferView
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->cube_blue_descriptor_set_,  // dstSet
            2,                                       // dstbinding
            0,                                       // dstArrayElement
            1,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
            nullptr,                                 // pImageInfo
            buffer_infos_blue,                       // pBufferInfo
            nullptr,                                 // pTexelBufferView
        }};

    app()->device()->vkUpdateDescriptorSets(app()->device(), 3, writes_fragment,
                                            0, nullptr);

    ::VkImageView raw_views[2] = {color_view(frame_data),
                                  depth_view(frame_data)};

    // Create a framebuffer with depth and image attachments
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_pass_,                              // renderPass
        2,                                          // attachmentCount
        raw_views,                                  // attachments
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

    VkClearValue clears[2];
    vulkan::MemoryClear(&clears[0]);
    vulkan::MemoryClear(&clears[1]);
    clears[1].depthStencil.depth = 1.0f;

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *frame_data->framebuffer_,                 // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        2,                                // clearValueCount
        clears                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &frame_data->cube_red_descriptor_set_->raw_set(), 0, nullptr);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *red_pipeline_);
    cube_.Draw(&cmdBuffer);

    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &frame_data->cube_green_descriptor_set_->raw_set(), 0, nullptr);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *green_pipeline_);
    cube_.Draw(&cmdBuffer);

    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &frame_data->cube_blue_descriptor_set_->raw_set(), 0, nullptr);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *blue_pipeline_);
    cube_.Draw(&cmdBuffer);

    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    (*frame_data->command_buffer_)
        ->vkEndCommandBuffer(*frame_data->command_buffer_);
  }

  virtual void Update(float time_since_last_render) override {
    model_data_->data().transform =
        model_data_->data().transform *
        Mat44::FromRotationMatrix(
            // Mat44::RotationX(3.14f * time_since_last_render / 5.0f) *
            Mat44::RotationY(3.14f * time_since_last_render / 5.0f));
  }

  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      DepthClipEnableFrameData* frame_data) override {
    // Update our uniform buffers.
    camera_data_->UpdateBuffer(queue, frame_index);
    model_data_->UpdateBuffer(queue, frame_index);
    color_red_data_->UpdateBuffer(queue, frame_index);
    color_green_data_->UpdateBuffer(queue, frame_index);
    color_blue_data_->UpdateBuffer(queue, frame_index);

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

  struct ColorData {
    Vector4 color;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> red_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> green_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> blue_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  VkDescriptorSetLayoutBinding cube_descriptor_set_layouts_[3];
  vulkan::VulkanModel cube_;

  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ColorData>> color_red_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ColorData>> color_green_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ColorData>> color_blue_data_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  VkPhysicalDeviceFeatures requested_features = {0};
  requested_features.depthClamp = VK_TRUE;
  requested_features.depthBiasClamp = VK_TRUE;
  DepthClipEnableSample sample(data, requested_features);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
