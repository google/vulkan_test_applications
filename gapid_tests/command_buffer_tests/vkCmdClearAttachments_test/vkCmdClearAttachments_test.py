# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from gapit_test_framework import gapit_test, require, require_equal, require_true
from gapit_test_framework import require_not_equal
from gapit_test_framework import GapitTest, get_read_offset_function
from vulkan_constants import VK_IMAGE_ASPECT_COLOR_BIT
from struct_offsets import VulkanStruct, ARRAY, FLOAT, UINT32_T

from array import array

CLEAR_RECT = [
    ("rect_offset_x", UINT32_T),
    ("rect_offset_y", UINT32_T),
    ("rect_extent_width", UINT32_T),
    ("rect_extent_height", UINT32_T),
    ("baseArrayLayer", UINT32_T),
    ("layerCount", UINT32_T),
]

CLEAR_ATTACHMENT = [
    ("aspectMask", UINT32_T),
    ("colorAttachment", UINT32_T),
    ("clearValue_color_float32", ARRAY, 4, FLOAT),
]


@gapit_test("vkCmdClearAttachments_test")
class ClearASingleLayerColorAttachment(GapitTest):

    def expect(self):
        """Expect the vkCmdClearAttachments() is called successfully. The
        pAttachments and pRects point to the expected values"""
        architecture = self.architecture

        cmd_clear_attachments = require(self.nth_call_of(
            "vkCmdClearAttachments", 1))
        require_not_equal(0, cmd_clear_attachments.int_commandBuffer)
        require_equal(1, cmd_clear_attachments.int_attachmentCount)
        require_not_equal(0, cmd_clear_attachments.hex_pAttachments)
        require_equal(1, cmd_clear_attachments.int_rectCount)
        require_not_equal(0, cmd_clear_attachments.hex_pRects)

        clear_rect = VulkanStruct(
            architecture, CLEAR_RECT, get_read_offset_function(
                cmd_clear_attachments, cmd_clear_attachments.hex_pRects))

        require_equal(0, clear_rect.rect_offset_x)
        require_equal(0, clear_rect.rect_offset_y)
        require_equal(32, clear_rect.rect_extent_width)
        require_equal(32, clear_rect.rect_extent_height)
        require_equal(0, clear_rect.baseArrayLayer)
        require_equal(1, clear_rect.layerCount)

        clear_attachment = VulkanStruct(
            architecture, CLEAR_ATTACHMENT, get_read_offset_function(
                cmd_clear_attachments, cmd_clear_attachments.hex_pAttachments))
        require_equal(VK_IMAGE_ASPECT_COLOR_BIT, clear_attachment.aspectMask)
        require_equal(0, clear_attachment.colorAttachment)
        require_true(
            array('f', [0.2 for i in range(4)]) ==
            array('f', clear_attachment.clearValue_color_float32))
