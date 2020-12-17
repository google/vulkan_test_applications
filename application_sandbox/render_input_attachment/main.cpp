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

#include <array>
#include <chrono>

#include "application_sandbox/sample_application_framework/sample_application.h"
#include "mathfu/matrix.h"
#include "mathfu/vector.h"
#include "support/entry/entry.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

uint32_t populating_attachments_frag[] =
#include "intermediate.frag.spv"
    ;

uint32_t rendering_output_frag[] =
#include "final.frag.spv"
    ;

uint32_t pass_through_vert[] =
#include "passthrough.vert.spv"
    ;

namespace plane_model {
#include "fullscreen_quad.obj.h"
}
const auto& plane_data = plane_model::model;

namespace simple_img {
#include "star.png.h"
}
const auto& src_data = simple_img::texture;

namespace {
// Unpack the source data according to the given formats and copy to the given
// destination.
void populateData(logging::Logger* log, uint8_t* dst, size_t size,
                  VkFormat staging_format, VkFormat target_format) {
  size_t target_pixel_width = 0;
  size_t staging_pixel_width = 0;
  // TODO: Handle more format
  switch (staging_format) {
    case VK_FORMAT_R8G8B8A8_UINT:
      staging_pixel_width = sizeof(uint32_t);
      break;
    default:
      break;
  }
  switch (target_format) {
    case VK_FORMAT_R8G8B8A8_UINT:
      target_pixel_width = sizeof(uint32_t);
      break;
    default:
      break;
  }
  // staging image must have a wider format than the target image to avoid
  // precision lost.
  LOG_ASSERT(!=, log, 0, target_pixel_width);
  LOG_ASSERT(!=, log, 0, staging_pixel_width);
  LOG_ASSERT(>=, log, staging_pixel_width, target_pixel_width);

  size_t count = 0;
  size_t i = 0;
  while (i < size) {
    // The source data is like: R8G8B8A8_UINT
    memcpy(&dst[i], &src_data.data[count++], sizeof(uint32_t));
    i += staging_pixel_width;
  }
}
}  // anonymous namespace

struct RenderInputAttachmentFrameData {
  // transfer destination images are the target of vkCmdCopyBufferToImage, they
  // are populated by copying commands.
  vulkan::ImagePointer trans_dst_img_;
  containers::unique_ptr<vulkan::VkImageView> trans_dst_img_view_;
  // attachment images are used as the input attachment when rendering the
  // swapchain images, at the same time they are populated as rendering target
  // from the staging images.
  vulkan::ImagePointer attachment_img_;
  containers::unique_ptr<vulkan::VkImageView> attachment_img_view_;
  // initial rendering command buffer contains the commands to populate the
  // intermediate attachment images, and should only be submitted once for each
  // swapchain image.
  containers::unique_ptr<vulkan::VkCommandBuffer>
      initial_rendering_command_buffer_;
  // followup command buffer contains the commands to be submitted once the
  // attachment images are populated, and should be submitted at every rendering
  // iteration.
  containers::unique_ptr<vulkan::VkCommandBuffer> followup_command_buffer_;

  containers::unique_ptr<vulkan::VkFramebuffer>
      populating_attachments_framebuffer_;
  containers::unique_ptr<vulkan::VkFramebuffer> rendering_output_framebuffer_;

  containers::unique_ptr<vulkan::DescriptorSet>
      populating_attachments_descriptor_set_;
  containers::unique_ptr<vulkan::DescriptorSet>
      rendering_output_descriptor_set_;

  // render_counter_ records the rendering number of times
  uint64_t render_counter_;
};

// This creates an application with 16MB of image memory, and defaults
// for host, and device buffer sizes.
class RenderQuadSample
    : public sample_application::Sample<RenderInputAttachmentFrameData> {
 public:
  RenderQuadSample(const entry::EntryData* data,
                   const VkPhysicalDeviceFeatures& requested_features)
      : data_(data),
        Sample<RenderInputAttachmentFrameData>(
            data->allocator(), data, 10, 512, 10, 1,
            sample_application::SampleOptions(), requested_features),
        plane_(data->allocator(), data->logger(), plane_data) {}
  virtual void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {
    plane_.InitializeData(app(), initialization_buffer);

    // 'Populating attachment' and 'rendering output' shares the
    // same descriptor sets layout binding, pipeline layout and most of the
    // attachment references.
    descriptor_set_layout_binding_ = {
        0,                                    // binding
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,  // desriptorType
        1,                                    // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,         // stageFlags
        nullptr                               // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        data_->allocator(),
        app()->CreatePipelineLayout({{descriptor_set_layout_binding_}}));

    VkAttachmentReference color_attachment = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference input_attachment = {
        1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    // Create the renderpass and the pipeline for the final rendering output
    // phase.
    auto o_c_att_desc = VkAttachmentDescription{
        0,                                        // flags
        render_format(),                          // format
        num_samples(),                            // samples
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stenilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stenilStoreOp
        VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL  // finalLayout
    };
    auto i_c_att_desc = VkAttachmentDescription{
        0,                                         // flags
        VK_FORMAT_R8G8B8A8_UINT,                   // format
        VK_SAMPLE_COUNT_1_BIT,                     // samples
        VK_ATTACHMENT_LOAD_OP_LOAD,                // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,              // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stenilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stenilStoreOp
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // initialLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL   // finalLayout
    };
    auto subpass_desc = VkSubpassDescription{
        0,                                // flags
        VK_PIPELINE_BIND_POINT_GRAPHICS,  // pipelineBindPoint
        1,                                // inputAttachmentCount
        &input_attachment,                // pInputAttachments
        1,                                // colorAttachmentCount
        &color_attachment,                // colorAttachment
        nullptr,                          // pResolveAttachments
        nullptr,                          // pDepthStencilAttachment
        0,                                // preserveAttachmentCount
        nullptr                           // pPreserveAttachments
    };

    rendering_output_render_pass_ =
        containers::make_unique<vulkan::VkRenderPass>(
            data_->allocator(),
            app()->CreateRenderPass(
                {
                    o_c_att_desc,  // Color Attachment
                    i_c_att_desc,  // Color Input Attachment
                },                 // AttachmentDescriptions
                {
                    subpass_desc,
                },  // SubpassDescriptions
                {}  // SubpassDependencies
                ));

    rendering_output_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->allocator(), app()->CreateGraphicsPipeline(
                                    pipeline_layout_.get(),
                                    rendering_output_render_pass_.get(), 0));
    rendering_output_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                          pass_through_vert);
    rendering_output_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                          rendering_output_frag);
    rendering_output_pipeline_->SetTopology(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    rendering_output_pipeline_->SetInputStreams(&plane_);
    rendering_output_pipeline_->SetScissor(scissor());
    rendering_output_pipeline_->SetViewport(viewport());
    rendering_output_pipeline_->SetSamples(num_samples());
    rendering_output_pipeline_->AddAttachment();
    rendering_output_pipeline_->Commit();

    // Create the renderpass for populating the attachment images.
    o_c_att_desc.format = VK_FORMAT_R8G8B8A8_UINT;
    o_c_att_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    i_c_att_desc.format = VK_FORMAT_R8G8B8A8_UINT;
    i_c_att_desc.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    i_c_att_desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    input_attachment.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    populating_attachments_render_pass_ =
        containers::make_unique<vulkan::VkRenderPass>(
            data_->allocator(),
            app()->CreateRenderPass(
                {
                    o_c_att_desc,  // Color Attachment
                    i_c_att_desc,  // Color Input Attachment
                },                 // AttachmentDescriptions
                {
                    subpass_desc,
                },  // SubpassDescriptions
                {}  // SubpassDependencies
                ));
    populating_attachments_pipeline_ =
        containers::make_unique<vulkan::VulkanGraphicsPipeline>(
            data_->allocator(),
            app()->CreateGraphicsPipeline(
                pipeline_layout_.get(),
                populating_attachments_render_pass_.get(), 0));
    populating_attachments_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT,
                                                "main", pass_through_vert);
    populating_attachments_pipeline_->AddShader(
        VK_SHADER_STAGE_FRAGMENT_BIT, "main", populating_attachments_frag);
    populating_attachments_pipeline_->SetTopology(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    populating_attachments_pipeline_->SetInputStreams(&plane_);
    populating_attachments_pipeline_->SetScissor(scissor());
    populating_attachments_pipeline_->SetViewport(viewport());
    populating_attachments_pipeline_->SetSamples(VK_SAMPLE_COUNT_1_BIT);
    populating_attachments_pipeline_->AddAttachment();
    populating_attachments_pipeline_->Commit();

    color_data_ = containers::make_unique<vulkan::BufferFrameData<ColorData>>(
        data_->allocator(), app(), num_swapchain_images,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    populateData(data_->logger(), color_data_->data().data.data(),
                 color_data_->data().data.size(), VK_FORMAT_R8G8B8A8_UINT,
                 VK_FORMAT_R8G8B8A8_UINT);
  }

  virtual void InitializeFrameData(
      RenderInputAttachmentFrameData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {
    frame_data->initial_rendering_command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());
    frame_data->followup_command_buffer_ =
        containers::make_unique<vulkan::VkCommandBuffer>(
            data_->allocator(), app()->GetCommandBuffer());
    frame_data->render_counter_ = 0;

    // Create the transfer-destination/attachment images
    VkImageCreateInfo img_info{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
        nullptr,                              // pNext
        0,                                    // flags
        VK_IMAGE_TYPE_2D,                     // imageType
        VK_FORMAT_R8G8B8A8_UINT,              // format
        {
            app()->swapchain().width(),   // width
            app()->swapchain().height(),  // height
            app()->swapchain().depth()    // depth
        },
        1,                        // mipLevels
        1,                        // arrayLayers
        VK_SAMPLE_COUNT_1_BIT,    // samples
        VK_IMAGE_TILING_OPTIMAL,  // tiling
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,                // sharingMode
        0,                                        // queueFamilyIndexCount
        nullptr,                                  // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
    };
    frame_data->trans_dst_img_ = app()->CreateAndBindImage(&img_info);
    img_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                     VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    frame_data->attachment_img_ = app()->CreateAndBindImage(&img_info);

    // Create image views
    // Image view of transfer-dest image
    VkImageViewCreateInfo view_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        *frame_data->trans_dst_img_,               // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        frame_data->trans_dst_img_->format(),      // format
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
         VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    ::VkImageView raw_trans_dst_view;
    LOG_ASSERT(==, data_->logger(), VK_SUCCESS,
               app()->device()->vkCreateImageView(
                   app()->device(), &view_info, nullptr, &raw_trans_dst_view));
    frame_data->trans_dst_img_view_ =
        containers::make_unique<vulkan::VkImageView>(
            data_->allocator(),
            vulkan::VkImageView(raw_trans_dst_view, nullptr, &app()->device()));
    // Image view for the color attachment image
    view_info.image = *frame_data->attachment_img_;
    view_info.format = frame_data->attachment_img_->format();
    ::VkImageView raw_att_view;
    LOG_ASSERT(==, data_->logger(), VK_SUCCESS,
               app()->device()->vkCreateImageView(app()->device(), &view_info,
                                                  nullptr, &raw_att_view));
    frame_data->attachment_img_view_ =
        containers::make_unique<vulkan::VkImageView>(
            data_->allocator(),
            vulkan::VkImageView(raw_att_view, nullptr, &app()->device()));

    // Create a framebuffer for populating the attachment images
    VkImageView views[2] = {
        *frame_data->attachment_img_view_,
        *frame_data->trans_dst_img_view_,
    };
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *populating_attachments_render_pass_,       // renderPass
        2,                                          // attachmentCount
        views,                                      // attachments
        app()->swapchain().width(),                 // width
        app()->swapchain().height(),                // height
        1                                           // layers
    };
    ::VkFramebuffer raw_populating_attachments_framebuffer;
    app()->device()->vkCreateFramebuffer(
        app()->device(), &framebuffer_create_info, nullptr,
        &raw_populating_attachments_framebuffer);
    frame_data->populating_attachments_framebuffer_ =
        containers::make_unique<vulkan::VkFramebuffer>(
            data_->allocator(),
            vulkan::VkFramebuffer(raw_populating_attachments_framebuffer,
                                  nullptr, &app()->device()));

    // Create a framebuffer for rendering
    views[0] = color_view(frame_data);
    views[1] = *frame_data->attachment_img_view_;
    framebuffer_create_info.renderPass = *rendering_output_render_pass_;
    ::VkFramebuffer raw_rendering_output_framebuffer;
    app()->device()->vkCreateFramebuffer(app()->device(),
                                         &framebuffer_create_info, nullptr,
                                         &raw_rendering_output_framebuffer);
    frame_data->rendering_output_framebuffer_ =
        containers::make_unique<vulkan::VkFramebuffer>(
            data_->allocator(),
            vulkan::VkFramebuffer(raw_rendering_output_framebuffer, nullptr,
                                  &app()->device()));

    // Update the descriptors used for populating attachment images
    frame_data->populating_attachments_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet({descriptor_set_layout_binding_}));
    VkDescriptorImageInfo input_attachment_info = {
        VK_NULL_HANDLE,                            // sampler
        *frame_data->trans_dst_img_view_,          // imageView
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // imageLayout
    };
    VkWriteDescriptorSet write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,               // sType
        nullptr,                                              // pNext
        *frame_data->populating_attachments_descriptor_set_,  // dstSet
        0,                                                    // dstBinding
        0,                                                    // dstArrayElement
        1,                                                    // descriptorCount
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,                  // descriptorType
        &input_attachment_info,                               // pImageInfo
        nullptr,                                              // pBufferInfo
        nullptr,  // pTexelBufferView
    };
    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                            nullptr);

    // Update the descripotors used for rendering output
    frame_data->rendering_output_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            data_->allocator(),
            app()->AllocateDescriptorSet({descriptor_set_layout_binding_}));
    input_attachment_info.imageView = *frame_data->attachment_img_view_;
    write.dstSet = *frame_data->rendering_output_descriptor_set_;
    app()->device()->vkUpdateDescriptorSets(app()->device(), 1, &write, 0,
                                            nullptr);

    // Recording the attachment populating commands: 1) copy data to trans_dst
    // images, 2) render trans_dst image to color attachment image.
    (*frame_data->initial_rendering_command_buffer_)
        ->vkBeginCommandBuffer(*frame_data->initial_rendering_command_buffer_,
                               &sample_application::kBeginCommandBuffer);
    // Copy data from source buffer to the trans_dst image
    // Buffer barriers to src
    VkBufferMemoryBarrier color_data_to_src{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,         // sType
        nullptr,                                         // pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,                    // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,                     // dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                         // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                         // dstQueueFamilyIndex
        color_data_->get_buffer(),                       // buffer
        color_data_->get_offset_for_frame(frame_index),  // offset
        color_data_->size(),                             // size
    };

    // Image barriers to dst
    VkImageMemoryBarrier trans_dst_img_undef_to_dst{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
        nullptr,                                 // pNext
        0,                                       // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,            // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,               // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,    // newLayout
        VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
        *frame_data->trans_dst_img_,             // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    (*frame_data->initial_rendering_command_buffer_)
        ->vkCmdPipelineBarrier(
            *frame_data->initial_rendering_command_buffer_,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                VK_PIPELINE_STAGE_HOST_BIT |
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                VK_PIPELINE_STAGE_TRANSFER_BIT,  // srcStageMask
            VK_PIPELINE_STAGE_TRANSFER_BIT,      // dstStageMask
            0,                                   // dependencyFlags
            0,                                   // memoryBarrierCount
            nullptr,                             // memoryBarrierCount
            1,                                   // bufferMemoryBarrierCount
            &color_data_to_src,                  // pBufferMemoryBarriers
            1,                                   // imageMemoryBarrierCount
            &trans_dst_img_undef_to_dst          // pImageMemoryBarriers
        );
    // copy from buf to img. The swapchain image must be larger in both
    // dimensions.
    LOG_ASSERT(>=, data_->logger(), app()->swapchain().width(), src_data.width);
    LOG_ASSERT(>=, data_->logger(), app()->swapchain().height(),
               src_data.height);
    uint32_t copy_width = src_data.width;
    uint32_t copy_height = src_data.height;
    VkBufferImageCopy copy_region{
        color_data_->get_offset_for_frame(frame_index),
        0,
        0,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {0, 0, 0},
        {copy_width, copy_height, 1},
    };
    (*frame_data->initial_rendering_command_buffer_)
        ->vkCmdCopyBufferToImage(
            *frame_data->initial_rendering_command_buffer_,
            color_data_->get_buffer(), *frame_data->trans_dst_img_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
    // trans_dst image layouts from dst to shader read
    VkImageMemoryBarrier trans_dst_img_to_shader_read{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // sType
        nullptr,                                   // pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,              // srcAccessMask
        VK_ACCESS_SHADER_READ_BIT,                 // dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,      // oldLayout
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // newLayout
        VK_QUEUE_FAMILY_IGNORED,                   // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                   // srcQueueFamilyIndex
        *frame_data->trans_dst_img_,               // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    (*frame_data->initial_rendering_command_buffer_)
        ->vkCmdPipelineBarrier(
            *frame_data->initial_rendering_command_buffer_,
            VK_PIPELINE_STAGE_TRANSFER_BIT,         // srcStageMask
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,  // dstStageMask
            0,                                      // dependencyFlags
            0,                                      // memoryBarrierCount
            nullptr,                                // memoryBarrierCount
            0,                                      // bufferMemoryBarrierCount
            nullptr,                                // pBufferMemoryBarriers
            1,                                      // imageMemoryBarrierCount
            &trans_dst_img_to_shader_read           // pImageMemoryBarriers
        );

    // Renders the content in the color/depth trans_dst images to the
    // color/depth attachment images.
    VkRenderPassBeginInfo begin_populating_attachments_renderpass = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,          // sType
        nullptr,                                           // pNext
        *populating_attachments_render_pass_,              // renderPass
        *frame_data->populating_attachments_framebuffer_,  // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        0,                                // clearValueCount
        nullptr                           // clears
    };

    (*frame_data->initial_rendering_command_buffer_)
        ->vkCmdBeginRenderPass(*frame_data->initial_rendering_command_buffer_,
                               &begin_populating_attachments_renderpass,
                               VK_SUBPASS_CONTENTS_INLINE);
    (*frame_data->initial_rendering_command_buffer_)
        ->vkCmdBindPipeline(*frame_data->initial_rendering_command_buffer_,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            *populating_attachments_pipeline_);
    (*frame_data->initial_rendering_command_buffer_)
        ->vkCmdBindDescriptorSets(
            *frame_data->initial_rendering_command_buffer_,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            ::VkPipelineLayout(*pipeline_layout_), 0, 1,
            &frame_data->populating_attachments_descriptor_set_->raw_set(), 0,
            nullptr);
    plane_.Draw(frame_data->initial_rendering_command_buffer_.get());
    (*frame_data->initial_rendering_command_buffer_)
        ->vkCmdEndRenderPass(*frame_data->initial_rendering_command_buffer_);

    (*frame_data->initial_rendering_command_buffer_)
        ->vkEndCommandBuffer(*frame_data->initial_rendering_command_buffer_);

    // Recording the output rendering command buffer
    (*frame_data->followup_command_buffer_)
        ->vkBeginCommandBuffer(*frame_data->followup_command_buffer_,
                               &sample_application::kBeginCommandBuffer);
    (*frame_data->followup_command_buffer_)
        ->vkCmdPipelineBarrier(*frame_data->followup_command_buffer_,
                               VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                               VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0,
                               nullptr, 0, nullptr, 0, nullptr);
    VkRenderPassBeginInfo begin_rendering_output_renderpass{
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,    // sType
        nullptr,                                     // pNext
        *rendering_output_render_pass_,              // renderpass
        *frame_data->rendering_output_framebuffer_,  // framebuffer
        {{0, 0},
         {app()->swapchain().width(),
          app()->swapchain().height()}},  // renderArea
        0,                                // clearValueCount
        nullptr                           // clears
    };
    (*frame_data->followup_command_buffer_)
        ->vkCmdBeginRenderPass(*frame_data->followup_command_buffer_,
                               &begin_rendering_output_renderpass,
                               VK_SUBPASS_CONTENTS_INLINE);
    (*frame_data->followup_command_buffer_)
        ->vkCmdBindPipeline(*frame_data->followup_command_buffer_,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            *rendering_output_pipeline_);
    (*frame_data->followup_command_buffer_)
        ->vkCmdBindDescriptorSets(
            *frame_data->followup_command_buffer_,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            ::VkPipelineLayout(*pipeline_layout_), 0, 1,
            &frame_data->rendering_output_descriptor_set_->raw_set(), 0,
            nullptr);
    plane_.Draw(frame_data->followup_command_buffer_.get());

    (*frame_data->followup_command_buffer_)
        ->vkCmdEndRenderPass(*frame_data->followup_command_buffer_);
    (*frame_data->followup_command_buffer_)
        ->vkEndCommandBuffer(*frame_data->followup_command_buffer_);
  }

  virtual void Update(float time_since_last_render) override {
    // Do not update any data in this sample.
  }

  virtual void Render(vulkan::VkQueue* queue, size_t frame_index,
                      RenderInputAttachmentFrameData* frame_data) override {
    if (frame_data->render_counter_ == 0) {
      color_data_->UpdateBuffer(queue, frame_index);
      ::VkCommandBuffer cmd_bufs[2] = {
          frame_data->initial_rendering_command_buffer_->get_command_buffer(),
          frame_data->followup_command_buffer_->get_command_buffer()};
      VkSubmitInfo init_submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                    nullptr,
                                    0,
                                    nullptr,
                                    nullptr,
                                    2,
                                    cmd_bufs,
                                    0,
                                    nullptr};
      app()->render_queue()->vkQueueSubmit(
          app()->render_queue(), 1, &init_submit_info,
          static_cast<VkFence>(VK_NULL_HANDLE));
    } else {
      VkSubmitInfo init_submit_info{
          VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
          nullptr,                        // pNext
          0,                              // waitSemaphoreCount
          nullptr,                        // pWaitSemaphores
          nullptr,                        // pWaitDstStageMask,
          1,                              // commandBufferCount
          &frame_data->followup_command_buffer_->get_command_buffer(),
          0,       // signalSemaphoreCount
          nullptr  // pSignalSemaphores
      };
      app()->render_queue()->vkQueueSubmit(
          app()->render_queue(), 1, &init_submit_info,
          static_cast<VkFence>(VK_NULL_HANDLE));
    }
    frame_data->render_counter_++;
  }

 private:
  struct ColorData {
    std::array<uint8_t, sizeof(src_data.data)> data;
  };

  const entry::EntryData* data_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline>
      populating_attachments_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline>
      rendering_output_pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass>
      populating_attachments_render_pass_;
  containers::unique_ptr<vulkan::VkRenderPass> rendering_output_render_pass_;
  VkDescriptorSetLayoutBinding descriptor_set_layout_binding_;

  containers::unique_ptr<vulkan::BufferFrameData<ColorData>> color_data_;

  vulkan::VulkanModel plane_;
};

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");
  VkPhysicalDeviceFeatures requested_features = {0};
  RenderQuadSample sample(data, requested_features);
  sample.Initialize();

  while (!sample.should_exit() && !data->WindowClosing()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
