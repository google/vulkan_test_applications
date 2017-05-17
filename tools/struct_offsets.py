#!/usr/bin/python
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
"""This contains functions determining element offsets in a structure.

Executing this file directly will run its tests.
This only works for single-nested structures. This also only
handles arrays of scalars, and not arrays of arrays of elements.
"""

import struct
import sys

# These values are meant to be used with the struct layout functions
UINT32_T, INT32_T, SIZE_T, POINTER, UINT64_T, FLOAT, CHAR, ARRAY = range(8)

BOOL32 = UINT32_T
HANDLE = UINT64_T
DEVICE_SIZE = UINT64_T


def get_size_alignment(architecture):
    """Returns a dictionary of struct tags to tuples of size and
    alignment requirements."""
    return {
        UINT32_T: (4, 4),
        INT32_T: (4, 4),
        SIZE_T: (architecture.int_sizeSize, architecture.int_sizeAlignment),
        POINTER: (architecture.int_pointerSize, architecture.int_pointerAlignment),
        UINT64_T: (architecture.int_i64Size, architecture.int_i64Alignment),
        FLOAT: (4, 4),
        CHAR: (1, 1)
    }


def align_to_next(number, alignment):
    """Returns number if number % alignment is 0 or returns the next
    number such that number % alignment is 0"""
    if number % alignment == 0:
        return number
    return number + alignment - (number % alignment)


def decode(data, ty):
    """Decodes data according to the given type."""
    assert ty != ARRAY, "ARRAY is not supported"
    if (ty == CHAR or ty == UINT32_T or ty == UINT64_T or
            ty == SIZE_T or ty == POINTER or ty == INT32_T):
        return long(data)  # Just use long for all of them.
    if ty == FLOAT:
        val = struct.unpack('f', struct.pack('I', data))  # Do a bitcast.
        assert len(val) == 1, "more than 4-bytes found for float"
        return val[0]
    assert False, "unknown type: {}".format(ty)


class VulkanStruct(object):

    """Represents a vulkan structure.

    Given an array of elements, which are represented as pairs of element name
    and type tag, and a |get_data| function, which returns an object given
    an offset and size, this class allows querying the offset and values of
    any individual element.
    """

    def __init__(self, architecture, elements, get_data):
        self.offsets = []
        self.parameters = {}
        self.total_size = 0
        self.build_struct(architecture, elements, get_data)

    def __getattr__(self, name):
        if name in self.parameters:
            return self.parameters[name]
        raise AttributeError("Could not find struct member " + name + "\n")

    def get_offset_of_nth_element(self, element):
        return self.offsets[element]

    def build_struct(self, architecture, elements, get_data):
        offset = 0
        sizes_and_alignments = get_size_alignment(architecture)
        largest_alignment = 0

        for element in elements:
            name = element[0]
            ty = element[1]
            if ty is ARRAY:
                array_size = element[2]
                array_member_ty = element[3]
                size_and_alignment = sizes_and_alignments[array_member_ty]
                if (size_and_alignment[1] > largest_alignment):
                    largest_alignment = size_and_alignment[1]
                offset = align_to_next(offset, size_and_alignment[1])
                self.offsets.append(offset)
                value = []
                for i in range(array_size):
                    value.append(
                        decode(get_data(offset, size_and_alignment[0]),
                               array_member_ty))
                    offset += size_and_alignment[0]
                self.parameters[name] = value
            else:
                size_and_alignment = sizes_and_alignments[ty]
                if (size_and_alignment[1] > largest_alignment):
                    largest_alignment = size_and_alignment[1]
                offset = align_to_next(offset, size_and_alignment[1])
                self.offsets.append(offset)
                value = get_data(offset, size_and_alignment[0])
                self.parameters[name] = decode(value, ty)
                offset += size_and_alignment[0]
        self.total_size = align_to_next(offset, largest_alignment)


def expect_offset_eq(struct_name, pointer_size, u64_alignment, struct_elements,
                     offsets):
    """Creates a VulkanStruct with the given pointer_size, u64_alignment, and
    elements.

    Checks that the offsets calculated are the same as the offsets
    expected.

    A message will be printed identifying the test and the result.
    If there is a failure, then it will return false.
    """
    # Create a mock architecture object. The only thing the object requires
    # is pointer_size
    test_name = struct_name + "." + \
        str(pointer_size) + "." + str(u64_alignment)
    architecture = type("Architecture", (),
                        {"int_PointerSize": pointer_size,
                         "extra_FieldAlignments": type("FieldAlignments", (), {
                             "int_U64Alignment": u64_alignment
                         })})
    # We don't care the values of the struct members here
    dummy_data_getter = lambda offset, size: 0
    struct = VulkanStruct(architecture, struct_elements, dummy_data_getter)
    success = True
    print "[ " + "RUN".ljust(10) + " ] " + test_name
    for i in range(0, len(offsets)):
        if struct.get_offset_of_nth_element(i) != offsets[i]:
            print "Error: Element " + str(
                i) + " did not have the expected offset"
            print "   Expected: [" + str(offsets[i]) + "] but got [" + str(
                struct.get_offset_of_nth_element(i)) + "]"
            success = False
    if success:
        print "[ " + "OK".rjust(10) + " ] " + test_name
        return True
    else:
        print "[ " + "FAIL".rjust(10) + " ] " + test_name
        return False


def test_structs_offsets(name, elements, x86_offsets, x86_64_offsets,
                         armv7a_offsets, arm64_offsets):
    """Runs expect_offset_eq with the given test name for  each of the given
    architectures."""
    success = True
    success &= expect_offset_eq(name + ".x86", 4, 4, elements, x86_offsets)
    success &= expect_offset_eq(name + ".x64_64", 8, 8, elements,
                                x86_64_offsets)
    success &= expect_offset_eq(name + ".armv7a", 4, 8, elements,
                                armv7a_offsets)
    success &= expect_offset_eq(name + ".arm64", 8, 8, elements, arm64_offsets)
    return success


def main():
    success = True
    success &= test_structs_offsets(
        "uint32_struct", [("first", UINT32_T)],
        x86_offsets=[0],
        x86_64_offsets=[0],
        armv7a_offsets=[0],
        arm64_offsets=[0])

    success &= test_structs_offsets(
        "2uint32_struct", [("first", UINT32_T), ("second", UINT32_T)],
        x86_offsets=[0, 4],
        x86_64_offsets=[0, 4],
        armv7a_offsets=[0, 4],
        arm64_offsets=[0, 4])

    success &= test_structs_offsets(
        "pointer_struct", [("first", UINT32_T), ("second", POINTER),
                           ("third", UINT32_T)],
        x86_offsets=[0, 4, 8],
        x86_64_offsets=[0, 8, 16],
        armv7a_offsets=[0, 4, 8],
        arm64_offsets=[0, 8, 16])

    success &= test_structs_offsets(
        "size_t_struct", [("first", SIZE_T), ("second", POINTER),
                          ("third", UINT32_T)],
        x86_offsets=[0, 4, 8],
        x86_64_offsets=[0, 8, 16],
        armv7a_offsets=[0, 4, 8],
        arm64_offsets=[0, 8, 16])

    success &= test_structs_offsets(
        "handle_struct", [("first", UINT32_T), ("second", HANDLE),
                          ("third", SIZE_T), ("forth", HANDLE)],
        x86_offsets=[0, 4, 12, 16],
        x86_64_offsets=[0, 8, 16, 24],
        armv7a_offsets=[0, 8, 16, 24],
        arm64_offsets=[0, 8, 16, 24])

    success &= test_structs_offsets(
        "char_struct", [("first", CHAR), ("second", CHAR), ("third", UINT32_T),
                        ("forth", CHAR)],
        x86_offsets=[0, 1, 4, 8],
        x86_64_offsets=[0, 1, 4, 8],
        armv7a_offsets=[0, 1, 4, 8],
        arm64_offsets=[0, 1, 4, 8])

    success &= test_structs_offsets(
        "array_struct_middle", [("first", UINT32_T),
                                ("second", ARRAY, 4, UINT32_T)],
        x86_offsets=[0, 4],
        x86_64_offsets=[0, 4],
        armv7a_offsets=[0, 4],
        arm64_offsets=[0, 4])

    success &= test_structs_offsets(
        "array_struct_start", [("first", ARRAY, 3, UINT32_T),
                               ("second", SIZE_T)],
        x86_offsets=[0, 12],
        x86_64_offsets=[0, 16],
        armv7a_offsets=[0, 12],
        arm64_offsets=[0, 16])
    return success


if __name__ == "__main__":
    sys.exit(0 if main() else -1)
