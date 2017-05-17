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

COLOR_VALUE = [
    ("float32", ARRAY, 4, FLOAT)
]

SUBRESOURCE_RANGE = [
    ("aspectMask", UINT32_T),
    ("baseMipLevel", UINT32_T),
    ("levelCount", UINT32_T),
    ("baseArrayLayer", UINT32_T),
    ("layerCount", UINT32_T),
]


@gapit_test("vkCmdClearColorImage_test")
class Clear2DColorImageSingleLayerSingleLevel(GapitTest):
    def expect(self):
        """1. Expects vkCmdClearColorImage() is called and traced successfully."""
        architecture = self.architecture
        clear_color_image = require(self.nth_call_of("vkCmdClearColorImage", 1))
        require_not_equal(0, clear_color_image.int_commandBuffer)
        require_not_equal(0, clear_color_image.int_image)
        require_equal(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clear_color_image.int_imageLayout)
        require_not_equal(0, clear_color_image.hex_pColor)
        require_equal(1, clear_color_image.int_rangeCount)
        require_not_equal(0, clear_color_image.hex_pRanges)

        color = VulkanStruct(
            architecture, COLOR_VALUE, get_read_offset_function(
                clear_color_image, clear_color_image.hex_pColor))
        require_true(
            array('f', [0.2 for i in range(4)]) ==
            array('f', color.float32))

        subresource_range = VulkanStruct(
            architecture, SUBRESOURCE_RANGE, get_read_offset_function(
                clear_color_image, clear_color_image.hex_pRanges))
        require_equal(VK_IMAGE_ASPECT_COLOR_BIT, subresource_range.aspectMask)
        require_equal(0, subresource_range.baseMipLevel)
        require_equal(1, subresource_range.levelCount)
        require_equal(0, subresource_range.baseArrayLayer)
        require_equal(1, subresource_range.layerCount)
