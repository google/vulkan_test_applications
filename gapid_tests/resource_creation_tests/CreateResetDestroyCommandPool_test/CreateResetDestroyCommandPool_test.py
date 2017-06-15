# Copyright 2017 Google Inc.
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

from gapit_test_framework import gapit_test, require, require_equal
from gapit_test_framework import require_not_equal, GapitTest
from gapit_test_framework import get_read_offset_function, get_write_offset_function
from vulkan_constants import *
from struct_offsets import VulkanStruct, UINT32_T, POINTER, HANDLE, DEVICE_SIZE
from struct_offsets import ARRAY, CHAR

COMMAND_POOL_CREATE_INFO = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("queueFamilyIndex", UINT32_T),
]

COMMAND_POOL = [("handle", HANDLE)]


@gapit_test("CreateResetDestroyCommandPool_test")
class TransientBitCommandPool(GapitTest):

    def expect(self):
        """1. Expects a command pool created with
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, then reseted with flag of value 0,
        and finally destroyed"""

        architecture = self.architecture
        create_command_pool = require(self.nth_call_of("vkCreateCommandPool",
                                                       1))
        device = create_command_pool.int_device
        require_not_equal(0, device)
        require_not_equal(0, create_command_pool.hex_pCreateInfo)
        require_equal(0, create_command_pool.hex_pAllocator)
        require_not_equal(0, create_command_pool.hex_pCommandPool)
        require_equal(VK_SUCCESS, int(create_command_pool.return_val))

        create_info = VulkanStruct(
            architecture, COMMAND_POOL_CREATE_INFO, get_read_offset_function(
                create_command_pool, create_command_pool.hex_pCreateInfo))
        require_equal(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                      create_info.sType)
        require_equal(0, create_info.pNext)
        require_equal(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, create_info.flags)
        require_equal(0, create_info.queueFamilyIndex)

        command_pool = VulkanStruct(
            architecture, COMMAND_POOL, get_write_offset_function(
                create_command_pool, create_command_pool.hex_pCommandPool))
        require_not_equal(0, command_pool.handle)

        reset_pool = require(self.next_call_of("vkResetCommandPool"))
        require_equal(device, reset_pool.int_device)
        require_equal(command_pool.handle, reset_pool.int_commandPool)
        require_equal(0, reset_pool.flags)

        destroy_command_pool = require(self.next_call_of(
            "vkDestroyCommandPool"))
        require_equal(device, destroy_command_pool.int_device)
        require_equal(
            command_pool.handle, destroy_command_pool.int_commandPool)


@gapit_test("CreateResetDestroyCommandPool_test")
class ResetCommandBufferBitCommandPool(GapitTest):

    def expect(self):
        """2. Expects a command pool created with
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, then reseted with flag
        of value 0, and finally destroyed"""

        architecture = self.architecture
        create_command_pool = require(self.nth_call_of("vkCreateCommandPool",
                                                       2))
        device = create_command_pool.int_device
        require_not_equal(0, device)
        require_not_equal(0, create_command_pool.hex_pCreateInfo)
        require_equal(0, create_command_pool.hex_pAllocator)
        require_not_equal(0, create_command_pool.hex_pCommandPool)
        require_equal(VK_SUCCESS, int(create_command_pool.return_val))

        create_info = VulkanStruct(
            architecture, COMMAND_POOL_CREATE_INFO, get_read_offset_function(
                create_command_pool, create_command_pool.hex_pCreateInfo))
        require_equal(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                      create_info.sType)
        require_equal(0, create_info.pNext)
        require_equal(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                      create_info.flags)
        require_equal(0, create_info.queueFamilyIndex)

        command_pool = VulkanStruct(
            architecture, COMMAND_POOL, get_write_offset_function(
                create_command_pool, create_command_pool.hex_pCommandPool))
        require_not_equal(0, command_pool.handle)

        reset_pool = require(self.next_call_of("vkResetCommandPool"))
        require_equal(device, reset_pool.int_device)
        require_equal(command_pool.handle, reset_pool.int_commandPool)
        require_equal(0, reset_pool.flags)

        destroy_command_pool = require(self.next_call_of(
            "vkDestroyCommandPool"))
        require_equal(device, destroy_command_pool.int_device)
        require_equal(
            command_pool.handle, destroy_command_pool.int_commandPool)


@gapit_test("CreateResetDestroyCommandPool_test")
class ResetCommandBufferBitTransientBitCommandPool(GapitTest):

    def expect(self):
        """3. Expects a command pool created with
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, then reset with flag of value
        VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT, and finally destroyed"""

        architecture = self.architecture
        create_command_pool = require(self.nth_call_of("vkCreateCommandPool",
                                                       3))
        device = create_command_pool.int_device
        require_not_equal(0, device)
        require_not_equal(0, create_command_pool.hex_pCreateInfo)
        require_equal(0, create_command_pool.hex_pAllocator)
        require_not_equal(0, create_command_pool.hex_pCommandPool)
        require_equal(VK_SUCCESS, int(create_command_pool.return_val))

        create_info = VulkanStruct(
            architecture, COMMAND_POOL_CREATE_INFO, get_read_offset_function(
                create_command_pool, create_command_pool.hex_pCreateInfo))
        require_equal(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                      create_info.sType)
        require_equal(0, create_info.pNext)
        require_equal(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
                      | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, create_info.flags)
        require_equal(0, create_info.queueFamilyIndex)

        command_pool = VulkanStruct(
            architecture, COMMAND_POOL, get_write_offset_function(
                create_command_pool, create_command_pool.hex_pCommandPool))
        require_not_equal(0, command_pool.handle)

        reset_pool = require(self.next_call_of("vkResetCommandPool"))
        require_equal(device, reset_pool.int_device)
        require_equal(command_pool.handle, reset_pool.int_commandPool)
        require_equal(VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT, reset_pool.flags)

        destroy_command_pool = require(self.next_call_of(
            "vkDestroyCommandPool"))
        require_equal(device, destroy_command_pool.int_device)
        require_equal(
            command_pool.handle, destroy_command_pool.int_commandPool)


@gapit_test("CreateResetDestroyCommandPool_test")
class EmptyBitCommandPool(GapitTest):

    def expect(self):
        """3. Expects a command pool created with empty flag bit, then reseted
        with VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT, and finally
        destroyed"""

        architecture = self.architecture
        create_command_pool = require(self.nth_call_of("vkCreateCommandPool",
                                                       4))
        device = create_command_pool.int_device
        require_not_equal(0, device)
        require_not_equal(0, create_command_pool.hex_pCreateInfo)
        require_equal(0, create_command_pool.hex_pAllocator)
        require_not_equal(0, create_command_pool.hex_pCommandPool)
        require_equal(VK_SUCCESS, int(create_command_pool.return_val))

        create_info = VulkanStruct(
            architecture, COMMAND_POOL_CREATE_INFO, get_read_offset_function(
                create_command_pool, create_command_pool.hex_pCreateInfo))
        require_equal(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                      create_info.sType)
        require_equal(0, create_info.pNext)
        require_equal(0, create_info.flags)
        require_equal(0, create_info.queueFamilyIndex)

        command_pool = VulkanStruct(
            architecture, COMMAND_POOL, get_write_offset_function(
                create_command_pool, create_command_pool.hex_pCommandPool))
        require_not_equal(0, command_pool.handle)

        reset_pool = require(self.next_call_of("vkResetCommandPool"))
        require_equal(device, reset_pool.int_device)
        require_equal(command_pool.handle, reset_pool.int_commandPool)
        require_equal(VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT, reset_pool.flags)

        destroy_command_pool = require(self.next_call_of(
            "vkDestroyCommandPool"))
        require_equal(device, destroy_command_pool.int_device)
        require_equal(
            command_pool.handle, destroy_command_pool.int_commandPool)
