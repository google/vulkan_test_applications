// Copyright 2019 Google Inc.
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
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

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

int main_entry(const entry::EntryData* data) {
  logging::Logger* log = data->logger();
  log->LogInfo("Application Startup");
  VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR
      pipeline_executable_info_features{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR,
          nullptr, true};

  vulkan::VulkanApplication app(
      data->allocator(), data->logger(), data, {},
      {VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME}, {0}, 1024 * 128,
      1024 * 128, 1024 * 128, 1024 * 128, false, false, false, 0, false, false,
      VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, false, false, nullptr, false, false,
      &pipeline_executable_info_features);

  vulkan::VkDevice& device = app.device();

  vulkan::VulkanModel cube(data->allocator(), data->logger(), cube_data);

  VkDescriptorSetLayoutBinding cube_descriptor_set_layouts_[2] = {
      {
          0,                                  // binding
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
          1,                                  // descriptorCount
          VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
          nullptr                             // pImmutableSamplers
      },
      {
          1,                                  // binding
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
          1,                                  // descriptorCount
          VK_SHADER_STAGE_VERTEX_BIT,         // stageFlags
          nullptr                             // pImmutableSamplers
      }};

  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout =
      containers::make_unique<vulkan::PipelineLayout>(
          data->allocator(),
          app.CreatePipelineLayout({{cube_descriptor_set_layouts_[0],
                                     cube_descriptor_set_layouts_[1]}}));

  VkAttachmentReference color_attachment = {
      0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  uint32_t image_resolution = 1024;

  VkSampleCountFlagBits num_samples = VK_SAMPLE_COUNT_1_BIT;
  VkViewport viewport = {0.0f,
                         0.0f,
                         static_cast<float>(image_resolution),
                         static_cast<float>(image_resolution),
                         0.0f,
                         1.0f};

  VkRect2D scissor = {{0, 0}, {image_resolution, image_resolution}};

  containers::unique_ptr<vulkan::VkRenderPass> render_pass =
      containers::make_unique<vulkan::VkRenderPass>(
          data->allocator(),
          app.CreateRenderPass(
              {{
                  0,                                         // flags
                  app.swapchain().format(),                  // format
                  num_samples,                               // samples
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

  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> cube_pipeline =
      containers::make_unique<vulkan::VulkanGraphicsPipeline>(
          data->allocator(), app.CreateGraphicsPipeline(pipeline_layout.get(),
                                                        render_pass.get(), 0));
  cube_pipeline->AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                           cube_vertex_shader);
  cube_pipeline->AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                           cube_fragment_shader);
  cube_pipeline->SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  cube_pipeline->SetInputStreams(&cube);
  cube_pipeline->SetViewport(viewport);
  cube_pipeline->SetScissor(scissor);
  cube_pipeline->SetSamples(num_samples);
  cube_pipeline->AddAttachment();
  cube_pipeline->flags() =
      VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR |
      VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR;
  cube_pipeline->Commit();

  VkPipelineInfoKHR pipeline_info{VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR, nullptr,
                                  *cube_pipeline};
  uint32_t executable_count;

  device->vkGetPipelineExecutablePropertiesKHR(device, &pipeline_info,
                                               &executable_count, nullptr);

  containers::vector<VkPipelineExecutablePropertiesKHR> executable_properties(
      executable_count,
      {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR, nullptr},
      data->allocator());

  device->vkGetPipelineExecutablePropertiesKHR(
      device, &pipeline_info, &executable_count, executable_properties.data());

  for (uint32_t i = 0; i < executable_count; i++) {
    VkPipelineExecutableInfoKHR executable_info{
        VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR, nullptr, *cube_pipeline,
        i};
    uint32_t statistic_count;

    device->vkGetPipelineExecutableStatisticsKHR(device, &executable_info,
                                                 &statistic_count, nullptr);

    containers::vector<VkPipelineExecutableStatisticKHR> statistic(
        statistic_count,
        {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR, nullptr},
        data->allocator());

    device->vkGetPipelineExecutableStatisticsKHR(
        device, &executable_info, &statistic_count, statistic.data());

    uint32_t internal_representation_count;

    device->vkGetPipelineExecutableInternalRepresentationsKHR(
        device, &executable_info, &internal_representation_count, nullptr);

    containers::vector<VkPipelineExecutableInternalRepresentationKHR>
        internal_representation(internal_representation_count, {},
                                data->allocator());

    device->vkGetPipelineExecutableInternalRepresentationsKHR(
        device, &executable_info, &internal_representation_count,
        internal_representation.data());

    log->LogInfo(
        "============= Shader executable ===================================");
    log->LogInfo("Name          : ", executable_properties[i].name);
    log->LogInfo("Description   : ", executable_properties[i].description);
    log->LogInfo("Subgroup size : ", executable_properties[i].subgroupSize);

    log->LogInfo(
        "============= Shader executable statistic =========================");
    for (const auto& s : statistic) {
      log->LogInfo("Name          : ", s.name);
      log->LogInfo("Description   : ", s.description);

      switch (s.format) {
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
          log->LogInfo("Value         : ", s.value.b32);
          break;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
          log->LogInfo("Value         : ", s.value.i64);
          break;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
          log->LogInfo("Value         : ", s.value.u64);
          break;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
          log->LogInfo("Value         : ", s.value.f64);
          break;
        default:
          LOG_CRASH(log, "Unexpected format");
      }
      log->LogInfo("");
    }
    log->LogInfo(
        "============= Shader executable internal representation ===========");
    for (const auto& ir : internal_representation) {
      log->LogInfo("Name          : ", ir.name);
      log->LogInfo("Description   : ", ir.description);
      if (ir.isText) {
        log->LogInfo("Text          : ", ir.pData);
      }
    }
  }

  log->LogInfo("Application Shutdown");
  return 0;
}
