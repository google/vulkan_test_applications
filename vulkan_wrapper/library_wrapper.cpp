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

#include "vulkan_wrapper/library_wrapper.h"

namespace vulkan {

LibraryWrapper::LibraryWrapper(containers::Allocator* allocator,
                               logging::Logger* logger)
    : logger_(logger) {
  vulkan_lib_ = dynamic_loader::OpenLibrary(allocator, "vulkan");
  if (vulkan_lib_) {
    if (vulkan_lib_->is_valid()) {
      logger_->LogInfo("Successfully opened vulkan library");
      vulkan_lib_->Resolve("vkGetInstanceProcAddr", &vkGetInstanceProcAddr);
    }
    if (!vkGetInstanceProcAddr) {
      vulkan_lib_ = nullptr;
      logger_->LogError(
          "Could not resolve vkGetInstanceProcAddr from libvulkan");
    } else {
      logger_->LogInfo("Resolved vkGetInstanceProcAddr.");
    }
  } else {
    logger_->LogError("Could not find libvulkan");
  }
}

PFN_vkVoidFunction LibraryWrapper::getProcAddr(::VkInstance instance,
                                               const char* function) {
  return vkGetInstanceProcAddr(instance, function);
}
}  // namespace vulkan
