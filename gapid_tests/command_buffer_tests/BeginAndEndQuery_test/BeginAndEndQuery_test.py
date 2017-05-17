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
from vulkan_constants import *


@gapit_test("BeginAndEndQuery_test")
class QueryWithPreciseBit(GapitTest):

    def expect(self):
        begin_query = require(
            self.nth_call_of("vkCmdBeginQuery", 1))
        end_query = require(
            self.nth_call_of("vkCmdEndQuery", 1))

        cmd_buf = begin_query.int_commandBuffer
        require_not_equal(0, cmd_buf)
        query_pool = begin_query.int_queryPool
        require_not_equal(0, query_pool)
        require_equal(0, begin_query.int_query)
        require_equal(VK_QUERY_CONTROL_PRECISE_BIT, begin_query.int_flags)

        require_equal(cmd_buf, end_query.int_commandBuffer)
        require_equal(query_pool, end_query.int_queryPool)
        require_equal(0, end_query.int_query)


@gapit_test("BeginAndEndQuery_test")
class QueryWithoutPreciseBit(GapitTest):

    def expect(self):
        begin_query = require(
            self.nth_call_of("vkCmdBeginQuery", 2))
        end_query = require(
            self.nth_call_of("vkCmdEndQuery", 2))

        cmd_buf = begin_query.int_commandBuffer
        require_not_equal(0, cmd_buf)
        query_pool = begin_query.int_queryPool
        require_not_equal(0, query_pool)
        require_equal(1, begin_query.int_query)
        require_equal(0, begin_query.int_flags)

        require_equal(cmd_buf, end_query.int_commandBuffer)
        require_equal(query_pool, end_query.int_queryPool)
        require_equal(1, end_query.int_query)
