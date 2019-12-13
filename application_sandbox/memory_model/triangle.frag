/* Copyright 2019 Google Inc.
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
#extension GL_KHR_memory_scope_semantics : require
#pragma use_vulkan_memory_model

layout(location = 0) out vec4 out_color;
layout(location = 1) in vec2 texcoord;

layout (binding = 2, set = 0) buffer counter_data {
    uint counter;
};

void main() {
    uint value = atomicAdd(counter, 1, gl_ScopeQueueFamily, gl_StorageSemanticsBuffer, gl_SemanticsAcquireRelease);
    if (value < 10000){
       out_color = vec4(texcoord, 0.0, 1.0);
    }else
    {
       out_color = vec4(1.0, 1.0, 1.0, 1.0);
    }
}