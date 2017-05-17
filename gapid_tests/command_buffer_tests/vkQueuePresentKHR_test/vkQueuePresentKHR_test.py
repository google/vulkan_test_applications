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
from struct_offsets import VulkanStruct, UINT32_T, POINTER
from vulkan_constants import *


PRESENT_INFO = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("waitSemaphoreCount", UINT32_T),
    ("pWaitSemaphores", POINTER),
    ("swapchainCount", UINT32_T),
    ("pSwapchains", POINTER),
    ("pImageIndices", POINTER),
    ("pResults", POINTER),
]


@gapit_test("vkQueuePresentKHR_test")
class QueuePresentWithoutSemaphoreAndResultsArray(GapitTest):
    def expect(self):
        """1. Expects a call of vkQueuePresentKHR() without waiting semaphore and
        results array"""
        architecture = self.architecture
        queue_present = require(self.nth_call_of("vkQueuePresentKHR", 1))
        require_not_equal(0, queue_present.int_queue)
        require_not_equal(0, queue_present.hex_pPresentInfo)
        require_equal(VK_SUCCESS, int(queue_present.return_val))

        present_info = VulkanStruct(
            architecture, PRESENT_INFO, get_read_offset_function(
                queue_present, queue_present.hex_pPresentInfo))

        require_equal(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, present_info.sType)
        require_equal(0, present_info.pNext)
        require_equal(0, present_info.waitSemaphoreCount)
        require_equal(0, present_info.pWaitSemaphores)
        require_equal(1, present_info.swapchainCount)
        require_not_equal(0, present_info.pSwapchains)
        require_not_equal(0, present_info.pImageIndices)
        require_equal(0, present_info.pResults)
