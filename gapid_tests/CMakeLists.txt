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

add_custom_target(GAPID_TESTS)

function(add_gapid_test name)
    add_vulkan_executable(${name} ${ARGN})
    add_dependencies(GAPID_TESTS ${name})
endfunction()

add_subdirectory(command_buffer_tests)
add_subdirectory(extension_tests)
add_subdirectory(initialization_tests)
add_subdirectory(resource_acquisition_tests)
add_subdirectory(resource_binding_tests)
add_subdirectory(resource_creation_tests)
add_subdirectory(synchronization_tests)
add_subdirectory(traits_query_tests)
