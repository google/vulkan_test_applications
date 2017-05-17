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

from gapit_test_framework import gapit_test, require, require_equal, require_true
from gapit_test_framework import require_not_equal, little_endian_bytes_to_int
from gapit_test_framework import GapitTest, get_read_offset_function
import gapit_test_framework
from vulkan_constants import *


@gapit_test("vkCmdCopyQueryPoolResults_test")
class AllFourQueryResultsIn32BitWithNoFlagCopyWithOffsets(GapitTest):

    def expect(self):
        """1. Expects vkCmdCopyQueryPoolResults() is called with firstQuery: 0,
        queryCount: 4 stride: 4 and dstOffset: 16."""
        copy_query_pool_results = require(self.nth_call_of(
            "vkCmdCopyQueryPoolResults", 1))
        require_not_equal(0, copy_query_pool_results.int_commandBuffer)
        require_not_equal(0, copy_query_pool_results.int_queryPool)
        require_equal(0, copy_query_pool_results.int_firstQuery)
        require_equal(4, copy_query_pool_results.int_queryCount)
        require_not_equal(0, copy_query_pool_results.int_dstBuffer)
        require_equal(16, copy_query_pool_results.int_dstOffset)
        require_equal(4, copy_query_pool_results.int_stride)
        require_equal(0, copy_query_pool_results.int_flags)


@gapit_test("vkCmdCopyQueryPoolResults_test")
class FifthToEighthQueryResultsIn64BitWithWaitBitCopyWithZeroOffsets(GapitTest):

    def expect(self):
        """2. Expects vkCmdCopyQueryPoolResults() is called with firstQuery: 4,
        queryCount: 4, stride: 8 and dstOffset: 0."""
        copy_query_pool_results = require(self.nth_call_of(
            "vkCmdCopyQueryPoolResults", 2))
        require_not_equal(0, copy_query_pool_results.int_commandBuffer)
        require_not_equal(0, copy_query_pool_results.int_queryPool)
        require_equal(4, copy_query_pool_results.int_firstQuery)
        require_equal(4, copy_query_pool_results.int_queryCount)
        require_not_equal(0, copy_query_pool_results.int_dstBuffer)
        require_equal(0, copy_query_pool_results.int_dstOffset)
        require_equal(8, copy_query_pool_results.int_stride)
        require_equal(VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT,
                      copy_query_pool_results.int_flags)


@gapit_test("vkCmdCopyQueryPoolResults_test")
class AllFourQueryResultsIn32BitAnd12StrideWithPartialAndAvailabilityBitWithZeroOffset(GapitTest):

    def expect(self):
        """3. Expects vkCmdCopyQueryPoolResults() is called with firstQuery: 0,
        queryCount: 4, stride: 12 and dstOffset: 0."""
        copy_query_pool_results = require(self.nth_call_of(
            "vkCmdCopyQueryPoolResults", 3))
        require_not_equal(0, copy_query_pool_results.int_commandBuffer)
        require_not_equal(0, copy_query_pool_results.int_queryPool)
        require_equal(0, copy_query_pool_results.int_firstQuery)
        require_equal(4, copy_query_pool_results.int_queryCount)
        require_not_equal(0, copy_query_pool_results.int_dstBuffer)
        require_equal(0, copy_query_pool_results.int_dstOffset)
        require_equal(12, copy_query_pool_results.int_stride)
        require_equal(VK_QUERY_RESULT_PARTIAL_BIT
                      | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT,
                      copy_query_pool_results.int_flags)
