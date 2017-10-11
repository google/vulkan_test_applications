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

# Log must exist first since other libraries depend on it
add_vulkan_subdirectory(log)

add_vulkan_subdirectory(android_native_app_glue)
add_vulkan_subdirectory(containers)
add_vulkan_subdirectory(dynamic_loader)
add_vulkan_subdirectory(entry)
add_vulkan_subdirectory(math_common)
