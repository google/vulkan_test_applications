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

from gapit_test_framework import gapit_test, GapitTest
from gapit_test_framework import require, require_equal, require_not_equal
from gapit_test_framework import little_endian_bytes_to_int
from vulkan_constants import *
from struct_offsets import VulkanStruct, UINT32_T, HANDLE, POINTER, DEVICE_SIZE

WRITE_DESCRIPTOR_SET_ELEMENTS = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("dstSet", HANDLE),
    ("dstBinding", UINT32_T),
    ("dstArrayElement", UINT32_T),
    ("descriptorCount", UINT32_T),
    ("descriptorType", UINT32_T),
    ("pImageInfo", POINTER),
    ("pBufferInfo", POINTER),
    ("pTexelBufferView", POINTER),
]

COPY_DESCRIPTOR_SET_ELEMENTS = [
    ("sType", UINT32_T),
    ("pNext", POINTER),
    ("srcSet", HANDLE),
    ("srcBinding", UINT32_T),
    ("srcArrayElement", UINT32_T),
    ("dstSet", HANDLE),
    ("dstBinding", UINT32_T),
    ("dstArrayElement", UINT32_T),
    ("descriptorCount", UINT32_T),
]

DESCRIPTOR_BUFFER_INFO_ELEMENTS = [
    ("buffer", HANDLE),
    ("offset", DEVICE_SIZE),
    ("range", DEVICE_SIZE),
]

DESCRIPTOR_IMAGE_INFO_ELEMENTS = [
    ("sampler", HANDLE),
    ("imageView", HANDLE),
    ("imageLayout", UINT32_T),
]


def get_buffer(test):
    """Returns the next buffer handle created by vkCreateBuffer."""
    create = require(test.next_call_of("vkCreateBuffer"))
    require_equal(VK_SUCCESS, int(create.return_val))
    buf = little_endian_bytes_to_int(
        require(create.get_write_data(
            create.hex_pBuffer, NON_DISPATCHABLE_HANDLE_SIZE)))
    require_not_equal(0, buf)
    return buf


def get_descriptor_set(test, index):
    """Returns the descriptor set handle created by the |index|th
    (starting from 1) vkAllocateDescriptorSet."""
    allocate = require(test.nth_call_of("vkAllocateDescriptorSets", index))
    require_equal(VK_SUCCESS, int(allocate.return_val))
    d_set = little_endian_bytes_to_int(
        require(allocate.get_write_data(
            allocate.hex_pDescriptorSets, NON_DISPATCHABLE_HANDLE_SIZE)))
    require_not_equal(0, d_set)
    return d_set


def get_write_descriptor_set(update_atom, architecture, count):
    """Returns |count| VulkanStructs representing the VkWriteDescriptorSet
    structs used in the given |update_atom| atom."""
    writes = [VulkanStruct(architecture, WRITE_DESCRIPTOR_SET_ELEMENTS,
                           lambda offset, size: little_endian_bytes_to_int(
                               require(update_atom.get_read_data(
                                   update_atom.hex_pDescriptorWrites + offset,
                                   size))))]
    struct_size = writes[-1].total_size
    for i in range(1, count):
        writes.append(
            VulkanStruct(architecture, WRITE_DESCRIPTOR_SET_ELEMENTS,
                         lambda offset, size: little_endian_bytes_to_int(
                             require(update_atom.get_read_data(
                                 (update_atom.hex_pDescriptorWrites +
                                  i * struct_size + offset),
                                 size)))))
    return writes


def get_copy_descriptor_set(update_atom, architecture, count):
    """Returns |count| VulkanStructs representing the VkCopyDescriptorSet
    structs used in the given |update_atom| atom."""
    copies = [VulkanStruct(architecture, COPY_DESCRIPTOR_SET_ELEMENTS,
                           lambda offset, size: little_endian_bytes_to_int(
                               require(update_atom.get_read_data(
                                   update_atom.hex_pDescriptorCopies + offset,
                                   size))))]
    struct_size = copies[-1].total_size
    for i in range(1, count):
        copies.append(
            VulkanStruct(architecture, COPY_DESCRIPTOR_SET_ELEMENTS,
                         lambda offset, size: little_endian_bytes_to_int(
                             require(update_atom.get_read_data(
                                 (update_atom.hex_pDescriptorCopies +
                                  i * struct_size + offset),
                                 size)))))
    return copies


def get_buffer_info(update_atom, architecture, base, count):
    """Returns |count| VulkanStructs representing the VkDescriptorBufferInfo
    structs used in the VkWriteDescriptorSet parameter of the given
    |update_atom| atom."""
    infos = [VulkanStruct(architecture, DESCRIPTOR_BUFFER_INFO_ELEMENTS,
                          lambda offset, size: little_endian_bytes_to_int(
                              require(update_atom.get_read_data(
                                  base + offset, size))))]
    buffer_info_size = infos[-1].total_size

    for i in range(1, count):
        infos.append(VulkanStruct(
            architecture, DESCRIPTOR_BUFFER_INFO_ELEMENTS,
            lambda offset, size: little_endian_bytes_to_int(require(
                update_atom.get_read_data(
                    base + i * buffer_info_size + offset, size)))))
    return infos


def get_image_info(update_atom, architecture, base):
    """Returns a VulkanStruct representing the VkDescriptorImageInfo
    struct used in the VkWriteDescriptorSet parameter of the given
    |update_atom| atom."""
    return VulkanStruct(
        architecture, DESCRIPTOR_IMAGE_INFO_ELEMENTS,
        lambda offset, size: little_endian_bytes_to_int(
            require(update_atom.get_read_data(base + offset, size))))


def get_sampler(test):
    """Returns the next VkSampler created in |test|."""
    create = require(test.next_call_of("vkCreateSampler"))
    require_equal(VK_SUCCESS, int(create.return_val))
    require_not_equal(0, create.int_device)
    require_not_equal(0, create.hex_pSampler)
    sampler = little_endian_bytes_to_int(
        require(create.get_write_data(
            create.hex_pSampler, NON_DISPATCHABLE_HANDLE_SIZE)))
    require_not_equal(0, sampler)
    return sampler


@gapit_test("vkUpdateDescriptorSets_test")
class ZeroWritesZeroCopy(GapitTest):

    def expect(self):
        """1. Zero writes and zero copies."""
        update_atom = require(self.nth_call_of("vkUpdateDescriptorSets", 1))
        require_equal(0, update_atom.descriptorWriteCount)
        require_equal(0, update_atom.pDescriptorWrites)
        require_equal(0, update_atom.descriptorCopyCount)
        require_equal(0, update_atom.pDescriptorCopies)


@gapit_test("vkUpdateDescriptorSets_test")
class OneWriteZeroCopy(GapitTest):

    def expect(self):
        """2. One write and zero copies."""

        arch = self.architecture
        # Get the VkDescriptorSet handle returned from the driver.
        # This will also locate us to the proper position in the stream
        # so we can call next_call_of() for querying the other atoms.
        d_set = get_descriptor_set(self, 1)
        buf = get_buffer(self)

        update_atom = require(self.next_call_of("vkUpdateDescriptorSets"))
        require_equal(1, update_atom.descriptorWriteCount)
        require_not_equal(0, update_atom.pDescriptorWrites)
        require_equal(0, update_atom.descriptorCopyCount)
        require_equal(0, update_atom.pDescriptorCopies)

        # Check VkWriteDescriptorSet
        write = get_write_descriptor_set(update_atom, arch, 1)[0]
        require_equal(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, write.sType)
        require_equal(0, write.pNext)
        require_equal(d_set, write.dstSet)
        require_equal(0, write.dstBinding)
        require_equal(0, write.dstArrayElement)
        require_equal(2, write.descriptorCount)
        require_equal(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, write.descriptorType)
        require_equal(0, write.pImageInfo)
        require_not_equal(0, write.pBufferInfo)
        require_equal(0, write.pTexelBufferView)

        # Check VkDescriptorBufferInfo
        bufinfo = get_buffer_info(update_atom, arch, write.pBufferInfo, 2)
        require_equal(buf, bufinfo[0].buffer)
        require_equal(000, bufinfo[0].offset)
        require_equal(512, bufinfo[0].range)
        require_equal(buf, bufinfo[1].buffer)
        require_equal(512, bufinfo[1].offset)
        require_equal(512, bufinfo[1].range)


@gapit_test("vkUpdateDescriptorSets_test")
class TwoWritesZeroCopy(GapitTest):

    def expect(self):
        """3. Two writes and zero copies."""

        arch = self.architecture
        # Get the VkDescriptorSet handle returned from the driver.
        # This will also locate us to the proper position in the stream
        # so we can call next_call_of() for querying the other atoms.
        d_set = get_descriptor_set(self, 2)
        sampler = get_sampler(self)

        update_atom = require(self.next_call_of("vkUpdateDescriptorSets"))
        require_equal(2, update_atom.descriptorWriteCount)
        require_not_equal(0, update_atom.pDescriptorWrites)
        require_equal(0, update_atom.descriptorCopyCount)
        require_equal(0, update_atom.pDescriptorCopies)

        # Check VkWriteDescriptorSet
        writes = get_write_descriptor_set(update_atom, arch, 2)
        for i in range(2):
            require_equal(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                          writes[i].sType)
            require_equal(0, writes[i].pNext)
            require_equal(d_set, writes[i].dstSet)
            require_equal(0, writes[i].dstBinding)
            require_equal(1, writes[i].descriptorCount)
            require_equal(VK_DESCRIPTOR_TYPE_SAMPLER,
                          writes[i].descriptorType)
            require_not_equal(0, writes[i].pImageInfo)
            require_equal(0, writes[i].pBufferInfo)
            require_equal(0, writes[i].pTexelBufferView)
        require_equal(0, writes[0].dstArrayElement)
        require_equal(1, writes[1].dstArrayElement)

        # Check VkDescriptorImageInfo
        imginfo = get_image_info(update_atom, arch, writes[0].pImageInfo)
        require_equal(sampler, imginfo.sampler)
        require_equal(0, imginfo.imageView)
        require_equal(VK_IMAGE_LAYOUT_GENERAL, imginfo.imageLayout)

        imginfo = get_image_info(update_atom, arch, writes[1].pImageInfo)
        require_equal(sampler, imginfo.sampler)
        require_equal(0, imginfo.imageView)
        require_equal(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      imginfo.imageLayout)


@gapit_test("vkUpdateDescriptorSets_test")
class ZeroWritesTwoCopies(GapitTest):

    def expect(self):
        """4. Zero writes and two copies."""

        arch = self.architecture
        # Get the VkDescriptorSet handle returned from the driver.
        # This will also locate us to the proper position in the stream
        # so we can call next_call_of() for querying the other atoms.
        d_set0 = get_descriptor_set(self, 3)
        d_set1 = get_descriptor_set(self, 1)  # the next one after the previous

        # The first call to vkUpdateDesriptorSets writes the descriptors.
        require(self.next_call_of("vkUpdateDescriptorSets"))
        # The second call to vkUpdateDescriptorSets copies the descriptors.
        update_atom = require(self.next_call_of("vkUpdateDescriptorSets"))
        require_equal(0, update_atom.descriptorWriteCount)
        require_equal(0, update_atom.pDescriptorWrites)
        require_equal(2, update_atom.descriptorCopyCount)
        require_not_equal(0, update_atom.pDescriptorCopies)

        # Check VkCopyDescriptorSet
        copies = get_copy_descriptor_set(update_atom, arch, 2)
        for i in range(2):
            require_equal(VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
                          copies[i].sType)
            require_equal(0, copies[i].pNext)
            require_equal(0, copies[i].srcBinding)
            require_equal(0, copies[i].dstBinding)
        require_equal(d_set0, copies[0].srcSet)
        require_equal(d_set1, copies[1].srcSet)
        require_equal(d_set1, copies[0].dstSet)
        require_equal(d_set1, copies[1].dstSet)
        require_equal(0, copies[0].srcArrayElement)
        require_equal(1, copies[1].srcArrayElement)
        require_equal(0, copies[0].dstArrayElement)
        require_equal(3, copies[1].dstArrayElement)
        require_equal(1, copies[0].descriptorCount)
        require_equal(2, copies[1].descriptorCount)
