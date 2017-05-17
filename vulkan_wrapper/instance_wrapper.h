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

#ifndef VULKAN_WRAPPER_INSTANCE_WRAPPER_H_
#define VULKAN_WRAPPER_INSTANCE_WRAPPER_H_

#include <cstring>
#include <memory>

#include "support/containers/unique_ptr.h"
#include "support/log/log.h"

#include "vulkan_helpers/vulkan_header_wrapper.h"
#include "vulkan_wrapper/function_table.h"
#include "vulkan_wrapper/lazy_function.h"
#include "vulkan_wrapper/library_wrapper.h"

namespace vulkan {

// VkInstance wraps a native vulkan VkInstance handle. It provides
// lazily initialized function pointers for all of its initialization
// methods. It will automatically call VkDestroyInstance when it
// goes out of scope.
class VkInstance {
 public:
  VkInstance(containers::Allocator* container_allocator, ::VkInstance instance,
             VkAllocationCallbacks* allocator, LibraryWrapper* wrapper)
      : instance_(instance),
        has_allocator_(allocator != nullptr),
        wrapper_(wrapper) {
    if (has_allocator_) {
      allocator_ = *allocator;
    } else {
      memset(&allocator_, 0, sizeof(allocator_));
    }
    functions_ = containers::make_unique<InstanceFunctions>(
        container_allocator, instance_, getProcAddrFunction(),
        wrapper_->GetLogger());
    // functions_.reset(new InstanceFunctions(instance_, getProcAddrFunction(),
    // wrapper_->GetLogger()));
  }

  VkInstance(VkInstance&& other)
      : instance_(other.instance_),
        allocator_(other.allocator_),
        wrapper_(other.wrapper_),
        has_allocator_(other.has_allocator_),
        functions_(std::move(other.functions_)) {
    other.instance_ = VK_NULL_HANDLE;
  }

  VkInstance(const VkInstance&) = delete;
  VkInstance& operator=(const VkInstance&) = delete;

  ~VkInstance() {
    if (instance_ != VK_NULL_HANDLE) {
      functions_->vkDestroyInstance(instance_,
                                    has_allocator_ ? &allocator_ : nullptr);
    }
  }

  logging::Logger* GetLogger() { return wrapper_->GetLogger(); }
  LibraryWrapper* get_wrapper() { return wrapper_; }

  // To conform to the VkSubObjects::Traits interface
  PFN_vkGetInstanceProcAddr getProcAddrFunction() {
    return wrapper_->getProcAddrFunction();
  }

  InstanceFunctions* functions() { return functions_.get(); }

 private:
  ::VkInstance instance_;
  bool has_allocator_;
  // Intentionally keep a copy of the callbacks, they are just a bunch of
  // pointers, but it means we don't force our user to keep the allocator struct
  // around forever.
  VkAllocationCallbacks allocator_;
  LibraryWrapper* wrapper_;
  containers::unique_ptr<InstanceFunctions> functions_;

 public:
  ::VkInstance get_instance() const { return instance_; }
  operator ::VkInstance() const { return instance_; }
  InstanceFunctions* operator->() { return functions_.get(); }
  InstanceFunctions& operator*() { return *functions_.get(); }
};

}  // namespace vulkan

#endif  // VULKAN_WRAPPER_INSTANCE_WRAPPER_H_
