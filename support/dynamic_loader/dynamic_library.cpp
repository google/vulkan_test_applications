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

#include "support/dynamic_loader/dynamic_library.h"
#include "support/containers/allocator.h"

#include <string>
#if defined _WIN32
#include <windows.h>
namespace dynamic_loader {
class InternalDynamicLibrary : public DynamicLibrary {
 public:
  // This will search using the default search operations,
  // which is to say, absolute, if the path was absolute, followed
  // by LD_LIBRARY_PATH.
  InternalDynamicLibrary(const char* lib_name) {
    std::string lib_with_extension = lib_name;
    lib_with_extension += "-1.dll";
    // We choose RTLD_LAZY because we expect most of the functions
    // in this library to be resolved by other calls to dlsym.
    lib_ = LoadLibrary(lib_with_extension.c_str());
  }

  ~InternalDynamicLibrary() override {
    if (is_valid()) {
      CloseHandle(lib_);
    }
  }

  void* ResolveFunction(const char* function_name) override {
    return GetProcAddress(lib_, function_name);
  }
  bool is_valid() override { return nullptr != lib_; }

 private:
  HMODULE lib_;
};
}
#else
#include <dlfcn.h>

namespace dynamic_loader {
class InternalDynamicLibrary : public DynamicLibrary {
 public:
  // This will search using the default search operations,
  // which is to say, absolute, if the path was absolute, followed
  // by LD_LIBRARY_PATH.
  InternalDynamicLibrary(const char* lib_name) {
    std::string lib_with_extension = lib_name;
    lib_with_extension = "lib" + lib_with_extension + ".so";
    // We choose RTLD_LAZY because we expect most of the functions
    // in this library to be resolved by other calls to dlsym.
    lib_ = dlopen(lib_with_extension.c_str(), RTLD_LAZY);
  }

  ~InternalDynamicLibrary() override {
    if (is_valid()) {
      dlclose(lib_);
    }
  }

  void* ResolveFunction(const char* function_name) override {
    return dlsym(lib_, function_name);
  }
  bool is_valid() override { return nullptr != lib_; }

 private:
  void* lib_;
};
}
#endif
namespace dynamic_loader {
containers::unique_ptr<DynamicLibrary> OpenLibrary(
    containers::Allocator* allocator, const char* name) {
  containers::unique_ptr<InternalDynamicLibrary> lib(
      containers::make_unique<InternalDynamicLibrary>(allocator, name));
  if (!lib->is_valid()) {
    return nullptr;
  }
  return std::move(lib);
}
}  // namespace  dynamic_loader
