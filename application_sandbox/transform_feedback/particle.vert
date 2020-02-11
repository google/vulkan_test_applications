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

#include "particle_data_shared.h"

layout(location = 0) in vec3 in_position;
layout(location = 1) out vec3 color;

layout (binding = 0) uniform frame {
    Vector4 aspect_data;
};

void main() {
    vec3 position = in_position;
    gl_Position = vec4(position.xy, 0.0f, 1.0f);
    gl_Position.x /= aspect_data.x;

	color = abs(in_position.xyz);

	gl_PointSize = 2.0f;
}