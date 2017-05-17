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

IMAGE_BLIT = [
    ("srcSubresource_aspectMask", UINT32_T),
    ("srcSubresource_mipLevel", UINT32_T),
    ("srcSubresource_baseArrayLayer", UINT32_T),
    ("srcSubresource_layerCount", UINT32_T),
    ("srcOffset_0_x", UINT32_T),
    ("srcOffset_0_y", UINT32_T),
    ("srcOffset_0_z", UINT32_T),
    ("srcOffset_1_x", UINT32_T),
    ("srcOffset_1_y", UINT32_T),
    ("srcOffset_1_z", UINT32_T),
    ("dstSubresource_aspectMask", UINT32_T),
    ("dstSubresource_mipLevel", UINT32_T),
    ("dstSubresource_baseArrayLayer", UINT32_T),
    ("dstSubresource_layerCount", UINT32_T),
    ("dstOffset_0_x", UINT32_T),
    ("dstOffset_0_y", UINT32_T),
    ("dstOffset_0_z", UINT32_T),
    ("dstOffset_1_x", UINT32_T),
    ("dstOffset_1_y", UINT32_T),
    ("dstOffset_1_z", UINT32_T),
]


@gapit_test("vkCmdBlitImage_test")
class BlitToSameColorImageWithLinearFilter(GapitTest):

    def expect(self):
        """Check the arguments to vkCmdBlitImage"""
        architecture = self.architecture
        blit_image = require(self.nth_call_of("vkCmdBlitImage", 1))

        require_not_equal(0, blit_image.int_commandBuffer)
        require_not_equal(0, blit_image.int_srcImage)
        require_equal(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      blit_image.int_srcImageLayout)
        require_not_equal(0, blit_image.int_dstImage)
        require_equal(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      blit_image.int_dstImageLayout)
        require_equal(1, blit_image.int_regionCount)
        require_equal(VK_FILTER_LINEAR, blit_image.int_filter)

        image_blit = VulkanStruct(architecture, IMAGE_BLIT,
                                  get_read_offset_function(
                                      blit_image, blit_image.hex_pRegions))

        require_equal(VK_IMAGE_ASPECT_COLOR_BIT,
                      image_blit.srcSubresource_aspectMask)
        require_equal(0, image_blit.srcSubresource_mipLevel)
        require_equal(0, image_blit.srcSubresource_baseArrayLayer)
        require_equal(1, image_blit.srcSubresource_layerCount)
        require_equal(0, image_blit.srcOffset_0_x)
        require_equal(0, image_blit.srcOffset_0_y)
        require_equal(0, image_blit.srcOffset_0_z)
        require_equal(32, image_blit.srcOffset_1_x)
        require_equal(32, image_blit.srcOffset_1_y)
        require_equal(1, image_blit.srcOffset_1_z)
        require_equal(VK_IMAGE_ASPECT_COLOR_BIT,
                      image_blit.dstSubresource_aspectMask)
        require_equal(0, image_blit.dstSubresource_mipLevel)
        require_equal(0, image_blit.dstSubresource_baseArrayLayer)
        require_equal(1, image_blit.dstSubresource_layerCount)
        require_equal(0, image_blit.dstOffset_0_x)
        require_equal(0, image_blit.dstOffset_0_y)
        require_equal(0, image_blit.dstOffset_0_z)
        require_equal(32, image_blit.dstOffset_1_x)
        require_equal(32, image_blit.dstOffset_1_y)
        require_equal(1, image_blit.dstOffset_1_z)
