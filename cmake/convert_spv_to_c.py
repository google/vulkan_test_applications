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

"""This turns a SPIR-V binary into a file with contents that look like
a C initializer list."""

import argparse
import sys
import struct


def main():
    parser = argparse.ArgumentParser(
        description='Convert an .spv file to a c file:')
    parser.add_argument('spv', help='SPIR-V binary file to convert')
    parser.add_argument(
        '-o', default="", help='output filename (Defaults to <input>.inc)')
    args = parser.parse_args()
    if not args.o:
        args.o = args.spv + ".inc"

    try:
        first = True
        with open(args.o, "w") as out:
            out.write("{")
            with open(args.spv, "rb") as spv:
                for word in iter(lambda: spv.read(4), ''):
                    if not first:
                        out.write(",\n")
                    value = struct.unpack("<I", word)[0]
                    out.write(str(value))
                    first = False
            out.write("}\n")
    except IOError as err:
        print err
        return -1
    return 0

if __name__ == '__main__':
    sys.exit(main())
