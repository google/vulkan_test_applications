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
from vulkan_constants import *


@gapit_test("vkCreateInstance_NullApp_test")
class NullApplicationInfoTest(GapitTest):

    def expect(self):
        """Expect that the applicationInfoPointer is null for the first
         vkCreateInstance"""
        architecture = self.architecture

        create_instance = require(self.next_call_of("vkCreateInstance"))
        require_not_equal(create_instance.hex_pCreateInfo, 0)

        create_info_memory = require(
            create_instance.get_read_data(create_instance.hex_pCreateInfo,
                                          architecture.int_integerSize))
        require_equal(
            little_endian_bytes_to_int(create_info_memory),
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO)

        applicationInfoPointer = require(
            create_instance.get_read_data(create_instance.hex_pCreateInfo +
                                          architecture.int_pointerSize * 3,
                                          architecture.int_pointerSize))
        require_equal(little_endian_bytes_to_int(applicationInfoPointer), 0)


@gapit_test("vkCreateInstance_NonNullApp_test")
class NonNullApplicationInfoTest(GapitTest):

    def expect(self):
        """Expect that the applicationInfoPointer is not null for the second
         vkCreateInstance, and that it contains some of the expected data."""
        architecture = self.architecture

        create_instance = require(self.nth_call_of("vkCreateInstance", 1))
        require_not_equal(create_instance.hex_pCreateInfo, 0)

        create_info_memory = require(
            create_instance.get_read_data(create_instance.hex_pCreateInfo,
                                          architecture.int_integerSize))
        require_equal(
            little_endian_bytes_to_int(create_info_memory),
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO)

        applicationInfoPointer = require(
            create_instance.get_read_data(create_instance.hex_pCreateInfo +
                                          architecture.int_pointerSize * 3,
                                          architecture.int_pointerSize))
        require_not_equal(little_endian_bytes_to_int(
            applicationInfoPointer), 0)
        # The 2nd pointer(3rd element) in VkApplicationInfo should be the string
        # Application
        application_info_application_name_ptr = require(
            create_instance.get_read_data(
                little_endian_bytes_to_int(applicationInfoPointer) + 2 *
                architecture.int_pointerSize, architecture.int_pointerSize))
        application_name = require(
            create_instance.get_read_string(
                little_endian_bytes_to_int(
                    application_info_application_name_ptr)))
        require_equal(application_name, "Application")
