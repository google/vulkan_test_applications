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
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstant {
    float time;
} pushConstant;

layout(location = 0) out vec3 fragColor;

float theta[3] = float[](2.4981, 0.6435, 4.71238898038);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    vec2 pos = vec2(cos(theta[gl_VertexIndex] + pushConstant.time), sin(theta[gl_VertexIndex] + pushConstant.time));
    vec2 adjPos = vec2(0.0, 0.125) + 0.625 * pos;
    gl_Position = vec4(adjPos, 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
