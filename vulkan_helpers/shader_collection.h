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

#ifndef VULKAN_HELPERS_SHADER_COLLECTION_H_
#define VULKAN_HELPERS_SHADER_COLLECTION_H_

#include <cassert>

namespace vulkan {

// This class can be used to multiplex between different shaders:
// GLSL shaders compiled to SPIR-V through glslc
// HLSL shaders compiled to SPIR-V through glslc
// HLSL shaders compiled to SPIR-V through DXC
// The class can return appropriate shaders based on the given 'shader_compiler'
// configuration: shader-compiler={dxc-hlsl, glslc-glsl, glslc-hlsl}
class ShaderCollection {
 public:
  using u32vec = std::vector<uint32_t>;
  ShaderCollection(logging::Logger* log, const char* shader_compiler,
                   u32vec& glslc_glsl_vertex_shader,
                   u32vec& glslc_glsl_fragment_shader,
                   u32vec& glslc_hlsl_vertex_shader,
                   u32vec& glslc_hlsl_fragment_shader,
                   u32vec& dxc_hlsl_vertex_shader,
                   u32vec& dxc_hlsl_fragment_shader) {
    // Find out the shader compiler
    if (strncmp(shader_compiler, "glslc-glsl", 10) == 0) {
      vertex_shader = glslc_glsl_vertex_shader.data();
      fragment_shader = glslc_glsl_fragment_shader.data();
      vertex_shader_word_count = (uint32_t)glslc_glsl_vertex_shader.size();
      fragment_shader_word_count = (uint32_t)glslc_glsl_fragment_shader.size();
    } else if (strncmp(shader_compiler, "glslc-hlsl", 10) == 0) {
      vertex_shader = glslc_hlsl_vertex_shader.data();
      fragment_shader = glslc_hlsl_fragment_shader.data();
      vertex_shader_word_count = (uint32_t)glslc_hlsl_vertex_shader.size();
      fragment_shader_word_count = (uint32_t)glslc_hlsl_fragment_shader.size();
    } else if (strncmp(shader_compiler, "dxc-hlsl", 8) == 0) {
      vertex_shader = dxc_hlsl_vertex_shader.data();
      fragment_shader = dxc_hlsl_fragment_shader.data();
      vertex_shader_word_count = (uint32_t)dxc_hlsl_vertex_shader.size();
      fragment_shader_word_count = (uint32_t)dxc_hlsl_fragment_shader.size();
    } else {
      LOG_ASSERT(==, log, shader_compiler,
                 "glslc-glsl or glslc-hlsl or dxc-hlsl");
    }
  }
  uint32_t* vertexShader() { return vertex_shader; }
  uint32_t* fragmentShader() { return fragment_shader; }
  uint32_t vertexShaderWordCount() { return vertex_shader_word_count; }
  uint32_t fragmentShaderWordCount() { return fragment_shader_word_count; }

 private:
  // Selected shader
  uint32_t* vertex_shader;
  uint32_t* fragment_shader;
  uint32_t vertex_shader_word_count;
  uint32_t fragment_shader_word_count;
};

}  // namespace vulkan

#endif  //  VULKAN_HELPERS_SHADER_COLLECTION_H_
