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

layout (location = 1) out vec2 texcoord;

layout (binding = 0, set = 0) uniform camera_data {
    layout(column_major) mat4x4 projection;
};

layout (binding = 1, set = 0) uniform model_data {
    layout(column_major) mat4x4 transform;
};

void main() {
    gl_Position =  projection * transform * get_position();
    texcoord = get_texcoord();
}