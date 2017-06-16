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
from vulkan_constants import *


@gapit_test("SetStencilMask_test")
class SetStencilCompareMask(GapitTest):

    def expect(self):
        first_set_stencil = require(self.next_call_of("vkCmdSetStencilCompareMask"))
        require_not_equal(0, first_set_stencil.int_commandBuffer)
        require_equal(VK_STENCIL_FACE_FRONT_BIT,
                      first_set_stencil.int_faceMask)
        require_equal(0, first_set_stencil.int_compareMask)

        second_set_stencil = require(self.next_call_of("vkCmdSetStencilCompareMask"))
        require_not_equal(0, second_set_stencil.int_commandBuffer)
        require_equal(VK_STENCIL_FACE_BACK_BIT,
                      second_set_stencil.int_faceMask)
        require_equal(10, second_set_stencil.int_compareMask)

        third_set_stencil = require(self.next_call_of("vkCmdSetStencilCompareMask"))
        require_not_equal(0, third_set_stencil.int_commandBuffer)
        require_equal(VK_STENCIL_FRONT_AND_BACK,
                      third_set_stencil.int_faceMask)
        require_equal(0xFFFFFFFF, third_set_stencil.int_compareMask)


@gapit_test("SetStencilMask_test")
class SetStencilWriteMask(GapitTest):

    def expect(self):
        first_set_stencil = require(self.next_call_of("vkCmdSetStencilWriteMask"))
        require_not_equal(0, first_set_stencil.int_commandBuffer)
        require_equal(VK_STENCIL_FACE_FRONT_BIT,
                      first_set_stencil.int_faceMask)
        require_equal(0, first_set_stencil.int_compareMask)

        second_set_stencil = require(self.next_call_of("vkCmdSetStencilWriteMask"))
        require_not_equal(0, second_set_stencil.int_commandBuffer)
        require_equal(VK_STENCIL_FACE_BACK_BIT,
                      second_set_stencil.int_faceMask)
        require_equal(10, second_set_stencil.int_compareMask)

        third_set_stencil = require(self.next_call_of("vkCmdSetStencilWriteMask"))
        require_not_equal(0, third_set_stencil.int_commandBuffer)
        require_equal(VK_STENCIL_FRONT_AND_BACK,
                      third_set_stencil.int_faceMask)
        require_equal(0xFFFFFFFF, third_set_stencil.int_compareMask)
