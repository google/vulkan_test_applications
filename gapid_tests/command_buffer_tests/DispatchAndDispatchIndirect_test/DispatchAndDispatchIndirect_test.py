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


@gapit_test("DispatchAndDispatchIndirect_test")
class DispatchGroupCountX512(GapitTest):

    def expect(self):
        dispatch = require(
            self.next_call_of("vkCmdDispatch"))

        require_not_equal(0, dispatch.int_commandBuffer)
        require_equal(512, dispatch.int_groupCountX)
        require_equal(1, dispatch.int_groupCountY)
        require_equal(1, dispatch.int_groupCountZ)


@gapit_test("DispatchAndDispatchIndirect_test")
class DispatchIndirectWithOffsetZero(GapitTest):

    def expect(self):
        dispatch_indirect = require(
            self.next_call_of("vkCmdDispatchIndirect"))

        require_not_equal(0, dispatch_indirect.int_commandBuffer)
        require_not_equal(0, dispatch_indirect.int_buffer)
        require_equal(0, dispatch_indirect.int_offset)
