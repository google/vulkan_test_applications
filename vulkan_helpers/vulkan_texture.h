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

#ifndef VULKAN_HELPERS_VULKAN_TEXTURE_H_
#define VULKAN_HELPERS_VULKAN_TEXTURE_H_

#include "support/containers/allocator.h"
#include "support/containers/vector.h"
#include "support/log/log.h"
#include "vulkan_helpers/vulkan_application.h"

#include <initializer_list>

namespace vulkan {
// TODO(awoloszyn): Handle MIP chains.
// TODO(awoloszyn): Handle Arrays.
// TODO(awoloszyn): Handle cube-maps.
struct VulkanTexture {
 public:
  // A standard VulkanTexture object. It is expected to be used with the
  // output from convert_obj_to_c.py. That is, positions, texture_coords
  // and normals are expected to be sequential in memory. If a non-zero
  // |sparse_binding_block_size| is given, the texture image will be sparsely
  // bound with the given block size aligned to the image's memory alignment.
  VulkanTexture(containers::Allocator* allocator, logging::Logger* logger,
                VkFormat format, size_t width, size_t height, const void* data,
                size_t data_size, size_t sparse_binding_block_size = 0u,
                size_t multiplanar_plane_count = 0u,
                size_t downsampled_width = 0u, size_t downsampled_height = 0u)
      : allocator_(allocator),
        logger_(logger),
        format_(format),
        width_(width),
        height_(height),
        data_(data),
        data_size_(data_size),
        sparse_binding_block_size_(sparse_binding_block_size),
        multiplanar_plane_count_(multiplanar_plane_count),
        downsampled_width_(downsampled_width),
        downsampled_height_(downsampled_height),
        image_(nullptr) {}

  // Constructs a vulkan model from the output of the convert_img_to_c.py
  // script.
  template <typename T>
  VulkanTexture(containers::Allocator* allocator, logging::Logger* logger,
                const T& t, size_t sparse_binding_block_size = 0u,
                size_t multiplanar_plane_count = 0u,
                size_t downsampled_width = 0u,
                size_t downsampled_height = 0u)
      : VulkanTexture(allocator, logger, t.format, t.width, t.height,
                      static_cast<const void*>(t.data), sizeof(t.data),
                      sparse_binding_block_size, multiplanar_plane_count,
                      downsampled_width, downsampled_height) {}

  // Creates the image object.
  // Also creates a temporary buffer object for the upload data
  // If this image has already been initialized, then this re-initializes it.
  // Returns the temporary buffer used to upload the image. This should/can
  // be safely deleted once the given command buffer has executed.
  // The image is transitioned into "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL"
  // during the upload operation.
  void InitializeData(vulkan::VulkanApplication* application,
                      vulkan::VkCommandBuffer* cmdBuffer,
                      VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
                      VkImageCreateFlags flags = 0,
                      void* pNext = nullptr) {
    VkBufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,  // sType
        nullptr,                               // pNext
        0,                                     // flags
        data_size_,                            // size
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,      // usage
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr};
    upload_buffer_ = application->CreateAndBindHostBuffer(&create_info);
    void* copy_base = upload_buffer_->base_address();
    memcpy(copy_base, data_, data_size_);
    upload_buffer_->flush();

    VkImageCreateInfo image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,  // sType
        pNext,                                // pNext
        flags,                                // flags
        VK_IMAGE_TYPE_2D,                     // type
        format_,                              // format
        {static_cast<uint32_t>(width_), static_cast<uint32_t>(height_),
         1},                                      // Extent
        1,                                        // mipLevels
        1,                                        // arrayLayers
        VK_SAMPLE_COUNT_1_BIT,                    // sampleCount
        VK_IMAGE_TILING_OPTIMAL,                  // tiling
        usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT,  // usage
        VK_SHARING_MODE_EXCLUSIVE,                // sharingMode
        0,                                        // queueFamilyIndexCount
        nullptr,                                  // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED                 // initialLayout
    };
    if (sparse_binding_block_size_ > 0u) {
      image_create_info.flags =
          image_create_info.flags | VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
      sparse_image_ = application->CreateAndBindSparseImage(&image_create_info,
          sparse_binding_block_size_);
    } else if (IsFormatMultiplanar(format_)) {
      VkSamplerYcbcrConversionImageFormatProperties ycbcr_conversion_properties{
          VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES_KHR,
          NULL, 0};
      VkImageFormatProperties2 image_format_properties2{
          VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
          &ycbcr_conversion_properties,
          {}};
      VkPhysicalDeviceImageFormatInfo2 physical_format_properties{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
          NULL,
          format_,
          VkImageType::VK_IMAGE_TYPE_2D,
          VkImageTiling::VK_IMAGE_TILING_LINEAR,
          VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT,
          0};
      application->instance()->vkGetPhysicalDeviceImageFormatProperties2(
          application->device().physical_device(), &physical_format_properties,
          &image_format_properties2);

      multiplanar_plane_count_ =
          ycbcr_conversion_properties.combinedImageSamplerDescriptorCount;

      downsampled_width_ = width_;
      downsampled_height_ = height_;
      if (FormatDownsamplesWidth(format_)) {
        downsampled_width_ = width_ / 2;
      } else if (FormatDownsamplesWidthAndHeight(format_)) {
        downsampled_width_ = width_ / 2;
        downsampled_height_ = height_ / 2;
      }

      image_create_info.pNext = &ycbcr_conversion_properties;
      image_ = application->CreateAndBindMultiPlanarImage(&image_create_info);
    }
    else {
      image_ = application->CreateAndBindImage(&image_create_info);
    }

    VkImageViewCreateInfo view_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        pNext,                                     // pNext
        0,                                         // flags
        image(),                                   // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        format_,                                   // format
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
         VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    ::VkImageView raw_view;
    LOG_ASSERT(
        ==, logger_, VK_SUCCESS,
        application->device()->vkCreateImageView(
            application->device(), &view_create_info, nullptr, &raw_view));
    image_view_ = containers::make_unique<vulkan::VkImageView>(
        allocator_,
        vulkan::VkImageView(raw_view, nullptr, &application->device()));

    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
        nullptr,                                 // pNext
        0,                                       // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,            // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,               // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,    // newLayout
        VK_QUEUE_FAMILY_IGNORED,                 // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                 // dstQueueFamilyIndex
        image(),                                 // image
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
    VkBufferMemoryBarrier buffer_barrier = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,  // sType
        nullptr,                                  // pNext
        VK_ACCESS_HOST_WRITE_BIT,                 // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,              // dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                  // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                  // dstQueueFamilyIndex
        *upload_buffer_,                          // buffer
        0,                                        // offset
        data_size_,                               // size
    };

    (*cmdBuffer)
        ->vkCmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                               VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
                               &buffer_barrier, 1, &barrier);

    if (multiplanar_plane_count_ > 1) {
      VkBufferImageCopy copy_params[3] = {
          {
              0,                                       // bufferOffset
              0,                                       // bufferRowLength
              0,                                       // bufferImageHeight
              {VK_IMAGE_ASPECT_PLANE_0_BIT, 0, 0, 1},  // imageSubresource
              {0, 0, 0},                               // offset
              {static_cast<uint32_t>(width_), static_cast<uint32_t>(height_),
               1}  // extent
          },
          {
              width_ * height_,                        // bufferOffset
              0,                                       // bufferRowLength
              0,                                       // bufferImageHeight
              {VK_IMAGE_ASPECT_PLANE_1_BIT, 0, 0, 1},  // imageSubresource
              {0, 0, 0},                               // offset
              {static_cast<uint32_t>(downsampled_width_),
               static_cast<uint32_t>(downsampled_height_), 1}  // extent
          },
          {
              width_ * height_ +
                  downsampled_width_ * downsampled_height_,  // bufferOffset
              0,                                             // bufferRowLength
              0,                                       // bufferImageHeight
              {VK_IMAGE_ASPECT_PLANE_2_BIT, 0, 0, 1},  // imageSubresource
              {0, 0, 0},                               // offset
              {static_cast<uint32_t>(downsampled_width_),
               static_cast<uint32_t>(downsampled_height_), 1}  // extent
          },
      };

      (*cmdBuffer)
          ->vkCmdCopyBufferToImage(*cmdBuffer, *upload_buffer_, image(),
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   multiplanar_plane_count_, copy_params);
    } else {
      VkBufferImageCopy copy_params = {
          0,                                     // bufferOffset
          0,                                     // bufferRowLength
          0,                                     // bufferImageHeight
          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},  // imageSubresource
          {0, 0, 0},                             // offset
          {static_cast<uint32_t>(width_), static_cast<uint32_t>(height_),
           1}  // extent
      };

      (*cmdBuffer)
          ->vkCmdCopyBufferToImage(*cmdBuffer, *upload_buffer_, image(),
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 3,
                                   &copy_params);
    }
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    (*cmdBuffer)
        ->vkCmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                               VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0,
                               nullptr, 0, nullptr, 1, &barrier);
  }

  // When the initialiation is complete, call this method, and the temporary
  // buffer will be released.
  void InitializationComplete() { upload_buffer_.reset(); }

  ::VkImage image() const {
    return image_ != nullptr ? ::VkImage(*image_) : ::VkImage(*sparse_image_);
  }
  ::VkImageView view() const { return *image_view_; }

  // Return true if format is multiplanar
  bool IsFormatMultiplanar(VkFormat format) {
    return (format_ >= VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM &&
            format_ <= VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM) ||
           (format_ >= VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 &&
            format_ <= VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16) ||
           (format_ >= VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 &&
            format_ <= VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16) ||
           (format_ >= VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM &&
            format_ <= VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM);
  }

  // Return true if format downsamples width
  bool FormatDownsamplesWidth(VkFormat format) {
    return format_ == VK_FORMAT_G8B8G8R8_422_UNORM ||
           format_ == VK_FORMAT_B8G8R8G8_422_UNORM ||
           format_ == VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM ||
           format_ == VK_FORMAT_G8_B8R8_2PLANE_422_UNORM ||
           format_ == VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 ||
           format_ == VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 ||
           format_ == VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 ||
           format_ == VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 ||
           format_ == VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 ||
           format_ == VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 ||
           format_ == VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 ||
           format_ == VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 ||
           format_ == VK_FORMAT_G16B16G16R16_422_UNORM ||
           format_ == VK_FORMAT_B16G16R16G16_422_UNORM ||
           format_ == VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM ||
           format_ == VK_FORMAT_G16_B16R16_2PLANE_422_UNORM;
  }

  // Return true if format downsamples width and height
  bool FormatDownsamplesWidthAndHeight(VkFormat format) {
    return format_ == VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM ||
           format_ == VK_FORMAT_G8_B8R8_2PLANE_420_UNORM ||
           format_ == VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 ||
           format_ == VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 ||
           format_ == VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 ||
           format_ == VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 ||
           format_ == VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM ||
           format_ == VK_FORMAT_G16_B16R16_2PLANE_420_UNORM;
  }

 private:
  VkFormat format_;
  size_t width_;
  size_t height_;
  const void* data_;
  const size_t data_size_;
  containers::Allocator* allocator_;
  logging::Logger* logger_;
  size_t sparse_binding_block_size_;
  size_t multiplanar_plane_count_;
  size_t downsampled_width_;
  size_t downsampled_height_;

  containers::unique_ptr<vulkan::VulkanApplication::Buffer> upload_buffer_;
  containers::unique_ptr<vulkan::VulkanApplication::Image> image_;
  containers::unique_ptr<vulkan::VulkanApplication::SparseImage> sparse_image_;
  containers::unique_ptr<vulkan::VkImageView> image_view_;
};

}  // namespace vulkan
#endif  // VULKAN_HELPERS_VULKAN_TEXTURE_H_
