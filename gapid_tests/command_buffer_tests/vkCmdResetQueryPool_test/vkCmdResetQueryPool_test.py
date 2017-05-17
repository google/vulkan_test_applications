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


@gapit_test("vkCmdResetQueryPool_test")
class ClearAllQueriesInAFourQueriesPool(GapitTest):

    def expect(self):
        """1. Expects vkCmdResetQueryPool() is called with firstQuery: 0,
        queryCount: 4."""
        reset_query_pool = require(self.nth_call_of(
            "vkCmdResetQueryPool", 1))
        require_not_equal(0, reset_query_pool.int_commandBuffer)
        require_not_equal(0, reset_query_pool.int_queryPool)
        require_equal(0, reset_query_pool.int_firstQuery)
        require_equal(4, reset_query_pool.int_queryCount)


@gapit_test("vkCmdResetQueryPool_test")
class ClearQueryOneToFiveInASevenQueriesPool(GapitTest):

    def expect(self):
        """2. Expects vkCmdResetQueryPool() is called with firstQuery: 1,
        queryCount: 5."""
        reset_query_pool = require(self.nth_call_of(
            "vkCmdResetQueryPool", 2))
        require_not_equal(0, reset_query_pool.int_commandBuffer)
        require_not_equal(0, reset_query_pool.int_queryPool)
        require_equal(1, reset_query_pool.int_firstQuery)
        require_equal(5, reset_query_pool.int_queryCount)
