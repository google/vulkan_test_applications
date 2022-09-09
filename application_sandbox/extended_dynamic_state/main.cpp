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

VkPhysicalDeviceExtendedDynamicStateFeaturesEXT kExtendedDynamicStateFeatures =
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
     nullptr, VK_TRUE};

const VkFormat kDepthStencilFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

struct ExtendedDynamicStateFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet>
      extended_dynamic_state_descriptor_set_;

  // The sample application assumes the depth format to  VK_FORMAT_D16_UNORM.
  // As we need to use stencil aspect, we declare another depth_stencil image
  // and its view here.
  vulkan::ImagePointer depth_stencil_image_;
  containers::unique_ptr<vulkan::VkImageView> depth_stencil_image_view_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class ExtendedDynamicStateSample
    : public sample_application::Sample<ExtendedDynamicStateFrameData> {
 public:
  ExtendedDynamicStateSample(const entry::EntryData* data)
      : data_(data),
        Sample<ExtendedDynamicStateFrameData>(
            data->allocator(), data, 1, 512, 1, 1,
            sample_application::SampleOptions()
                .EnableDepthBuffer()
                .EnableStencil()
                .AddDeviceExtensionStructure(&kExtendedDynamicStateFeatures),
            {0}, {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
            {VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME}),
        extended_dynamic_state_(data->allocator(), data->logger(), cube_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    extended_dynamic_state_.InitializeData(app(), initialization_buffer);

    extended_dynamic_state_descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    extended_dynamic_state_descriptor_set_layouts_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout(
            {{extended_dynamic_state_descriptor_set_layouts_[0],
              extended_dynamic_state_descriptor_set_layouts_[1]}}));

    VkAttachmentReference depth_attachment = {
        0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentReference color_attachment = {
        1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass(
            {{
                 0,                             // flags
                 kDepthStencilFormat,           // format
                 num_samples(),                 // samples
                 VK_ATTACHMENT_LOAD_OP_CLEAR,   // loadOp
                 VK_ATTACHMENT_STORE_OP_STORE,  // storeOp
                 VK_ATTACHMENT_LOAD_OP_CLEAR,   // stencilLoadOp
                 VK_ATTACHMENT_STORE_OP_STORE,  // stencilStoreOp
                 VK_IMAGE_LAYOUT_UNDEFINED,     // initialLayout
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL  // finalLayout
             },
             {
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
                &color_attachment,                // colorAttachmentg
                nullptr,                          // pResolveAttachments
                &depth_attachment,                // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    extended_dynamic_state_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->allocator(),
            app()->CreateGraphicsPipeline(pipeline_layout_.get(),
                                          render_pass_.get(), 0));
    extended_dynamic_state_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT,
                                                "main", cube_vertex_shader);
    extended_dynamic_state_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT,
                                                "main", cube_fragment_shader);
    extended_dynamic_state_pipeline_->SetInputStreams(&extended_dynamic_state_);
    VkViewport unusedViewport = {
        0.0f,
        0.0f,
        static_cast<float>(app()->swapchain().width()) / 2.0f,
        static_cast<float>(app()->swapchain().height()) / 2.0f,
        0.0f,
        1.0f};
    extended_dynamic_state_pipeline_->SetViewport(unusedViewport);
    extended_dynamic_state_pipeline_->SetScissor(scissor());
    extended_dynamic_state_pipeline_->SetSamples(num_samples());
    extended_dynamic_state_pipeline_->AddAttachment();

    // Set state in VkGraphicsPipelineCreateInfo that will be overwritten
    // by the dynamic state during command buffer recording.
    extended_dynamic_state_pipeline_->SetCullMode(VK_CULL_MODE_FRONT_AND_BACK);
    extended_dynamic_state_pipeline_->DepthStencilState()
        .depthBoundsTestEnable = VK_TRUE;
    extended_dynamic_state_pipeline_->DepthStencilState().minDepthBounds = 1.0f;
    extended_dynamic_state_pipeline_->DepthStencilState().maxDepthBounds = 0.0f;
    extended_dynamic_state_pipeline_->DepthStencilState().depthTestEnable =
        VK_FALSE;
    extended_dynamic_state_pipeline_->DepthStencilState().depthWriteEnable =
        VK_FALSE;
    extended_dynamic_state_pipeline_->DepthStencilState().depthCompareOp =
        VK_COMPARE_OP_NEVER;
    extended_dynamic_state_pipeline_->DepthStencilState().stencilTestEnable =
        VK_FALSE;
    VkStencilOpState stencilState = {VK_STENCIL_OP_ZERO,
                                     VK_STENCIL_OP_ZERO,
                                     VK_STENCIL_OP_ZERO,
                                     VK_COMPARE_OP_ALWAYS,
                                     255,
                                     255,
                                     1};
    extended_dynamic_state_pipeline_->DepthStencilState().front = stencilState;
    extended_dynamic_state_pipeline_->DepthStencilState().back = stencilState;

    extended_dynamic_state_pipeline_->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
    extended_dynamic_state_pipeline_->SetTopology(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT);
    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_CULL_MODE_EXT);
    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT);
    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT);
    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT);
    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT);
    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_FRONT_FACE_EXT);
    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT);
    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT_EXT);
    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_STENCIL_OP_EXT);
    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT);
    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT_EXT);

    // Not part of VK_EXT_extended_dynamic_state.
    extended_dynamic_state_pipeline_->AddDynamicState(
        VK_DYNAMIC_STATE_STENCIL_REFERENCE);

    extended_dynamic_state_pipeline_->Commit();

    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data_ = containers::make_unique<vulkan::BufferFrameData<ModelData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    camera_data_->data().projection_matrix =
        Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data_->data().transform = Mat44::FromTranslationVector(
        mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f});
  }

  virtual void InitializeFrameData(
      ExtendedDynamicStateFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    frame_data->extended_dynamic_state_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet(
                {extended_dynamic_state_descriptor_set_layouts_[0],
                 extended_dynamic_state_descriptor_set_layouts_[1]}));

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
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,               // sType
        nullptr,                                              // pNext
        *frame_data->extended_dynamic_state_descriptor_set_,  // dstSet
        0,                                                    // dstbinding
        0,                                                    // dstArrayElement
        2,                                                    // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,                    // descriptorType
        nullptr,                                              // pImageInfo
        buffer_infos,                                         // pBufferInfo
        nullptr,  // pTexelBufferView
    };

    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                            nullptr);

    ::VkImageView raw_views[2] = {depth_view(frame_data),
                                  color_view(frame_data)};

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
    clears[0].depthStencil.depth = 0.95f;
    clears[0].depthStencil.stencil = 0;

    vulkan::MemoryClear(&clears[1]);
    clears[1].color.float32[2] = 1.0f;

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

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *extended_dynamic_state_pipeline_);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &frame_data->extended_dynamic_state_descriptor_set_->raw_set(), 0,
        nullptr);

    VkBuffer buffers[3] = {extended_dynamic_state_.VertexBuffer(),
                           extended_dynamic_state_.VertexBuffer(),
                           extended_dynamic_state_.VertexBuffer()};
    VkDeviceSize offsets[3] = {
        0, extended_dynamic_state_.NumVertices() * vulkan::POSITION_SIZE,
        extended_dynamic_state_.NumVertices() *
            (vulkan::POSITION_SIZE + vulkan::TEXCOORD_SIZE)};
    cmdBuffer->vkCmdBindIndexBuffer(cmdBuffer,
                                    extended_dynamic_state_.IndexBuffer(), 0,
                                    VK_INDEX_TYPE_UINT32);

    VkDeviceSize sizes[3] = {
        extended_dynamic_state_.NumVertices() * vulkan::POSITION_SIZE,
        extended_dynamic_state_.NumVertices() * vulkan::TEXCOORD_SIZE,
        extended_dynamic_state_.NumVertices() * vulkan::NORMAL_SIZE};
    VkDeviceSize strides[3] = {vulkan::POSITION_SIZE, vulkan::TEXCOORD_SIZE,
                               vulkan::NORMAL_SIZE};
    cmdBuffer->vkCmdBindVertexBuffers2EXT(cmdBuffer, 0, 3, buffers, offsets,
                                          sizes, strides);

    // In total there are 4 cubes that could be drawn.

    // The pipeline has VK_CULL_MODE_FRONT_AND_BACK, so this is necessary to
    // display anything.
    cmdBuffer->vkCmdSetCullModeEXT(cmdBuffer, VK_CULL_MODE_BACK_BIT);
    // The original depth bounds test would fail everything, so we disable it.
    cmdBuffer->vkCmdSetDepthBoundsTestEnableEXT(cmdBuffer, VK_FALSE);
    // The original pipeline has depth test disabled. This will fail one cube at
    // the bottom left.
    cmdBuffer->vkCmdSetDepthTestEnableEXT(cmdBuffer, VK_TRUE);
    cmdBuffer->vkCmdSetDepthCompareOpEXT(cmdBuffer, VK_COMPARE_OP_LESS);
    // This fixes the front face.
    cmdBuffer->vkCmdSetFrontFaceEXT(cmdBuffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    // The original pipeline has triangle strip topology, fixing it here.
    cmdBuffer->vkCmdSetPrimitiveTopologyEXT(
        cmdBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // The scissor is set up so that only the cubes on the right side pass.
    VkRect2D scissor = {{(int32_t)(app()->swapchain().width() / 2), 0},
                        {(uint32_t)(app()->swapchain().width() / 2),
                         (uint32_t)(app()->swapchain().height())}};
    cmdBuffer->vkCmdSetScissorWithCountEXT(cmdBuffer, 1, &scissor);
    // Fixes the viewport so that the entire frame is used.
    VkViewport v = viewport();
    cmdBuffer->vkCmdSetViewportWithCountEXT(cmdBuffer, 1, &v);

    // Enable the depth write and draw for the depth information which will
    // be used in the subsequent draw.
    cmdBuffer->vkCmdSetDepthWriteEnableEXT(cmdBuffer, VK_TRUE);

    // This is just to write depth, we clear it after.
    cmdBuffer->vkCmdDrawIndexed(
        cmdBuffer, static_cast<uint32_t>(extended_dynamic_state_.NumIndices()),
        4 /* instanceCount */, 0, 0, 0);
    VkClearValue clearValue = {};
    clearValue.color.float32[0] = 1.0f;
    clearValue.color.float32[1] = 0.0f;
    clearValue.color.float32[2] = 1.0f;
    clearValue.color.float32[3] = 0.0f;
    VkClearAttachment clearAttachment = {VK_IMAGE_ASPECT_COLOR_BIT, 0,
                                         clearValue};
    VkClearRect clearRect = {{{0, 0},
                              {(uint32_t)(app()->swapchain().width()),
                               (uint32_t)(app()->swapchain().height())}},
                             0,
                             1};
    cmdBuffer->vkCmdClearAttachments(cmdBuffer, 1, &clearAttachment, 1,
                                     &clearRect);

    // This draw will not draw anything, since the two cubes will fail the
    // stencil test. Stencil state is set to equal to a reference value of 1.
    // This draw will simply increment the stencil value so that the next draw,
    // can actually draw the cubes.
    cmdBuffer->vkCmdSetStencilTestEnableEXT(cmdBuffer, VK_TRUE);
    cmdBuffer->vkCmdSetStencilOpEXT(
        cmdBuffer, VK_STENCIL_FACE_FRONT_AND_BACK /* faceMask */,
        VK_STENCIL_OP_INCREMENT_AND_CLAMP /* failOp */,
        VK_STENCIL_OP_KEEP /* passOp */, VK_STENCIL_OP_ZERO /* depthFailOp */,
        VK_COMPARE_OP_EQUAL /* compareOp */);
    cmdBuffer->vkCmdSetStencilReference(cmdBuffer,
                                        VK_STENCIL_FACE_FRONT_AND_BACK, 1);
    cmdBuffer->vkCmdDrawIndexed(
        cmdBuffer, static_cast<uint32_t>(extended_dynamic_state_.NumIndices()),
        4 /* instanceCount */, 0, 0, 0);
    // Actually draw the cubes.

    // This relies on the depth being written in a previous draw.
    cmdBuffer->vkCmdSetDepthCompareOpEXT(cmdBuffer, VK_COMPARE_OP_EQUAL);
    cmdBuffer->vkCmdDrawIndexed(
        cmdBuffer, static_cast<uint32_t>(extended_dynamic_state_.NumIndices()),
        4 /* instanceCount */, 0, 0, 0);

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
                      ExtendedDynamicStateFrameData* frame_data) override {
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
    Mat44 projection_matrix;
  };

  struct ModelData {
    Mat44 transform;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline>
      extended_dynamic_state_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  VkDescriptorSetLayoutBinding
      extended_dynamic_state_descriptor_set_layouts_[2];
  vulkan::VulkanModel extended_dynamic_state_;

  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_data_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  ExtendedDynamicStateSample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
