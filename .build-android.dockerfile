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

# Needed to not be stuck on apt commands that trigger a tzdata update
# that expects user input to select a timezone.
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq && \
apt-get install -qq -y git gcc g++ ninja-build python3 python3-pip cmake libxcb1-dev openjdk-8-jdk wget unzip && \
pip3 install pillow

WORKDIR /root
RUN git clone https://github.com/google/shaderc.git && \
git clone https://github.com/google/googletest.git /root/shaderc/third_party/googletest && \
git clone https://github.com/KhronosGroup/glslang.git /root/shaderc/third_party/glslang && \
git clone https://github.com/KhronosGroup/SPIRV-Tools.git /root/shaderc/third_party/spirv-tools && \
git clone https://github.com/KhronosGroup/SPIRV-Headers.git /root/shaderc/third_party/spirv-tools/external/spirv-headers

WORKDIR /root/shaderc/build
RUN cmake -GNinja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DSHADERC_SKIP_TESTS=ON \
  -DSPIRV_SKIP_TESTS=ON .. && \
ninja glslc_exe && ninja glslc/install

WORKDIR /root/sdk
RUN wget -q https://dl.google.com/android/repository/sdk-tools-linux-4333796.zip && \
unzip -qq sdk-tools-linux-4333796.zip && rm -f sdk-tools-linux-4333796.zip && \
yes | ./tools/bin/sdkmanager "platforms;android-26" "build-tools;25.0.0" ndk-bundle > /dev/null && \
yes | ./tools/bin/sdkmanager "cmake;3.6.4111459" > /dev/null

WORKDIR /root/vulkan_test_applications/build-android
RUN cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -DANDROID_SDK=/root/sdk \
  -DANDROID_NDK=/root/sdk/ndk-bundle -DBUILD_APKS=ON \
  -DCMAKE_GLSL_COMPILER=glslc -DANDROID_ABIS=armeabi-v7a ..

CMD ["ninja", "-j2", "application_sandbox/all"]
