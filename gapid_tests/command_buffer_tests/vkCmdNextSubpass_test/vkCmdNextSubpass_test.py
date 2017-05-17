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


@gapit_test("vkCmdNextSubpass_test")
class vkCmdNextSubpass(GapitTest):

    def expect(self):
        next_subpass = require(self.next_call_of("vkCmdNextSubpass"))

        require_not_equal(0, next_subpass.int_commandBuffer)
        require_equal(VK_SUBPASS_CONTENTS_INLINE, next_subpass.int_contents)
