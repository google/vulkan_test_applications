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

add_vulkan_static_library(vulkan_wrapper
    SOURCES
        command_buffer_wrapper.h
        descriptor_set_wrapper.h
        device_wrapper.h
        function_table.h
        instance_wrapper.h
        lazy_function.h
        library_wrapper.h
        library_wrapper.cpp
        sub_objects.h
        swapchain.h
    LIBS
        dynamic_loader
        containers)
