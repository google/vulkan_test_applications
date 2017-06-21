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
from gapit_test_framework import require_true, require_false
from gapit_test_framework import require_not_equal, little_endian_bytes_to_int
from gapit_test_framework import GapitTest
from vulkan_constants import *
from struct_offsets import VulkanStruct, UINT32_T, FLOAT, POINTER

SAMPLER_CREATE_INFO_ELEMENTS = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("magFilter", UINT32_T),
    ("minFilter", UINT32_T),
    ("mipmapMode", UINT32_T),
    ("addressModeU", UINT32_T),
    ("addressModeV", UINT32_T),
    ("addressModeW", UINT32_T),
    ("mipLodBias", FLOAT),
    ("anisotropyEnable", UINT32_T),
    ("maxAnisotropy", FLOAT),
    ("compareEnable", UINT32_T),
    ("compareOp", UINT32_T),
    ("minLod", FLOAT),
    ("maxLod", FLOAT),
    ("borderColor", UINT32_T),
    ("unnormalizedCoordinates", UINT32_T),
]


def check_create_sampler(test):
    """Gets the next vkCreateSampler call atom, and checks its return value and
    arguments. Also checks the returned sampler handle is not null.  Returns the
    checked vkCreateSampler atom, device and sampler handle.  This method does
    not check the content of the VkSamplerCreateInfo struct used in the
    vkCreateSampler call.
    """
    create_sampler = require(test.next_call_of("vkCreateSampler"))
    require_equal(VK_SUCCESS, int(create_sampler.return_val))
    device = create_sampler.int_device
    require_not_equal(0, device)
    p_create_info = create_sampler.hex_pCreateInfo
    require_not_equal(0, p_create_info)
    p_sampler = create_sampler.hex_pSampler
    require_not_equal(0, p_sampler)
    sampler = little_endian_bytes_to_int(require(create_sampler.get_write_data(
        p_sampler, NON_DISPATCHABLE_HANDLE_SIZE)))
    require_not_equal(0, sampler)
    return create_sampler, device, sampler


def check_destroy_sampler(test, device, sampler):
    """Checks that the next vkDestroySampler command call atom has the
    passed-in |device| and |sampler| handle value.
    """
    destroy_sampler = require(test.next_call_of("vkDestroySampler"))
    require_equal(device, destroy_sampler.int_device)
    require_equal(sampler, destroy_sampler.int_sampler)


def get_sampler_create_info(create_sampler, architecture):
    """Returns a VulkanStruct representing the VkSamplerCreateInfo struct
    used in the given create_sampler command."""
    return VulkanStruct(
        architecture, SAMPLER_CREATE_INFO_ELEMENTS,
        lambda offset, size: little_endian_bytes_to_int(require(
            create_sampler.get_read_data(
                create_sampler.hex_pCreateInfo + offset, size))))


@gapit_test("CreateDestroySampler_test")
class NormalizedCoordinates(GapitTest):

    def expect(self):
        """1. Expects a sampler with normalized coordinates."""

        architecture = self.architecture
        device_properties = require(
            self.nth_call_of("vkGetPhysicalDeviceProperties", 1))

        create_sampler, device, sampler = check_create_sampler(self)
        info = get_sampler_create_info(create_sampler, architecture)
        require_equal(info.sType, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, 0)
        require_equal(info.magFilter, VK_FILTER_NEAREST)
        require_equal(info.minFilter, VK_FILTER_LINEAR)
        require_equal(info.mipmapMode, VK_SAMPLER_MIPMAP_MODE_LINEAR)
        require_equal(info.addressModeU, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
        require_equal(info.addressModeV, VK_SAMPLER_ADDRESS_MODE_REPEAT)
        require_equal(info.addressModeW,
                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
        require_equal(info.mipLodBias, -1.)
        require_false(info.anisotropyEnable)
        require_equal(info.maxAnisotropy, 1.)
        require_false(info.compareEnable)
        require_equal(info.compareOp, VK_COMPARE_OP_ALWAYS)
        require_equal(info.minLod, 1.)
        require_equal(info.maxLod, 2.)
        require_equal(info.borderColor, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK)
        require_false(info.unnormalizedCoordinates)

        check_destroy_sampler(self, device, sampler)


@gapit_test("CreateDestroySampler_test")
class UnnormalizedCoordinates(GapitTest):

    def expect(self):
        """2. Expects a sampler with unnormalized coordinates."""

        architecture = self.architecture
        device_properties = require(
            self.nth_call_of("vkGetPhysicalDeviceProperties", 2))
        create_device = self.next_call_of("vkCreateDevice")
        if create_device[0] is None:
            raise GapidUnsupportedException(
                "physical device feature: samplerAnisotropy not supported")

        create_sampler, device, sampler = check_create_sampler(self)
        info = get_sampler_create_info(create_sampler, architecture)
        require_equal(info.sType, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, 0)
        require_equal(info.magFilter, VK_FILTER_NEAREST)
        require_equal(info.minFilter, VK_FILTER_NEAREST)
        require_equal(info.mipmapMode, VK_SAMPLER_MIPMAP_MODE_NEAREST)
        require_equal(info.addressModeU,
                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
        require_equal(info.addressModeV,
                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
        require_equal(info.addressModeW,
                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
        require_equal(info.mipLodBias, .0)
        require_false(info.anisotropyEnable)
        require_equal(info.maxAnisotropy, .0)
        require_false(info.compareEnable)
        require_equal(info.compareOp, VK_COMPARE_OP_LESS)
        require_equal(info.minLod, 0.)
        require_equal(info.maxLod, 0.)
        require_equal(info.borderColor, VK_BORDER_COLOR_INT_OPAQUE_WHITE)
        require_true(info.unnormalizedCoordinates)

        check_destroy_sampler(self, device, sampler)
