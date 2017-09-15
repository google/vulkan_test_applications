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

#include "vulkan_helpers/vulkan_application.h"

#include <algorithm>
#include <fstream>
#include <tuple>

#include "support/containers/unordered_map.h"
#include "vulkan_helpers/helper_functions.h"
#include "vulkan_helpers/vulkan_model.h"

typedef void(VKAPI_PTR* PFN_vkSetSwapchainCallback)(
    VkSwapchainKHR, void(void*, uint8_t*, size_t), void*);

namespace vulkan {
VkDescriptorPool DescriptorSet::CreateDescriptorPool(
    containers::Allocator* allocator, VkDevice* device,
    std::initializer_list<VkDescriptorSetLayoutBinding> bindings) {
  containers::unordered_map<uint32_t, uint32_t> counts(allocator);
  for (auto binding : bindings) {
    counts[static_cast<uint32_t>(binding.descriptorType)] +=
        binding.descriptorCount;
  }

  containers::vector<VkDescriptorPoolSize> pool_sizes(allocator);
  pool_sizes.reserve(counts.size());
  for (auto p : counts) {
    pool_sizes.push_back({static_cast<VkDescriptorType>(p.first), p.second});
  }

  return vulkan::CreateDescriptorPool(
      device, static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), 1);
}

DescriptorSet::DescriptorSet(
    containers::Allocator* allocator, VkDevice* device,
    std::initializer_list<VkDescriptorSetLayoutBinding> bindings)
    : pool_(CreateDescriptorPool(allocator, device, bindings)),
      layout_(CreateDescriptorSetLayout(allocator, device, bindings)),
      set_(AllocateDescriptorSet(device, pool_.get_raw_object(),
                                 layout_.get_raw_object())) {}

VulkanApplication::VulkanApplication(
    containers::Allocator* allocator, logging::Logger* log,
    const entry::entry_data* entry_data,
    const std::initializer_list<const char*> extensions,
    const VkPhysicalDeviceFeatures& features, uint32_t host_buffer_size,
    uint32_t device_image_size, uint32_t device_buffer_size,
    uint32_t coherent_buffer_size, bool use_async_compute_queue)
    : allocator_(allocator),
      log_(log),
      entry_data_(entry_data),
      swapchain_images_(allocator_),
      render_queue_(nullptr),
      present_queue_(nullptr),
      render_queue_index_(0u),
      present_queue_index_(0u),
      library_wrapper_(allocator_, log_),
      instance_(CreateInstanceForApplication(allocator_, &library_wrapper_,
                                             entry_data_)),
      surface_(CreateDefaultSurface(&instance_, entry_data_)),
      device_(CreateDevice(extensions, features, use_async_compute_queue)),
      swapchain_(CreateDefaultSwapchain(&instance_, &device_, &surface_,
                                        allocator_, render_queue_index_,
                                        present_queue_index_, entry_data_)),
      command_pool_(CreateDefaultCommandPool(allocator_, device_)),
      pipeline_cache_(CreateDefaultPipelineCache(&device_)),
      should_exit_(false) {
  if (!device_.is_valid()) {
    return;
  }

  if (entry_data->options.output_frame >= 1) {
    PFN_vkSetSwapchainCallback set_callback =
        reinterpret_cast<PFN_vkSetSwapchainCallback>(
            device_.getProcAddrFunction()(device_, "vkSetSwapchainCallback"));

    struct cb_data {
      uint32_t output_frame;
      const char* file_name;
      logging::Logger* log;
      const entry::entry_data* dat;
      std::atomic<bool>& should_exit;
      static void fn(void* obj, uint8_t* data, size_t size) {
        cb_data* d = reinterpret_cast<cb_data*>(obj);
        if (d->output_frame == 0) return;

        if (size != d->dat->width * d->dat->height * 4) {
          d->log->LogError("Invalid data size");
          exit(-1);
        }
        if (--d->output_frame == 0) {
          // Because we are in the callback swapchain, we know that the
          // image size is 100x100, and the format is rgba
          std::ofstream ppm;
          ppm.open(d->file_name);
          ppm << "P6 " << d->dat->width << " " << d->dat->height << " 255\n";

          for (size_t i = 0; i < size; ++i) {
            if (i % 4 == 3) continue;
            ppm << char(*(data + i));
          }
          ppm.close();
          d->should_exit.store(true);
        }
      }
    };
    auto cb = new cb_data{
        static_cast<uint32_t>(entry_data->options.output_frame),
        entry_data->options.output_file, log_, entry_data_, should_exit_};
    set_callback(swapchain_, &cb_data::fn, cb);
  }

  vulkan::LoadContainer(log_, device_->vkGetSwapchainImagesKHR,
                        &swapchain_images_, device_, swapchain_);
  // Relevant spec sections for determining what memory we will be allowed
  // to use for our buffer allocations.
  //  The memoryTypeBits member is identical for all VkBuffer objects created
  //  with the same value for the flags and usage members in the
  //  VkBufferCreateInfo structure passed to vkCreateBuffer. Further, if
  //  usage1 and usage2 of type VkBufferUsageFlags are such that the bits set
  //  in usage2 are a subset of the bits set in usage1, and they have the same
  //  flags, then the bits set in memoryTypeBits returned for usage1 must be a
  //  subset of the bits set in memoryTypeBits returned for usage2, for all
  //  values of flags.

  // Therefore we should be able to satisfy all buffer requests for non
  // sparse memory bindings if we do the following:
  // For our host visible bits, we will use:
  // VK_BUFFER_USAGE_TRANSFER_SRC_BIT
  // VK_BUFFER_USAGE_TRANSFER_DST_BIT
  // For our device buffers we will use ALL bits.
  // This means we can use this memory for everything.
  // Furthermore for both types, we will have ZERO flags
  // set (we do not want to do sparse binding.)

  containers::unique_ptr<VulkanArena>* device_memories[3] = {
      &host_accessible_heap_, &device_only_buffer_heap_, &coherent_heap_};
  uint32_t device_memory_sizes[3] = {host_buffer_size, device_buffer_size,
                                     coherent_buffer_size};

  const uint32_t kAllBufferBits =
      (VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT << 1) - 1;

  VkBufferUsageFlags usages[3] = {
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      kAllBufferBits, kAllBufferBits};
  VkMemoryPropertyFlags property_flags[3] = {
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  for (size_t i = 0; i < 3; ++i) {
    // 1) Create a tiny buffer so that we can determine what memory flags are
    // required.
    VkBufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // flags
        1,                                     // size
        usages[i],                             // usage
        VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
        0,                                     // queueFamilyIndexCount
        nullptr,                               //  pQueueFamilyIndices
    };
    ::VkBuffer buffer;
    LOG_ASSERT(==, log_,
               device_->vkCreateBuffer(device_, &create_info, nullptr, &buffer),
               VK_SUCCESS);
    // Get the memory requirements for this buffer.
    VkMemoryRequirements requirements;
    device_->vkGetBufferMemoryRequirements(device_, buffer, &requirements);
    device_->vkDestroyBuffer(device_, buffer, nullptr);

    uint32_t memory_index = GetMemoryIndex(
        &device_, log_, requirements.memoryTypeBits, property_flags[i]);
    *device_memories[i] = containers::make_unique<VulkanArena>(
        allocator_, allocator_, log_, device_memory_sizes[i], memory_index,
        &device_,
        (property_flags[i] & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) != 0);
  }

  // Same idea as above, but for image memory.
  // The relevant bits from the spec are:
  //  The memoryTypeBits member is identical for all VkImage objects created
  //  with the same combination of values for the tiling member and the
  //  VK_IMAGE_CREATE_SPARSE_BINDING_BIT bit of the flags member and the
  //  VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT of the usage member in the
  //  VkImageCreateInfo structure passed to vkCreateImage.
  {
    VkImageCreateInfo image_create_info{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
        nullptr,                              // pNext
        0,                                    // flags
        VK_IMAGE_TYPE_2D,                     // imageType
        VK_FORMAT_R8G8B8A8_UNORM,             // format
        {
            // extent
            1,  // width
            1,  // height
            1,  // depth
        },
        1,                                    // mipLevels
        1,                                    // arrayLayers
        VK_SAMPLE_COUNT_1_BIT,                // samples
        VK_IMAGE_TILING_OPTIMAL,              // tiling
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,            // sharingMode
        0,                                    // queueFamilyIndexCount
        nullptr,                              // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,            // initialLayout
    };
    ::VkImage image;
    LOG_ASSERT(==, log_, device_->vkCreateImage(device_, &image_create_info,
                                                nullptr, &image),
               VK_SUCCESS);
    VkMemoryRequirements requirements;
    device_->vkGetImageMemoryRequirements(device_, image, &requirements);
    device_->vkDestroyImage(device_, image, nullptr);

    uint32_t memory_index =
        GetMemoryIndex(&device_, log_, requirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    device_only_image_heap_ = containers::make_unique<VulkanArena>(
        allocator_, allocator_, log_, device_image_size, memory_index, &device_,
        false);
  }
}

VkDevice VulkanApplication::CreateDevice(
    const std::initializer_list<const char*> extensions,
    const VkPhysicalDeviceFeatures& features, bool create_async_compute_queue) {
  // Since this is called by the constructor be careful not to
  // use any data other than what has already been initialized.
  // allocator_, log_, entry_data_, library_wrapper_, instance_,
  // surface_

  vulkan::VkDevice device(vulkan::CreateDeviceForSwapchain(
      allocator_, &instance_, &surface_, &render_queue_index_,
      &present_queue_index_, extensions, features,
      entry_data_->options.prefer_separate_present,
      create_async_compute_queue ? &compute_queue_index_ : nullptr));
  if (device.is_valid()) {
    if (render_queue_index_ == present_queue_index_) {
      render_queue_concrete_ = containers::make_unique<VkQueue>(
          allocator_, GetQueue(&device, render_queue_index_));
      render_queue_ = render_queue_concrete_.get();
      present_queue_ = render_queue_concrete_.get();
    } else {
      render_queue_concrete_ = containers::make_unique<VkQueue>(
          allocator_, GetQueue(&device, render_queue_index_));
      present_queue_concrete_ = containers::make_unique<VkQueue>(
          allocator_, GetQueue(&device, present_queue_index_));
      render_queue_ = render_queue_concrete_.get();
      present_queue_ = present_queue_concrete_.get();
    }
    if (create_async_compute_queue && compute_queue_index_ != 0xFFFFFFFF) {
      async_compute_queue_concrete_ = containers::make_unique<VkQueue>(
          allocator_,
          GetQueue(&device, compute_queue_index_,
                   compute_queue_index_ == render_queue_index_ ? 1 : 0));
    }
  }
  return std::move(device);
}

containers::unique_ptr<VulkanApplication::Image>
VulkanApplication::CreateAndBindImage(const VkImageCreateInfo* create_info) {
  ::VkImage image;
  LOG_ASSERT(==, log_,
             device_->vkCreateImage(device_, create_info, nullptr, &image),
             VK_SUCCESS);
  VkMemoryRequirements requirements;
  device_->vkGetImageMemoryRequirements(device_, image, &requirements);

  ::VkDeviceMemory memory;
  ::VkDeviceSize offset;

  AllocationToken* token = device_only_image_heap_->AllocateMemory(
      requirements.size, requirements.alignment, &memory, &offset, nullptr);

  device_->vkBindImageMemory(device_, image, memory, offset);

  // We have to do it this way because Image is private and friended,
  // so we cannot go through make_unique.
  Image* img = new (allocator_->malloc(sizeof(Image)))
      Image(device_only_image_heap_.get(), token,
            VkImage(image, nullptr, &device_), create_info->format);

  return containers::unique_ptr<Image>(
      img, containers::UniqueDeleter(allocator_, sizeof(Image)));
}

containers::unique_ptr<VkImageView> VulkanApplication::CreateImageView(
    const Image* image, VkImageViewType view_type,
    const VkImageSubresourceRange& subresource_range) {
  VkImageViewCreateInfo create_info{
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
      nullptr,                                   // pNext
      0,                                         // flags
      image->get_raw_image(),                    // image
      view_type,                                 // viewType
      image->format(),                           // format
      {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
       VK_COMPONENT_SWIZZLE_A},
      subresource_range,
  };
  ::VkImageView raw_view;
  LOG_ASSERT(==, log_, device_->vkCreateImageView(device_, &create_info,
                                                  nullptr, &raw_view),
             VK_SUCCESS);
  return containers::make_unique<vulkan::VkImageView>(
      allocator_, VkImageView(raw_view, nullptr, &device_));
}

containers::unique_ptr<VulkanApplication::Buffer>
VulkanApplication::CreateAndBindBuffer(VulkanArena* heap,
                                       const VkBufferCreateInfo* create_info) {
  ::VkBuffer buffer;
  LOG_ASSERT(==, log_,
             device_->vkCreateBuffer(device_, create_info, nullptr, &buffer),
             VK_SUCCESS);
  // Get the memory requirements for this buffer.
  VkMemoryRequirements requirements;
  device_->vkGetBufferMemoryRequirements(device_, buffer, &requirements);
  ::VkDeviceMemory memory;
  ::VkDeviceSize offset;
  char* base_address;

  AllocationToken* token =
      heap->AllocateMemory(requirements.size, requirements.alignment, &memory,
                           &offset, &base_address);

  device_->vkBindBufferMemory(device_, buffer, memory, offset);

  Buffer* buff = new (allocator_->malloc(sizeof(Buffer))) Buffer(
      heap, token, VkBuffer(buffer, nullptr, &device_), base_address, device_,
      memory, offset, requirements.size, &(device_->vkFlushMappedMemoryRanges),
      &(device_->vkInvalidateMappedMemoryRanges));
  return containers::unique_ptr<Buffer>(
      buff, containers::UniqueDeleter(allocator_, sizeof(Buffer)));
}

containers::unique_ptr<VulkanApplication::Buffer>
VulkanApplication::CreateAndBindHostBuffer(
    const VkBufferCreateInfo* create_info) {
  return CreateAndBindBuffer(host_accessible_heap_.get(), create_info);
}

containers::unique_ptr<VulkanApplication::Buffer>
VulkanApplication::CreateAndBindCoherentBuffer(
    const VkBufferCreateInfo* create_info) {
  return CreateAndBindBuffer(coherent_heap_.get(), create_info);
}

containers::unique_ptr<VulkanApplication::Buffer>
VulkanApplication::CreateAndBindDefaultExclusiveHostBuffer(
    VkDeviceSize size, VkBufferUsageFlags usages) {
  VkBufferCreateInfo create_info{
      /* sType = */ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* size = */ size,
      /* usage = */ usages,
      /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
      /* queueFamilyIndexCount = */ 0,
      /* pQueueFamilyIndices = */ nullptr,
  };
  return CreateAndBindHostBuffer(&create_info);
}

containers::unique_ptr<VulkanApplication::Buffer>
VulkanApplication::CreateAndBindDefaultExclusiveCoherentBuffer(
    VkDeviceSize size, VkBufferUsageFlags usages) {
  VkBufferCreateInfo create_info{
      /* sType = */ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* size = */ size,
      /* usage = */ usages,
      /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
      /* queueFamilyIndexCount = */ 0,
      /* pQueueFamilyIndices = */ nullptr,
  };
  return CreateAndBindCoherentBuffer(&create_info);
}

containers::unique_ptr<VulkanApplication::Buffer>
VulkanApplication::CreateAndBindDeviceBuffer(
    const VkBufferCreateInfo* create_info) {
  return CreateAndBindBuffer(device_only_buffer_heap_.get(), create_info);
}

containers::unique_ptr<VulkanApplication::Buffer>
VulkanApplication::CreateAndBindDefaultExclusiveDeviceBuffer(
    VkDeviceSize size, VkBufferUsageFlags usages) {
  VkBufferCreateInfo create_info{
      /* sType = */ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* size = */ size,
      /* usage = */ usages,
      /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
      /* queueFamilyIndexCount = */ 0,
      /* pQueueFamilyIndices = */ nullptr,
  };
  return CreateAndBindDeviceBuffer(&create_info);
}

containers::unique_ptr<VkBufferView> VulkanApplication::CreateBufferView(
    ::VkBuffer buffer, VkFormat format, VkDeviceSize offset,
    VkDeviceSize range) {
  VkBufferViewCreateInfo create_info{
      VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,  // sType
      nullptr,                                   // pNext
      0,                                         // flags
      buffer,                                    // buffer
      format,                                    // format
      offset,                                    // offset
      range,                                     // range
  };
  ::VkBufferView raw_view;
  LOG_ASSERT(==, log_, device_->vkCreateBufferView(device_, &create_info,
                                                   nullptr, &raw_view),
             VK_SUCCESS);
  return containers::make_unique<vulkan::VkBufferView>(
      allocator_, VkBufferView(raw_view, nullptr, &device_));
}

std::tuple<bool, VkCommandBuffer, BufferPointer>
VulkanApplication::FillImageLayersData(
    Image* img, const VkImageSubresourceLayers& image_subresource,
    const VkOffset3D& image_offset, const VkExtent3D& image_extent,
    VkImageLayout initial_img_layout, const containers::vector<uint8_t>& data,
    std::initializer_list<::VkSemaphore> wait_semaphores,
    std::initializer_list<::VkSemaphore> signal_semaphores, ::VkFence fence) {
  auto failure_return = std::make_tuple(
      false, VkCommandBuffer(static_cast<::VkCommandBuffer>(VK_NULL_HANDLE),
                             &command_pool_, &device_),
      BufferPointer(nullptr));
  if (!img) {
    log_->LogError("FillImageLayersData(): The given *img is nullptr");
    return failure_return;
  }
  size_t image_size = GetImageExtentSizeInBytes(image_extent, img->format()) *
                      image_subresource.layerCount;
  if (data.size() < image_size) {
    log_->LogError(
        "FillImageLayersData(): Not Enough data to fill the image layers");
    return failure_return;
  }

  containers::vector<::VkSemaphore> waits(wait_semaphores, allocator_);
  containers::vector<::VkSemaphore> signals(signal_semaphores, allocator_);
  containers::vector<VkPipelineStageFlags> wait_dst_stage_masks(
      waits.size(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, allocator_);

  // Prepare the buffer to be used for data copying.
  VkBufferCreateInfo buf_create_info{
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      nullptr,
      0,
      data.size(),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr,
  };
  BufferPointer src_buffer = CreateAndBindHostBuffer(&buf_create_info);
  std::copy_n(data.begin(), data.size(), src_buffer->base_address());
  src_buffer->flush();

  // Get a command buffer and add commands/barriers to it.
  VkCommandBuffer command_buffer = GetCommandBuffer();
  VkCommandBufferBeginInfo cmd_begin_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
  command_buffer->vkBeginCommandBuffer(command_buffer, &cmd_begin_info);
  // Add a buffer barrier so that the flushed memory becomes visible to the
  // device.
  VkBufferMemoryBarrier buffer_barrier{
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
      nullptr,
      VK_ACCESS_HOST_WRITE_BIT,
      VK_ACCESS_TRANSFER_READ_BIT,
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      *src_buffer,
      0,
      data.size(),
  };
  // Add an image barrier to change the layout set its access bit to transfer
  // write.
  VkImageMemoryBarrier image_barrier{
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      nullptr,
      0,  // Change to write access, no read-after-write risk.
      VK_ACCESS_TRANSFER_WRITE_BIT,
      initial_img_layout,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      *img,
      // subresource range, only deal one mip level
      {
          image_subresource.aspectMask, image_subresource.mipLevel, 1,
          image_subresource.baseArrayLayer, image_subresource.layerCount,
      }};
  command_buffer->vkCmdPipelineBarrier(
      command_buffer, VK_PIPELINE_STAGE_HOST_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &buffer_barrier, 1,
      &image_barrier);
  // Copy data to the image.
  VkBufferImageCopy copy_info{
      0, 0, 0, image_subresource, image_offset, image_extent};
  command_buffer->vkCmdCopyBufferToImage(command_buffer, *src_buffer, *img,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                         1, &copy_info);
  // Add a global barrier at the end to make sure the data written to the
  // image is available globally.
  VkMemoryBarrier end_barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
                              VK_ACCESS_TRANSFER_WRITE_BIT, kAllReadBits};
  command_buffer->vkCmdPipelineBarrier(
      command_buffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,     // The data in image is produced at
                                          // 'trasfer' stage
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  // The data should be available at
                                          // the
                                          // very begining for following
                                          // commands.
      0, 1, &end_barrier, 0, nullptr, 0, nullptr);

  command_buffer->vkEndCommandBuffer(command_buffer);
  // Submit the command buffer.
  ::VkCommandBuffer raw_cmd_buf = command_buffer.get_command_buffer();
  VkSubmitInfo submit_info{
      VK_STRUCTURE_TYPE_SUBMIT_INFO,               // sType
      nullptr,                                     // pNext
      uint32_t(waits.size()),                      // waitSemaphoreCount
      waits.size() == 0 ? nullptr : waits.data(),  // pWaitSemaphores
      waits.size() == 0 ? nullptr
                        : wait_dst_stage_masks.data(),  // pWaitDstStageMask
      1,                                                // commandBufferCount
      &raw_cmd_buf,                                     // pCommandBuffers
      uint32_t(signals.size()),                         // signalSemaphoreCount
      signals.size() == 0 ? nullptr : signals.data()    // pSignalSemaphores
  };
  (*render_queue_)->vkQueueSubmit(render_queue(), 1, &submit_info, fence);
  return std::make_tuple(true, std::move(command_buffer),
                         std::move(src_buffer));
}

const size_t MAX_UPDATE_SIZE = 65536;
void VulkanApplication::FillSmallBuffer(Buffer* buffer, const void* data,
                                        size_t data_size, size_t buffer_offset,
                                        VkCommandBuffer* command_buffer,
                                        VkAccessFlags target_usage) {
  LOG_ASSERT(==, log_, 0, data_size % 4);
  size_t upload_offset = 0;
  while (upload_offset != data_size) {
    size_t upload_left = (data_size - upload_offset);
    size_t to_upload =
        upload_left < MAX_UPDATE_SIZE ? upload_left : MAX_UPDATE_SIZE;

    (*command_buffer)
        ->vkCmdUpdateBuffer(
            *command_buffer, *buffer, buffer_offset + upload_offset, to_upload,
            static_cast<const void*>(static_cast<const uint8_t*>(data) +
                                     upload_offset));
    upload_offset += to_upload;
  }

  VkBufferMemoryBarrier barrier = {
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // sType
      nullptr,                                  // pNext
      VK_ACCESS_HOST_WRITE_BIT,                 // srcAccessMask
      target_usage,                             // dstAccessMask
      VK_QUEUE_FAMILY_IGNORED,                  // srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,                  // dstQueueFamilyIndex
      *buffer,
      0,
      data_size};

  (*command_buffer)
      ->vkCmdPipelineBarrier(*command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr,
                             1, &barrier, 0, nullptr);
}

void VulkanApplication::FillHostVisibleBuffer(Buffer* buffer, const void* data,
                                              size_t data_size,
                                              size_t buffer_offset,
                                              VkCommandBuffer* command_buffer,
                                              VkAccessFlags dst_accesses,
                                              VkPipelineStageFlags dst_stages) {
  char* p = buffer->base_address();
  if (!p) {
    return;
  }
  const char* d = reinterpret_cast<const char*>(data);
  size_t size = buffer->size() < data_size ? buffer->size() : data_size;
  memcpy(p + buffer_offset, d, size);
  buffer->flush();
  if (command_buffer) {
    VkBufferMemoryBarrier buf_barrier{
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
        dst_accesses,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        *buffer,
        0,
        VK_WHOLE_SIZE};

    (*command_buffer)
        ->vkCmdPipelineBarrier(
            *command_buffer,
            VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
            dst_stages, 0, 0, nullptr, 1, &buf_barrier, 0, nullptr);
  }
}

bool VulkanApplication::DumpImageLayersData(
    Image* img, const VkImageSubresourceLayers& image_subresource,
    const VkOffset3D& image_offset, const VkExtent3D& image_extent,
    VkImageLayout initial_img_layout, containers::vector<uint8_t>* data,
    std::initializer_list<::VkSemaphore> wait_semaphores) {
  if (!img) {
    log_->LogError("DumpImageLayersData(): The given *img is nullptr");
    return false;
  }

  containers::vector<::VkSemaphore> waits(wait_semaphores, allocator_);
  containers::vector<VkPipelineStageFlags> wait_dst_stage_masks(
      waits.size(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, allocator_);

  // Prepare the dst buffer.
  size_t image_size = GetImageExtentSizeInBytes(image_extent, img->format()) *
                      image_subresource.layerCount;
  if (image_size == 0) {
    log_->LogError(
        "DumpImageLayersData(): The size of the dump source image layers is "
        "0, "
        "this might be caused by an unrecognized image format");
    return false;
  }

  data->reserve(image_size);
  VkBufferCreateInfo buf_create_info = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
      nullptr,                               // pNext
      0,                                     // createFlags
      image_size,                            // size
      VK_BUFFER_USAGE_TRANSFER_DST_BIT,      // usage
      VK_SHARING_MODE_EXCLUSIVE,             // sharingMode
      0,                                     // queueFamilyIndexCount
      nullptr                                // pQueueFamilyIndices
  };
  vulkan::BufferPointer dst_buffer = CreateAndBindHostBuffer(&buf_create_info);

  // Get a command buffer and add commands/barriers to it.
  VkCommandBuffer command_buffer = GetCommandBuffer();
  VkCommandBufferBeginInfo cmd_begin_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr};
  command_buffer->vkBeginCommandBuffer(command_buffer, &cmd_begin_info);

  // Add a buffer barrier to set the access bit to transfer write.
  VkBufferMemoryBarrier buffer_barrier{
      VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
      nullptr,
      0,  // Change to write access, no read-after-write risk
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      *dst_buffer,
      0,
      data->size(),
  };
  // Add an image barrier to change the layout and set its access bit to
  // transfer read.
  VkImageMemoryBarrier image_barrier{
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      nullptr,
      kAllWriteBits,
      VK_ACCESS_TRANSFER_READ_BIT,
      initial_img_layout,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      *img,
      // subresource range, only deal with one mip level
      {
          image_subresource.aspectMask, image_subresource.mipLevel, 1,
          image_subresource.baseArrayLayer, image_subresource.layerCount,
      }};
  command_buffer->vkCmdPipelineBarrier(
      command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &buffer_barrier, 1,
      &image_barrier);
  // Copy data from the image.
  VkBufferImageCopy copy_info{
      0, 0, 0, image_subresource, image_offset, image_extent};
  command_buffer->vkCmdCopyImageToBuffer(command_buffer, *img,
                                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                         *dst_buffer, 1, &copy_info);

  // Add a global barrier to make sure the data written to buffer is available
  // globally.
  VkMemoryBarrier end_barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
                              VK_ACCESS_TRANSFER_WRITE_BIT, kAllReadBits};
  command_buffer->vkCmdPipelineBarrier(
      command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &end_barrier, 0, nullptr, 0, nullptr);
  command_buffer->vkEndCommandBuffer(command_buffer);
  // Submit the command buffer.
  ::VkCommandBuffer raw_cmd_buf = command_buffer.get_command_buffer();
  VkSubmitInfo submit_info{
      VK_STRUCTURE_TYPE_SUBMIT_INFO,               // sType
      nullptr,                                     // pNext
      uint32_t(waits.size()),                      // waitSemaphoreCount
      waits.size() == 0 ? nullptr : waits.data(),  // pWaitSemaphores
      waits.size() == 0 ? nullptr
                        : wait_dst_stage_masks.data(),  // pWaitDstStageMask
      1,                                                // commandBufferCount
      &raw_cmd_buf,                                     // pCommandBuffers
      0,                                                // signalSemaphoreCount
      nullptr                                           // pSignalSemaphores
  };
  (*render_queue_)
      ->vkQueueSubmit(render_queue(), 1, &submit_info,
                      static_cast<::VkFence>(VK_NULL_HANDLE));
  (*render_queue_)->vkQueueWaitIdle(render_queue());
  // Copy the data from the buffer to |data|.
  dst_buffer->invalidate();
  std::for_each(dst_buffer->base_address(),
                dst_buffer->base_address() + image_size,
                [&data](uint8_t c) { data->push_back(c); });
  return true;
}

// These linked-list nodes are ordered by offset into the heap.
// the first node has a prev of nullptr, and the last node has a next of
// nullptr.
struct AllocationToken {
  AllocationToken* next;
  AllocationToken* prev;
  ::VkDeviceSize allocationSize;
  ::VkDeviceSize offset;
  // Location into the map of unused chunks. This is only valid when
  // in_use == false.
  containers::ordered_multimap<::VkDeviceSize, AllocationToken*>::iterator
      map_location;
  bool in_use;
};

VulkanArena::VulkanArena(containers::Allocator* allocator, logging::Logger* log,
                         ::VkDeviceSize buffer_size, uint32_t memory_type_index,
                         VkDevice* device, bool map)
    : allocator_(allocator),
      freeblocks_(allocator_),
      first_block_(nullptr),
      base_address_(nullptr),
      device_(*device),
      unmap_memory_function_(nullptr),
      memory_(VK_NULL_HANDLE, nullptr, device),
      log_(log) {
  // Actually allocate the bytes for this heap.
  VkMemoryAllocateInfo allocate_info{
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,  // sType
      nullptr,                                 // pNext
      buffer_size,                             // allocationSize
      memory_type_index};

  VkResult res = VK_SUCCESS;
  ::VkDeviceMemory device_memory;
  VkDeviceSize original_size = buffer_size;

  const auto& memory_properties = device->physical_device_memory_properties();

  log->LogInfo("Trying to allocate ", buffer_size, " bytes from heap that has ",
               memory_properties
                   .memoryHeaps[memory_properties.memoryTypes[memory_type_index]
                                    .heapIndex]
                   .size,
               " bytes.");

  do {
    if (res == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
      log->LogInfo("Could not allocate ", buffer_size,
                   " bytes of "
                   "device memory. Attempting to allocate ",
                   static_cast<size_t>(buffer_size * 0.75), " bytes instead");
      buffer_size =
          static_cast<VkDeviceSize>(static_cast<float>(buffer_size) * 0.75f);
      allocate_info.allocationSize = buffer_size;
    }

    res = (*device)->vkAllocateMemory(*device, &allocate_info, nullptr,
                                      &device_memory);
    // If we cannot even allocate 1/4 of the requested memory, it is time to
    // fail.
  } while (res == VK_ERROR_OUT_OF_DEVICE_MEMORY &&
           buffer_size > original_size / 4);
  LOG_ASSERT(==, log, VK_SUCCESS, res);
  memory_.initialize(device_memory);

  // Create a new pointer that is the first block of memory. It contains
  // all of the memory in the arena.
  first_block_ = allocator_->construct<AllocationToken>(AllocationToken{
      nullptr, nullptr, buffer_size, 0, freeblocks_.end(), false});

  // Since this has not been used yet, add it to our freeblock_ map.
  first_block_->map_location =
      freeblocks_.insert(std::make_pair(buffer_size, first_block_));

  if (map) {
    // If we were asked to map this memory. (i.e. it is meant to be host
    // visible), then do it now.
    LOG_ASSERT(
        ==, log, VK_SUCCESS,
        (*device)->vkMapMemory(*device, memory_, 0, buffer_size, 0,
                               reinterpret_cast<void**>(&base_address_)));
    // Store off the devices unmap memory function for the future, we only
    // want to keep a reference to the raw device, and not the
    // vulkan::VkDevice since vulkan::VkDevice is movable.
    unmap_memory_function_ = &(*device)->vkUnmapMemory;
  }
}

VulkanArena::~VulkanArena() {
  // Make sure that there is only one block left, and that is is not in use.
  // This will trigger if someone has not freed all the memory before the
  // heap has been destroyed.
  LOG_ASSERT(==, log_, true, first_block_->next == nullptr);
  LOG_ASSERT(==, log_, false, first_block_->in_use);
  if (base_address_) {
    (*unmap_memory_function_)(device_, memory_);
  }
  allocator_->destroy(first_block_);
}

// The maximum value for nonCoherentAtomSize from the vulkan spec.
// Table 31.2. Required Limits
// See 10.2.1. Host Access to Device Memory Objects for
// a description of why this must be used.
static const ::VkDeviceSize kMaxNonCoherentAtomSize = 256;

AllocationToken* VulkanArena::AllocateMemory(::VkDeviceSize size,
                                             ::VkDeviceSize alignment,
                                             ::VkDeviceMemory* memory,
                                             ::VkDeviceSize* offset,
                                             char** base_address) {
  // If we are mapped memory, then no matter what alignment says, we
  // must also be aligned to kMaxNonCoherentAtomSize AND
  // for all intents and purposes our size must be a multiple of
  // kMaxNonCoherentAtomSize
  if (base_address_) {
    alignment = alignment > kMaxNonCoherentAtomSize ? alignment
                                                    : kMaxNonCoherentAtomSize;
    if ((size % kMaxNonCoherentAtomSize) != 0) {
      size += (kMaxNonCoherentAtomSize - (size % kMaxNonCoherentAtomSize));
    }
  }

  // We use alignment - 1 quite a bit, so store it off here.
  const ::VkDeviceSize align_m_1 = alignment - 1;
  LOG_ASSERT(>, log_, alignment, 0);  // Alignment must be > 0
  LOG_ASSERT(==, log_, !(alignment & (align_m_1)),
             true);  // Alignment must be power of 2.

  // This is the maximum amount of memory we will potentially have to
  // allocate in order to satisfy the alignment.
  ::VkDeviceSize to_allocate = size + align_m_1;

  // Find a block that contains at LEAST enough memory for our allocation.
  auto it = freeblocks_.lower_bound(to_allocate);
  // Fail if there is not even a single free-block that can hold our
  // allocation.
  LOG_ASSERT(==, log_, true, freeblocks_.end() != it);

  AllocationToken* token = it->second;
  // Remove the block that we found from the freeblock map.
  freeblocks_.erase(it);

  // total_offset is the offset from the base of the entire arena to the
  // correctly aligned base inside of the given block.
  ::VkDeviceSize total_offset = (token->offset + (align_m_1)) & ~(align_m_1);
  // offset_from_start is the offset from the start of the block to
  // the alignment location.
  ::VkDeviceSize offset_from_start = total_offset - token->offset;

  // Our block may satisfy the alignment already, so only actually allocate
  // the amount of memory we need.
  // TODO(awoloszyn): If we find fragmentation to be a problem here, then
  //   eventually actually allocate the total. If we do not do this,
  //   then if we have (for example) a 128byte aligned block and we need
  //   4K of memory, we wont be able to re-use this block for another 4K
  //   allocation.
  ::VkDeviceSize total_allocated =
      to_allocate - (align_m_1 - offset_from_start);

  // Remove the memory from the block.
  // Push the block's base up by the allocated memory
  token->allocationSize -= total_allocated;
  token->offset += total_allocated;

  // Create a new block that contains the memory in question.
  AllocationToken* new_token = allocator_->construct<AllocationToken>(
      AllocationToken{nullptr, token->prev, total_allocated, total_offset,
                      freeblocks_.end(), true});

  if (token->allocationSize > 0) {
    // If there is still some space in this allocation, put it back, so we can
    // get more out of it later.
    token->map_location =
        freeblocks_.insert(std::make_pair(token->allocationSize, token));

    // Hook up all of our linked-list nodes.
    new_token->next = token;
    if (!token->prev) {
      first_block_ = new_token;
    } else {
      new_token->prev = token->prev;
      new_token->prev->next = new_token;
    }
    token->prev = new_token;
    new_token->next = token;
    if (first_block_ == token) {
      first_block_ = new_token;
    }
  } else {
    // token happens to now be an empty block. So let's not put it back.
    if (token->next) {
      new_token->next = token->next;
      token->next->prev = new_token;
    }
    if (token->prev) {
      token->prev->next = new_token;
    } else {
      first_block_ = new_token;
    }

    allocator_->destroy(token);
  }
  *memory = memory_;
  *offset = total_offset;
  if (base_address) {
    *base_address = base_address_ ? base_address_ + total_offset : nullptr;
  }
  return new_token;
}

void VulkanArena::FreeMemory(AllocationToken* token) {
  bool atAll = false;
  // First try to coalesce this with its previous block.
  while (token->prev && !token->prev->in_use) {
    atAll = true;
    // Take the previous token out of the map, and merge it with this one.
    AllocationToken* prev_token = token->prev;
    prev_token->allocationSize += token->allocationSize;
    prev_token->next = token->next;
    if (token->next) {
      token->next->prev = prev_token;
    }
    // Remove the previous block from freeblocks_,
    // we have now merged with it.
    freeblocks_.erase(prev_token->map_location);
    allocator_->destroy(token);
    token = prev_token;
  }
  // Now try to coalesce this with any subsequent blocks.
  while (token->next && !token->next->in_use) {
    atAll = true;
    // Take the previous token out of the map, and merge it with this one.
    AllocationToken* next_token = token->next;
    token->allocationSize += next_token->allocationSize;
    token->next = next_token->next;
    if (token->next) {
      token->next->prev = token;
    }
    // Remove the next block from freeblocks_,
    // we have now merged with it.
    freeblocks_.erase(next_token->map_location);
    allocator_->destroy(next_token);
  }
  // This block is no longer being used.
  token->in_use = false;
  // Push it back into freeblocks_.
  token->map_location =
      freeblocks_.insert(std::make_pair(token->allocationSize, token));
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(containers::Allocator* allocator,
                                               PipelineLayout* layout,
                                               VulkanApplication* application,
                                               VkRenderPass* render_pass,
                                               uint32_t subpass)
    : render_pass_(*render_pass),
      subpass_(subpass),
      application_(application),
      stages_(allocator),
      dynamic_states_(allocator),
      vertex_binding_descriptions_(allocator),
      vertex_attribute_descriptions_(allocator),
      shader_modules_(allocator),
      attachments_(allocator),
      layout_(*layout),
      contained_stages_(0),
      pipeline_(VK_NULL_HANDLE, nullptr, &application->device()) {
  MemoryClear(&vertex_input_state_);
  MemoryClear(&input_assembly_state_);
  MemoryClear(&tessellation_state_);
  MemoryClear(&viewport_state_);
  MemoryClear(&rasterization_state_);
  MemoryClear(&multisample_state_);
  MemoryClear(&depth_stencil_state_);
  MemoryClear(&color_blend_state_);
  MemoryClear(&dynamic_state_);
  MemoryClear(&viewport_);
  MemoryClear(&scissor_);

  dynamic_states_.resize(2);
  dynamic_states_[0] = VK_DYNAMIC_STATE_VIEWPORT;
  dynamic_states_[1] = VK_DYNAMIC_STATE_SCISSOR;

  vertex_input_state_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  input_assembly_state_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

  tessellation_state_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

  viewport_state_.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state_.viewportCount = 1;
  viewport_state_.pViewports = nullptr;
  viewport_state_.scissorCount = 1;
  viewport_state_.pScissors = nullptr;

  rasterization_state_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterization_state_.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization_state_.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization_state_.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterization_state_.lineWidth = 1.0f;

  multisample_state_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample_state_.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  depth_stencil_state_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_state_.depthTestEnable = VK_TRUE;
  depth_stencil_state_.depthWriteEnable = VK_TRUE;
  depth_stencil_state_.depthCompareOp = VK_COMPARE_OP_LESS;

  color_blend_state_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

  dynamic_state_.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
}

void VulkanGraphicsPipeline::SetCullMode(VkCullModeFlagBits mode) {
  rasterization_state_.cullMode = mode;
}

void VulkanGraphicsPipeline::SetFrontFace(VkFrontFace face) {
  rasterization_state_.frontFace = face;
}

void VulkanGraphicsPipeline::SetRasterizationFill(VkPolygonMode mode) {
  rasterization_state_.polygonMode = mode;
}

void VulkanGraphicsPipeline::AddShader(VkShaderStageFlagBits stage,
                                       const char* entry, uint32_t* code,
                                       uint32_t numCodeWords) {
  LOG_ASSERT(==, application_->GetLogger(), 0,
             static_cast<uint32_t>(stage) & contained_stages_);
  LOG_ASSERT(==, application_->GetLogger(), stage,
             stage & VK_SHADER_STAGE_ALL_GRAPHICS);
  contained_stages_ |= stage;
  VkShaderModuleCreateInfo create_info{
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,  // sType
      nullptr,                                      // pNext
      0,                                            // flags
      numCodeWords * 4,                             // codeSize
      code                                          // pCode
  };

  ::VkShaderModule module;
  LOG_ASSERT(==, application_->GetLogger(), VK_SUCCESS,
             application_->device()->vkCreateShaderModule(
                 application_->device(), &create_info, nullptr, &module));
  shader_modules_.push_back(
      VkShaderModule(module, nullptr, &application_->device()));

  stages_.push_back({
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,  // sType
      nullptr,                                              // pNext
      0,                                                    // flags
      stage,                                                // stage
      shader_modules_.back(),                               // module
      entry,                                                // name
      nullptr  // pSpecializationInfo
  });
}

void VulkanGraphicsPipeline::SetTopology(VkPrimitiveTopology topology,
                                         uint32_t patch_size) {
  input_assembly_state_.topology = topology;
  if ((contained_stages_ & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) ||
      (contained_stages_ & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)) {
    tessellation_state_.patchControlPoints = patch_size;
  }
}

void VulkanGraphicsPipeline::SetViewport(const VkViewport& viewport) {
  auto state = std::find(dynamic_states_.begin(), dynamic_states_.end(),
                         VK_DYNAMIC_STATE_VIEWPORT);
  if (state != dynamic_states_.end()) {
    dynamic_states_.erase(state);
  }
  viewport_ = viewport;
  viewport_state_.pViewports = &viewport_;
}

void VulkanGraphicsPipeline::SetScissor(const VkRect2D& scissor) {
  auto state = std::find(dynamic_states_.begin(), dynamic_states_.end(),
                         VK_DYNAMIC_STATE_SCISSOR);
  if (state != dynamic_states_.end()) {
    dynamic_states_.erase(state);
  }
  scissor_ = scissor;
  viewport_state_.pScissors = &scissor_;
}

void VulkanGraphicsPipeline::SetSamples(VkSampleCountFlagBits samples) {
  multisample_state_.rasterizationSamples = samples;
}

void VulkanGraphicsPipeline::AddAttachment() {
  attachments_.push_back(
      {VK_FALSE, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
       VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
       VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT});
}

void VulkanGraphicsPipeline::AddAttachment(
    const VkPipelineColorBlendAttachmentState& state) {
  attachments_.push_back(state);
}

void VulkanGraphicsPipeline::AddDynamicState(VkDynamicState dynamic_state) {
  dynamic_states_.push_back(dynamic_state);
}

void VulkanGraphicsPipeline::AddInputStream(
    uint32_t stride, VkVertexInputRate input_rate,
    std::initializer_list<InputStream> inputs) {
  vertex_binding_descriptions_.push_back(
      {static_cast<uint32_t>(vertex_binding_descriptions_.size()), stride,
       input_rate});
  for (auto input : inputs) {
    vertex_attribute_descriptions_.push_back(
        {input.binding,
         static_cast<uint32_t>(vertex_binding_descriptions_.size() - 1),
         input.format, input.offset});
  }
}

void VulkanGraphicsPipeline::SetInputStreams(VulkanModel* model) {
  model->GetAssemblyInfo(&vertex_binding_descriptions_,
                         &vertex_attribute_descriptions_);
}

void VulkanGraphicsPipeline::Commit() {
  vertex_input_state_.vertexBindingDescriptionCount =
      static_cast<uint32_t>(vertex_binding_descriptions_.size());
  vertex_input_state_.pVertexBindingDescriptions =
      vertex_binding_descriptions_.data();
  vertex_input_state_.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(vertex_attribute_descriptions_.size());
  vertex_input_state_.pVertexAttributeDescriptions =
      vertex_attribute_descriptions_.data();

  VkPipelineTessellationStateCreateInfo* info = nullptr;
  if ((contained_stages_ & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) ||
      (contained_stages_ & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)) {
    info = &tessellation_state_;
  }

  VkPipelineDynamicStateCreateInfo* dynamic_info = nullptr;
  if (!dynamic_states_.empty()) {
    dynamic_info = &dynamic_state_;
    dynamic_state_.dynamicStateCount =
        static_cast<uint32_t>(dynamic_states_.size());
    dynamic_state_.pDynamicStates = dynamic_states_.data();
  }

  color_blend_state_.attachmentCount =
      static_cast<uint32_t>(attachments_.size());
  color_blend_state_.pAttachments = attachments_.data();

  VkGraphicsPipelineCreateInfo create_info{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,  // sType
      nullptr,                                          // pNext
      0,                                                // flags
      static_cast<uint32_t>(stages_.size()),            // stageCount
      stages_.data(),                                   // pStage
      &vertex_input_state_,                             // pVertexInputState
      &input_assembly_state_,                           // pInputAssemblyState
      info,                                             // pTessellationState
      &viewport_state_,                                 // pViewportState
      &rasterization_state_,                            // pRasterizationState
      &multisample_state_,                              // pMultisampleState
      &depth_stencil_state_,                            // pDepthStencilState
      &color_blend_state_,                              // pColorBlendState
      dynamic_info,                                     // pDynamicState
      layout_,                                          // layout
      render_pass_,                                     // renderPass
      subpass_,                                         // subpass
      VK_NULL_HANDLE,                                   // basePipelineHandle
      0                                                 // basePipelineIndex
  };
  ::VkPipeline pipeline;
  LOG_ASSERT(==, application_->GetLogger(), VK_SUCCESS,
             application_->device()->vkCreateGraphicsPipelines(
                 application_->device(), application_->pipeline_cache(), 1,
                 &create_info, nullptr, &pipeline));
  pipeline_.initialize(pipeline);
}

VulkanComputePipeline::VulkanComputePipeline(
    containers::Allocator* allocator, PipelineLayout* layout,
    VulkanApplication* application,
    const VkShaderModuleCreateInfo& shader_module_create_info,
    const char* shader_entry, const VkSpecializationInfo* specialization_info)
    : application_(application),
      pipeline_(VK_NULL_HANDLE, nullptr, &application->device()),
      shader_module_(VK_NULL_HANDLE, nullptr, &application->device()),
      layout_(*layout) {
  ::VkShaderModule raw_module;
  LOG_ASSERT(==, application_->GetLogger(), VK_SUCCESS,
             application_->device()->vkCreateShaderModule(
                 application_->device(), &shader_module_create_info, nullptr,
                 &raw_module));
  shader_module_.initialize(raw_module);
  VkPipelineShaderStageCreateInfo shader_stage_create_info{
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,  // sType
      nullptr,                                              // pNext
      0,                                                    // flags
      VK_SHADER_STAGE_COMPUTE_BIT,                          // stage
      shader_module_,                                       // module
      shader_entry,                                         // name
      specialization_info                                   // pSpecializationInfo
  };

  VkComputePipelineCreateInfo pipeline_create_info{
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,  // sType
      nullptr,                                         // pNext
      0,                                               // flags
      shader_stage_create_info,                        // stage
      layout_,                                         // layout
      VK_NULL_HANDLE,                                  // basePipelineHandle
      0,                                               // basePipelineIndex
  };

  ::VkPipeline pipeline;
  LOG_ASSERT(==, application_->GetLogger(), VK_SUCCESS,
             application_->device()->vkCreateComputePipelines(
                 application_->device(), application_->pipeline_cache(), 1,
                 &pipeline_create_info, nullptr, &pipeline));
  pipeline_.initialize(pipeline);
}

::VkDeviceSize VulkanApplication::Image::size() const {
  return token_ ? token_->allocationSize : 0u;
}
}  // namespace vulkan
