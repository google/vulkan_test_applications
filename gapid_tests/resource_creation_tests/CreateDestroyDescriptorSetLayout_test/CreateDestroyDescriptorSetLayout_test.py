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
from gapit_test_framework import
from vulkan_constants import *
from struct_offsets import VulkanStruct, UINT32_T, FLOAT, POINTER

DESCRIPTOR_SET_LAYOUT_CREATE_INFO_ELEMENTS = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("flags", UINT32_T),
    ("bindingCount", UINT32_T),
    ("pBindings", POINTER),
]

BINDING_ELEMENTS = [
    ("binding", UINT32_T),
    ("descriptorType", UINT32_T),
    ("descriptorCount", UINT32_T),
    ("stageFlags", UINT32_T),
    ("pImmutableSamplers", POINTER),
]


def check_create_descriptor_set_layout(test, index):
    """Gets the |index|'th vkCreateDescriptorSetLayout call atom, and checks its
    return value and arguments. Also checks the returned descriptor set layout
    handle is not null. Returns the checked vkCreateDescriptorSetLayout atom,
    device and descriptor set layout handle. This method does not check the
    content of the VkDescriptorSetLayoutCreateInfo struct used in the
    vkCreateDescriptorSetLayout call.
    """
    create_descriptor_set_layout = require(
        test.nth_call_of("vkCreateDescriptorSetLayout", index))
    require_equal(VK_SUCCESS, int(create_descriptor_set_layout.return_val))
    device = create_descriptor_set_layout.int_device
    require_not_equal(0, device)
    p_create_info = create_descriptor_set_layout.hex_pCreateInfo
    require_not_equal(0, p_create_info)
    p_set_layout = create_descriptor_set_layout.hex_pSetLayout
    require_not_equal(0, p_set_layout)
    descriptor_set_layout = little_endian_bytes_to_int(
        require(create_descriptor_set_layout.get_write_data(
            p_set_layout, NON_DISPATCHABLE_HANDLE_SIZE)))
    require_not_equal(0, descriptor_set_layout)
    return create_descriptor_set_layout, device, descriptor_set_layout


def check_destroy_descriptor_set_layout(test, device, descriptor_set_layout):
    """Checks that the next vkDestroyDescriptorSetLayout command call atom has
    the passed-in |device| and |descriptor_set_layout| handle value.
    """
    destroy_descriptor_set_layout = require(
        test.next_call_of("vkDestroyDescriptorSetLayout"))
    require_equal(device, destroy_descriptor_set_layout.int_device)
    require_equal(descriptor_set_layout,
                  destroy_descriptor_set_layout.int_descriptorSetLayout)


def get_descriptor_set_layout_create_info(create_descriptor_set_layout,
                                          architecture):
    """Returns a VulkanStruct representing the VkDescriptorSetLayoutCreateInfo
    struct used in the given |create_descriptor_set_layout| command."""
    return VulkanStruct(
        architecture, DESCRIPTOR_SET_LAYOUT_CREATE_INFO_ELEMENTS,
        lambda offset, size: little_endian_bytes_to_int(require(
            create_descriptor_set_layout.get_read_data(
                create_descriptor_set_layout.hex_pCreateInfo + offset, size))))


def get_binding(create_descriptor_set_layout, architecture, create_info, index):
    """Returns a VulkanStruct representing the |index|'th (starting from 0)
    VkDescriptorSetLayoutBinding struct baked in |create_info| for the
    |create_descriptor_set_layout| atom."""
    binding_offset = create_info.pBindings + index * (
        # Ok, this is a little bit tricky. We have 4 32-bit integers and a
        # pointer in VkDescriptorSetLayoutBinding struct. On 32-bit
        # architecture, pointer size is 4, so alignement should be fine.
        # On 64-bit architecture, pointer size is 8, the struct totally
        # occupies (4 * 4 + 8) bytes, so alignment should also be fine.
        4 * 4 + int(architecture.int_pointerSize))
    return VulkanStruct(
        architecture, BINDING_ELEMENTS,
        lambda offset, size: little_endian_bytes_to_int(require(
            create_descriptor_set_layout.get_read_data(
                binding_offset + offset, size))))


def get_samplers(test, count):
    """Returns the next |count| VkSamplers created in test."""
    samplers = []
    for i in range(count):
        create = require(test.next_call_of("vkCreateSampler"))
        require_equal(VK_SUCCESS, int(create.return_val))
        require_not_equal(0, create.int_device)
        require_not_equal(0, create.hex_pSampler)
        sampler = little_endian_bytes_to_int(
            require(create.get_write_data(
                create.hex_pSampler, NON_DISPATCHABLE_HANDLE_SIZE)))
        require_not_equal(0, sampler)
        samplers.append(sampler)
    return samplers


def get_bound_sampler(create_descriptor_set_layout, architecture,
                      binding_info, index):
    """Returns the |index|'th sampler contained in the |binding_info|
    VkDescriptorSetLayoutBinding struct's pImmutableSamplers field."""
    offset = (binding_info.pImmutableSamplers +
              index * NON_DISPATCHABLE_HANDLE_SIZE)
    return little_endian_bytes_to_int(
        require(create_descriptor_set_layout.get_read_data(
            offset, NON_DISPATCHABLE_HANDLE_SIZE)))


@gapit_test("CreateDestroyDescriptorSetLayout_test")
class ZeroBindings(GapitTest):

    def expect(self):
        """1. Zero bindings."""

        architecture = self.architecture

        create_descriptor_set_layout, device, descriptor_set_layout = \
            check_create_descriptor_set_layout(self, 1)
        info = get_descriptor_set_layout_create_info(
            create_descriptor_set_layout, architecture)
        require_equal(info.sType,
                      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, 0)
        require_equal(info.bindingCount, 0)
        require_equal(info.pBindings, 0)

        check_destroy_descriptor_set_layout(
            self, device, descriptor_set_layout)


@gapit_test("CreateDestroyDescriptorSetLayout_test")
class ThreeBindings(GapitTest):

    def expect(self):
        """2. Three bindings."""

        arch = self.architecture

        create_descriptor_set_layout, device, descriptor_set_layout = \
            check_create_descriptor_set_layout(self, 2)
        info = get_descriptor_set_layout_create_info(
            create_descriptor_set_layout, arch)
        require_equal(info.sType,
                      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, 0)
        require_equal(info.bindingCount, 3)
        require_not_equal(info.pBindings, 0)

        # The 1st binding.
        binding = get_binding(create_descriptor_set_layout, arch, info, 0)
        require_equal(binding.binding, 0)
        require_equal(binding.descriptorType, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
        require_equal(binding.descriptorCount, 6)
        require_equal(binding.stageFlags, (VK_SHADER_STAGE_VERTEX_BIT |
                                           VK_SHADER_STAGE_FRAGMENT_BIT))
        require_equal(binding.pImmutableSamplers, 0)

        # The 2nd binding.
        binding = get_binding(create_descriptor_set_layout, arch, info, 1)
        require_equal(binding.binding, 2)
        require_equal(
            binding.descriptorType, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        require_equal(binding.descriptorCount, 1)
        require_equal(binding.stageFlags, VK_SHADER_STAGE_VERTEX_BIT)
        require_equal(binding.pImmutableSamplers, 0)

        # The 3rd binding.
        binding = get_binding(create_descriptor_set_layout, arch, info, 2)
        require_equal(binding.binding, 5)
        require_equal(
            binding.descriptorType, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        require_equal(binding.descriptorCount, 0)
        require_equal(binding.stageFlags, 0xdeadbeef)
        require_equal(binding.pImmutableSamplers, 0)

        check_destroy_descriptor_set_layout(
            self, device, descriptor_set_layout)


@gapit_test("CreateDestroyDescriptorSetLayout_test")
class TwoBindingsWithSamplers(GapitTest):

    def expect(self):
        """3. Two bindings with samplers."""

        arch = self.architecture

        # Get the actual sampler handles returned by the driver.
        expected_samplers = get_samplers(self, 3)

        create_descriptor_set_layout, device, descriptor_set_layout = \
            check_create_descriptor_set_layout(self, 1)
        info = get_descriptor_set_layout_create_info(
            create_descriptor_set_layout, arch)
        require_equal(info.sType,
                      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
        require_equal(info.pNext, 0)
        require_equal(info.flags, 0)
        require_equal(info.bindingCount, 2)
        require_not_equal(info.pBindings, 0)

        # The 1st binding.
        binding = get_binding(create_descriptor_set_layout, arch, info, 0)
        require_equal(binding.binding, 2)
        require_equal(binding.descriptorType, VK_DESCRIPTOR_TYPE_SAMPLER)
        require_equal(binding.descriptorCount, 3)
        require_equal(binding.stageFlags, VK_SHADER_STAGE_ALL)
        # We gave it a nullptr for this binding.
        require_equal(binding.pImmutableSamplers, 0)

        # The 2nd binding.
        binding = get_binding(create_descriptor_set_layout, arch, info, 1)
        require_equal(binding.binding, 7)
        require_equal(binding.descriptorType, VK_DESCRIPTOR_TYPE_SAMPLER)
        require_equal(binding.descriptorCount, 3)
        require_equal(binding.stageFlags, VK_SHADER_STAGE_ALL)
        # We gave it a valid pointer for this binding.
        require_not_equal(binding.pImmutableSamplers, 0)

        # Let's check that the atom actually captures all sampler handles
        # generated by the driver.
        for i in range(3):
            sampler = get_bound_sampler(create_descriptor_set_layout,
                                        arch, binding, i)
            require_equal(expected_samplers[i], sampler)

        check_destroy_descriptor_set_layout(
            self, device, descriptor_set_layout)


@gapit_test("CreateDestroyDescriptorSetLayout_test")
class DestroyNullDescriptorSetLayout(GapitTest):

    def expect(self):
        """4. Destroys a null descriptor set layout handle."""
        device_properties = require(
            self.next_call_of("vkGetPhysicalDeviceProperties"))

        destroy_descriptor_set_layout = require(
            self.nth_call_of("vkDestroyDescriptorSetLayout", 4))
        require_equal(0,
                      destroy_descriptor_set_layout.int_descriptorSetLayout)
