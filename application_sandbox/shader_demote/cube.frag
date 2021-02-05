/* Copyright 2021 Google Inc.
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
layout (location = 1) in vec2 texcoord;

#extension GL_EXT_demote_to_helper_invocation : require

void main() {
	if (length(vec2(texcoord.x - 0.5, texcoord.y - 0.5)) <= 0.25) {
		demote;
	}
    out_color = vec4(texcoord, 0.0, 1.0);
}
