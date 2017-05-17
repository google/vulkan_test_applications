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

from gapit_test_framework import gapit_test, GapitTest
from gapit_test_framework import require, require_equal, require_not_equal


@gapit_test("DrawCommands_test")
class vkCmdDraw(GapitTest):

    def expect(self):
        draw = require(
            self.next_call_of("vkCmdDraw"))

        require_not_equal(0, draw.int_commandBuffer)
        require_equal(3, draw.int_vertexCount)
        require_equal(1, draw.int_instanceCount)
        require_equal(0, draw.int_firstVertex)
        require_equal(0, draw.int_firstInstance)


@gapit_test("DrawCommands_test")
class vkCmdDrawIndexed(GapitTest):

    def expect(self):
        draw = require(
            self.next_call_of("vkCmdDrawIndexed"))

        require_not_equal(0, draw.int_commandBuffer)
        require_equal(3, draw.int_indexCount)
        require_equal(1, draw.int_instanceCount)
        require_equal(0, draw.int_firstIndex)
        require_equal(0, draw.int_vertexOffset)
        require_equal(0, draw.int_firstInstance)


@gapit_test("DrawCommands_test")
class vkCmdDrawIndirect(GapitTest):

    def expect(self):
        draw = require(
            self.next_call_of("vkCmdDrawIndirect"))

        require_not_equal(0, draw.int_commandBuffer)
        require_not_equal(0, draw.int_buffer)
        require_equal(0, draw.int_offset)
        require_equal(1, draw.int_drawCount)
        require_equal(0, draw.int_stride)


@gapit_test("DrawCommands_test")
class vkCmdDrawIndexedIndirect(GapitTest):

    def expect(self):
        draw = require(
            self.next_call_of("vkCmdDrawIndexedIndirect"))

        require_not_equal(0, draw.int_commandBuffer)
        require_not_equal(0, draw.int_buffer)
        require_equal(0, draw.int_offset)
        require_equal(1, draw.int_drawCount)
        require_equal(0, draw.int_stride)
