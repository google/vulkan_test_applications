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
from gapit_test_framework import require_not_equal, little_endian_bytes_to_int
from gapit_test_framework import GapitTest
from vulkan_constants import *


@gapit_test("BeginAndEndCommandBuffer_test")
class NullInheritanceInfoTest(GapitTest):

    def expect(self):
        """Expect that the pInheritanceInfo is null for the first
        vkBeginCommandBuffer"""
        architecture = self.architecture

        begin_command_buffer = require(self.next_call_of(
            "vkBeginCommandBuffer"))
        command_buffer_value = begin_command_buffer.int_commandBuffer
        require_not_equal(command_buffer_value, 0)

        # The command buffer begin info struct should start with correct
        # sType value.
        begin_info_stype_memory = require(begin_command_buffer.get_read_data(
            begin_command_buffer.hex_pBeginInfo, architecture.int_integerSize))
        require_equal(
            little_endian_bytes_to_int(begin_info_stype_memory),
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO)

        # The inheritance info pointer should be a nullptr.
        inheritance_info_pointer_memory = require(
            begin_command_buffer.get_read_data(
                begin_command_buffer.hex_pBeginInfo +
                architecture.int_pointerSize * 3,
                architecture.int_pointerSize))
        require_equal(
            little_endian_bytes_to_int(inheritance_info_pointer_memory), 0)

        # vkEndCommandBuffer() returns correctly
        end_command_buffer = require(self.next_call_of("vkEndCommandBuffer"))
        require_equal(end_command_buffer.int_commandBuffer,
                      command_buffer_value)
        require_equal(VK_SUCCESS, int(end_command_buffer.return_val))


@gapit_test("BeginAndEndCommandBuffer_test")
class NonNullInheritanceInfoTest(GapitTest):

    def expect(self):
        """Expect that the pInheritanceInfo is not null for the second
        vkBeginCommandBuffer, and that it contains some of the expected data."""
        architecture = self.architecture

        begin_command_buffer = require(self.nth_call_of("vkBeginCommandBuffer",
                                                        2))
        command_buffer_value = begin_command_buffer.int_commandBuffer
        require_not_equal(command_buffer_value, 0)

        # The command buffer begin info struct should start with correct sType
        # value.
        begin_info_stype_memory = require(begin_command_buffer.get_read_data(
            begin_command_buffer.hex_pBeginInfo, architecture.int_integerSize))
        require_equal(
            little_endian_bytes_to_int(begin_info_stype_memory),
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO)

        # The inheritance info pointer should not be a nullptr.
        inheritance_info_pointer_memory = require(
            begin_command_buffer.get_read_data(
                begin_command_buffer.hex_pBeginInfo +
                architecture.int_pointerSize * 3,
                architecture.int_pointerSize))
        inheritance_info_addr = little_endian_bytes_to_int(
            inheritance_info_pointer_memory)
        require_not_equal(inheritance_info_addr, 0)

        # The inheritance info struct should start with correct sType value.
        inheritance_info_stype_memory = require(
            begin_command_buffer.get_read_data(inheritance_info_addr,
                                               architecture.int_integerSize))
        require_equal(
            little_endian_bytes_to_int(inheritance_info_stype_memory),
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO)

        # The last field of inheritance info struct: pipelineStatistics should
        # have value 0.
        # TODO(qining): Add tests for other fields.
        inheritance_info_pipeline_statistics_memory = require(
            begin_command_buffer.get_read_data(
                inheritance_info_addr + architecture.int_pointerSize * 2 +
                NON_DISPATCHABLE_HANDLE_SIZE * 3 + architecture.int_integerSize
                * 2, architecture.int_integerSize))
        require_equal(
            little_endian_bytes_to_int(
                inheritance_info_pipeline_statistics_memory), 0)

        # vkEndCommandBuffer() returns correctly
        end_command_buffer = require(self.next_call_of("vkEndCommandBuffer"))
        require_equal(end_command_buffer.int_commandBuffer,
                      command_buffer_value)
        require_equal(VK_SUCCESS, int(end_command_buffer.return_val))
