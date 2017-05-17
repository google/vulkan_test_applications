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

from gapit_test_framework import gapit_test, require, require_equal, require_true
from gapit_test_framework import require_not_equal, little_endian_bytes_to_int
from gapit_test_framework import GapitTest, get_read_offset_function
import gapit_test_framework
from struct_offsets import VulkanStruct, UINT32_T, FLOAT, ARRAY
from vulkan_constants import *

from array import array

DEPTH_STENCIL_VALUE = [("depth", FLOAT), ("stencil", UINT32_T),]

SUBRESOURCE_RANGE = [
    ("aspectMask", UINT32_T),
    ("baseMipLevel", UINT32_T),
    ("levelCount", UINT32_T),
    ("baseArrayLayer", UINT32_T),
    ("layerCount", UINT32_T),
]


@gapit_test("vkCmdClearDepthStencilImage_test")
class ClearDepthImage(GapitTest):

    def expect(self):
        """1. Expects vkCmdClearDepthStencilImage() is called and traced
        successfully."""
        architecture = self.architecture
        clear_depth_image = require(self.nth_call_of(
            "vkCmdClearDepthStencilImage", 1))
        require_not_equal(0, clear_depth_image.int_commandBuffer)
        require_not_equal(0, clear_depth_image.int_image)
        require_equal(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      clear_depth_image.int_imageLayout)
        require_not_equal(0, clear_depth_image.hex_pDepthStencil)
        require_equal(1, clear_depth_image.int_rangeCount)
        require_not_equal(0, clear_depth_image.hex_pRanges)

        depth_stencil = VulkanStruct(
            architecture, DEPTH_STENCIL_VALUE, get_read_offset_function(
                clear_depth_image, clear_depth_image.hex_pDepthStencil))
        # float comparison
        require_true(array('f', [0.2]) == array('f', [depth_stencil.depth]))
        require_equal(1, depth_stencil.stencil)

        subresource_range = VulkanStruct(
            architecture, SUBRESOURCE_RANGE, get_read_offset_function(
                clear_depth_image, clear_depth_image.hex_pRanges))
        require_equal(VK_IMAGE_ASPECT_DEPTH_BIT, subresource_range.aspectMask)
        require_equal(0, subresource_range.baseMipLevel)
        require_equal(1, subresource_range.levelCount)
        require_equal(0, subresource_range.baseArrayLayer)
        require_equal(1, subresource_range.layerCount)
