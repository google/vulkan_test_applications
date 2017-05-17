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
from struct_offsets import VulkanStruct, UINT32_T, SIZE_T, POINTER
from struct_offsets import HANDLE, FLOAT, CHAR, ARRAY
from vulkan_constants import *

#/*
# memory_requirements = VulkanStruct(
#            architecture, MEMORY_REQUIREMENTS, get_write_offset_function(
#                get_image_memory_requirements,
#                get_image_memory_requirements.hex_pMemoryRequirements))
#*/

SUBMIT_INFO = [
    ("sType", UINT32_T), ("pNext", POINTER), ("waitSemaphoreCount", UINT32_T),
    ("pWaitSemaphores", POINTER), ("pWaitDstStageMask", POINTER),
    ("commandBufferCount", UINT32_T), ("pCommandBuffers", POINTER),
    ("signalSemaphoreCount", UINT32_T), ("pSignalSemaphores", POINTER)
]


@gapit_test("QueueSubmitAndWait_test")
class ZeroSubmits(GapitTest):

    def expect(self):
        architecture = self.architecture
        queue_submit = require(self.nth_call_of("vkQueueSubmit", 1))
        require_not_equal(0, queue_submit.int_queue)
        require_equal(0, queue_submit.int_submitCount)
        require_equal(0, queue_submit.hex_pSubmits)
        require_equal(0, queue_submit.int_fence)

        queue_wait_idle = require(self.next_call_of("vkQueueWaitIdle"))
        require_equal(queue_wait_idle.int_queue, queue_submit.int_queue)


@gapit_test("QueueSubmitAndWait_test")
class OneSubmitZeroCommandBuffers(GapitTest):

    def expect(self):
        architecture = self.architecture
        queue_submit = require(self.nth_call_of("vkQueueSubmit", 2))
        require_not_equal(0, queue_submit.int_queue)
        require_not_equal(0, queue_submit.int_submitCount)
        require_not_equal(0, queue_submit.hex_pSubmits)
        require_equal(0, queue_submit.int_fence)

        queue_wait_idle = require(self.next_call_of("vkQueueWaitIdle"))
        require_equal(queue_wait_idle.int_queue, queue_submit.int_queue)

        queue_submit_info = VulkanStruct(
            architecture, SUBMIT_INFO,
            get_read_offset_function(queue_submit, queue_submit.hex_pSubmits))

        require_equal(VK_STRUCTURE_TYPE_SUBMIT_INFO, queue_submit_info.sType)
        require_equal(0, queue_submit_info.pNext)
        require_equal(0, queue_submit_info.waitSemaphoreCount)
        require_equal(0, queue_submit_info.pWaitSemaphores)
        require_equal(0, queue_submit_info.pWaitDstStageMask)
        require_equal(0, queue_submit_info.commandBufferCount)
        require_equal(0, queue_submit_info.pCommandBuffers)
        require_equal(0, queue_submit_info.signalSemaphoreCount)
        require_equal(0, queue_submit_info.pSignalSemaphores)


@gapit_test("QueueSubmitAndWait_test")
class OneCommandBuffer(GapitTest):

    def expect(self):
        architecture = self.architecture
        queue_submit = require(self.nth_call_of("vkQueueSubmit", 3))
        require_not_equal(0, queue_submit.int_queue)
        require_equal(1, queue_submit.int_submitCount)
        require_not_equal(0, queue_submit.hex_pSubmits)
        require_equal(0, queue_submit.int_fence)

        queue_wait_idle = require(self.next_call_of("vkQueueWaitIdle"))
        require_equal(queue_wait_idle.int_queue, queue_submit.int_queue)

        queue_submit_info = VulkanStruct(
            architecture, SUBMIT_INFO,
            get_read_offset_function(queue_submit, queue_submit.hex_pSubmits))

        require_equal(VK_STRUCTURE_TYPE_SUBMIT_INFO, queue_submit_info.sType)
        require_equal(0, queue_submit_info.pNext)
        require_equal(0, queue_submit_info.waitSemaphoreCount)
        require_equal(0, queue_submit_info.pWaitSemaphores)
        require_equal(0, queue_submit_info.pWaitDstStageMask)
        require_equal(1, queue_submit_info.commandBufferCount)
        require_not_equal(0, queue_submit_info.pCommandBuffers)
        require_equal(0, queue_submit_info.signalSemaphoreCount)
        require_equal(0, queue_submit_info.pSignalSemaphores)

        # commandBuffers are dispatchable handles, i.e. pointers
        command_buffer = little_endian_bytes_to_int(
            require(
                queue_submit.get_read_data(queue_submit_info.pCommandBuffers,
                                           architecture.int_pointerSize)))
        require_not_equal(command_buffer, 0)


@gapit_test("QueueSubmitAndWait_test")
class TwoCommandBuffers(GapitTest):

    def expect(self):
        architecture = self.architecture
        queue_submit = require(self.nth_call_of("vkQueueSubmit", 4))
        require_not_equal(0, queue_submit.int_queue)
        require_not_equal(0, queue_submit.int_submitCount)
        require_not_equal(0, queue_submit.hex_pSubmits)
        require_equal(0, queue_submit.int_fence)

        queue_wait_idle = require(self.next_call_of("vkQueueWaitIdle"))
        require_equal(queue_wait_idle.int_queue, queue_submit.int_queue)

        queue_submit_info = VulkanStruct(
            architecture, SUBMIT_INFO,
            get_read_offset_function(queue_submit, queue_submit.hex_pSubmits))
        require_equal(VK_STRUCTURE_TYPE_SUBMIT_INFO, queue_submit_info.sType)
        require_equal(0, queue_submit_info.pNext)
        require_equal(0, queue_submit_info.waitSemaphoreCount)
        require_equal(0, queue_submit_info.pWaitSemaphores)
        require_equal(0, queue_submit_info.pWaitDstStageMask)
        require_equal(2, queue_submit_info.commandBufferCount)
        require_not_equal(0, queue_submit_info.pCommandBuffers)
        require_equal(0, queue_submit_info.signalSemaphoreCount)
        require_equal(0, queue_submit_info.pSignalSemaphores)

        # commandBuffers are dispatchable handles, i.e. pointers
        command_buffer = little_endian_bytes_to_int(
            require(
                queue_submit.get_read_data(queue_submit_info.pCommandBuffers,
                                           architecture.int_pointerSize)))
        command_buffer2 = little_endian_bytes_to_int(
            require(
                queue_submit.get_read_data(queue_submit_info.pCommandBuffers +
                                           architecture.int_pointerSize,
                                           architecture.int_pointerSize)))
        require_not_equal(command_buffer, 0)
        require_not_equal(command_buffer2, 0)
