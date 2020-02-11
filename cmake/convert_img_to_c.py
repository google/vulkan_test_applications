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

"""This turns an image file into a c header of the form
struct {
 VK_FORMAT format;
 size_t width;
 size_t height;
 <data_type> data[num_datums];
} model = {
 "Format",
 "width",
 "height",
 {{data_0}, {data_1}, {data_2} ...}
};

"""

import argparse
import sys
import re
from PIL import Image


def main():
    parser = argparse.ArgumentParser(
        description='Convert an image file to a c file:' +
            ' The format is auto-detected')
    parser.add_argument('img', help='apk to run')
    parser.add_argument(
        '-o', default="", help='output filename (Defaults to input.<ext>.h)')
    parser.add_argument(
        '--verbose', action='store_true', help='enable verbose output')
    args = parser.parse_args()
    if not args.o:
        args.o = args.img + ".h"

    data_types = {
        "1": "uint8_t",
        "L": "uint8_t",
        "RGB": "struct {uint8_t r; uint8_t g; uint8_t b; uint8_t a;}",
        "RGBA": "struct {uint8_t r; uint8_t g; uint8_t b; uint8_t a;}",
        "I": "uint32_t",
        "F": "float",
        "YCbCr": "uint8_t"
    }

    vulkan_types = {
        "1": "VK_FORMAT_R8_UNORM",
        "L": "VK_FORMAT_R8_UNORM",
        # We use RGBA8 here because RGB is not a guaranteed format
        "RGB": "VK_FORMAT_R8G8B8A8_UNORM",
        "RGBA": "VK_FORMAT_R8G8B8A8_UNORM",
        "I": "VK_FORMAT_R32_UNORM",
        "F": "VK_FORMAT_R32_SFLOAT",
        # We use G8_B8_R8_3PLANE_420_UNORM here because it the most widely supported multiplanar format
        "YCbCr": "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM"
    }

    try:
        image = Image.open(args.img)
        data = image.getdata()

        if (".jpg" in args.img):
            image.mode = "YCbCr"

        with open(args.o, "w") as f:
            f.write("const struct {\n")
            f.write(" VkFormat format;\n")
            f.write(" size_t width;")
            f.write(" size_t height;")
            
            if (image.mode == "YCbCr"):
                f.write(
                    " " + data_types[image.mode] +
                    " data[" + str(image.size[0] * image.size[1] * 2) + "];\n} texture = { \n")
            else:
                f.write(
                    " " + data_types[image.mode] +
                    " data[" + str(image.size[0] * image.size[1]) + "];\n} texture = { \n")
            
            f.write("   " + vulkan_types[image.mode] + ",\n")
            f.write("   " + str(image.size[0]) + ",\n")
            f.write("   " + str(image.size[1]) + ",\n")
            f.write("   {\n")

            if (image.mode == "YCbCr"):
                images = image.split()
                image_plane_count = min(len(images), 3)
                for i in range(0, image_plane_count):
                    image = images[(i + 2) % 3]
                    if (i != 0):
                        image = image.resize((int(image.width / 2), int(image.height / 2)))
                    for j in range(0, image.size[0]):
                        if j != 0:
                            f.write(",\n")
                        f.write("       ")
                        for k in range(0, image.size[1]):
                            if k != 0:
                                f.write(", ")
                            pixel = image.getpixel((j, k))
                            f.write(str(pixel))
                    if i != (image_plane_count - 1):
                        f.write(",\n")
            else:
                for i in range(0, image.size[0]):
                    if i != 0:
                        f.write(",\n")
                    f.write("       ")
                    for j in range(0, image.size[1]):
                        if j != 0:
                            f.write(", ")
                        pixel = image.getpixel((i, j))
                        if isinstance(pixel, tuple):
                            f.write("{")
                            for j in range(0, len(pixel)):
                                if j != 0:
                                    f.write(", ")
                                f.write(str(pixel[j]))
                            f.write("}")
                        else:
                            f.write(str(pixel))
            f.write("\n   }\n")
            f.write("};\n")
    except IOError as err:
        print(err)
        return -1
    return 0

if __name__ == '__main__':
    sys.exit(main())
