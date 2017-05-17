# Copyright 2017 Google Inc.
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
#

add_vulkan_static_library(vulkan_helpers
    SOURCES
        helper_functions.h
        helper_functions.cpp
        known_device_infos.h
        known_device_infos.cpp
        structs.h
        structs.cpp
        buffer_frame_data.h
        vulkan_texture.h
        vulkan_model.h
        vulkan_header_wrapper.h
        vulkan_application.h
        vulkan_application.cpp
    LIBS
        vulkan_wrapper
        containers)
