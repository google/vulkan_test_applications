# Copyright 2018 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

FROM ubuntu

COPY . /root/vulkan_test_applications

RUN apt-get update -qq && \
apt-get install -qq -y git gcc g++ ninja-build python python-pip cmake libxcb1-dev

RUN pip install pillow

WORKDIR /root
RUN git clone https://github.com/google/shaderc.git && \
git clone https://github.com/google/googletest.git /root/shaderc/third_party/googletest && \
git clone https://github.com/google/glslang.git /root/shaderc/third_party/glslang && \
git clone https://github.com/KhronosGroup/SPIRV-Tools.git /root/shaderc/third_party/spirv-tools && \
git clone https://github.com/KhronosGroup/SPIRV-Headers.git /root/shaderc/third_party/spirv-tools/external/spirv-headers

WORKDIR /root/shaderc/build
RUN cmake -GNinja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DSHADERC_SKIP_TESTS=ON \
  -DSPIRV_SKIP_TESTS=ON .. && \
ninja glslc_exe

WORKDIR /root/vulkan_test_applications/build-linux
RUN cmake -GNinja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_GLSL_COMPILER=/root/shaderc/build/glslc/glslc ..

CMD ["ninja", "all"]
