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
from struct_offsets import VulkanStruct, UINT32_T, SIZE_T, POINTER, INT32_T
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY
from vulkan_constants import *

VIEWPORT = [
    ("x", FLOAT),
    ("y", FLOAT),
    ("width", FLOAT),
    ("height", FLOAT),
    ("minDepth", FLOAT),
    ("maxDepth", FLOAT)
]

RECT2D = [
    ("offset_x", INT32_T),
    ("offset_y", INT32_T),
    ("extent_width", INT32_T),
    ("extent_height", INT32_T)
]


@gapit_test("SetViewportScissorAndBindPipeline_test")
class SetViewport(GapitTest):

    def expect(self):
        architecture = self.architecture
        set_viewport = require(self.next_call_of("vkCmdSetViewport"))

        require_not_equal(0, set_viewport.int_commandBuffer)
        require_equal(0, set_viewport.int_firstViewport)
        require_equal(1, set_viewport.int_viewportCount)
        require_not_equal(0, set_viewport.hex_pViewports)

        viewport = VulkanStruct(
            architecture, VIEWPORT,
            get_read_offset_function(set_viewport, set_viewport.hex_pViewports))
        require_equal(0.0, viewport.x)
        require_equal(0.0, viewport.y)
        require_not_equal(0.0, viewport.width)
        require_not_equal(0.0, viewport.height)
        require_equal(0.0, viewport.minDepth)
        require_equal(1.0, viewport.maxDepth)


@gapit_test("SetViewportScissorAndBindPipeline_test")
class SetScissor(GapitTest):

    def expect(self):
        architecture = self.architecture
        set_scissor = require(self.next_call_of("vkCmdSetScissor"))

        require_not_equal(0, set_scissor.int_commandBuffer)
        require_equal(0, set_scissor.int_firstScissor)
        require_equal(1, set_scissor.int_scissorCount)
        require_not_equal(0, set_scissor.hex_pScissors)

        scissor = VulkanStruct(
            architecture, RECT2D,
            get_read_offset_function(set_scissor, set_scissor.hex_pScissors))
        require_equal(0, scissor.offset_x)
        require_equal(0, scissor.offset_x)
        require_not_equal(0, scissor.extent_width)
        require_not_equal(0, scissor.extent_width)


@gapit_test("SetViewportScissorAndBindPipeline_test")
class BindGraphicsPipeline(GapitTest):

    def expect(self):
        architecture = self.architecture
        bind_pipeline = require(self.next_call_of("vkCmdBindPipeline"))

        require_not_equal(0, bind_pipeline.int_commandBuffer)
        require_equal(VK_PIPELINE_BIND_POINT_GRAPHICS,
                      bind_pipeline.int_pipelineBindPoint)
        require_not_equal(0, bind_pipeline.int_pipeline)
