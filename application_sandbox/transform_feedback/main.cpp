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

#include "particle_data_shared.h"

uint32_t simulation_shader[] =
#include "particle_update.vert.spv"
    ;

uint32_t particle_fragment_shader[] =
#include "particle.frag.spv"
    ;

uint32_t particle_vertex_shader[] =
#include "particle.vert.spv"
    ;

VkPhysicalDeviceTransformFeedbackFeaturesEXT transform_feedback_feature{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT,  // sType
    nullptr,                                                            // pNext
    true,  // transformFeedback
    true   // geometryStreams
};

struct TransformFeedbackFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> draw_command_buffer_;
  containers::unique_ptr<vulkan::VkCommandBuffer> transform_feedback_command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> transform_feedback_framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> particle_descriptor_set_;
  containers::unique_ptr<vulkan::VkSemaphore> render_semaphore_;
};

class TransformFeedbackSample
    : public sample_application::Sample<TransformFeedbackFrameData> {
 public:
  TransformFeedbackSample(const entry::EntryData* data)
      : data_(data),
        Sample<TransformFeedbackFrameData>(
            data->allocator(), data, 1, 512, 32, 1,
            sample_application::SampleOptions().AddDeviceExtensionStructure(
                &transform_feedback_feature),
            {0},
            {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
            {VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME}) {}

  void prepareTransformFeedbackPipeline() {
    particle_descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };

    transform_feedback_pipeline_layout_ =
        containers::make_unique<vulkan::PipelineLayout>(
            data_->allocator(), app()->CreatePipelineLayout(
                                    {{particle_descriptor_set_layouts_[0]}}));

    transform_feedback_render_pass_ =
        containers::make_unique<vulkan::VkRenderPass>(
            data_->allocator(),
            app()->CreateRenderPass(
                {},  // AttachmentDescriptions
                {{
                    0,                                // flags
                    VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
                    0,                                // inputAttachmentCount
                    nullptr,                          // pInputAttachments
                    0,                                // colorAttachmentCount
                    nullptr,                          // colorAttachment
                    nullptr,                          // pResolveAttachments
                    nullptr,                          // pDepthStencilAttachment
                    0,                                // preserveAttachmentCount
                    nullptr                           // pPreserveAttachments
                }},                                   // SubpassDescriptions
                {}                                    // SubpassDependencies
                ));

    transform_feedback_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->allocator(), app()->CreateGraphicsPipeline(
                                    transform_feedback_pipeline_layout_.get(),
                                    transform_feedback_render_pass_.get(), 0));
    transform_feedback_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                            simulation_shader);
    transform_feedback_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

    vulkan::VulkanGraphicsPipeline::InputStream input_stream{
        0,                           // binding
        VK_FORMAT_R32G32B32_SFLOAT,  // format
        0                            // offset
    };

    transform_feedback_pipeline_->AddInputStream(
        sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX, {input_stream});

    transform_feedback_pipeline_->SetViewport(viewport());
    transform_feedback_pipeline_->SetScissor(scissor());
    transform_feedback_pipeline_->SetSamples(num_samples());
    transform_feedback_pipeline_->AddAttachment();
    transform_feedback_pipeline_->Commit();
  }

  void prepareDrawPipeline() {
    particle_descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout({{particle_descriptor_set_layouts_[0]}}));

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

    particle_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->allocator(),
            app()->CreateGraphicsPipeline(pipeline_layout_.get(),
                                          render_pass_.get(), 0));
    particle_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                  particle_vertex_shader);
    particle_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                  particle_fragment_shader);
    particle_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

    vulkan::VulkanGraphicsPipeline::InputStream input_stream{
        0,                           // binding
        VK_FORMAT_R32G32B32_SFLOAT,  // format
        0                            // offset
    };

    particle_pipeline_->AddInputStream(
        sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX, {input_stream});
    particle_pipeline_->SetViewport(viewport());
    particle_pipeline_->SetScissor(scissor());
    particle_pipeline_->SetSamples(num_samples());
    particle_pipeline_->AddAttachment(VkPipelineColorBlendAttachmentState{
        VK_TRUE, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE,
        VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE,
        VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT});
    particle_pipeline_->Commit();
  }

  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    aspect_buffer_ = containers::make_unique<vulkan::BufferFrameData<Vector4>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    // All of this is the fairly standard setup for rendering.
	VkBufferCreateInfo transform_feedback_buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // flags
        sizeof(float) * 3 * TOTAL_PARTICLES,   // size
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT |
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr};
    transform_feedback_buffer_ = app()->CreateAndBindHostBuffer(
        &transform_feedback_buffer_create_info);

    vertex_buffer_ = containers::make_unique<
        vulkan::BufferFrameData<float[3 * TOTAL_PARTICLES]>>(
        data_->allocator(), app(), 1, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	float init_data[3 * TOTAL_PARTICLES];

    srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < 3 * TOTAL_PARTICLES; ++i) {
      init_data[i] = float((double)rand() / (double)(RAND_MAX - 1));
      init_data[i] -= 0.5f;
      init_data[i] *= 3.0f;
    }

    memcpy(&vertex_buffer_->data()[0], &init_data[0], vertex_buffer_->size());

    prepareTransformFeedbackPipeline();
    prepareDrawPipeline();
  }

  virtual void InitializeFrameData(
      TransformFeedbackFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    frame_data->transform_feedback_command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    frame_data->draw_command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    frame_data->particle_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(), app()->AllocateDescriptorSet(
                                    {particle_descriptor_set_layouts_[0]}));

    frame_data->render_semaphore_ =
        containers::make_unique<vulkan::VkSemaphore>(
            data_->allocator(), vulkan::CreateSemaphore(&app()->device()));
    ::VkImageView raw_view = color_view(frame_data);

    VkSubmitInfo init_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask,
        0,                              // commandBufferCount
        nullptr,
        0,                              // signalSemaphoreCount
        &frame_data->render_semaphore_->get_raw_object()  // pSignalSemaphores
    };

    app()->render_queue()->vkQueueSubmit(app()->render_queue(), 1,
                                         &init_submit_info,
                                         static_cast<VkFence>(VK_NULL_HANDLE));

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

    VkFramebufferCreateInfo transform_feedback_framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *transform_feedback_render_pass_,           // renderPass
        0,                                          // attachmentCount
        nullptr,                                    // attachments
        app()->swapchain().width(),                 // width
        app()->swapchain().height(),                // height
        1                                           // layers
    };

    ::VkFramebuffer raw_transform_feedback_framebuffer;
    app()->device()->vkCreateFramebuffer(app()->device(), &transform_feedback_framebuffer_create_info, nullptr,
                                         &raw_transform_feedback_framebuffer);
    frame_data->transform_feedback_framebuffer_ =
        containers::make_unique<vulkan::VkFramebuffer>(
        data_->allocator(),
        vulkan::VkFramebuffer(raw_transform_feedback_framebuffer, nullptr,
                              &app()->device()));
  }

  virtual void InitializationComplete() override {
  }

  virtual void Update(float delta_time) override {
    aspect_buffer_->data().x =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();

    aspect_buffer_->data().y = delta_time;
  }

  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      TransformFeedbackFrameData* data) override {

    aspect_buffer_->UpdateBuffer(&app()->render_queue(), frame_index);
    vertex_buffer_->UpdateBuffer(&app()->render_queue(), 0);

    // Write that buffer into the descriptor sets.
    VkDescriptorBufferInfo buffer_infos[1] = {
        {
            aspect_buffer_->get_buffer(),                       // buffer
            aspect_buffer_->get_offset_for_frame(frame_index),  // offset
            aspect_buffer_->size(),                             // range
        }};

    VkWriteDescriptorSet writes[1]{
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *data->particle_descriptor_set_,         // dstSet
            0,                                       // dstbinding
            0,                                       // dstArrayElement
            1,                                       // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
            nullptr,                                 // pImageInfo
            &buffer_infos[0],                        // pBufferInfo
            nullptr,                                 // pTexelBufferView
        }
    };

    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, writes, 0,
                                            nullptr);

    VkClearValue clear;
    vulkan::MemoryClear(&clear);
    clear.color.float32[3] = 1.0f;

    // Record our command-buffer for transform feedback
    (*data->transform_feedback_command_buffer_)
        ->vkResetCommandBuffer((*data->transform_feedback_command_buffer_), 0);
    (*data->transform_feedback_command_buffer_)
        ->vkBeginCommandBuffer((*data->transform_feedback_command_buffer_),
                               &sample_application::kBeginCommandBuffer);
    vulkan::VkCommandBuffer& transform_feedback_cmd_buffer =
        (*data->transform_feedback_command_buffer_);

    VkRenderPassBeginInfo transform_feedback_pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *transform_feedback_render_pass_,          // renderPass
        *data->transform_feedback_framebuffer_,    // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        0,                                // clearValueCount
        &clear                            // clears
    };

    transform_feedback_cmd_buffer->vkCmdBeginRenderPass(
        transform_feedback_cmd_buffer, &transform_feedback_pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

	VkBuffer tf_buffer = (*transform_feedback_buffer_.get());
    VkDeviceSize offsets[1] = {0};
    transform_feedback_cmd_buffer->vkCmdBindTransformFeedbackBuffersEXT(
        transform_feedback_cmd_buffer, 0, 1, &tf_buffer, offsets, nullptr);

    transform_feedback_cmd_buffer->vkCmdBeginTransformFeedbackEXT(
        transform_feedback_cmd_buffer, 0, 0, nullptr, nullptr);

    transform_feedback_cmd_buffer->vkCmdBindPipeline(
        transform_feedback_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *transform_feedback_pipeline_);

    transform_feedback_cmd_buffer->vkCmdBindDescriptorSets(
        transform_feedback_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*transform_feedback_pipeline_layout_), 0, 1,
        &data->particle_descriptor_set_->raw_set(), 0, nullptr);

    ::VkBuffer vertex_buffers[1] = {vertex_buffer_->get_buffer()};
    ::VkDeviceSize vertex_offsets[1] = {0};
    transform_feedback_cmd_buffer->vkCmdBindVertexBuffers(
        transform_feedback_cmd_buffer, 0, 1, vertex_buffers, vertex_offsets);

    transform_feedback_cmd_buffer->vkCmdDraw(transform_feedback_cmd_buffer,
                                             TOTAL_PARTICLES, 1, 0, 0);

    transform_feedback_cmd_buffer->vkCmdEndTransformFeedbackEXT(
        transform_feedback_cmd_buffer, 0, 0, nullptr, nullptr);

    transform_feedback_cmd_buffer->vkCmdEndRenderPass(
        transform_feedback_cmd_buffer);

    (*data->transform_feedback_command_buffer_)
        ->vkEndCommandBuffer(*data->transform_feedback_command_buffer_);

    VkSubmitInfo transform_feedback_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask,
        1,                              // commandBufferCount
        &(data->transform_feedback_command_buffer_
              ->get_command_buffer()),  // pCommandBuffers
        0,                              // signalSemaphoreCount
        nullptr                         // pSignalSemaphores
    };

    app()->render_queue()->vkQueueSubmit(app()->render_queue(), 1,
                                         &transform_feedback_submit_info,
                                         static_cast<VkFence>(VK_NULL_HANDLE));

    // Record our command-buffer for rendering this frame
    (*data->draw_command_buffer_)
        ->vkResetCommandBuffer((*data->draw_command_buffer_), 0);
    (*data->draw_command_buffer_)
        ->vkBeginCommandBuffer((*data->draw_command_buffer_),
                               &sample_application::kBeginCommandBuffer);
    vulkan::VkCommandBuffer& cmdBuffer = (*data->draw_command_buffer_);

    // The rest of the normal drawing.
    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *data->framebuffer_,                       // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        1,                                // clearValueCount
        &clear                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *particle_pipeline_);

    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &data->particle_descriptor_set_->raw_set(), 0, nullptr);

    cmdBuffer->vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertex_buffers,
                                      vertex_offsets);
    cmdBuffer->vkCmdDraw(cmdBuffer, TOTAL_PARTICLES, 1, 0, 0);

    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    (*data->draw_command_buffer_)
        ->vkEndCommandBuffer(*data->draw_command_buffer_);

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

    VkSubmitInfo init_submit_info{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
        nullptr,                        // pNext
        0,                              // waitSemaphoreCount
        nullptr,                        // pWaitSemaphores
        nullptr,                        // pWaitDstStageMask,
        1,                              // commandBufferCount
        &(data->draw_command_buffer_->get_command_buffer()),  // pCommandBuffers
        0,                                          // signalSemaphoreCount
        &data->render_semaphore_->get_raw_object()  // pSignalSemaphores
    };

    app()->render_queue()->vkQueueSubmit(app()->render_queue(), 1,
                                         &init_submit_info,
                                         static_cast<VkFence>(VK_NULL_HANDLE));

    app()->render_queue()->vkQueueWaitIdle(app()->render_queue());

    memcpy(&vertex_buffer_->data()[0],
           transform_feedback_buffer_.get()->base_address(),
           vertex_buffer_->size());
  }

 private:
  const entry::EntryData* data_;

  // All of the data needed for the transform feedback rendering pipeline.
  VkDescriptorSetLayoutBinding transform_feedback_descriptor_set_layouts_[4];
  containers::unique_ptr<vulkan::PipelineLayout>
      transform_feedback_pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline>
      transform_feedback_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> transform_feedback_render_pass_;

  // All of the data needed for the particle rendering pipeline.
  VkDescriptorSetLayoutBinding particle_descriptor_set_layouts_[1];
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> particle_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;

  // This ssbo just contains the aspect ratio. We use a Vector4 just so we
  // get proper alignment. The second float stores dt
  containers::unique_ptr<vulkan::BufferFrameData<Vector4>> aspect_buffer_;
  // A buffer that we use for transform feedback.
  containers::unique_ptr<vulkan::VulkanApplication::Buffer>
      transform_feedback_buffer_;
  // A buffer that we use for vertex data.
  containers::unique_ptr<vulkan::BufferFrameData<float[3 * TOTAL_PARTICLES]>>
      vertex_buffer_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  TransformFeedbackSample sample(data);
  if (!sample.is_valid()) {
    data->logger()->LogInfo("Application is invalid.");
    return -1;
  }
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();
  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
