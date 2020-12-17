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

#ifndef VULKAN_WRAPPER_LIBRARY_WRAPPER_H_
#define VULKAN_WRAPPER_LIBRARY_WRAPPER_H_

#include "support/containers/allocator.h"
#include "support/containers/unique_ptr.h"
#include "support/dynamic_loader/dynamic_library.h"
#include "support/log/log.h"
#include "vulkan_helpers/vulkan_header_wrapper.h"
#include "vulkan_wrapper/lazy_function.h"

namespace vulkan {

class LibraryWrapper;
template <typename T>
using LazyLibraryFunction = LazyFunction<T, ::VkInstance, LibraryWrapper>;
// This wraps the vulkan library. It provides lazily initialized functions
// for all global-scope functions.
class LibraryWrapper {
 public:
  LibraryWrapper(containers::Allocator* allocator, logging::Logger* logger);
  bool is_valid() { return vulkan_lib_ && vulkan_lib_->is_valid(); }

#define LAZY_FUNCTION(function)                  \
  LazyLibraryFunction<PFN_##function> function = \
      LazyLibraryFunction<PFN_##function>(nullptr, #function, this)
  LAZY_FUNCTION(vkCreateInstance);
  LAZY_FUNCTION(vkEnumerateInstanceExtensionProperties);
  LAZY_FUNCTION(vkEnumerateInstanceLayerProperties);
#undef LAZY_FUNCTION
  logging::Logger* GetLogger() { return logger_; }

  PFN_vkVoidFunction getProcAddr(::VkInstance instance, const char* function);

  PFN_vkGetInstanceProcAddr getProcAddrFunction() {
    return vkGetInstanceProcAddr;
  }

 private:
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

  logging::Logger* logger_;
  containers::unique_ptr<dynamic_loader::DynamicLibrary> vulkan_lib_;
};
}  // namespace vulkan

#endif  // VULKAN_WRAPPER_LIBRARY_WRAPPER_H_
