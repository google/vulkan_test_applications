# Copyright 2020 Google Inc.
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

from gapit_test_framework import gapit_test, GapitTest
from gapit_test_framework import require, require_equal, require_not_equal

@gapit_test("vkTrimCommandPool_test")
class TrimCommandPool(GapitTest):

    def expect(self):
        """1. Expects a default command pool that is trimmed for redundant
        memory"""

        architecture = self.architecture

        trim_command_pool = require(self.next_call_of("vkTrimCommandPool"))
        require_not_equal(0, trim_command_pool.int_device)
        require_not_equal(0, trim_command_pool.int_commandPool)
        require_equal(0, trim_command_pool.flags)
