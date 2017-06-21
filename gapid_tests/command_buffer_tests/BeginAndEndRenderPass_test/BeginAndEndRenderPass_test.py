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

from gapit_test_framework import gapit_test, require, require_equal
from gapit_test_framework import require_not_equal
from gapit_test_framework import GapitTest, get_read_offset_function
from vulkan_constants import VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO
from struct_offsets import VulkanStruct, ARRAY, FLOAT, UINT32_T, POINTER, HANDLE

RENDER_PASS_BEGIN_INFO = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("renderPass", HANDLE),
    ("framebuffer", HANDLE),
    ("renderArea_offset_x", UINT32_T),
    ("renderArea_offset_y", UINT32_T),
    ("renderArea_extent_width", UINT32_T),
    ("renderArea_extent_height", UINT32_T),
    ("clearValueCount", UINT32_T),
    ("pClearValues", POINTER),
]

CLEAR_COLOR_UNORM_VALUE = [
    ("color_float32", ARRAY, 4, UINT32_T),
]

CLEAR_DEPTH_STENCIL_VALUE = [
    ("depthStencil_depth", FLOAT),
    ("depthStencil_stencil", UINT32_T),
]


@gapit_test("BeginAndEndRenderPass_test")
class BeginRenderPassWithoutAttachment(GapitTest):

    def expect(self):
        """Expect the vkCmdBeginRenderPass() is called and successfully. The
        RenderPassBeginInfo does not contain any ClearValues"""
        architecture = self.architecture

        begin_render_pass = require(
            self.nth_call_of("vkCmdBeginRenderPass", 1))
        require_not_equal(0, begin_render_pass.int_commandBuffer)

        begin_render_pass_info = VulkanStruct(
            architecture, RENDER_PASS_BEGIN_INFO, get_read_offset_function(
                begin_render_pass, begin_render_pass.hex_pRenderPassBegin))

        require_equal(VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                      begin_render_pass_info.sType)
        require_equal(0, begin_render_pass_info.pNext)
        require_not_equal(0, begin_render_pass_info.renderPass)
        require_not_equal(0, begin_render_pass_info.framebuffer)
        require_equal(5, begin_render_pass_info.renderArea_offset_x)
        require_equal(5, begin_render_pass_info.renderArea_offset_y)
        require_equal(32, begin_render_pass_info.renderArea_extent_width)
        require_equal(32, begin_render_pass_info.renderArea_extent_height)
        require_equal(0, begin_render_pass_info.clearValueCount)
        require_equal(0, begin_render_pass_info.pClearValues)

        end_render_pass = require(self.next_call_of("vkCmdEndRenderPass"))
        require_equal(begin_render_pass.int_commandBuffer,
                      end_render_pass.int_commandBuffer)


@gapit_test("BeginAndEndRenderPass_test")
class BeginColorAttachmentRenderPass(GapitTest):

    def expect(self):
        """Expect the vkCmdBeginRenderPass() is called successfully and the
        RenderPassBeginInfo should contain a pointer of one ClearValue"""
        architecture = self.architecture

        begin_render_pass = require(
            self.nth_call_of("vkCmdBeginRenderPass", 2))
        require_not_equal(0, begin_render_pass.int_commandBuffer)

        begin_render_pass_info = VulkanStruct(
            architecture, RENDER_PASS_BEGIN_INFO, get_read_offset_function(
                begin_render_pass, begin_render_pass.hex_pRenderPassBegin))

        require_equal(VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                      begin_render_pass_info.sType)
        require_equal(0, begin_render_pass_info.pNext)
        require_not_equal(0, begin_render_pass_info.renderPass)
        require_not_equal(0, begin_render_pass_info.framebuffer)
        require_equal(0, begin_render_pass_info.renderArea_offset_x)
        require_equal(0, begin_render_pass_info.renderArea_offset_y)
        require_equal(0, begin_render_pass_info.clearValueCount)
        require_equal(0, begin_render_pass_info.pClearValues)

        end_render_pass = require(self.next_call_of("vkCmdEndRenderPass"))
        require_equal(begin_render_pass.int_commandBuffer,
                      end_render_pass.int_commandBuffer)
