# Copyright 2017 Google Inc.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from gapit_test_framework import gapit_test, require, require_equal
from gapit_test_framework import require_not_equal, little_endian_bytes_to_int
from gapit_test_framework import GapitTest, get_read_offset_function
import gapit_test_framework
from struct_offsets import VulkanStruct, UINT32_T, SIZE_T, POINTER
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY, DEVICE_SIZE, INT32_T
from vulkan_constants import *

BUFFER_IMAGE_COPY = [
    ("bufferOffset", DEVICE_SIZE),
    ("bufferRowLength", UINT32_T),
    ("bufferImageHeight", UINT32_T),
    ("imageSubresource_aspectMask", UINT32_T),
    ("imageSubresource_mipLevel", UINT32_T),
    ("imageSubresource_baseArrayLayer", UINT32_T),
    ("imageSubresource_layerCount", UINT32_T),
    ("imageOffset_x", INT32_T),
    ("imageOffset_y", INT32_T),
    ("imageOffset_z", INT32_T),
    ("imageExtent_width", UINT32_T),
    ("imageExtent_height", UINT32_T),
    ("imageExtent_depth", UINT32_T)
]


@gapit_test("BufferImageCopy_test")
class CopyImageToBuffer(GapitTest):

    def expect(self):
        """Check the arguments to vkCmdCopyImageToBuffer"""
        architecture = self.architecture
        copy_image_to_buffer = require(
            self.nth_call_of("vkCmdCopyImageToBuffer", 1))

        require_not_equal(0, copy_image_to_buffer.int_commandBuffer)
        require_not_equal(0, copy_image_to_buffer.int_srcImage)
        require_equal(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      copy_image_to_buffer.int_srcImageLayout)
        require_not_equal(0, copy_image_to_buffer.int_dstBuffer)
        require_equal(1, copy_image_to_buffer.int_regionCount)
        require_not_equal(0, copy_image_to_buffer.hex_pRegions)

        buffer_image_copy = VulkanStruct(
            architecture, BUFFER_IMAGE_COPY,
            get_read_offset_function(copy_image_to_buffer,
                                     copy_image_to_buffer.hex_pRegions))

        require_equal(0, buffer_image_copy.bufferOffset)
        require_equal(0, buffer_image_copy.bufferRowLength)
        require_equal(0, buffer_image_copy.bufferImageHeight)
        require_equal(VK_IMAGE_ASPECT_COLOR_BIT,
                      buffer_image_copy.imageSubresource_aspectMask)
        require_equal(0, buffer_image_copy.imageSubresource_mipLevel)
        require_equal(0, buffer_image_copy.imageSubresource_baseArrayLayer)
        require_equal(1, buffer_image_copy.imageSubresource_layerCount)
        require_equal(0, buffer_image_copy.imageOffset_x)
        require_equal(0, buffer_image_copy.imageOffset_y)
        require_equal(0, buffer_image_copy.imageOffset_z)
        require_equal(32, buffer_image_copy.imageExtent_width)
        require_equal(32, buffer_image_copy.imageExtent_height)
        require_equal(1, buffer_image_copy.imageExtent_depth)


@gapit_test("BufferImageCopy_test")
class CopyBufferToImage(GapitTest):

    def expect(self):
        """Check the arguments to vkCmdCopyBufferToImage"""
        architecture = self.architecture
        copy_buffer_to_image = require(
            self.nth_call_of("vkCmdCopyBufferToImage", 1))

        require_not_equal(0, copy_buffer_to_image.int_commandBuffer)
        require_not_equal(0, copy_buffer_to_image.int_srcBuffer)
        require_not_equal(0, copy_buffer_to_image.int_dstImage)
        require_equal(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      copy_buffer_to_image.int_dstImageLayout)
        require_equal(1, copy_buffer_to_image.int_regionCount)
        require_not_equal(0, copy_buffer_to_image.hex_pRegions)

        buffer_image_copy = VulkanStruct(
            architecture, BUFFER_IMAGE_COPY,
            get_read_offset_function(copy_buffer_to_image,
                                     copy_buffer_to_image.hex_pRegions))

        require_equal(0, buffer_image_copy.bufferOffset)
        require_equal(0, buffer_image_copy.bufferRowLength)
        require_equal(0, buffer_image_copy.bufferImageHeight)
        require_equal(VK_IMAGE_ASPECT_COLOR_BIT,
                      buffer_image_copy.imageSubresource_aspectMask)
        require_equal(0, buffer_image_copy.imageSubresource_mipLevel)
        require_equal(0, buffer_image_copy.imageSubresource_baseArrayLayer)
        require_equal(1, buffer_image_copy.imageSubresource_layerCount)
        require_equal(0, buffer_image_copy.imageOffset_x)
        require_equal(0, buffer_image_copy.imageOffset_y)
        require_equal(0, buffer_image_copy.imageOffset_z)
        require_equal(32, buffer_image_copy.imageExtent_width)
        require_equal(32, buffer_image_copy.imageExtent_height)
        require_equal(1, buffer_image_copy.imageExtent_depth)
