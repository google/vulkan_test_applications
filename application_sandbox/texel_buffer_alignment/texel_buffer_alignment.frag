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

layout(location = 0) out vec4 out_color;
layout(location = 1) in vec3 normal;

layout(set = 0, binding = 2) uniform samplerBuffer uniformData;
layout(set = 0, binding = 3, r8) uniform imageBuffer storageData;

void main() {
    float r0 = texelFetch(uniformData, 0).r;
    float g0 = texelFetch(uniformData, 1).r;
    float b0 = texelFetch(uniformData, 2).r;
    float r1 = imageLoad(storageData, 0).r;
    float g1 = imageLoad(storageData, 1).r;
    float b1 = imageLoad(storageData, 2).r;

    vec3 u = vec3(r0, g0, b0);
    vec3 s = vec3(r1, g1, b1);
    out_color.rgb = vec3(0.5, 0.5, 0.5) + (normal*u - normal*s)/2.0;
    out_color.a = 1.0;
}
