#include "cube_render.h"

namespace cube_model {
#include "cube.obj.h"
}

namespace {
    uint32_t cube_vertex_shader[] =
    #include "cube.vert.spv"
        ;

    uint32_t cube_fragment_shader[] =
    #include "cube.frag.spv"
        ;
    const auto& cube_data = cube_model::model;
}

CubeRender::CubeRender(const entry::EntryData* data) 
    : cube_(data->allocator(), data->logger(), cube_data) {}

void CubeRender::InitializeCubeData(vulkan::VulkanApplication* app,
    containers::Allocator* allocator,
    CubeVulkanInfo vulkanInfo,
    vulkan::VkCommandBuffer* initialization_buffer,  
    size_t num_swapchain_images) {
    cube_.InitializeData(app, initialization_buffer);

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

    pipeline_layout_ = containers::make_unique<vulkan::PipelineLayout>(
        allocator,
        app->CreatePipelineLayout({{cube_descriptor_set_layouts_[0],
                                      cube_descriptor_set_layouts_[1]}}));

    VkAttachmentReference color_attachment = {
        0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    render_pass_ = containers::make_unique<vulkan::VkRenderPass>(
        allocator,
        app->CreateRenderPass(
            {{
                0,                                         // flags
                vulkanInfo.format,                         // format
                vulkanInfo.num_samples,                    // samples
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
                nullptr,                          // pDepthStencilAttachment
                0,                                // preserveAttachmentCount
                nullptr                           // pPreserveAttachments
            }},                                   // SubpassDescriptions
            {}                                    // SubpassDependencies
            ));

    cube_pipeline_ = containers::make_unique<vulkan::VulkanGraphicsPipeline>(
        allocator,
        app->CreateGraphicsPipeline(pipeline_layout_.get(),
                                      render_pass_.get(), 0));
    cube_pipeline_->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                              cube_vertex_shader);
    cube_pipeline_->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                              cube_fragment_shader);
    cube_pipeline_->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    cube_pipeline_->SetInputStreams(&cube_);
    cube_pipeline_->SetViewport(vulkanInfo.viewport);
    cube_pipeline_->SetScissor(vulkanInfo.scissor);
    cube_pipeline_->SetSamples(vulkanInfo.num_samples);
    cube_pipeline_->AddAttachment();
    cube_pipeline_->Commit();

    camera_data_ = containers::make_unique<vulkan::BufferFrameData<CameraData>>(
        allocator, app, num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    model_data_ = containers::make_unique<vulkan::BufferFrameData<ModelData>>(
        allocator, app, num_swapchain_images,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    float aspect =
        (float)app->swapchain().width() / (float)app->swapchain().height();
    camera_data_->data().projection_matrix =
        Mat4x4::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
        Mat4x4::Perspective(1.5708f, aspect, 0.1f, 100.0f);

    model_data_->data().transform = Mat4x4::FromTranslationVector(
        mathfu::Vector<float, 3>{0.0f, 0.0f, -3.0f});
}

void CubeRender::InitializeFrameData(vulkan::VulkanApplication* app,
    CubeRenderData* renderData,
    containers::Allocator* allocator,
    VkImageView colorView,
    size_t frame_index) {

    renderData->cube_descriptor_set_ =
        containers::make_unique<vulkan::DescriptorSet>(
            allocator,
            app->AllocateDescriptorSet({cube_descriptor_set_layouts_[0],
                                          cube_descriptor_set_layouts_[1]}));

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
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
        nullptr,                                 // pNext
        *(renderData->cube_descriptor_set_),       // dstSet
        0,                                       // dstbinding
        0,                                       // dstArrayElement
        2,                                       // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
        nullptr,                                 // pImageInfo
        buffer_infos,                            // pBufferInfo
        nullptr,                                 // pTexelBufferView
    };

    app->device()->vkUpdateDescriptorSets(app->device(), 1, &write, 0,
                                            nullptr);

    ::VkImageView raw_view = colorView;

    // Create a framebuffer with depth and image attachments
    VkFramebufferCreateInfo framebuffer_create_info{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        *render_pass_,                              // renderPass
        1,                                          // attachmentCount
        &raw_view,                                  // attachments
        app->swapchain().width(),                 // width
        app->swapchain().height(),                // height
        1                                           // layers
    };

    ::VkFramebuffer raw_framebuffer;
    app->device()->vkCreateFramebuffer(
        app->device(), &framebuffer_create_info, nullptr, &raw_framebuffer);
    renderData->framebuffer_ = containers::make_unique<vulkan::VkFramebuffer>(
        allocator,
        vulkan::VkFramebuffer(raw_framebuffer, nullptr, &app->device()));
}


void CubeRender::RecordRenderCmds(vulkan::VulkanApplication* app,
    CubeRenderData* renderData, vulkan::VkCommandBuffer& cmdBuffer) {
    VkClearValue clear;
    vulkan::MemoryClear(&clear);

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        *render_pass_,                             // renderPass
        *(renderData->framebuffer_),                 // framebuffer
        {{0, 0},
         {app->swapchain().width(),
          app->swapchain().height()}},  // renderArea
        1,                                // clearValueCount
        &clear                            // clears
    };

    cmdBuffer->vkCmdBeginRenderPass(cmdBuffer, &pass_begin,
                                    VK_SUBPASS_CONTENTS_INLINE);

    cmdBuffer->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 *cube_pipeline_);
    cmdBuffer->vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        ::VkPipelineLayout(*pipeline_layout_), 0, 1,
        &(renderData->cube_descriptor_set_->raw_set()), 0, nullptr);
    cube_.Draw(&cmdBuffer);
    cmdBuffer->vkCmdEndRenderPass(cmdBuffer);
}

void CubeRender::Update(float time_since_last_render) {
    model_data_->data().transform = model_data_->data().transform *
        Mat4x4::FromRotationMatrix(
            Mat4x4::RotationX(3.14f * time_since_last_render) *
            Mat4x4::RotationY(3.14f * time_since_last_render * 0.5f));
}

void CubeRender::Render(vulkan::VkQueue* queue, size_t frame_index) {
    // Update our uniform buffers.
    camera_data_->UpdateBuffer(queue, frame_index);
    model_data_->UpdateBuffer(queue, frame_index);
}