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
#extension GL_ARB_enhanced_layouts: require

#include "particle_data_shared.h"

layout(location = 0) in vec3 in_position;
layout (location = 1, xfb_buffer = 0, xfb_offset = 0) out vec3 out_position;

layout (binding = 0) uniform frame {
    Vector4 aspect_data;
};

void main() {
    gl_PointSize = 2.0f;

    out_position = in_position;

	float rot_val = aspect_data.y * 3.14f * 0.5f;
	mat2 rot_mat = mat2(cos(rot_val), -sin(rot_val), sin(rot_val), cos(rot_val));

	out_position.xy = rot_mat * out_position.xy;
}
