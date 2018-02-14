/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#version 450

layout(location = 0) out vec4 out_color;
out float gl_FragDepth;

layout(input_attachment_index = 0, binding = 0, set = 0) uniform usubpassInput color_data;
layout(input_attachment_index = 1, binding = 0, set = 0) uniform usubpassInput depth_data;

void main() {
    uint u = subpassLoad(color_data).r;
    out_color[0] = (u&0xFF)/255.0;
    out_color[1] = ((u>>8)&0xFF)/255.0;
    out_color[2] = ((u>>16)&0xFF)/255.0;
    out_color[3] = ((u>>24)&0xFF)/255.0;

    gl_FragDepth = (subpassLoad(depth_data).r)/float(uint(0xFFFFFFFF));
}