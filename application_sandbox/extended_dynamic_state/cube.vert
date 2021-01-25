/* Copyright 2020 Google Inc.
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
#include "models/model_setup.glsl"

layout (location = 1) out vec2 texcoord;

layout (binding = 0, set = 0) uniform camera_data {
    layout(column_major) mat4x4 projection;
};

layout (binding = 1, set = 0) uniform model_data {
    layout(column_major) mat4x4 transform;
};

void main() {
    vec4 posn = transform * get_position();
	float x_offset = -2.0 + (gl_InstanceIndex % 2) * 4.0f;
	float y_offset = -1.0;
	if (gl_InstanceIndex > 1) {
		y_offset = 1.0f;
	}
	float z_offset = 0.0f;
	if (gl_InstanceIndex == 0) {
		z_offset = -4.0f;
	}
	posn += vec4(x_offset, y_offset, z_offset, 0);
    gl_Position =  projection * posn;
    texcoord = get_texcoord();
}
