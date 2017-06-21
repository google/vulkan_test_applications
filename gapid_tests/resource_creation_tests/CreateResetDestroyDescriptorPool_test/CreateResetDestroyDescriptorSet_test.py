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
from gapit_test_framework import require_not_equal, little_endian_bytes_to_int
from gapit_test_framework import GapitTest
from vulkan_constants import *
from struct_offsets import VulkanStruct, UINT32_T, POINTER

DESCRIPTOR_POOL_CREATE_INFO_ELEMENTS = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("maxSets", UINT32_T),
    ("poolSizeCount", UINT32_T),
    ("pPoolSizes", POINTER),
]

DESCRIPTOR_POOL_SIZE_ELEMENTS = [
    ("type", UINT32_T),
    ("descriptorCount", UINT32_T),
]


def check_create_descriptor_pool(test, index):
    """Gets the |index|'th vkCreateDescriptorPool call atom, and checks its
    return value and arguments. Also checks the returned descriptor pool handle
    is not null. Returns the checked vkCreateDescriptorPool atom, device and
    descriptor pool handle. This method does not check the content of the
    VkDescriptorPoolCreateInfo struct used in the vkCreateDescriptorPool call.
    """
    create_descriptor_pool = require(
        test.nth_call_of("vkCreateDescriptorPool", index))
    require_equal(VK_SUCCESS, int(create_descriptor_pool.return_val))
    device = create_descriptor_pool.int_device
    require_not_equal(0, device)
    p_create_info = create_descriptor_pool.hex_pCreateInfo
    require_not_equal(0, p_create_info)
    p_descriptor_pool = create_descriptor_pool.hex_pDescriptorPool
    require_not_equal(0, p_descriptor_pool)
    descriptor_pool = little_endian_bytes_to_int(
        require(create_descriptor_pool.get_write_data(
            p_descriptor_pool, NON_DISPATCHABLE_HANDLE_SIZE)))
    require_not_equal(0, descriptor_pool)
    return create_descriptor_pool, device, descriptor_pool


def check_reset_descriptor_pool(test, device, descriptor_pool):
    """Checks that the next vkResetDescriptorPool command call atom has the
    passed-in |device| and |descriptor_pool| handle value.
    """
    reset_descriptor_pool = require(
        test.next_call_of("vkResetDescriptorPool"))
    require_equal(device, reset_descriptor_pool.int_device)
    require_equal(descriptor_pool, reset_descriptor_pool.int_descriptorPool)
    require_equal(0, reset_descriptor_pool.int_flags)


def check_destroy_descriptor_pool(test, device, descriptor_pool):
    """Checks that the next vkDestroyDescriptorPool command call atom has the
    passed-in |device| and |descriptor_pool| handle value.
    """
    destroy_descriptor_pool = require(
        test.next_call_of("vkDestroyDescriptorPool"))
    require_equal(device, destroy_descriptor_pool.int_device)
    require_equal(descriptor_pool, destroy_descriptor_pool.int_descriptorPool)


def get_descriptor_pool_create_info(create_descriptor_pool, architecture):
    """Returns a VulkanStruct representing the VkDescriptorPoolCreateInfo struct
    used in the given create_descriptor_pool command."""
    return VulkanStruct(
        architecture, DESCRIPTOR_POOL_CREATE_INFO_ELEMENTS,
        lambda offset, size: little_endian_bytes_to_int(require(
            create_descriptor_pool.get_read_data(
                create_descriptor_pool.hex_pCreateInfo + offset, size))))


def get_pool_size(create_descriptor_pool, architecture, create_info, index):
    """Returns a VulkanStruct representing the VkDescriptorPoolSize struct."""
    pool_size_offset = (create_info.pPoolSizes +
                        # (4 + 4) is the size of a VkDescriptorPoolSize struct.
                        index * (4 + 4))
    return VulkanStruct(
        architecture, DESCRIPTOR_POOL_SIZE_ELEMENTS,
        lambda offset, size: little_endian_bytes_to_int(require(
            create_descriptor_pool.get_read_data(
                pool_size_offset + offset, size))))


@gapit_test("CreateResetDestroyDescriptorPool_test")
class NoFlagOneDescriptorType(GapitTest):

    def expect(self):
        """1. No create flags, max two sets, one descriptor type"""

        arch = self.architecture

        create_atom, device, descriptor_pool = \
            check_create_descriptor_pool(self, 1)
        info = get_descriptor_pool_create_info(create_atom, arch)
        require_equal(
            info.sType, VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, 0)
        require_equal(info.maxSets, 1)
        require_equal(info.poolSizeCount, 1)

        pool_size = get_pool_size(create_atom, arch, info, 0)
        require_equal(pool_size.type, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
        require_equal(pool_size.descriptorCount, 1)

        check_reset_descriptor_pool(self, device, descriptor_pool)
        check_destroy_descriptor_pool(self, device, descriptor_pool)


@gapit_test("CreateResetDestroyDescriptorPool_test")
class WithFlagMoreDescriptorTypes(GapitTest):

    def expect(self):
        """2. With create flags, max ten sets, more descriptor types"""

        arch = self.architecture

        create_atom, device, descriptor_pool = \
            check_create_descriptor_pool(self, 2)
        info = get_descriptor_pool_create_info(create_atom, arch)
        require_equal(
            info.sType, VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags,
                      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
        require_equal(info.maxSets, 10)
        require_equal(info.poolSizeCount, 3)

        pool_size = get_pool_size(create_atom, arch, info, 0)
        require_equal(pool_size.type, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
        require_equal(pool_size.descriptorCount, 42)

        pool_size = get_pool_size(create_atom, arch, info, 1)
        require_equal(pool_size.type, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        require_equal(pool_size.descriptorCount, 5)

        pool_size = get_pool_size(create_atom, arch, info, 2)
        require_equal(pool_size.type, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
        require_equal(pool_size.descriptorCount, 8)

        check_reset_descriptor_pool(self, device, descriptor_pool)
        check_destroy_descriptor_pool(self, device, descriptor_pool)
