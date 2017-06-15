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

"""This turns a .obj file into a c header of the form
struct {
 size_t num_vertices;
 float positions[num_vertices*3];
 float uv[num_vertices*2];
 float normals[num_vertices*3];
 size_t num_indices;
 uint32_t indices[num_indices];
} model = {
 "num_vertices",
 {"vertex0.x", "vertex0.y", "vertex0.z", "vertex1.x", "vertex1.y", ...},
 {"vertex0.u", "vertex0.v", "vertex1.u", "vertex1.v", ...},
 {"vertex0.nx", "vertex0.ny", "vertex0.nz", "vertex1.nx", ...},
 "num_indices",
 {"index0", "index1", "index2", "index3" ... }
};

The result is an indexed vertex-list
"""

import argparse
import sys
import re


def main():
    parser = argparse.ArgumentParser(
        description='Convert a .obj file to a c file')
    parser.add_argument('obj', help='apk to run')
    parser.add_argument(
        '-o', default="", help='output filename (Defaults to input.obj.h)')
    parser.add_argument(
        '--verbose', action='store_true', help='enable verbose output')
    args = parser.parse_args()
    if not args.o:
        args.o = args.obj + ".h"

    positions = []
    tex_coords = []
    normals = []
    faces = []
    vertices = []
    indices = []

    number = r'(-?\d+(?:.\d+)?(?:e-?\d+)?)'

    position_matcher = re.compile(
        r'v ' + number + r' ' + number + r' ' + number + r'.*')
    texture_matcher = re.compile(
        r'vt ' + number + r' ' + number + r'.*')
    normal_matcher = re.compile(
        r'vn ' + number + r' ' + number + r' ' + number + r'.*')
    face_matcher = re.compile(
        r'f (\d+)/(\d+)/(\d+) (\d+)/(\d+)/(\d+) (\d+)/(\d+)/(\d+)')

    try:
        with open(args.obj, "r") as f:
            content = f.readlines()
            for line in content:
                if line.startswith('v '):
                    match = position_matcher.match(line)
                    if not match:
                        print 'Could not parse vertex ' + line
                        return -1
                    positions.append(
                        (float(match.group(1)),
                         float(match.group(2)),
                         float(match.group(3))))
                elif line.startswith('vt '):
                    match = texture_matcher.match(line)
                    if not match:
                        print 'Could not parse texture_coord ' + line
                        return -1
                    tex_coords.append(
                        (float(match.group(1)),
                         float(match.group(2))))
                elif line.startswith('vn '):
                    match = normal_matcher.match(line)
                    if not match:
                        print 'Could not parse normal ' + line
                        return -1
                    normals.append(
                        (float(match.group(1)),
                         float(match.group(2)),
                         float(match.group(3))))
                elif line.startswith('f '):
                    match = face_matcher.match(line)
                    if not match:
                        print 'Could not parse face ' + line
                        return -1
                    face_verts = [
                        (int(match.group(1)),
                         int(match.group(2)),
                         int(match.group(3))),
                        (int(match.group(4)),
                         int(match.group(5)),
                         int(match.group(6))),
                        (int(match.group(7)),
                         int(match.group(8)),
                         int(match.group(9))),
                    ]
                    constructed_face_verts = [
                        (positions[face_verts[0][0] - 1],
                         tex_coords[face_verts[0][1] - 1],
                         normals[face_verts[0][2] - 1]),
                        (positions[face_verts[1][0] - 1],
                         tex_coords[face_verts[1][1] - 1],
                         normals[face_verts[1][2] - 1]),
                        (positions[face_verts[2][0] - 1],
                         tex_coords[face_verts[2][1] - 1],
                         normals[face_verts[2][2] - 1])
                    ]
                    for cfv in constructed_face_verts:
                        if cfv in vertices:
                            indices.append(vertices.index(cfv))
                        else:
                            vertices.append(cfv)
                            indices.append(len(vertices) - 1)
        with open(args.o, "w") as f:
            num_vertices = len(vertices)
            f.write("const struct {\n")
            f.write("    size_t num_vertices;\n")
            f.write("    float positions[" + str(num_vertices * 3) + "];\n")
            f.write("    float uv[" + str(num_vertices * 2) + "];\n")
            f.write("    float normals[" + str(num_vertices * 3) + "];\n")
            f.write("    size_t num_indices;\n")
            f.write("    uint32_t indices[" + str(len(indices)) + "];\n")
            f.write("} model = {\n")
            f.write(str(len(vertices)) + ",\n")
            f.write("/*positions*/      {")
            first_element = True
            for vertex in vertices:
                if not first_element:
                    f.write(", ")
                f.write(str(vertex[0][0]) + "f, " + str(vertex[0][1]) +
                        "f, " + str(vertex[0][2]) + "f")
                first_element = False
            f.write("},\n")
            f.write("/*texture_coords*/ {")
            first_element = True
            for vertex in vertices:
                if not first_element:
                    f.write(", ")
                f.write(str(vertex[1][0]) + "f, " + str(vertex[1][1]) + "f")
                first_element = False
            f.write("},\n")
            f.write("/*normals*/        {")
            first_element = True
            for vertex in vertices:
                if not first_element:
                    f.write(", ")
                f.write(str(vertex[2][0]) + "f, " + str(vertex[2][1]) +
                        "f, " + str(vertex[2][2]) + "f")
                first_element = False
            f.write("},\n")
            f.write(str(len(indices)) + ",\n")
            f.write("/*indices*/        {")
            first_element = True
            for index in indices:
                if not first_element:
                    f.write(", ")
                f.write(str(index))
                first_element = False
            f.write("}\n")
            f.write("};\n")
            f.write("#ifndef _WIN32\n")
            f.write("static_assert(model.positions + " +
                    str(num_vertices * 3) + " == model.uv, " +
                    "\"Memory layout is not as expected\");\n")
            f.write("static_assert(model.uv + " +
                    str(num_vertices * 2) + " == model.normals, " +
                    "\"Memory layout is not as expected\");\n")
            f.write("#endif\n")
    except IOError as err:
        print err
        return -1
    return 0

if __name__ == '__main__':
    sys.exit(main())
