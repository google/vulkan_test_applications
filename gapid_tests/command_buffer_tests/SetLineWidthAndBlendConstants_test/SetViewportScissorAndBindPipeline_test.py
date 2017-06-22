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
from gapit_test_framework import GapidUnsupportedException
from struct_offsets import VulkanStruct, UINT32_T, SIZE_T, POINTER, INT32_T
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY
from vulkan_constants import *


@gapit_test("SetLineWidthAndBlendConstants_test")
class SetBlendConstants(GapitTest):

    def expect(self):
        set_blend_constants = require(self.next_call_of(
            "vkCmdSetBlendConstants"))

        require_not_equal(0, set_blend_constants.int_commandBuffer)
        require_equal([1.25, 2.5, 5.0, 10.0],
                      set_blend_constants.float_blendConstants)


@gapit_test("SetLineWidthAndBlendConstants_test")
class SetLineWidthOnePointZero(GapitTest):

    def expect(self):
        """The first call to vkCmdSetLineWidth should have line width value: 1.0
        """
        set_line_width = require(self.nth_call_of("vkCmdSetLineWidth", 1))

        require_not_equal(0, set_line_width.int_commandBuffer)
        require_equal(1.0, set_line_width.float_lineWidth)


@gapit_test("SetLineWidthWideLines_test")
class SetLineWidthTwoPointZero(GapitTest):

    def expect(self):
        """The second call to vkCmdSetLineWidth should have line width value: 2.0
        """
        create_device = self.nth_call_of("vkCreateDevice", 1)
        if create_device[0] is None:
            raise GapidUnsupportedException(
                "physical device feature: wideLine not supported")

        set_line_width = require(self.nth_call_of("vkCmdSetLineWidth", 1))

        require_not_equal(0, set_line_width.int_commandBuffer)
        require_equal(2.0, set_line_width.float_lineWidth)
