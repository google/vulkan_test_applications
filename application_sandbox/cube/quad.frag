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
layout(input_attachment_index = 0, binding = 0, set = 0) uniform usubpassInput color_data;

void main() {
    uint r = subpassLoad(color_data).r;
    uint g = subpassLoad(color_data).g;
    uint b = subpassLoad(color_data).b;
    uint a = subpassLoad(color_data).a;
    out_color.r = (r&0xFF)/255.0;
    out_color.g = (g&0xFF)/255.0;
    out_color.b = (b&0xFF)/255.0;
    out_color.a = (a&0xFF)/255.0;

    // out_color = vec4(1.0, 0.0, 0.0, 1.0);
}