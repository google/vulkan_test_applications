/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "support/entry/entry.h"
#include "support/log/log.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/known_device_infos.h"
#include "vulkan_helpers/structs.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/library_wrapper.h"

uint32_t render_frag_shader[] =
#include "simple_fragment.frag.spv"
    ;

uint32_t render_vert_shader[] =
#include "simple_vertex.vert.spv"
    ;

uint32_t input_attachment_vert_shader[] =
#include "hardcode_full_screen_quad.vert.spv"
    ;

uint32_t input_attachment_frag_shader[] =
#include "only_red_channel.frag.spv"
    ;

namespace {
// Geometry data of the triangle to be drawn
const float kVertices[] = {
    -0.5, -0.5, 0.5,  // point 1
    0.0,  0.5,  1.0,  // point 2
    0.5,  -0.5, 0.0,  // point 3
};
const vulkan::VulkanGraphicsPipeline::InputStream kVerticesStream{
    0,                           // binding
    VK_FORMAT_R32G32B32_SFLOAT,  // format
    0,                           // offset
};

const float kUV[] = {
    0.0, 0.0,  // point 1
    1.0, 0.0,  // point 2
    0.0, 1.0,  // point 3
};
const vulkan::VulkanGraphicsPipeline::InputStream kUVStream{
    1,                        // binding
    VK_FORMAT_R32G32_SFLOAT,  // format
    0,                        // offset
};

const VkCommandBufferBeginInfo kCommandBufferBeginInfo{
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
    VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr,
};

const VkSampleCountFlagBits kSampleCountBit = VK_SAMPLE_COUNT_1_BIT;

// A helper function that change the layout of the given image.
void EnqueueImageLayoutTransition(
    ::VkImage img, VkImageAspectFlags aspect, VkImageLayout old_layout,
    VkImageLayout new_layout, VkAccessFlags src_access_mask,
    VkAccessFlags dst_access_mask, VkPipelineStageFlags src_stage,
    VkPipelineStageFlags dst_stage, vulkan::VkCommandBuffer* cmd_buf_ptr) {
  if (!cmd_buf_ptr) return;
  auto& cmd_buf = *cmd_buf_ptr;
  VkImageMemoryBarrier barrier = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
      nullptr,                                 // pNext
      src_access_mask,                         // srcAccessMask
      dst_access_mask,                         // dstAccessMask
      old_layout,                              // oldLayout
      new_layout,                              // newLayout
      VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,                 // dstQueueFamilyIndex
      img,                                     // image
      {aspect, 0, 1, 0, 1}};
  cmd_buf->vkCmdPipelineBarrier(cmd_buf, src_stage, dst_stage, 0, 0, nullptr, 0,
                                nullptr, 1, &barrier);
}
}  // anonymous namespace

int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data, {},
                                {}, 1024 * 128, 1024 * 1024 * 1024);
  vulkan::VkDevice& device = app.device();

  auto fence = vulkan::CreateFence(&app.device(), false);
  uint32_t image_index = 0;
  app.device()->vkAcquireNextImageKHR(app.device(), app.swapchain(),
                                      0xFFFFFFFFFFFFFFFF, VkSemaphore(0), fence,
                                      &image_index);
  app.device()->vkWaitForFences(app.device(), 1, &fence.get_raw_object(),
                                VK_TRUE, 0xFFFFFFFFFFFFFFFF);

  // Create vertex buffers and index buffer
  auto vertices_buf = app.CreateAndBindDefaultExclusiveHostBuffer(
      sizeof(kVertices),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  auto uv_buf = app.CreateAndBindDefaultExclusiveHostBuffer(
      sizeof(kUV),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  const ::VkBuffer vertex_buffers[2] = {*vertices_buf, *uv_buf};
  const ::VkDeviceSize vertex_buffer_offsets[2] = {0, 0};

  // Create subpasses and render pass.
  VkAttachmentReference swapchain_image_attachment = {
      0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference write_intermediate_image_attachment = {
      1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference read_intermediate_image_attachment = {
      1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  vulkan::VkRenderPass render_pass = app.CreateRenderPass(
      {
          {
              0,                                 // flags
              app.swapchain().format(),          // format
              kSampleCountBit,                   // samples
              VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp
              VK_ATTACHMENT_STORE_OP_STORE,      // storeOp
              VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stenilLoadOp
              VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stenilStoreOp
              VK_IMAGE_LAYOUT_UNDEFINED,         // initialLayout
              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR    // finalLayout
          },
          {
              0,                                        // flags
              app.swapchain().format(),                 // format
              kSampleCountBit,                          // samples
              VK_ATTACHMENT_LOAD_OP_CLEAR,              // loadOp
              VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
              VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stenilLoadOp
              VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stenilStoreOp
              VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL  // finalLayout
          },
      },  // AttachmentDescriptions
      {
          {
              0,                                     // flags
              VK_PIPELINE_BIND_POINT_GRAPHICS,       // pipelineBindPoint
              0,                                     // inputAttachmentCount
              nullptr,                               // pInputAttachments
              1,                                     // colorAttachmentCount
              &write_intermediate_image_attachment,  // colorAttachment
              nullptr,                               // pResolveAttachments
              nullptr,                               // pDepthStencilAttachment
              0,                                     // preserveAttachmentCount
              nullptr                                // pPreserveAttachments
          },
          {
              0,                                    // flags
              VK_PIPELINE_BIND_POINT_GRAPHICS,      // pipelineBindPoint
              1,                                    // inputAttachmentCount
              &read_intermediate_image_attachment,  // pInputAttachments
              1,                                    // colorAttachmentCount
              &swapchain_image_attachment,          // colorAttachment
              nullptr,                              // pResolveAttachments
              nullptr,                              // pDepthStencilAttachment
              0,                                    // preserveAttachmentCount
              nullptr                               // pPreserveAttachments
          },
      },
      {
          {
              0,                                              // srcSubpass
              1,                                              // dstSubpass
              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,          // dstStageMask
              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // srcAccessMask
              VK_ACCESS_SHADER_READ_BIT,                      // dstAccessMask
              0,                                              // flags
          },
          {
              1,                                              // srcSubpass
              VK_SUBPASS_EXTERNAL,                            // dstSubpass
              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
              VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,           // dstStageMask
              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // srcAccessMask
              VK_ACCESS_MEMORY_READ_BIT,                      // dstAccessMask
              0,                                              // flags
          },
      }  // SubpassDependencies
      );

  // Create pipeline for the first subpass
  vulkan::PipelineLayout first_subpass_pipeline_layout(
      app.CreatePipelineLayout({{}}));
  vulkan::VulkanGraphicsPipeline first_subpass_pipeline =
      app.CreateGraphicsPipeline(&first_subpass_pipeline_layout, &render_pass,
                                 0);
  first_subpass_pipeline.AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                   render_vert_shader);
  first_subpass_pipeline.AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                   render_frag_shader);
  first_subpass_pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  first_subpass_pipeline.AddInputStream(
      sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX, {kVerticesStream});
  first_subpass_pipeline.AddInputStream(
      sizeof(float) * 2, VK_VERTEX_INPUT_RATE_VERTEX, {kUVStream});
  first_subpass_pipeline.SetScissor(
      {{
           0,  // offset.x
           0,  // offset.y
       },
       {
           app.swapchain().width(),  // extent.width
           app.swapchain().height()  // extent.height
       }});
  first_subpass_pipeline.SetViewport({
      0.0f,                                          // x
      0.0f,                                          // y
      static_cast<float>(app.swapchain().width()),   // width
      static_cast<float>(app.swapchain().height()),  // height
      0.0f,                                          // minDepth
      1.0f,                                          // maxDepth
  });
  first_subpass_pipeline.SetSamples(kSampleCountBit);
  first_subpass_pipeline.AddAttachment();
  first_subpass_pipeline.Commit();

  // Create pipeline for the second subpass
  VkDescriptorSetLayoutBinding second_subpass_descriptor_set_binding{
      0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr};
  vulkan::PipelineLayout second_subpass_pipeline_layout(
      app.CreatePipelineLayout({{second_subpass_descriptor_set_binding}}));
  vulkan::VulkanGraphicsPipeline second_subpass_pipeline =
      app.CreateGraphicsPipeline(&second_subpass_pipeline_layout, &render_pass,
                                 1);
  second_subpass_pipeline.AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                                    input_attachment_vert_shader);
  second_subpass_pipeline.AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                                    input_attachment_frag_shader);
  second_subpass_pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  second_subpass_pipeline.SetScissor(
      {{
           0,  // offset.x
           0,  // offset.y
       },
       {
           app.swapchain().width(),  // extent.width
           app.swapchain().height()  // extent.height
       }});
  second_subpass_pipeline.SetViewport({
      0.0f,                                          // x
      0.0f,                                          // y
      static_cast<float>(app.swapchain().width()),   // width
      static_cast<float>(app.swapchain().height()),  // height
      0.0f,                                          // minDepth
      1.0f,                                          // maxDepth
  });
  second_subpass_pipeline.SetSamples(kSampleCountBit);
  second_subpass_pipeline.AddAttachment();
  second_subpass_pipeline.Commit();

  // Create image view for the swapchain image
  VkImageViewCreateInfo swapchain_image_view_create_info{
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
      nullptr,                                   // pNext
      0,                                         // flags
      app.swapchain_images()[image_index],       // image
      VK_IMAGE_VIEW_TYPE_2D,                     // viewType
      app.swapchain().format(),                  // format
      {
          VK_COMPONENT_SWIZZLE_IDENTITY,  // components.r
          VK_COMPONENT_SWIZZLE_IDENTITY,  // components.g
          VK_COMPONENT_SWIZZLE_IDENTITY,  // components.b
          VK_COMPONENT_SWIZZLE_IDENTITY,  // components.a
      },
      {
          VK_IMAGE_ASPECT_COLOR_BIT,  // subresourceRange.aspectMask
          0,                          // subresourceRange.baseMipLevel
          1,                          // subresourceRange.levelCount
          0,                          // subresourceRange.baseArrayLayer
          1,                          // subresourceRange.layerCount
      },
  };
  ::VkImageView raw_swapchain_image_view;
  LOG_ASSERT(==, data->logger(), app.device()->vkCreateImageView(
                                app.device(), &swapchain_image_view_create_info,
                                nullptr, &raw_swapchain_image_view),
             VK_SUCCESS);
  vulkan::VkImageView swapchain_image_view(raw_swapchain_image_view, nullptr,
                                           &app.device());

  // Create image and view for the input attachment image
  VkImageCreateInfo intermediate_image_create_info{
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      nullptr,
      0,
      VK_IMAGE_TYPE_2D,
      app.swapchain().format(),
      {app.swapchain().width(), app.swapchain().height(),
       app.swapchain().depth()},
      1,
      1,
      kSampleCountBit,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr,
      VK_IMAGE_LAYOUT_UNDEFINED};
  vulkan::ImagePointer intermediate_image =
      app.CreateAndBindImage(&intermediate_image_create_info);
  VkImageViewCreateInfo intermediate_image_view_create_info =
      swapchain_image_view_create_info;
  intermediate_image_view_create_info.image = *intermediate_image;
  ::VkImageView raw_intermediate_image_view;
  LOG_ASSERT(==, data->logger(),
             app.device()->vkCreateImageView(
                 app.device(), &intermediate_image_view_create_info, nullptr,
                 &raw_intermediate_image_view),
             VK_SUCCESS);
  vulkan::VkImageView intermediate_image_view(raw_intermediate_image_view,
                                              nullptr, &app.device());

  // Create framebuffer
  ::VkImageView attachments[2] = {raw_swapchain_image_view,
                                  raw_intermediate_image_view};
  VkFramebufferCreateInfo framebuffer_create_info{
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
      nullptr,                                    // pNext
      0,                                          // flags
      render_pass,                                // renderPass
      2,                                          // attachmentCount
      attachments,                                // attachments
      app.swapchain().width(),                    // width
      app.swapchain().height(),                   // height
      1                                           // layers
  };
  ::VkFramebuffer raw_framebuffer;
  app.device()->vkCreateFramebuffer(app.device(), &framebuffer_create_info,
                                    nullptr, &raw_framebuffer);
  vulkan::VkFramebuffer framebuffer(raw_framebuffer, nullptr, &app.device());

  // Create render pass begin info
  VkClearValue clear_values[4]{{0}, {0}, {0}, {0}};
  VkRenderPassBeginInfo pass_begin{
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
      nullptr,                                   // pNext
      render_pass,                               // renderPass
      framebuffer,                               // framebuffer
      {{0, 0},
       {app.swapchain().width(), app.swapchain().height()}},  // renderArea
      2,                                                      // clearValueCount
      clear_values                                            // clears
  };

  // Create descriptor for reading input attachment
  vulkan::DescriptorSet intermediate_image_descriptor_set =
      app.AllocateDescriptorSet({second_subpass_descriptor_set_binding});
  VkDescriptorImageInfo intermediate_image_info{
      VK_NULL_HANDLE, raw_intermediate_image_view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  VkWriteDescriptorSet write_descriptor_set{
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
      nullptr,                                 // pNext
      intermediate_image_descriptor_set,       // dstSet
      0,                                       // dstBinding
      0,                                       // dstArrayElement
      1,                                       // descriptorType
      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,     // descriptorType
      &intermediate_image_info,                // pImageInfo
      nullptr,                                 // pBufferInfo
      nullptr,                                 // pTexelBufferView
  };
  app.device()->vkUpdateDescriptorSets(app.device(), 1, &write_descriptor_set,
                                       0, nullptr);

  // Draw
  auto cmd_buf = app.GetCommandBuffer();
  cmd_buf->vkBeginCommandBuffer(cmd_buf, &kCommandBufferBeginInfo);
  app.FillHostVisibleBuffer(
      &*vertices_buf, reinterpret_cast<const char*>(kVertices),
      sizeof(kVertices), 0, &cmd_buf, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
  app.FillHostVisibleBuffer(
      &*uv_buf, reinterpret_cast<const char*>(kUV), sizeof(kUV), 0, &cmd_buf,
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);

  // First subpass
  cmd_buf->vkCmdBeginRenderPass(cmd_buf, &pass_begin,
                                VK_SUBPASS_CONTENTS_INLINE);
  cmd_buf->vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                             first_subpass_pipeline);
  cmd_buf->vkCmdBindVertexBuffers(cmd_buf, 0, 2, vertex_buffers,
                                  vertex_buffer_offsets);
  cmd_buf->vkCmdDraw(cmd_buf, 3, 1, 0, 0);

  // Second subpass
  cmd_buf->vkCmdNextSubpass(cmd_buf, VK_SUBPASS_CONTENTS_INLINE);
  cmd_buf->vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                             second_subpass_pipeline);
  cmd_buf->vkCmdBindDescriptorSets(
      cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, second_subpass_pipeline_layout,
      0, 1, &intermediate_image_descriptor_set.raw_set(), 0, nullptr);
  cmd_buf->vkCmdDraw(cmd_buf, 6, 1, 0, 0);

  cmd_buf->vkCmdEndRenderPass(cmd_buf);

  app.EndAndSubmitCommandBufferAndWaitForQueueIdle(&cmd_buf, &app.render_queue());

  VkPresentInfoKHR present_info{
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr,      0,      nullptr, 1,
      &app.swapchain().get_raw_object(),  &image_index, nullptr};
  app.present_queue()->vkQueuePresentKHR(app.present_queue(), &present_info);
  app.device()->vkDeviceWaitIdle(app.device());

  data->logger()->LogInfo("Application Shutdown");
  return 0;
}
