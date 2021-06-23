#include "render_quad.h"

namespace quad_model {
#include "fullscreen_quad.obj.h"
}

namespace {
    uint32_t quad_vertex_shader[] =
    #include "quad.vert.spv"
        ;

    uint32_t quad_fragment_shader[] =
    #include "quad.frag.spv"
        ;
    const auto& quad_data = quad_model::model;
}

RenderQuad::RenderQuad(const entry::EntryData* data) 
    : quad_(data->allocator(), data->logger(), quad_data) {}

void RenderQuad::InitializeQuadData(vulkan::VulkanApplication* app,
    containers::Allocator* allocator,
    QuadVulkanInfo vulkanInfo,
    vulkan::VkCommandBuffer* initialization_buffer,
    size_t num_swapchain_images) {
    quad_.InitializeData(app, initialization_buffer);

    descriptor_set_layout_binding_ = {
        0,                                    // binding
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,  // desriptorType
        1,                                    // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,         // stageFlags
        nullptr                               // pImmutableSamplers
    };

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        allocator, app->CreatePipelineLayout({{descriptor_set_layout_binding_}}));

    VkAttachmentReference input_attachment = {
        0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkAttachmentReference color_attachment = {
        1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        allocator,
        app->CreateRenderPass(
            {
                {
                    0,                                         // flags
                    VK_FORMAT_R8G8B8A8_UINT,                   // format
                    vulkanInfo.num_samples,                    // samples
                    VK_ATTACHMENT_LOAD_OP_LOAD,                // loadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stencilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stencilStoreOp
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // initialLayout
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL   // finalLayout
                },  // Input Attachment
                {
                    0,                                        // flags
                    vulkanInfo.colorFormat,                   // format
                    vulkanInfo.num_samples,                   // samples
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // loadOp
                    VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stencilLoadOp
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stencilStoreOp
                    VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL  // finalLayout
                },  // Color Attachment
            },      // AttachmentDescriptions
            {{
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
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        allocator,
        app->CreateGraphicsPipeline(pipeline_layout_.get(), render_pass_.get(), 0));
    pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",quad_vertex_shader);
    pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", quad_fragment_shader);
    pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline_->SetInputStreams(&quad_);
    pipeline_->SetViewport(vulkanInfo.viewport);
    pipeline_->SetScissor(vulkanInfo.scissor);
    pipeline_->SetSamples(vulkanInfo.num_samples);
    pipeline_->AddAttachment();

    pipeline_->Commit();
}

void RenderQuad::InitializeFrameData(vulkan::VulkanApplication* app,
    RenderQuadData* renderData,
    containers::Allocator* allocator,
    const VkImageView& inputView,
    const VkImageView& colorView,
    size_t frame_index) {

    // Create a framebuffer for rendering
    VkImageView views[2] = {
        inputView,
        colorView,
    };

    // Create a framebuffer with attachments
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_pass_,                              // renderPass
        2,                                          // attachmentCount
        views,                                      // attachments
        app->swapchain().width(),                   // width
        app->swapchain().height(),                  // height
        1                                           // layers
    };

    ::VkFramebuffer raw_framebuffer;
    app->device()->vkCreateFramebuffer(
        app->device(), &framebuffer_create_info, nullptr, &raw_framebuffer);
    renderData->framebuffer_ = containers::make_unique<vulkan::VkFramebuffer>(
        allocator,
        vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app->device()));

    renderData->descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(allocator,
            app->AllocateDescriptorSet({descriptor_set_layout_binding_}));

    VkDescriptorImageInfo input_attachment_info = {
        VK_NULL_HANDLE,                            // sampler
        inputView,                                 // imageView
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // imageLayout
    };
    VkWriteDescriptorSet write{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        *renderData->descriptor_set_,            // dstSet
        0,                                       // dstBinding
        0,                                       // dstArrayElement
        1,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,     // descriptorType
        &input_attachment_info,                  // pImageInfo
        nullptr,                                 // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };
    app->device()->vkUpdateDescriptorSets(
        app->device(), 1, &write, 0, nullptr);
}

void RenderQuad::RecordRenderCmds(vulkan::VulkanApplication* app, RenderQuadData* renderData,
    vulkan::VkCommandBuffer& cmdBuffer, size_t frame_index) {

    VkClearValue clear;
    vulkan::MemoryClear(&clear);
    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *(renderData->framebuffer_),               // framebuffer
        {{0, 0},
         {app->swapchain().width(),
          app->swapchain().height()}},    // renderArea
        1,                                // clearValueCount
        &clear                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *pipeline_);

    cmdBuffer->vkCmdBindDescriptorSets(cmdBuffer,
                                  VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  ::VkPipelineLayout(*pipeline_layout_), 0, 1,
                                  &renderData->descriptor_set_->raw_set(), 0,
                                  nullptr);
    quad_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);
}

void RenderQuad::Update(float time_since_last_render) {
    // Do nothing
}

void RenderQuad::UpdateRenderData(vulkan::VkQueue* queue, size_t frame_index) {
    // Do nothing
}