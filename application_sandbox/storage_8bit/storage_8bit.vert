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

#extension GL_EXT_shader_8bit_storage: require

#include "models/model_setup.glsl"

layout (location = 1) out vec2 texcoord;
layout (location = 2) out float red;
layout (location = 3) out float green;
layout (location = 4) out float blue;

layout (binding = 0, set = 0) uniform camera_data {
    layout(column_major) mat4x4 projection;
};

layout (binding = 1, set = 0) uniform model_data {
    layout(column_major) mat4x4 transform;
};

layout (binding = 2, set = 0) uniform color_data {
	uint8_t red_amount;
	uint8_t green_amount;
	uint8_t blue_amount;
};

void main() {
    gl_Position =  projection * transform * get_position();
    texcoord = get_texcoord();
	uint r = uint(red_amount);
	uint g = uint(green_amount);
	uint b = uint(blue_amount);
	red = float(r) / 255.0f;
	green = float(g) / 255.0f;
	blue = float(b) / 255.0f;
}