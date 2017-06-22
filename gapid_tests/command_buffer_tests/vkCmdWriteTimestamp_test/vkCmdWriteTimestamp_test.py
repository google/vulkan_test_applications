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
from gapit_test_framework import GapitTest, GapidUnsupportedException
from vulkan_constants import *


@gapit_test("vkCmdWriteTimestamp_test")
class TimeStampForDraw(GapitTest):

    def expect(self):
        draw = self.next_call_of("vkCmdDraw")
        # If queue does not support vkCmdWriteTimestamp, vkCmdDraw will not be
        # called, so the test should be skipped.
        if draw[0] is None:
            raise GapidUnsupportedException(
                "phsical device queue family has zero value of timestampValidBits")
        first_write = require(self.next_call_of("vkCmdWriteTimestamp"))
        second_write = require(self.next_call_of("vkCmdWriteTimestamp"))
        require_not_equal(0, first_write.int_commandBuffer)
        require_equal(first_write.int_commandBuffer, second_write.int_commandBuffer)
        require_equal(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                      first_write.int_pipelineStage)
        require_equal(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                      second_write.int_pipelineStage)
        require_not_equal(0, first_write.int_queryPool)
        require_equal(first_write.int_queryPool, second_write.int_queryPool)
        require_equal(0, first_write.query)
        require_equal(1, second_write.query)


@gapit_test("vkCmdWriteTimestamp_test")
class TimeStampForDrawIndexed(GapitTest):

    def expect(self):
        draw = self.next_call_of("vkCmdDrawIndexed")
        # If queue does not support vkCmdWriteTimestamp, vkCmdDrawIndexed will not be
        # called, so the test should be skipped.
        if draw[0] is None:
            raise GapidUnsupportedException(
                "phsical device queue family has zero value of timestampValidBits")
        first_write = require(self.next_call_of("vkCmdWriteTimestamp"))
        second_write = require(self.next_call_of("vkCmdWriteTimestamp"))
        require_not_equal(0, first_write.int_commandBuffer)
        require_equal(first_write.int_commandBuffer, second_write.int_commandBuffer)
        require_equal(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                      first_write.int_pipelineStage)
        require_equal(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                      second_write.int_pipelineStage)
        require_not_equal(0, first_write.int_queryPool)
        require_equal(first_write.int_queryPool, second_write.int_queryPool)
        require_equal(0, first_write.query)
        require_equal(1, second_write.query)
