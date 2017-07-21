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
''' Converts SPIR-V binary into a C-style array of 32-bit words.
'''

import argparse
import binascii
import codecs
import os
import struct
import sys


def main():
    """Entry-point for SPIR-V binary conversion into C-style array of words"""
    parser = argparse.ArgumentParser(description='''
This applications converts SPIR-V binary into a C-style array of 32-bit words.
The output file name can be specified using "--output-file" option.
If not specified, the output file will had the same name as the input file
with an added ".c" suffix.
        ''')
    parser.add_argument("--output-file", nargs='?', help="Output file name")
    parser.add_argument('input_file', metavar='<filename>',
                        nargs=1, help='Input file name')

    args = parser.parse_args()
    input_file = args.input_file[0]
    output_file = input_file + ".c"
    if not args.output_file:
        print "No --output-file option was specified. The output will be " + \
            output_file
    else:
        input_file_dir = os.path.dirname(os.path.realpath(input_file))
        output_file = os.path.join(input_file_dir, args.output_file)

    print "input file = " + input_file
    print "output file = " + output_file

    words = []
    infile = open(input_file, mode='rb')
    try:
        # Read 4 bytes (32-bit word)
        word = infile.read(4)
        while word != "":
            word = struct.unpack('I', word)[0]
            words.append(word)
            word = infile.read(4)

    finally:
        infile.close()

    outfile = open(output_file, mode='w')
    outfile.write("{")
    first_word = True
    for word in words:
        if not first_word:
            outfile.write(",")
        first_word = False
        hex_word = "0x%0.8X" % word
        outfile.write(hex_word)
    outfile.write("}")
    outfile.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
