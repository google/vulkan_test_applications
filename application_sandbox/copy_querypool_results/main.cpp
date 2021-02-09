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
using Vector3 = mathfu::Vector<float, 3>;
namespace torus_model {
#include "torus_knot.obj.h"
}
const auto& torus_data = torus_model::model;

uint32_t torus_vertex_shader[] =
#include "wireframe.vert.spv"
    ;

uint32_t torus_fragment_shader[] =
#include "wireframe.frag.spv"
    ;

struct WireframeFrameData {
  containers::unique_ptr<vulkan::VkCommandBuffer> command_buffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> torus_descriptor_set_;
  containers::unique_ptr<vulkan::VkBufferView> query_pool_results_buf_view_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class CopyQueryPoolResultSample
    : public sample_application::Sample<WireframeFrameData> {
 public:
  CopyQueryPoolResultSample(const entry::EntryData* data,
                            const VkPhysicalDeviceFeatures& requested_features)
      : data_(data),
        Sample<WireframeFrameData>(data->allocator(), data, 1, 512, 1, 1,
                                   sample_application::SampleOptions()
                                       .EnableDepthBuffer()
                                       .EnableMultisampling(),
                                   requested_features),
        torus_(data->allocator(), data->logger(), torus_data),
        grey_scale_(0u),
        num_frames_(0u) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    num_frames_ = static_cast<uint32_t>(num_swapchain_images);

    // For GAPID, when Mid-Execution Capture is applied, we need to reconstruct
    // the graphics state before starting tracing the following graphics
    // commands.  However, the query results are dependent on the draw calls
    // before the reconstruction of the graphics state so they cannot be
    // reconstructed completely. This means the first several query pool
    // results obtained on the replay device can be incorrect when
    // Mid-Execution Capture is turned on.
    // This sample obtains the query pool results prior to the draw call for
    // each frame, and uses the query results to determine the greyscale of the
    // model to be drawn. In a normal run, the first draw call of each frame
    // will print a blank (just black screen), and the following ones will show
    // the model. When Mid-Execution Capture is applied, no matter on which
    // frame the tracing is started, we expect the first draw call of each
    // frame on the replay device will always be blank, and the following ones
    // will show the model.
    // E.g.: Mid-Execution Capture starting on Frame 100, Number of swapchain
    // images on tracing and replay device: 3.
    // Tracing device: First 3 frames are blank. Frame 100, 101, 102 show the
    // model.
    // Replay device: First 3 frames (corresponding to frame 100, 101, 102 on
    // the tracing device) are blank, following ones shows the model.

    // Create a occlusion query pool that contains a query for each frame.
    VkQueryPoolCreateInfo query_pool_create_info{
        VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,
        VK_QUERY_TYPE_OCCLUSION,
        uint32_t(num_swapchain_images),
        0,
    };
    query_pool_ = containers::make_unique<vulkan::VkQueryPool>(
        data_->allocator(),
        vulkan::CreateQueryPool(&app()->device(), query_pool_create_info));

    // Query before drawing anything to make sure the initial value of query
    // pool results are zero.
    for (size_t i = 0; i < num_swapchain_images; i++) {
      (*initialization_buffer)
          ->vkCmdResetQueryPool(*initialization_buffer, *query_pool_,
                                static_cast<uint32_t>(i), 1);
      (*initialization_buffer)
          ->vkCmdBeginQuery(*initialization_buffer, *query_pool_,
                            static_cast<uint32_t>(i),
                            VkQueryControlFlagBits(0));
      (*initialization_buffer)
          ->vkCmdEndQuery(*initialization_buffer, *query_pool_,
                          static_cast<uint32_t>(i));
    }

    // Create a buffer to store the query results for each frame, and to be
    // used in the fragment shader.
    size_t query_pool_result_buf_size =
        vulkan::kMaxOffsetAlignment * num_swapchain_images;

    VkBufferCreateInfo query_pool_results_buf_create_info{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr,
        0,
        query_pool_result_buf_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr};
    query_pool_results_buf_ =
        app()->CreateAndBindHostBuffer(&query_pool_results_buf_create_info);
    containers::vector<uint32_t> query_pool_init_values(
        query_pool_result_buf_size, 0xFFFFFFFF, data_->allocator());
    memcpy(query_pool_results_buf_->base_address(),
           query_pool_init_values.data(), query_pool_result_buf_size);
    query_pool_results_buf_->flush(0, query_pool_result_buf_size);

    torus_.InitializeData(app(), initialization_buffer);

    torus_descriptor_set_layouts_[0] = {
        0,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    torus_descriptor_set_layouts_[1] = {
        1,                                  // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        1,                                  // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
        nullptr                             // pImmutableSamplers
    };
    torus_descriptor_set_layouts_[2] = {
        2,                                        // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,  // descriptorType
        1,                                        // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,             // stageFlags
        nullptr                                   // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout({{
            torus_descriptor_set_layouts_[0], torus_descriptor_set_layouts_[1],
            torus_descriptor_set_layouts_[2],
        }}));

    VkAttachmentReference depth_attachment = {
        0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentReference color_attachment = {
        1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        data_->allocator(),
        app()->CreateRenderPass(
            {{
                 0,                                 // flags
                 depth_format(),                    // format
                 num_samples(),                     // samples
                 VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp
                 VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
                 VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stencilLoadOp
                 VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stencilStoreOp
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,  // initialLayout
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
                &color_attachment,                // colorAttachment
                nullptr,                          // pResolveAttachments
                &depth_attachment,                // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    torus_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        data_->allocator(),
        app()->CreateGraphicsPipeline(pipeline_layout_.get(),
                                      render_pass_.get(), 0));
    torus_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                               torus_vertex_shader);
    torus_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                               torus_fragment_shader);
    torus_pipeline_->AddDynamicState(VK_DYNAMIC_STATE_LINE_WIDTH);
    torus_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    torus_pipeline_->SetRasterizationFill(VK_POLYGON_MODE_LINE);
    torus_pipeline_->SetCullMode(VK_CULL_MODE_NONE);
    torus_pipeline_->SetInputStreams(&torus_);
    torus_pipeline_->SetViewport(viewport());
    torus_pipeline_->SetScissor(scissor());
    torus_pipeline_->SetSamples(num_samples());
    torus_pipeline_->AddAttachment();
    torus_pipeline_->Commit();

    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data_ = containers::make_unique<vulkan::BufferFrameData<ModelData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect =
        (float)app()->swapchain().width() / (float)app()->swapchain().height();
    camera_data_->data().projection_matrix =
        Mat44::FromScaleVector(Vector3{1.0f, -1.0f, 1.0f}) *
        Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data_->data().transform =
        Mat44::FromTranslationVector(Vector3{0.0, 0.0, -3.0}) *
        Mat44::FromScaleVector(Vector3{0.5, 0.5, 0.5});
  }

  virtual void InitializeFrameData(
      WireframeFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    // Create the buffer view for the query pool result buffer so the buffer
    // can be used in the fragment shader
    VkBufferViewCreateInfo query_pool_results_buf_view_create_info{
        VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *query_pool_results_buf_,                   // buffer
        VK_FORMAT_R32_UINT,                         // format
        vulkan::kMaxOffsetAlignment * frame_index,  // offset
        vulkan::kMaxOffsetAlignment,                // range
    };
    ::VkBufferView raw_buf_view;
    LOG_ASSERT(==, data_->logger(), VK_SUCCESS,
               app()->device()->vkCreateBufferView(
                   app()->device(), &query_pool_results_buf_view_create_info,
                   nullptr, &raw_buf_view));
    frame_data->query_pool_results_buf_view_ =
        containers::make_unique<vulkan::VkBufferView>(
            data_->allocator(),
            vulkan::VkBufferView(raw_buf_view, nullptr, &app()->device()));

    // Buffer memory barriers for the query/descriptor buffer.
    VkBufferMemoryBarrier to_store_query_results{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,    // sType
        nullptr,                                    // pNext
        VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,        // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,               // dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                    // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                    // dstQueueFamilyIndex
        *query_pool_results_buf_,                   // buffer
        vulkan::kMaxOffsetAlignment * frame_index,  // offset
        vulkan::kMaxOffsetAlignment,                // size
    };
    VkBufferMemoryBarrier to_use_query_results{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,    // sType
        nullptr,                                    // pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,               // srcAccessMask
        VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,        // dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                    // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                    // dstQueueFamilyIndex
        *query_pool_results_buf_,                   // buffer
        vulkan::kMaxOffsetAlignment * frame_index,  // offset
        vulkan::kMaxOffsetAlignment,                // size
    };

    // Populate the command buffer
    frame_data->command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());

    frame_data->torus_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet({
                torus_descriptor_set_layouts_[0],
                torus_descriptor_set_layouts_[1],
                torus_descriptor_set_layouts_[2],
            }));

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

    VkWriteDescriptorSet write[2] = {
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
            nullptr,                                 // pNext
            *frame_data->torus_descriptor_set_,      // dstSet
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
            *frame_data->torus_descriptor_set_,       // dstSet
            2,                                        // dstbinding
            0,                                        // dstArrayElement
            1,                                        // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,  // descriptorType
            nullptr,                                  // pImageInfo
            nullptr,                                  // pBufferInfo
            &frame_data->query_pool_results_buf_view_
                 ->get_raw_object()  // pTexelBufferView
        }};

    app()->device()->vkUpdateDescriptorSets(app()->device(), 2, write, 0,
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
    clears[0].depthStencil.depth = 1.0f;
    vulkan::MemoryClear(&clears[1]);

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

    // Reset the query for this frame in the query pool and then begin query.
    LOG_ASSERT(<, data_->logger(), frame_index, query_pool_results_buf_->size());
    cmdBuffer->vkCmdCopyQueryPoolResults(
        cmdBuffer, *query_pool_, static_cast<uint32_t>(frame_index),
        static_cast<uint32_t>(1), *query_pool_results_buf_,
        static_cast<VkDeviceSize>(frame_index * vulkan::kMaxOffsetAlignment),
        static_cast<VkDeviceSize>(sizeof(uint32_t)), VK_QUERY_RESULT_WAIT_BIT);
    cmdBuffer->vkCmdPipelineBarrier(
        cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 1,
        &to_use_query_results, 0, nullptr);
    cmdBuffer->vkCmdResetQueryPool(cmdBuffer, *query_pool_,
                                   static_cast<uint32_t>(frame_index), 1);
    cmdBuffer->vkCmdBeginQuery(cmdBuffer, *query_pool_,
                               static_cast<uint32_t>(frame_index),
                               VkQueryControlFlagBits(0));

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *torus_pipeline_);
    cmdBuffer->vkCmdSetLineWidth(cmdBuffer, 1.0f);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &frame_data->torus_descriptor_set_->raw_set(), 0, nullptr);
    torus_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);

    // End query for this frame and get the result in the query result buffer
    cmdBuffer->vkCmdEndQuery(cmdBuffer, *query_pool_,
                             static_cast<uint32_t>(frame_index));
    cmdBuffer->vkCmdPipelineBarrier(
        cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
        &to_store_query_results, 0, nullptr);

    (*frame_data->command_buffer_)
        ->vkEndCommandBuffer(*frame_data->command_buffer_);
  }

  virtual void Update(float time_since_last_render) override {
    model_data_->data().transform =
        model_data_->data().transform *
        Mat44::FromRotationMatrix(
            Mat44::RotationX(3.14f * time_since_last_render * 0.1f) *
            Mat44::RotationY(3.14f * time_since_last_render * 0.1f));
  }
  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      WireframeFrameData* frame_data) override {
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
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> torus_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  containers::unique_ptr<vulkan::VkQueryPool> query_pool_;
  vulkan::BufferPointer query_pool_results_buf_;
  VkDescriptorSetLayoutBinding torus_descriptor_set_layouts_[3];
  vulkan::VulkanModel torus_;

  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
  containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_data_;

  uint32_t grey_scale_;
  uint32_t num_frames_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  VkPhysicalDeviceFeatures requested_features = {0};
  requested_features.fillModeNonSolid = VK_TRUE;
  CopyQueryPoolResultSample sample(data, requested_features);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
