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

struct VSInput {
    float3 _position : POSITION;
    float2 _texture_coord : TEXCOORD;
    float3 _normal : NORMAL;
};

struct VSOutput {
    float4 Position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

cbuffer camera_data : register(b0) {
    float4x4 projection;
};

cbuffer model_data : register(b1) {
    float4x4 transform;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.Position =  mul(mul(projection, transform), float4(input._position.xyz, 1.0f));
    //output.Position = float4(input._position.x, -input._position.y, input._position.z, 1.0f);
    
    output.texcoord = input._texture_coord;
    return output;
}