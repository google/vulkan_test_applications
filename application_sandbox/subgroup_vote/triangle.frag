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
#extension GL_ARB_shader_group_vote: require

layout(location = 0) out vec4 out_color;
layout(location = 1) in vec2 texcoord;

void main() {
    if (allInvocationsARB(fract(texcoord.x * 10.0f) > 0.5))
    {
       out_color = vec4(texcoord, 0.0, 1.0);
    }else
    {
       out_color = vec4(1.0, 1.0, 1.0, 1.0);
    }
}