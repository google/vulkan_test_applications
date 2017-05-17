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
from gapit_test_framework import get_read_offset_function
from vulkan_constants import VK_SHADER_STAGE_VERTEX_BIT
from struct_offsets import VulkanStruct, ARRAY, CHAR


@gapit_test("vkCmdPushConstants_test")
class PushConstantsWithOffsetZeroToVertexStage(GapitTest):

    def expect(self):
        """Check the arguments and data to vkCmdPushConstants"""
        architecture = self.architecture
        push_constants = require(self.nth_call_of("vkCmdPushConstants", 1))

        require_not_equal(0, push_constants.int_commandBuffer)
        require_not_equal(0, push_constants.int_layout)
        require_equal(VK_SHADER_STAGE_VERTEX_BIT, push_constants.int_stageFlags)
        require_equal(0, push_constants.int_offset)
        require_equal(100, push_constants.int_size)
        require_not_equal(0, push_constants.hex_pValues)

        CHAR_ARRAY_100 = [("values", ARRAY, 100, CHAR)]
        data = VulkanStruct(architecture, CHAR_ARRAY_100,
                            get_read_offset_function(
                                push_constants, push_constants.hex_pValues))
        expected_data = [0xab for i in range(100)]
        require_equal(expected_data, data.values)
