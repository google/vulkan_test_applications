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
#include "models/model_setup.glsl"
#include "particle_data_shared.h"

layout (location = 1) out vec2 texcoord;
layout (location = 2) out float speed;

layout (binding = 0) buffer DrawData {
  draw_data drawData[];
};

layout (binding = 3) buffer frame {
    Vector4 aspect_data;
};

void main() {
    vec4 position = get_position() / 250.0f;
    gl_Position =
        vec4(position.xy + drawData[gl_InstanceIndex].position_speed.xy, 0.0f, 1.0f);
    gl_Position.x /= aspect_data.x;
    texcoord = get_texcoord();
    speed = length(drawData[gl_InstanceIndex].position_speed.zw);
}