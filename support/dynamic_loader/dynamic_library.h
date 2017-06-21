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

#ifndef SUPPORT_DYNAMIC_LOADER_DYNAMIC_LIBRARY_H_
#define SUPPORT_DYNAMIC_LOADER_DYNAMIC_LIBRARY_H_

#include <memory>

#include "support/containers/allocator.h"
#include "support/containers/unique_ptr.h"
#include "vulkan_helpers/vulkan_header_wrapper.h"

namespace dynamic_loader {
// This wraps a system-specific loaded dynamic library.
class DynamicLibrary {
 public:
   virtual ~DynamicLibrary() {};

  // This resolves a pointer from the opened dynamic library.
  // It automatically casts it to a function pointer of the passed
  // in pointer type.
  // Unfortunately we cannot validate that the function is in fact of the
  // requested type.
  // Returns true if the function pointer could be correctly resolved.
  template <typename T, typename... Args>
  bool Resolve(const char* name, T(VKAPI_PTR** val)(Args...)) {
    *val = reinterpret_cast<T(VKAPI_PTR*)(Args...)>(ResolveFunction(name));
    return *val != nullptr;
  }
  // Returns true if this library is valid.
  virtual bool is_valid() = 0;

 private:
  // Given a function name, this must return a pointer to the function,
  // or nullptr if the symbol could not be resolved.
  virtual void* ResolveFunction(const char* name) = 0;
};

// Returns a DynamicLibrary that has been opened using the system's internal
// library resolution. If a library could not be opened, it returns
// nullptr.
containers::unique_ptr<DynamicLibrary> OpenLibrary(
    containers::Allocator* allocator, const char* name);

}  // namespace dynamic_loader

#endif  // SUPPORT_DYNAMIC_LOADER_DYNAMIC_LIBRARY_H_
