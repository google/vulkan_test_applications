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

IMAGE_RESOLVE = [
    ("srcSubresource_aspectMask", UINT32_T),
    ("srcSubresource_mipLevel", UINT32_T),
    ("srcSubresource_baseArrayLayer", UINT32_T),
    ("srcSubresource_layerCount", UINT32_T),
    ("srcOffset_x", UINT32_T),
    ("srcOffset_y", UINT32_T),
    ("srcOffset_z", UINT32_T),
    ("dstSubresource_aspectMask", UINT32_T),
    ("dstSubresource_mipLevel", UINT32_T),
    ("dstSubresource_baseArrayLayer", UINT32_T),
    ("dstSubresource_layerCount", UINT32_T),
    ("dstOffset_x", UINT32_T),
    ("dstOffset_y", UINT32_T),
    ("dstOffset_z", UINT32_T),
    ("extent_width", UINT32_T),
    ("extent_height", UINT32_T),
    ("extent_depth", UINT32_T),
]


@gapit_test("vkCmdResolveImage_test")
class ResolveFrom2DOptimalTiling4XMultiSampledColorImage(GapitTest):

    def expect(self):
        """Check the arguments to vkCmdResolveImage"""
        architecture = self.architecture
        resolve_image = require(self.nth_call_of("vkCmdResolveImage", 1))

        require_not_equal(0, resolve_image.int_commandBuffer)
        require_not_equal(0, resolve_image.int_srcImage)
        require_equal(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      resolve_image.int_srcImageLayout)
        require_not_equal(0, resolve_image.int_dstImage)
        require_equal(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      resolve_image.int_dstImageLayout)
        require_equal(1, resolve_image.int_regionCount)

        image_resolve = VulkanStruct(
            architecture, IMAGE_RESOLVE, get_read_offset_function(
                resolve_image, resolve_image.hex_pRegions))

        require_equal(VK_IMAGE_ASPECT_COLOR_BIT,
                      image_resolve.srcSubresource_aspectMask)
        require_equal(0, image_resolve.srcSubresource_mipLevel)
        require_equal(0, image_resolve.srcSubresource_baseArrayLayer)
        require_equal(1, image_resolve.srcSubresource_layerCount)
        require_equal(0, image_resolve.srcOffset_x)
        require_equal(0, image_resolve.srcOffset_y)
        require_equal(0, image_resolve.srcOffset_z)
        require_equal(VK_IMAGE_ASPECT_COLOR_BIT,
                      image_resolve.dstSubresource_aspectMask)
        require_equal(0, image_resolve.dstSubresource_mipLevel)
        require_equal(0, image_resolve.dstSubresource_baseArrayLayer)
        require_equal(1, image_resolve.dstSubresource_layerCount)
        require_equal(0, image_resolve.dstOffset_x)
        require_equal(0, image_resolve.dstOffset_y)
        require_equal(0, image_resolve.dstOffset_z)
        require_equal(32, image_resolve.extent_width)
        require_equal(32, image_resolve.extent_height)
        require_equal(1, image_resolve.extent_depth)
