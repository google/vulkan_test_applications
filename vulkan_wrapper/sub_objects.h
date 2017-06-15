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

#ifndef VULKAN_WRAPPER_SUB_OBJECTS_H_
#define VULKAN_WRAPPER_SUB_OBJECTS_H_

#include "vulkan_helpers/vulkan_header_wrapper.h"
#include "vulkan_wrapper/device_wrapper.h"
#include "vulkan_wrapper/instance_wrapper.h"
#include "vulkan_wrapper/lazy_function.h"

namespace vulkan {

// VkSubObject wraps any object that is the child of an instance or device.
// It exists just so it gets cleaned up when it goes out of scope.
// T is expected to be a set of traits that describes the object in question.
// It should be of the form
// struct FooTraits {
//   using type = VulkanType;
//   using destruction_function_pointer_type =
//     Lazy{Instance|Device|..}Function<PFN_vkDestroyVulkanType>;
//   static destruction_function_pointer_type* get_destruction_function(
//       {Device|Instance|...}Functions* functions) {
//     return &functions->vkDestroyVulkanType;
//   }
// }
// O is expected to be a set of traits that describe the owner of this object.
// It should be of the form
// struct OwnerTraits {
//   using type = VulkanType; // Typically VkDevice or VkInstance
//   using proc_addr_function_type; // PFN_vkGetInstanceProcAddr for example
//   using raw_vulkan_type; // Raw vulkan type that this is associated with
// }
//}
template <typename T, typename O>
class VkSubObject {
  using type = typename T::type;
  using owner_type = typename O::type;
  using raw_owner_type = typename O::raw_vulkan_type;
  using proc_addr_type = typename O::proc_addr_function_type;
  using destruction_function_pointer_type =
      typename T::destruction_function_pointer_type;

  static_assert(std::is_copy_constructible<type>::value,
                "The type must be copy constructible.");

 public:
  // This does not retain a reference to the owner, or the
  // VkAllocationCallbacks object, it does take ownership of the object in
  // question.
  VkSubObject(type raw_object, VkAllocationCallbacks* allocator,
              owner_type* owner)
      : owner_(owner ? static_cast<raw_owner_type>(*owner)
                     : static_cast<raw_owner_type>(VK_NULL_HANDLE)),
        log_(owner ? owner->GetLogger() : nullptr),
        has_allocator_(allocator != nullptr),
        raw_object_(raw_object),
        destruction_function_(
            owner ? T::get_destruction_function(owner->functions()) : nullptr),
        get_proc_addr_fn_(nullptr) {
    if (allocator) {
      allocator_ = *allocator;
    }
    if (owner) {
      get_proc_addr_fn_ = owner->getProcAddrFunction();
    }
  }

  ~VkSubObject() { clean_up(); }

  VkSubObject(VkSubObject<T, O>&& other)
      : owner_(other.owner_),
        log_(other.log_),
        get_proc_addr_fn_(other.get_proc_addr_fn_),
        allocator_(other.allocator_),
        has_allocator_(other.has_allocator_),
        raw_object_(other.raw_object_),
        destruction_function_(other.destruction_function_) {
    other.raw_object_ = VK_NULL_HANDLE;
  }

  logging::Logger* GetLogger() { return log_; }

  void initialize(type raw_object) {
    LOG_ASSERT(==, log_, true, raw_object_ == VK_NULL_HANDLE);
    raw_object_ = raw_object;
  }

 private:
  inline void clean_up() {
    if (raw_object_) {
      LOG_ASSERT(!=, log_, destruction_function_, static_cast<void*>(nullptr));
      (*destruction_function_)(owner_, raw_object_,
                               has_allocator_ ? &allocator_ : nullptr);
      raw_object_ = VK_NULL_HANDLE;
    }
  }

  raw_owner_type owner_;
  logging::Logger* log_;
  proc_addr_type get_proc_addr_fn_;
  VkAllocationCallbacks allocator_;
  bool has_allocator_;
  type raw_object_;

  destruction_function_pointer_type destruction_function_;

 public:
  operator type() const { return raw_object_; }
  const type& get_raw_object() const { return raw_object_; }

  PFN_vkVoidFunction getProcAddr(raw_owner_type owner, const char* function) {
    return get_proc_addr_fn_(owner, function);
  }
};

struct InstanceTraits {
  using type = VkInstance;
  using proc_addr_function_type = PFN_vkGetInstanceProcAddr;
  using raw_vulkan_type = ::VkInstance;
  using function_table_type = InstanceFunctions;
};

struct DeviceTraits {
  using type = VkDevice;
  using proc_addr_function_type = PFN_vkGetDeviceProcAddr;
  using raw_vulkan_type = ::VkDevice;
};

struct CommandPoolTraits {
  using type = ::VkCommandPool;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyCommandPool>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyCommandPool;
  }
};
using VkCommandPool = VkSubObject<CommandPoolTraits, DeviceTraits>;

struct DescriptorPoolTraits {
  using type = ::VkDescriptorPool;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyDescriptorPool>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyDescriptorPool;
  }
};
using VkDescriptorPool = VkSubObject<DescriptorPoolTraits, DeviceTraits>;

struct DescriptorSetLayoutTraits {
  using type = ::VkDescriptorSetLayout;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyDescriptorSetLayout>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyDescriptorSetLayout;
  }
};
using VkDescriptorSetLayout =
    VkSubObject<DescriptorSetLayoutTraits, DeviceTraits>;

struct SurfaceTraits {
  using type = ::VkSurfaceKHR;
  using destruction_function_pointer_type =
      LazyInstanceFunction<PFN_vkDestroySurfaceKHR>*;
  static destruction_function_pointer_type get_destruction_function(
      InstanceFunctions* functions) {
    return &functions->vkDestroySurfaceKHR;
  }
};
using VkSurfaceKHR = VkSubObject<SurfaceTraits, InstanceTraits>;

struct ImageTraits {
  using type = ::VkImage;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyImage>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyImage;
  }
};
using VkImage = VkSubObject<ImageTraits, DeviceTraits>;

struct FenceTraits {
  using type = ::VkFence;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyFence>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyFence;
  }
};
using VkFence = VkSubObject<FenceTraits, DeviceTraits>;

struct EventTraits {
  using type = ::VkEvent;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyEvent>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyEvent;
  }
};
using VkEvent = VkSubObject<EventTraits, DeviceTraits>;

struct ImageViewTraits {
  using type = ::VkImageView;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyImageView>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyImageView;
  }
};
using VkImageView = VkSubObject<ImageViewTraits, DeviceTraits>;

struct SamplerTraits {
  using type = ::VkSampler;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroySampler>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroySampler;
  }
};
using VkSampler = VkSubObject<SamplerTraits, DeviceTraits>;

struct RenderPassTraits {
  using type = ::VkRenderPass;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyRenderPass>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyRenderPass;
  }
};
using VkRenderPass = VkSubObject<RenderPassTraits, DeviceTraits>;

struct FramebufferTraits {
  using type = ::VkFramebuffer;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyFramebuffer>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyFramebuffer;
  }
};
using VkFramebuffer = VkSubObject<FramebufferTraits, DeviceTraits>;

struct SemaphoreTraits {
  using type = ::VkSemaphore;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroySemaphore>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroySemaphore;
  }
};
using VkSemaphore = VkSubObject<SemaphoreTraits, DeviceTraits>;

struct PipelineCacheTraits {
  using type = ::VkPipelineCache;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyPipelineCache>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyPipelineCache;
  }
};
using VkPipelineCache = VkSubObject<PipelineCacheTraits, DeviceTraits>;

struct PipelineLayoutTraits {
  using type = ::VkPipelineLayout;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyPipelineLayout>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyPipelineLayout;
  }
};
using VkPipelineLayout = VkSubObject<PipelineLayoutTraits, DeviceTraits>;

struct PipelineTraits {
  using type = ::VkPipeline;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyPipeline>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyPipeline;
  }
};
using VkPipeline = VkSubObject<PipelineTraits, DeviceTraits>;

struct DeviceMemoryTraits {
  using type = ::VkDeviceMemory;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkFreeMemory>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkFreeMemory;
  }
};
using VkDeviceMemory = VkSubObject<DeviceMemoryTraits, DeviceTraits>;

struct ShaderModuleTraits {
  using type = ::VkShaderModule;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyShaderModule>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyShaderModule;
  }
};

using VkShaderModule = VkSubObject<ShaderModuleTraits, DeviceTraits>;

struct BufferTraits {
  using type = ::VkBuffer;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyBuffer>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyBuffer;
  }
};
using VkBuffer = VkSubObject<BufferTraits, DeviceTraits>;

struct BufferViewTraits {
  using type = ::VkBufferView;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyBufferView>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyBufferView;
  }
};
using VkBufferView = VkSubObject<BufferViewTraits, DeviceTraits>;

struct QueryPoolTraits {
  using type = ::VkQueryPool;
  using destruction_function_pointer_type =
      LazyDeviceFunction<PFN_vkDestroyQueryPool>*;
  static destruction_function_pointer_type get_destruction_function(
      DeviceFunctions* functions) {
    return &functions->vkDestroyQueryPool;
  }
};
using VkQueryPool = VkSubObject<QueryPoolTraits, DeviceTraits>;

}  // namespace vulkan

#endif  // VULKAN_WRAPPER_SUB_OBJECTS_H_
