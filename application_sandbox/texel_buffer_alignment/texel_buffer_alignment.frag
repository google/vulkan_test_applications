/* Copyright 2022 Google Inc.
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

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texcoord;

layout (location = 0) out vec4 out_color;

layout(set = 0, binding = 2) uniform samplerBuffer uniformData;
layout(set = 0, binding = 3, r8) uniform imageBuffer storageData;

void main() {
    int idx = int(floor(3*texcoord.y));
    float texel = 0;
    if (texcoord.x < 0.5) {
        texel = texelFetch(uniformData, idx).r;
    } else {
        texel = imageLoad(storageData, idx).r;
    }
    float lighting = abs(normal.z);
    vec3 color = vec3(1.0, 1.0, 1.0);
    out_color.rgb = texel*lighting*color;
    out_color.a = 1.0;
}
