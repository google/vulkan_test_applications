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
from gapit_test_framework import GapitTest
from gapit_test_framework import NVIDIA_K2200
from vulkan_constants import *


@gapit_test("SynchronizationCreationDestruction_test")
class SemaphoreCreateDestroyTest(GapitTest):

    def expect(self):
        architecture = self.architecture
        device_properties = require(
            self.next_call_of("vkGetPhysicalDeviceProperties"))

        create_semaphore = require(self.next_call_of("vkCreateSemaphore"))

        # Make sure the parameters are valid
        require_not_equal(create_semaphore.hex_pCreateInfo, 0)
        require_not_equal(create_semaphore.int_device, 0)
        require_not_equal(create_semaphore.hex_pSemaphore, 0)
        require_equal(create_semaphore.hex_pAllocator, 0)

        create_info_structure_type_memory = require(
            create_semaphore.get_read_data(create_semaphore.hex_pCreateInfo,
                                           architecture.int_integerSize))
        create_info_pNext_memory = require(
            create_semaphore.get_read_data(create_semaphore.hex_pCreateInfo +
                                           architecture.int_pointerSize,
                                           architecture.int_pointerSize))
        create_info_flags_memory = require(
            create_semaphore.get_read_data(create_semaphore.hex_pCreateInfo + 2
                                           * architecture.int_pointerSize,
                                           architecture.int_integerSize))

        # The struct should look like
        # {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 0, 0}
        require_equal(
            little_endian_bytes_to_int(create_info_structure_type_memory),
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO)
        require_equal(little_endian_bytes_to_int(create_info_pNext_memory), 0)
        require_equal(little_endian_bytes_to_int(create_info_flags_memory), 0)

        # pSemaphore is filled in by the call, should be a write observation
        returned_semaphore = require(
            create_semaphore.get_write_data(create_semaphore.hex_pSemaphore, 8))

        # We should have called destroy_semaphore with the same one
        destroy_semaphore = require(self.next_call_of("vkDestroySemaphore"))
        require_equal(
            little_endian_bytes_to_int(returned_semaphore),
            destroy_semaphore.int_semaphore)

        # Our second destroy_semaphore should have been called with
        # VK_NULL_HANDLE
        if self.not_device(device_properties, 0x5BCE4000, NVIDIA_K2200):
            destroy_semaphore_null = require(
                self.next_call_of("vkDestroySemaphore"))
            require_equal(0, destroy_semaphore_null.int_semaphore)
