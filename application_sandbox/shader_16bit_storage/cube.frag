/* Copyright 2021 Google Inc.
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

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types_float16: require

layout (location = 0) out vec4 out_color;
layout (location = 1) in vec2 texcoord;

layout (binding = 2, set = 0) uniform readonly color_data {
    u16vec3 color_amounts;
};

void main() {
    uint r = uint(color_amounts.r);
    uint g = uint(color_amounts.g);
    uint b = uint(color_amounts.b);
    float max_channel_value = float(255.0f);
    float red = float(r) / max_channel_value;
    float green = float(g) / max_channel_value;
    float blue = float(b) / max_channel_value;

    out_color = vec4(texcoord * vec2(red, green), blue, 1.0);
}