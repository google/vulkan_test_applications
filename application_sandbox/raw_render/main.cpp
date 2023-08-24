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
#include <thread>

#include "mathfu/matrix.h"
#include "mathfu/vector.h"
#include "shared_data.h"
#include "support/entry/entry.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

using Mat44 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;

namespace render_model {
#include "torus_knot.obj.h"
}

uint32_t render_vertex_shader[] =
#include "basic.vert.spv"
    ;

uint32_t render_fragment_shader[] =
#include "basic.frag.spv"
    ;

const auto& render_data = render_model::model;

struct CameraData {
  Mat44 projection_matrix;
};

struct ModelData {
  Mat44 transform;
};

template <typename T>
struct range_internal {
  struct range_end_iterator {
    T val;
  };
  struct range_iterator {
    T current;
    T step;
    range_iterator& operator++() {
      current += step;
      return *this;
    }
    bool operator!=(const range_end_iterator& e) const {
      if (step > 0) {
        return current < e.val;
      }
      return current > e.val;
    }
    const T& operator*() const { return current; }
  };
  explicit range_internal(T len) : first{0, 1}, last{len} {}
  range_internal(T begin, T end, T step = 1) : first{begin, step}, last{end} {}
  range_iterator begin() const { return first; }
  range_end_iterator end() const { return last; }

  range_iterator first;
  range_end_iterator last;
};

template <typename T>
typename range_internal<T> range(T len) {
  return range_internal<T>(len);
}

template <typename T>
typename range_internal<T> range(T begin, T end, T step = 1) {
  return range_internal<T>(begin, end, step);
}

static const size_t k_buffering_count = 3;
#define USE_SWAPCHAIN 0
static const size_t k_pseudo_swapchain_count = 3;

static const uint64_t k_send_height = 600;
static const uint64_t k_send_width = 1000;

// This sample will create N storage buffers, and bind them to successive
// locations in a compute shader.
int main_entry(const entry::EntryData* data) {
  data->logger()->LogInfo("Application Startup");

  vulkan::VulkanApplication app(data->allocator(), data->logger(), data,
                                vulkan::VulkanApplicationOptions()
                                    .SetDeviceImageSize(1024 * 1024 * 256)
#if !USE_SWAPCHAIN
                                    .SetCoherentBufferSize(1024 * 1024 * 256)
                                    .SetHostBufferSize(1024 * 1024 * 256)
                                    .DisablePresent()
#else
                                    .SetPreferredPresentMode(
                                        VK_PRESENT_MODE_MAILBOX_KHR)
#endif
  );
  vulkan::VkDevice& device = app.device();
#if USE_SWAPCHAIN
  const uint32_t width = app.swapchain().width();
  const uint32_t height = app.swapchain().height();
  VkFormat swapchain_format = app.swapchain().format();
#else
  const uint32_t width = data->width();
  const uint32_t height = data->height();
  VkFormat swapchain_format = VK_FORMAT_R8G8B8A8_UNORM;

  HANDLE hMapFile;
  char* mapped_buffer;
  HANDLE file = CreateFile("E:\\test.txt", GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           CREATE_ALWAYS, 0, 0);
  hMapFile =
      CreateFileMappingA(file, NULL, PAGE_READWRITE, 0, k_file_mapping_size, 0);
  mapped_buffer = reinterpret_cast<char*>(
      MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, k_file_mapping_size));
  image_mapping_header* header =
      reinterpret_cast<image_mapping_header*>(mapped_buffer);
  header->width = k_send_width;
  header->height = k_send_height;
  header->num_images = 3;
  header->header_lock = 0;
  header->frame_num = 0;
  header->image_to_read = ~static_cast<uint64_t>(0);
  header->image_being_read = ~static_cast<uint64_t>(0);
  header->image_being_written = ~static_cast<uint64_t>(0);
  header->image_offsets[0] = 4096;
  header->image_offsets[1] = header->image_offsets[0] + (width * height * 4);
  header->image_offsets[2] = header->image_offsets[1] + (width * height * 4);
#endif
  vulkan::VkCommandBuffer initialization_buffer =
      app.GetCommandBuffer(app.render_queue().index());
  app.BeginCommandBuffer(&initialization_buffer);

  vulkan::VulkanModel knot(data->allocator(), data->logger(), render_data);

  knot.InitializeData(&app, &initialization_buffer);
  app.EndAndSubmitCommandBufferAndWaitForQueueIdle(&initialization_buffer,
                                                   &app.render_queue());

  VkDescriptorSetLayoutBinding descriptor_set_layouts[] = {
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

  vulkan::PipelineLayout pipeline_layout = app.CreatePipelineLayout(
      {{descriptor_set_layouts[0], descriptor_set_layouts[1]}});

  VkAttachmentReference color_attachment = {
      0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depth_attachment = {
      1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  vulkan::VkRenderPass render_pass = app.CreateRenderPass(
      {{
           0,                                         // flags
           swapchain_format,                          // format
           VK_SAMPLE_COUNT_1_BIT,                     // samples
           VK_ATTACHMENT_LOAD_OP_CLEAR,               // loadOp
           VK_ATTACHMENT_STORE_OP_STORE,              // storeOp
           VK_ATTACHMENT_LOAD_OP_DONT_CARE,           // stencilLoadOp
           VK_ATTACHMENT_STORE_OP_DONT_CARE,          // stencilStoreOp
           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // initialLayout
#if USE_SWAPCHAIN
           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR  // finalLayout
#else
           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL  // finalLayout
#endif
       },
       {
           0,                                                // flags
           VK_FORMAT_D32_SFLOAT,                             // format
           VK_SAMPLE_COUNT_1_BIT,                            // samples
           VK_ATTACHMENT_LOAD_OP_CLEAR,                      // loadOp
           VK_ATTACHMENT_STORE_OP_STORE,                     // storeOp
           VK_ATTACHMENT_LOAD_OP_DONT_CARE,                  // stencilLoadOp
           VK_ATTACHMENT_STORE_OP_DONT_CARE,                 // stencilStoreOp
           VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,  // initialLayout
           VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL   // finalLayout
       }},
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
      }},
      {}  // SubpassDependencies
  );

  VkViewport default_viewport = {
      0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height),
      0.0f, 1.0f};

  VkRect2D default_scissor = {{0, 0}, {width, height}};

  vulkan::VulkanGraphicsPipeline render_pipeline =
      app.CreateGraphicsPipeline(&pipeline_layout, &render_pass, 0);
  render_pipeline.AddShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                            render_vertex_shader);
  render_pipeline.AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                            render_fragment_shader);
  render_pipeline.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  render_pipeline.SetInputStreams(&knot);
  render_pipeline.SetViewport(default_viewport);
  render_pipeline.SetScissor(default_scissor);
  render_pipeline.SetSamples(VK_SAMPLE_COUNT_1_BIT);
  render_pipeline.AddAttachment();
  render_pipeline.Commit();

  vulkan::BufferFrameData<CameraData> camera_data(
      &app, k_buffering_count, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
  vulkan::BufferFrameData<ModelData> model_data(
      &app, k_buffering_count, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
  float aspect = (float)width / (float)height;
  camera_data.data().projection_matrix =
      Mat44::FromScaleVector(mathfu::Vector<float, 3>{1.0f, -1.0f, 1.0f}) *
      Mat44::Perspective(1.5708f, aspect, 0.1f, 100.0f);

  model_data.data().transform = Mat44::FromTranslationVector(
      mathfu::Vector<float, 3>{0.0f, 0.0f, -10.0f});

  containers::vector<vulkan::VkFramebuffer> framebuffers(data->allocator());
  containers::vector<vulkan::VkImageView> image_views(data->allocator());
  containers::vector<vulkan::DescriptorSet> descriptor_sets(data->allocator());
  containers::vector<vulkan::VkFence> fences(data->allocator());
  containers::vector<vulkan::VkSemaphore> image_acquired_semaphores(
      data->allocator());
  containers::vector<vulkan::VkSemaphore> semaphores(data->allocator());
  containers::vector<vulkan::VkCommandBuffer> command_buffers(
      data->allocator());
  containers::vector<vulkan::ImagePointer> depth_stencils(data->allocator());
  containers::vector<vulkan::VkImageView> depth_stencil_views(
      data->allocator());
#if USE_SWAPCHAIN
  containers::vector<VkImage>& swap_images = app.swapchain_images();
#else
  containers::vector<vulkan::ImagePointer> images(data->allocator());
  containers::vector<VkImage> swap_images(data->allocator());
  containers::vector<vulkan::ImagePointer> blit_images(data->allocator());
  containers::vector<vulkan::BufferPointer> blit_image_buffers(
      data->allocator());
  containers::vector<HANDLE> available_image_semaphores(data->allocator());
  containers::vector<vulkan::VkFence> swap_fences(data->allocator());

  for (size_t i : range(k_pseudo_swapchain_count)) {
    {
      VkFenceCreateInfo create_info = {
          VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,  // sType
          nullptr,                              // pNext
          0                                     // flags
      };
      ::VkFence raw_fence;
      device->vkCreateFence(device, &create_info, nullptr, &raw_fence);
      swap_fences.push_back(vulkan::VkFence(raw_fence, nullptr, &device));
    }
    available_image_semaphores.push_back(CreateSemaphore(NULL, 1, 1, NULL));
    {
      VkImageCreateInfo create_info{
          VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
          nullptr,                              // pNext
          0,                                    // flags
          VK_IMAGE_TYPE_2D,                     // imageType
          swapchain_format,                     // format
          VkExtent3D{
              width,   // width
              height,  // height
              1,       // depth
          },
          1,                        // mipLevels
          1,                        // arrayLayers
          VK_SAMPLE_COUNT_1_BIT,    // samples
          VK_IMAGE_TILING_OPTIMAL,  // tiling
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT,  // usage
          VK_SHARING_MODE_EXCLUSIVE,            // sharingMode
          0,                                    // queueFamilyIndexCount
          nullptr,                              // pQueueFamilyIndices
          VK_IMAGE_LAYOUT_UNDEFINED};
      images.push_back(app.CreateAndBindImage(&create_info));
      swap_images.push_back(images.back()->get_raw_image());
    }
    {
      VkImageCreateInfo create_info{
          VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
          nullptr,                              // pNext
          0,                                    // flags
          VK_IMAGE_TYPE_2D,                     // imageType
          swapchain_format,                     // format
          VkExtent3D{
              k_send_width,   // width
              k_send_height,  // height
              1,              // depth
          },
          1,                        // mipLevels
          1,                        // arrayLayers
          VK_SAMPLE_COUNT_1_BIT,    // samples
          VK_IMAGE_TILING_OPTIMAL,  // tiling
          VK_IMAGE_USAGE_TRANSFER_DST_BIT |
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT,  // usage
          VK_SHARING_MODE_EXCLUSIVE,            // sharingMode
          0,                                    // queueFamilyIndexCount
          nullptr,                              // pQueueFamilyIndices
          VK_IMAGE_LAYOUT_UNDEFINED};
      blit_images.push_back(app.CreateAndBindImage(&create_info));
    }
    {
      VkBufferCreateInfo create_info{
          VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
          nullptr,                               // pNext
          0,                                     // flags
          k_send_width * k_send_height * 4,      // size
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,      // usage
          VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
          0,
          nullptr};

      // swap_image_buffers.push_back(
      //     app.CreateAndBindCoherentBuffer(&create_info));
      blit_image_buffers.push_back(app.CreateAndBindHostBuffer(&create_info));
    }
  }

#endif
  for (size_t i : range(swap_images.size())) {
    {
      VkImageViewCreateInfo create_info = {
          VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
          nullptr,                                   // pNext
          0,                                         // flags
          swap_images[i],                            // image
          VK_IMAGE_VIEW_TYPE_2D,                     // viewType
          swapchain_format,                          // format
          VkComponentMapping{
              VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY,
          },
          VkImageSubresourceRange{
              VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
              0,                          // baseMipLevel
              1,                          // levelCount
              0,                          // baseArrayLayer
              1,                          // layerCount
          }};
      ::VkImageView raw_view;
      device->vkCreateImageView(device, &create_info, nullptr, &raw_view);
      image_views.push_back(vulkan::VkImageView(raw_view, nullptr, &device));
    }

    {
      VkImageCreateInfo create_info{
          VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
          nullptr,                              // pNext
          0,                                    // flags
          VK_IMAGE_TYPE_2D,                     // imageType
          VK_FORMAT_D32_SFLOAT,                 // format
          VkExtent3D{
              width,   // width
              height,  // height
              1,       // depth
          },
          1,                                            // mipLevels
          1,                                            // arrayLayers
          VK_SAMPLE_COUNT_1_BIT,                        // samples
          VK_IMAGE_TILING_OPTIMAL,                      // tiling
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,  // usage
          VK_SHARING_MODE_EXCLUSIVE,                    // sharingMode
          0,                                            // queueFamilyIndexCount
          nullptr,                                      // pQueueFamilyIndices
          VK_IMAGE_LAYOUT_UNDEFINED};
      depth_stencils.push_back(app.CreateAndBindImage(&create_info));
    }

    {
      VkImageViewCreateInfo create_info = {
          VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
          nullptr,                                   // pNext
          0,                                         // flags
          depth_stencils[i]->get_raw_image(),        // image
          VK_IMAGE_VIEW_TYPE_2D,                     // viewType
          VK_FORMAT_D32_SFLOAT,                      // format
          VkComponentMapping{
              VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY,
          },
          VkImageSubresourceRange{
              VK_IMAGE_ASPECT_DEPTH_BIT,  // aspectMask
              0,                          // baseMipLevel
              1,                          // levelCount
              0,                          // baseArrayLayer
              1,                          // layerCount
          }};
      ::VkImageView raw_view;
      device->vkCreateImageView(device, &create_info, nullptr, &raw_view);
      depth_stencil_views.push_back(
          vulkan::VkImageView(raw_view, nullptr, &device));
    }
    {
      VkImageView views[2] = {image_views[i], depth_stencil_views[i]};

      VkFramebufferCreateInfo create_info{
          VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
          nullptr,                                    // pNext
          0,                                          // flags
          render_pass,                                // renderPass
          2,                                          // attachmentCount
          views,                                      // attachments
          width,                                      // width
          height,                                     // height
          1                                           // layers
      };
      ::VkFramebuffer raw_framebuffer;
      device->vkCreateFramebuffer(device, &create_info, nullptr,
                                  &raw_framebuffer);
      framebuffers.push_back(
          vulkan::VkFramebuffer(raw_framebuffer, nullptr, &device));
    }
  }

  for (size_t i : range(k_buffering_count)) {
    {
      descriptor_sets.push_back(app.AllocateDescriptorSet(
          {descriptor_set_layouts[0], descriptor_set_layouts[1]}));

      VkDescriptorBufferInfo buffer_infos[2] = {
          {
              camera_data.get_buffer(),             // buffer
              camera_data.get_offset_for_frame(i),  // offset
              camera_data.size(),                   // range
          },
          {
              model_data.get_buffer(),             // buffer
              model_data.get_offset_for_frame(i),  // offset
              model_data.size(),                   // range
          }};

      VkWriteDescriptorSet write{
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,  // sType
          nullptr,                                 // pNext
          descriptor_sets[i],                      // dstSet
          0,                                       // dstbinding
          0,                                       // dstArrayElement
          2,                                       // descriptorCount
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,       // descriptorType
          nullptr,                                 // pImageInfo
          buffer_infos,                            // pBufferInfo
          nullptr,                                 // pTexelBufferView
      };

      device->vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    {
      VkFenceCreateInfo create_info = {
          VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,  // sType
          nullptr,                              // pNext
          VK_FENCE_CREATE_SIGNALED_BIT          // flags
      };
      ::VkFence raw_fence;
      device->vkCreateFence(device, &create_info, nullptr, &raw_fence);
      fences.push_back(vulkan::VkFence(raw_fence, nullptr, &device));
    }
    {
      VkSemaphoreCreateInfo create_info = {
          VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,  // sType
          nullptr,                                  // pNext,
          0,                                        // flags
      };
      ::VkSemaphore raw_semaphore;
      device->vkCreateSemaphore(device, &create_info, nullptr, &raw_semaphore);
      image_acquired_semaphores.push_back(
          vulkan::VkSemaphore(raw_semaphore, nullptr, &device));

      device->vkCreateSemaphore(device, &create_info, nullptr, &raw_semaphore);
      semaphores.push_back(
          vulkan::VkSemaphore(raw_semaphore, nullptr, &device));
    }

    command_buffers.push_back(app.GetCommandBuffer(app.render_queue().index()));
  }

#if !USE_SWAPCHAIN
  HANDLE semaphore;
  semaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL);

  std::thread waiting_thread([&]() {
    size_t image_idx = 0;
    size_t last_start_idx = 0;
    auto last_time = std::chrono::high_resolution_clock::now();
    uint64_t last_image_written = ~static_cast<uint64_t>(0);
    while (true) {
      auto current_time = std::chrono::high_resolution_clock::now();
      if (current_time - last_time > std::chrono::seconds(1)) {
        data->logger()->LogInfo("Frames in the last second: ",
                                image_idx - last_start_idx);
        last_start_idx = image_idx;
        last_time = current_time;
      }

      WaitForSingleObject(semaphore, INFINITE);
      uint32_t fb_index = image_idx % k_pseudo_swapchain_count;
      device->vkWaitForFences(device, 1,
                              &swap_fences[fb_index].get_raw_object(), VK_TRUE,
                              0xFFFFFFFFFFFFFFFF);
      blit_image_buffers[fb_index]->invalidate();

      while (InterlockedCompareExchange64(&header->header_lock, 1, 0) != 0) {
      }
      uint64_t to_write = 0;
      if (last_image_written == -1) {
        to_write = 0;
      } else {
        for (uint64_t i = 0; i < 3; ++i) {
          if (i == last_image_written) {
            continue;
          }
          if (header->image_being_read == i) {
            continue;
          }
          to_write = i;
          header->image_being_written = i;
          break;
        }
      }
      char* location = mapped_buffer + header->image_offsets[to_write];
      InterlockedExchange64(&header->header_lock, 0);
      memcpy(location, blit_image_buffers[fb_index]->base_address(),
             k_send_width * k_send_height * 4);
      header->image_to_read = to_write;
      header->frame_num = image_idx;
      ReleaseSemaphore(available_image_semaphores[fb_index], 1, NULL);
      image_idx++;
    }
  });
#endif

  auto last_frame_time = std::chrono::high_resolution_clock::now();

  size_t frame_num = 0;
  size_t total_frame_num = 0;
  size_t last_reported_frame = 0;
  auto last_reported_time = last_frame_time;
  while (true) {
    auto current_time = std::chrono::high_resolution_clock::now();
    if (current_time - last_reported_time > std::chrono::seconds(1)) {
      data->logger()->LogInfo("Number of frames processed in the last second: ",
                              total_frame_num - last_reported_frame);
      last_reported_time = current_time;
      last_reported_frame = total_frame_num;
    }

    // The spec defines std::chrono::duration<float> to be time in seconds.
    auto time_diff = std::chrono::duration_cast<std::chrono::duration<float>>(
        current_time - last_frame_time);
    last_frame_time = current_time;

    // Step 1 wait until the resoures from the PREVIOUS render are done
    device->vkWaitForFences(device, 1, &fences[frame_num].get_raw_object(),
                            VK_TRUE, 0xFFFFFFFFFFFFFFFF);

    model_data.data().transform =
        model_data.data().transform *
        Mat44::FromRotationMatrix(
            Mat44::RotationX(3.14f * time_diff.count()) *
            Mat44::RotationY(3.14f * time_diff.count() * 0.5f));

    camera_data.UpdateBuffer(&app.render_queue(), frame_num);
    model_data.UpdateBuffer(&app.render_queue(), frame_num);

    device->vkResetFences(device, 1, &fences[frame_num].get_raw_object());
    uint32_t framebuffer_index;
#if USE_SWAPCHAIN
    device->vkAcquireNextImageKHR(device, app.swapchain(), 0xFFFFFFFFFFFFFFFF,
                                  image_acquired_semaphores[frame_num],
                                  VK_NULL_HANDLE, &framebuffer_index);
#else
    framebuffer_index = total_frame_num % k_pseudo_swapchain_count;
    WaitForSingleObject(available_image_semaphores[framebuffer_index],
                        INFINITE);
    device->vkResetFences(device, 1,
                          &swap_fences[framebuffer_index].get_raw_object());
#endif
    auto& cb = command_buffers[frame_num];
    cb->vkResetCommandBuffer(cb, 0);
    app.BeginCommandBuffer(&cb);

    // We always transition from undefined because we just don't care about
    // the previous contents.
    VkImageMemoryBarrier barriers[] = {
        {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // sType
            nullptr,                                   // pNext
            0,                                         // srcAccessMask
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,      // dstAccessMask
            VK_IMAGE_LAYOUT_UNDEFINED,                 // oldLayout
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // newLayout
            VK_QUEUE_FAMILY_IGNORED,                   // srcQueueFamilyIndex
            VK_QUEUE_FAMILY_IGNORED,                   // dstQueueFamilyIndex
            swap_images[framebuffer_index],
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        },
        {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,           // sType
         nullptr,                                          // pNext
         0,                                                // srcAccessMask
         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,             // dstAccessMask
         VK_IMAGE_LAYOUT_UNDEFINED,                        // oldLayout
         VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,  // newLayout
         VK_QUEUE_FAMILY_IGNORED,  // srcQueueFamilyIndex
         VK_QUEUE_FAMILY_IGNORED,  // dstQueueFamilyIndex
         *depth_stencils[framebuffer_index],
         {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}}};
    cb->vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                             0, nullptr, 0, nullptr, 2, barriers);

    VkClearValue clears[2];
    vulkan::MemoryClear(&clears[0]);
    clears[1] = {1.0f, 1};

    VkRenderPassBeginInfo pass_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
        nullptr,                                   // pNext
        render_pass,                               // renderPass
        framebuffers[framebuffer_index],           // framebuffer
        {{0, 0}, {width, height}},                 // renderArea
        2,                                         // clearValueCount
        clears                                     // clears
    };
    cb->vkCmdBeginRenderPass(cb, &pass_begin, VK_SUBPASS_CONTENTS_INLINE);

    cb->vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, render_pipeline);

    cb->vkCmdBindDescriptorSets(
        cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
        &descriptor_sets[frame_num].raw_set(), 0, nullptr);
    knot.Draw(&cb);
    cb->vkCmdEndRenderPass(cb);
#if USE_SWAPCHAIN
    app.EndAndSubmitCommandBuffer(
        &cb, &app.render_queue(), {image_acquired_semaphores[frame_num]},
        {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        {semaphores[frame_num]}, fences[frame_num]);
    VkPresentInfoKHR present_info = {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,       // sType
        nullptr,                                  // pNext
        1,                                        // waitSemaphoreCount
        &semaphores[frame_num].get_raw_object(),  // pWaitSemaphores
        1,                                        // swapchainCount
        &app.swapchain().get_raw_object(),        // pSwapchains
        &framebuffer_index,                       // pImageIndices
        nullptr,                                  // pResults
    };

    app.render_queue()->vkQueuePresentKHR(app.render_queue(), &present_info);

#else
    VkImageMemoryBarrier blit_barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
        nullptr,                                 // pNext
        0,                                       // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,            // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,               // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,    // newLayout
        VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                 // dstQueueFamilyIndex
        *blit_images[framebuffer_index],
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };

    cb->vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &blit_barrier);
    VkImageBlit image_region{
        VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {VkOffset3D{0, 0, 0}, VkOffset3D{static_cast<int32_t>(width),
                                         static_cast<int32_t>(height), 1}},
        VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {VkOffset3D{0, 0, 0}, VkOffset3D{k_send_width, k_send_height, 1}}};
    cb->vkCmdBlitImage(cb, swap_images[framebuffer_index],
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       *blit_images[framebuffer_index],
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_region,
                       VK_FILTER_LINEAR);
    blit_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    blit_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    blit_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blit_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    cb->vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &blit_barrier);

    VkBufferImageCopy copy = {
        0,  // bufferOffset
        0,  // bufferRowLength
        0,  // bufferImageHeight
        VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        VkOffset3D{0, 0, 0},
        VkExtent3D{k_send_width, k_send_height, 1}};

    cb->vkCmdCopyImageToBuffer(cb, *blit_images[framebuffer_index],
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               *blit_image_buffers[framebuffer_index], 1,
                               &copy);
    app.EndAndSubmitCommandBuffer(
        &cb, &app.render_queue(), {},
        {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}, {}, fences[frame_num]);
    app.render_queue()->vkQueueSubmit(app.render_queue(), 0, nullptr,
                                      swap_fences[framebuffer_index]);
    ReleaseSemaphore(semaphore, 1, NULL);
#endif

    frame_num = (frame_num + 1) % k_buffering_count;
    total_frame_num++;
    // Nothing goes after this statement in the loop
  }
}