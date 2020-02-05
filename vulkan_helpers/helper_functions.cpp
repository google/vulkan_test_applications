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

#include "vulkan_helpers/helper_functions.h"

#include <algorithm>
#include <tuple>

#include "support/containers/vector.h"
#include "support/log/log.h"

namespace vulkan {
VkInstance CreateEmptyInstance(containers::Allocator* allocator,
                               LibraryWrapper* wrapper) {
  // Test a non-nullptr pApplicationInfo
  VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO,
                             nullptr,
                             "TestApplication",
                             1,
                             "Engine",
                             0,
                             VK_MAKE_VERSION(1, 0, 0)};

  VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                            nullptr,
                            0,
                            &app_info,
                            0,
                            nullptr,
                            0,
                            nullptr};

  ::VkInstance raw_instance;
  LOG_ASSERT(==, wrapper->GetLogger(),
             wrapper->vkCreateInstance(&info, nullptr, &raw_instance),
             VK_SUCCESS);
  // vulkan::VkInstance will handle destroying the instance
  return vulkan::VkInstance(allocator, raw_instance, nullptr, wrapper);
}

VkInstance CreateDefaultInstance(containers::Allocator* allocator,
                                 LibraryWrapper* wrapper) {
  // Similar to Empty Instance, but turns on the platform specific
  // swapchian functions.
  // Test a non-nullptr pApplicationInfo
  VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO,
                             nullptr,
                             "TestApplication",
                             1,
                             "Engine",
                             0,
                             VK_MAKE_VERSION(1, 0, 0)};

  const char* extensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
#if defined __ANDROID__
    VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#elif defined __ggp__
    VK_GGP_STREAM_DESCRIPTOR_SURFACE_EXTENSION_NAME,
#elif defined __linux__
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined _WIN32
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined __APPLE__
    VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
#endif
  };

  wrapper->GetLogger()->LogInfo("Enabled Device Extensions: ");
  for (auto& extension : extensions) {
    wrapper->GetLogger()->LogInfo("    ", extension);
  }

  VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                            nullptr,
                            0,
                            &app_info,
                            0,
                            nullptr,
                            sizeof(extensions) / sizeof(extensions[0]),
                            extensions};

  ::VkInstance raw_instance;
  LOG_ASSERT(==, wrapper->GetLogger(),
             wrapper->vkCreateInstance(&info, nullptr, &raw_instance),
             VK_SUCCESS);
  // vulkan::VkInstance will handle destroying the instance
  return vulkan::VkInstance(allocator, raw_instance, nullptr, wrapper);
}

VkInstance CreateVerisonedInstanceForApplicaiton(
    containers::Allocator* allocator, LibraryWrapper* wrapper,
    const entry::EntryData* data, uint32_t version,
    const std::initializer_list<const char*> instance_extensions) {
  // Similar to CreateDefaultInstance, but turns on the virtual swapchain
  // if the requested by entry_data.

  VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO,
                             nullptr,
                             "TestApplication",
                             1,
                             "Engine",
                             0,
                             version};

  const char* default_extensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
#if defined __ANDROID__
    VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#elif defined __ggp__
    VK_GGP_STREAM_DESCRIPTOR_SURFACE_EXTENSION_NAME,
#elif defined __linux__
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined _WIN32
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined __APPLE__
    VK_MVK_MACOS_SURFACE_EXTENSION_NAME
#endif
  };

  const auto num_default_extensions =
      sizeof(default_extensions) / sizeof(default_extensions[0]);

  const char* validation_layer = "VK_LAYER_LUNARG_standard_validation";
  const char* callback_layer = "CallbackSwapchain";
  const char* layer = nullptr;

  if (data->output_frame_index() >= 0) {
    layer = callback_layer;
  } else if (data->validation()) {
		layer = validation_layer;
  }

  std::vector<const char*> extensions;
  extensions.reserve(num_default_extensions + instance_extensions.size());
  extensions.insert(extensions.end(), default_extensions,
                    default_extensions + num_default_extensions);
  extensions.insert(extensions.end(), instance_extensions.begin(),
                    instance_extensions.end());

  wrapper->GetLogger()->LogInfo("Enabled Instance Extensions: ");
  for (auto& extension : extensions) {
    wrapper->GetLogger()->LogInfo("    ", extension);
  }

  VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                            nullptr,
                            0,
                            &app_info,
                            !!layer,
                            &layer,
                            uint32_t(extensions.size()),
                            extensions.data()};

  ::VkInstance raw_instance;
  LOG_ASSERT(==, wrapper->GetLogger(),
             wrapper->vkCreateInstance(&info, nullptr, &raw_instance),
             VK_SUCCESS);
  // vulkan::VkInstance will handle destroying the instance
  return vulkan::VkInstance(allocator, raw_instance, nullptr, wrapper);
}

VkInstance CreateInstanceForApplication(
    containers::Allocator* allocator, LibraryWrapper* wrapper,
    const entry::EntryData* data,
    const std::initializer_list<const char*> extensions) {
  return CreateVerisonedInstanceForApplicaiton(
      allocator, wrapper, data, VK_MAKE_VERSION(1, 0, 0), extensions);
}

VkInstance Create11InstanceForApplication(
    containers::Allocator* allocator, LibraryWrapper* wrapper,
    const entry::EntryData* data,
    const std::initializer_list<const char*> extensions) {
  return CreateVerisonedInstanceForApplicaiton(
      allocator, wrapper, data, VK_MAKE_VERSION(1, 0, 0), extensions);
}

containers::vector<VkPhysicalDevice> GetPhysicalDevices(
    containers::Allocator* allocator, VkInstance& instance) {
  uint32_t device_count = 0;
  instance->vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

  containers::vector<VkPhysicalDevice> physical_devices(allocator);
  physical_devices.resize(device_count);
  LOG_ASSERT(==, instance.GetLogger(),
             instance->vkEnumeratePhysicalDevices(instance, &device_count,
                                                  physical_devices.data()),
             VK_SUCCESS);

  return std::move(physical_devices);
}

containers::vector<VkQueueFamilyProperties> GetQueueFamilyProperties(
    containers::Allocator* allocator, VkInstance& instance,
    ::VkPhysicalDevice device) {
  uint32_t count = 0;
  instance->vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

  LOG_ASSERT(>, instance.GetLogger(), count, 0u);
  containers::vector<VkQueueFamilyProperties> properties(allocator);
  properties.resize(count);
  instance->vkGetPhysicalDeviceQueueFamilyProperties(device, &count,
                                                     properties.data());
  return std::move(properties);
}

inline bool HasQueueFlags(const VkQueueFamilyProperties& property,
                          VkQueueFlags queue_flags) {
  return property.queueCount > 0 &&
         ((property.queueFlags & queue_flags) == queue_flags);
}

uint32_t GetGraphicsAndComputeQueueFamily(containers::Allocator* allocator,
                                          VkInstance& instance,
                                          ::VkPhysicalDevice device,
                                          bool use_protected_memory) {
  return GetQueueFamily(
      allocator, instance, device,
      VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT |
          (use_protected_memory ? VK_QUEUE_PROTECTED_BIT : 0));
}

inline uint32_t GetQueueFamily(containers::Allocator* allocator,
                               VkInstance& instance, ::VkPhysicalDevice device,
                               VkQueueFlags queue_flags) {
  auto properties = GetQueueFamilyProperties(allocator, instance, device);

  for (uint32_t i = 0; i < properties.size(); ++i) {
    if (HasQueueFlags(properties[i], queue_flags)) return i;
  }
  return ~0u;
}

// Any queue family that supports only compute, or any queue that supports
// both compute and graphics that is not the "first" one can be used
// for async compute.
uint32_t GetAsyncComputeQueueFamilyIndex(containers::Allocator* allocator,
                                         VkInstance& instance,
                                         ::VkPhysicalDevice device) {
  auto properties = GetQueueFamilyProperties(allocator, instance, device);
  int32_t graphics_queue = -1;
  int32_t num_graphics_queues = -1;
  for (uint32_t i = 0; i < properties.size(); ++i) {
    auto property = properties[i];
    if (property.queueCount > 0 &&
        (property.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
      if (graphics_queue == -1 &&
          HasQueueFlags(properties[i],
                        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
        graphics_queue = static_cast<int32_t>(i);
        num_graphics_queues = property.queueCount;
      } else {
        return i;
      }
    }
  }

  // If we could not find a secondary queue family that supports compute,
  // then return a queue from the primary graphics family.
  if (graphics_queue > -1 && num_graphics_queues > 1) {
    return graphics_queue;
  }
  return ~0u;
}

VkDevice CreateDefaultDevice(containers::Allocator* allocator,
                             VkInstance& instance,
                             bool require_graphics_compute_queue) {
  containers::vector<VkPhysicalDevice> physical_devices =
      GetPhysicalDevices(allocator, instance);
  float priority = 1.f;

  VkPhysicalDevice physical_device = physical_devices.front();

  VkPhysicalDeviceProperties properties;
  instance->vkGetPhysicalDeviceProperties(physical_device, &properties);

  const uint32_t queue_family_index =
      require_graphics_compute_queue
          ? GetGraphicsAndComputeQueueFamily(allocator, instance,
                                             physical_device, false)
          : GetQueueFamily(allocator, instance, physical_device,
                           VK_QUEUE_GRAPHICS_BIT);
  LOG_ASSERT(!=, instance.GetLogger(), queue_family_index, ~0u);

  VkDeviceQueueCreateInfo queue_info{
      /* sType = */ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* queueFamilyIndex = */ queue_family_index,
      /* queueCount */ 1,
      /* pQueuePriorities = */ &priority};

  const char* extensions[] = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

  VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                          nullptr,
                          0,
                          1,
                          &queue_info,
                          0,
                          nullptr,
                          sizeof(extensions) / sizeof(extensions[0]),
                          extensions,
                          nullptr};

  ::VkDevice raw_device;
  LOG_ASSERT(
      ==, instance.GetLogger(),
      instance->vkCreateDevice(physical_device, &info, nullptr, &raw_device),
      VK_SUCCESS);
  return vulkan::VkDevice(allocator, raw_device, nullptr, &instance,
                          &properties, physical_device);
}

bool SupportRequestPhysicalDeviceFeatures(
    VkInstance* instance, VkPhysicalDevice physical_device,
    const VkPhysicalDeviceFeatures& request_features) {
  VkPhysicalDeviceFeatures supported_features = {0};
  (*instance)->vkGetPhysicalDeviceFeatures(physical_device,
                                           &supported_features);

  // If the VkPhysicalDeviceFeatures changes, we need to update this piece of
  // code, e.g. adding checking for new feature flags.
  static_assert(
      sizeof(VkPhysicalDeviceFeatures) ==
          (uint32_t)offsetof(VkPhysicalDeviceFeatures, inheritedQueries) +
              sizeof(VkBool32),
      "VkPhysicalDeviceFeatures layout changed, "
      "SupportRequestPhysicalDeviceFeatures() needs to be updated.");

#define NotSupportFeature(feature_name)                                    \
  if (request_features.feature_name && !supported_features.feature_name) { \
    return false;                                                          \
  }
  NotSupportFeature(robustBufferAccess);
  NotSupportFeature(fullDrawIndexUint32);
  NotSupportFeature(imageCubeArray);
  NotSupportFeature(independentBlend);
  NotSupportFeature(geometryShader);
  NotSupportFeature(tessellationShader);
  NotSupportFeature(sampleRateShading);
  NotSupportFeature(dualSrcBlend);
  NotSupportFeature(logicOp);
  NotSupportFeature(multiDrawIndirect);
  NotSupportFeature(drawIndirectFirstInstance);
  NotSupportFeature(depthClamp);
  NotSupportFeature(depthBiasClamp);
  NotSupportFeature(fillModeNonSolid);
  NotSupportFeature(depthBounds);
  NotSupportFeature(wideLines);
  NotSupportFeature(largePoints);
  NotSupportFeature(alphaToOne);
  NotSupportFeature(multiViewport);
  NotSupportFeature(samplerAnisotropy);
  NotSupportFeature(textureCompressionETC2);
  NotSupportFeature(textureCompressionASTC_LDR);
  NotSupportFeature(textureCompressionBC);
  NotSupportFeature(occlusionQueryPrecise);
  NotSupportFeature(pipelineStatisticsQuery);
  NotSupportFeature(vertexPipelineStoresAndAtomics);
  NotSupportFeature(fragmentStoresAndAtomics);
  NotSupportFeature(shaderTessellationAndGeometryPointSize);
  NotSupportFeature(shaderImageGatherExtended);
  NotSupportFeature(shaderStorageImageExtendedFormats);
  NotSupportFeature(shaderStorageImageMultisample);
  NotSupportFeature(shaderStorageImageReadWithoutFormat);
  NotSupportFeature(shaderStorageImageWriteWithoutFormat);
  NotSupportFeature(shaderUniformBufferArrayDynamicIndexing);
  NotSupportFeature(shaderSampledImageArrayDynamicIndexing);
  NotSupportFeature(shaderStorageBufferArrayDynamicIndexing);
  NotSupportFeature(shaderStorageImageArrayDynamicIndexing);
  NotSupportFeature(shaderClipDistance);
  NotSupportFeature(shaderCullDistance);
  NotSupportFeature(shaderFloat64);
  NotSupportFeature(shaderInt64);
  NotSupportFeature(shaderInt16);
  NotSupportFeature(shaderResourceResidency);
  NotSupportFeature(shaderResourceMinLod);
  NotSupportFeature(sparseBinding);
  NotSupportFeature(sparseResidencyBuffer);
  NotSupportFeature(sparseResidencyImage2D);
  NotSupportFeature(sparseResidencyImage3D);
  NotSupportFeature(sparseResidency2Samples);
  NotSupportFeature(sparseResidency4Samples);
  NotSupportFeature(sparseResidency8Samples);
  NotSupportFeature(sparseResidency16Samples);
  NotSupportFeature(sparseResidencyAliased);
  NotSupportFeature(variableMultisampleRate);
  NotSupportFeature(inheritedQueries);
#undef NotSupportFeature
  return true;
}

namespace {
// A helper class to hold queue create info
struct QueueCreateInfo {
  QueueCreateInfo(containers::Allocator* allocator, uint32_t family_index,
                  VkDeviceQueueCreateFlags flags)
      : flags(flags),
        queue_family_index(family_index),
        queue_count(0),
        priorities(allocator) {}

  QueueCreateInfo(QueueCreateInfo&& _other)
      : flags(_other.flags),
        queue_family_index(_other.queue_family_index),
        queue_count(_other.queue_count),
        priorities(std::move(_other.priorities)) {}

  QueueCreateInfo(const QueueCreateInfo& _other)
      : flags(_other.flags),
        queue_family_index(_other.queue_family_index),
        queue_count(_other.queue_count),
        priorities(_other.priorities) {}

  QueueCreateInfo& operator=(const QueueCreateInfo& _other) {
    flags = _other.flags;
    queue_family_index = _other.queue_family_index;
    queue_count = _other.queue_count;
    priorities = _other.priorities;
    return *this;
  }

  inline void AddQueue(float priority) {
    queue_count++;
    priorities.push_back(priority);
  }
  inline VkDeviceQueueCreateInfo GetVkDeviceQueueCreateInfo() const {
    return VkDeviceQueueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                   nullptr,
                                   flags,
                                   queue_family_index,
                                   queue_count,
                                   priorities.data()};
  };

  VkDeviceQueueCreateFlags flags;
  uint32_t queue_family_index;
  uint32_t queue_count;
  containers::vector<float> priorities;
};
}  // namespace

VkDevice CreateDeviceForSwapchain(
    containers::Allocator* allocator, VkInstance* instance,
    VkSurfaceKHR* surface, uint32_t* present_queue_index,
    uint32_t* graphics_queue_index, bool use_protected_memory,
    const std::initializer_list<const char*> extensions,
    const VkPhysicalDeviceFeatures& features,
    bool try_to_find_separate_present_queue,
    uint32_t* async_compute_queue_index, uint32_t* sparse_binding_queue_index,
    bool use_host_query_reset, void* device_next) {
  containers::vector<VkPhysicalDevice> physical_devices =
      GetPhysicalDevices(allocator, *instance);
  float priority = 1.f;
  containers::vector<float> additional_priorities(allocator);
  additional_priorities.push_back(1.0f);

  for (auto device : physical_devices) {
    VkPhysicalDevice physical_device = device;

    if (!SupportRequestPhysicalDeviceFeatures(instance, physical_device,
                                              features)) {
      continue;
    }

    VkPhysicalDeviceProperties physical_device_properties;
    (*instance)->vkGetPhysicalDeviceProperties(device,
                                               &physical_device_properties);

    containers::vector<VkExtensionProperties> available_extensions(allocator);
    uint32_t num_extensions = 0;
    (*instance)->vkEnumerateDeviceExtensionProperties(device, nullptr,
                                                      &num_extensions, nullptr);

    available_extensions.resize(num_extensions);
    (*instance)->vkEnumerateDeviceExtensionProperties(
        device, nullptr, &num_extensions, available_extensions.data());

    bool valid_extensions = true;
    for (auto ext : extensions) {
      if (std::find_if(available_extensions.begin(), available_extensions.end(),
                       [&](const VkExtensionProperties& dat) {
                         return strcmp(ext, dat.extensionName) == 0;
                       }) == available_extensions.end()) {
        valid_extensions = false;
        break;
      }
    }
    if (!valid_extensions) {
      continue;
    }

    auto properties = GetQueueFamilyProperties(allocator, *instance, device);
    const uint32_t graphics_queue_family_index =
        GetGraphicsAndComputeQueueFamily(allocator, *instance, physical_device,
                                         use_protected_memory);
    uint32_t present_queue_family_index = 0;
    uint32_t backup_present_queue_family_index = 0xFFFFFFFF;
    for (; present_queue_family_index < properties.size();
         ++present_queue_family_index) {
      VkBool32 supports_swapchain = false;
      LOG_EXPECT(==, instance->GetLogger(),
                 (*instance)->vkGetPhysicalDeviceSurfaceSupportKHR(
                     device, present_queue_family_index, *surface,
                     &supports_swapchain),
                 VK_SUCCESS);
      if (supports_swapchain) {
        if (!try_to_find_separate_present_queue) {
          break;
        }
        if (backup_present_queue_family_index != 0xFFFFFFFF) {
          break;
        }
        if (present_queue_family_index != graphics_queue_family_index) {
          break;
        }
        backup_present_queue_family_index = present_queue_family_index;
      }
    }

    if (present_queue_family_index == properties.size() &&
        backup_present_queue_family_index == 0xFFFFFFFF) {
      continue;
    }
    if (present_queue_family_index == properties.size()) {
      present_queue_family_index = backup_present_queue_family_index;
    }

    containers::vector<QueueCreateInfo> queue_create_infos(allocator);
    queue_create_infos.reserve(4);

    queue_create_infos.emplace_back(QueueCreateInfo(
        allocator, graphics_queue_family_index,
        use_protected_memory ? VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT : 0));

    queue_create_infos.back().AddQueue(1.0f);
    if (graphics_queue_family_index != present_queue_family_index) {
      queue_create_infos.emplace_back(
          QueueCreateInfo(allocator, present_queue_family_index, 0));
      queue_create_infos.back().AddQueue(1.0f);
    }
    if (async_compute_queue_index != nullptr) {
      *async_compute_queue_index =
          GetAsyncComputeQueueFamilyIndex(allocator, *instance, device);
      if (*async_compute_queue_index != 0xFFFFFFFF) {
        bool added = false;
        for (auto& qi : queue_create_infos) {
          if (qi.queue_family_index == *async_compute_queue_index) {
            qi.AddQueue(0.5f);
            added = true;
            break;
          }
        }
        if (!added) {
          queue_create_infos.emplace_back(
              QueueCreateInfo(allocator, *async_compute_queue_index, 0));
          queue_create_infos.back().AddQueue(0.5f);
        }
      }
    }
    if (sparse_binding_queue_index != nullptr) {
      *sparse_binding_queue_index = GetQueueFamily(allocator, *instance, device,
                                                   VK_QUEUE_SPARSE_BINDING_BIT);
      if (*sparse_binding_queue_index != 0xFFFFFFFF) {
        bool exist = false;
        for (auto& qi : queue_create_infos) {
          if (qi.queue_family_index == *sparse_binding_queue_index) {
            // no need to create new separate queue for sparse binding.
            exist = true;
            break;
          }
        }
        if (!exist) {
          queue_create_infos.emplace_back(
              QueueCreateInfo(allocator, *sparse_binding_queue_index, 0));
          queue_create_infos.back().AddQueue(1.0f);
        }
      }
    }

    const char* forced_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    containers::vector<const char*> enabled_extensions(allocator);
    for (auto ext : forced_extensions) {
      enabled_extensions.push_back(ext);
    }
    for (auto ext : extensions) {
      enabled_extensions.push_back(ext);
    }

    containers::vector<VkDeviceQueueCreateInfo> raw_queue_infos(allocator);
    raw_queue_infos.reserve(4);
    for (const auto& qi : queue_create_infos) {
      raw_queue_infos.emplace_back(qi.GetVkDeviceQueueCreateInfo());
    }

    VkPhysicalDeviceHostQueryResetFeaturesEXT host_query_reset_feature{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT,
        device_next, use_host_query_reset};

    VkPhysicalDeviceFloatControlsPropertiesKHR float_control_properties{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES_KHR,
        &host_query_reset_feature,  // pNext
        VkShaderFloatControlsIndependenceKHR::
            VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL_KHR,  // denormBehaviorIndependence;
        VkShaderFloatControlsIndependenceKHR::
            VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL_KHR,  // roundingModeIndependence;
        1,  //  shaderSignedZeroInfNanPreserveFloat16;
        1,  //  shaderSignedZeroInfNanPreserveFloat32;
        1,  //  shaderSignedZeroInfNanPreserveFloat64;
        1,  //  shaderDenormPreserveFloat16;
        1,  //  shaderDenormPreserveFloat32;
        1,  //  shaderDenormPreserveFloat64;
        1,  //  shaderDenormFlushToZeroFloat16;
        1,  //  shaderDenormFlushToZeroFloat32;
        1,  //  shaderDenormFlushToZeroFloat64;
        1,  //  shaderRoundingModeRTEFloat16;
        1,  //  shaderRoundingModeRTEFloat32;
        1,  //  shaderRoundingModeRTEFloat64;
        1,  //  shaderRoundingModeRTZFloat16;
        1,  //  shaderRoundingModeRTZFloat32;
        1,  //  shaderRoundingModeRTZFloat64;
    };

    VkPhysicalDeviceProtectedMemoryFeatures protected_memory_feature{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES,
        &float_control_properties, use_protected_memory};

    VkDeviceCreateInfo info{
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,           // stype
        &protected_memory_feature,                      // pNext
        0,                                              // flags
        static_cast<uint32_t>(raw_queue_infos.size()),  // queueCreateInfoCount
        raw_queue_infos.data(),                         // pQueueCreateInfos
        0,                                              // enabledLayerCount
        nullptr,                                        // ppEnabledLayerNames
        static_cast<uint32_t>(
            enabled_extensions.size()),  // enabledExtensionCount
        enabled_extensions.data(),       // ppEnabledExtensionNames
        &features                        // ppEnabledFeatures
    };

    ::VkDevice raw_device;
    LOG_ASSERT(==, instance->GetLogger(),
               (*instance)->vkCreateDevice(physical_device, &info, nullptr,
                                           &raw_device),
               VK_SUCCESS);

    instance->GetLogger()->LogInfo("Enabled Device Extensions: ");
    for (auto& extension : enabled_extensions) {
      instance->GetLogger()->LogInfo("    ", extension);
    }

    *present_queue_index = present_queue_family_index;
    *graphics_queue_index = graphics_queue_family_index;
    return vulkan::VkDevice(allocator, raw_device, nullptr, instance,
                            &physical_device_properties, physical_device);
  }
  instance->GetLogger()->LogError(
      "Could not find physical device or queue that can present");

  VkPhysicalDeviceProperties throwaway_properties;
  return vulkan::VkDevice(allocator, VK_NULL_HANDLE, nullptr, instance,
                          &throwaway_properties, VK_NULL_HANDLE);
}

containers::vector<QueueCreateInfo> GetAcceptableQueues(
    containers::Allocator* allocator, VkPhysicalDevice physical_device,
    VkInstance* instance, VkSurfaceKHR* surface, uint32_t* present_queue_index,
    uint32_t* graphics_queue_index,
    const std::initializer_list<const char*> extensions,
    const VkPhysicalDeviceFeatures& features,
    bool try_to_find_separate_present_queue,
    uint32_t* async_compute_queue_index, uint32_t* sparse_binding_queue_index) {
  if (!SupportRequestPhysicalDeviceFeatures(instance, physical_device,
                                            features)) {
    return containers::vector<QueueCreateInfo>();
  }

  VkPhysicalDeviceProperties physical_device_properties;
  (*instance)->vkGetPhysicalDeviceProperties(physical_device,
                                             &physical_device_properties);

  containers::vector<VkExtensionProperties> available_extensions(allocator);
  uint32_t num_extensions = 0;
  (*instance)->vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                                    &num_extensions, nullptr);

  available_extensions.resize(num_extensions);
  (*instance)->vkEnumerateDeviceExtensionProperties(
      physical_device, nullptr, &num_extensions, available_extensions.data());

  bool valid_extensions = true;
  for (auto ext : extensions) {
    if (std::find_if(available_extensions.begin(), available_extensions.end(),
                     [&](const VkExtensionProperties& dat) {
                       return strcmp(ext, dat.extensionName) == 0;
                     }) == available_extensions.end()) {
      valid_extensions = false;
      break;
    }
  }
  if (!valid_extensions) {
    return containers::vector<QueueCreateInfo>();
  }

  auto properties =
      GetQueueFamilyProperties(allocator, *instance, physical_device);
  const uint32_t graphics_queue_family_index = GetGraphicsAndComputeQueueFamily(
      allocator, *instance, physical_device, false);
  uint32_t present_queue_family_index = 0;
  uint32_t backup_present_queue_family_index = 0xFFFFFFFF;
  for (; present_queue_family_index < properties.size();
       ++present_queue_family_index) {
    VkBool32 supports_swapchain = false;
    LOG_EXPECT(==, instance->GetLogger(),
               (*instance)->vkGetPhysicalDeviceSurfaceSupportKHR(
                   physical_device, present_queue_family_index, *surface,
                   &supports_swapchain),
               VK_SUCCESS);
    if (supports_swapchain) {
      if (!try_to_find_separate_present_queue) {
        break;
      }
      if (backup_present_queue_family_index != 0xFFFFFFFF) {
        break;
      }
      if (present_queue_family_index != graphics_queue_family_index) {
        break;
      }
      backup_present_queue_family_index = present_queue_family_index;
    }
  }

  if (present_queue_family_index == properties.size() &&
      backup_present_queue_family_index == 0xFFFFFFFF) {
    return containers::vector<QueueCreateInfo>();
  }
  if (present_queue_family_index == properties.size()) {
    present_queue_family_index = backup_present_queue_family_index;
  }

  containers::vector<QueueCreateInfo> queue_create_infos(allocator);
  queue_create_infos.reserve(4);
  queue_create_infos.emplace_back(
      QueueCreateInfo(allocator, graphics_queue_family_index, 0));
  queue_create_infos.back().AddQueue(1.0f);
  if (graphics_queue_family_index != present_queue_family_index) {
    queue_create_infos.emplace_back(
        QueueCreateInfo(allocator, present_queue_family_index, 0));
    queue_create_infos.back().AddQueue(1.0f);
  }
  if (async_compute_queue_index != nullptr) {
    *async_compute_queue_index =
        GetAsyncComputeQueueFamilyIndex(allocator, *instance, physical_device);
    if (*async_compute_queue_index != 0xFFFFFFFF) {
      bool added = false;
      for (auto& qi : queue_create_infos) {
        if (qi.queue_family_index == *async_compute_queue_index) {
          qi.AddQueue(0.5f);
          added = true;
          break;
        }
      }
      if (!added) {
        queue_create_infos.emplace_back(
            QueueCreateInfo(allocator, *async_compute_queue_index, 0));
        queue_create_infos.back().AddQueue(0.5f);
      }
    }
  }
  if (sparse_binding_queue_index != nullptr) {
    *sparse_binding_queue_index = GetQueueFamily(
        allocator, *instance, physical_device, VK_QUEUE_SPARSE_BINDING_BIT);
    if (*sparse_binding_queue_index != 0xFFFFFFFF) {
      bool exist = false;
      for (auto& qi : queue_create_infos) {
        if (qi.queue_family_index == *sparse_binding_queue_index) {
          // no need to create new separate queue for sparse binding.
          exist = true;
          break;
        }
      }
      if (!exist) {
        queue_create_infos.emplace_back(
            QueueCreateInfo(allocator, *sparse_binding_queue_index, 0));
        queue_create_infos.back().AddQueue(1.0f);
      }
    }
  }
  *present_queue_index = present_queue_family_index;
  *graphics_queue_index = graphics_queue_family_index;
  return std::move(queue_create_infos);
}

VkDevice CreateDeviceGroupForSwapchain(
    containers::Allocator* allocator, VkInstance* instance,
    VkSurfaceKHR* surface, uint32_t* present_queue_index,
    uint32_t* graphics_queue_index,
    const std::initializer_list<const char*> extensions,
    const VkPhysicalDeviceFeatures& features,
    bool try_to_find_separate_present_queue,
    uint32_t* async_compute_queue_index, uint32_t* sparse_binding_queue_index,
    void* device_next) {
  uint32_t count = 0;
  LOG_ASSERT(
      ==, instance->GetLogger(), VK_SUCCESS,
      (*instance)->vkEnumeratePhysicalDeviceGroups(*instance, &count, nullptr));

  containers::vector<VkPhysicalDeviceGroupProperties> props(allocator);
  props.resize(count);
  LOG_ASSERT(==, instance->GetLogger(), VK_SUCCESS,
             (*instance)->vkEnumeratePhysicalDeviceGroups(*instance, &count,
                                                          props.data()));

  float priority = 1.f;
  containers::vector<float> additional_priorities(allocator);
  additional_priorities.push_back(1.0f);
  for (auto& group : props) {
    uint32_t present_queue_indices[VK_MAX_DEVICE_GROUP_SIZE];
    uint32_t graphics_queue_indices[VK_MAX_DEVICE_GROUP_SIZE];
    uint32_t async_compute_queue_indices[VK_MAX_DEVICE_GROUP_SIZE];
    uint32_t sparse_binding_queue_indices[VK_MAX_DEVICE_GROUP_SIZE];

    containers::vector<QueueCreateInfo> queue_create_infos(allocator);

    for (size_t i = 0; i < group.physicalDeviceCount; ++i) {
      auto newQueueCreateInfos = GetAcceptableQueues(
          allocator, group.physicalDevices[i], instance, surface,
          &present_queue_indices[i], &graphics_queue_indices[i], extensions,
          features, try_to_find_separate_present_queue,
          async_compute_queue_index ? &async_compute_queue_indices[i] : nullptr,
          sparse_binding_queue_index ? &sparse_binding_queue_indices[i]
                                     : nullptr);
      if (newQueueCreateInfos.size() == 0) {
        continue;
      }

      if (queue_create_infos.size() == 0) {
        queue_create_infos = std::move(newQueueCreateInfos);
      } else {
        if (present_queue_indices[i] != present_queue_indices[0]) {
          instance->GetLogger()->LogError(
              "Device group presentation queues do not match");
          continue;
        }
        if (graphics_queue_indices[i] != graphics_queue_indices[0]) {
          instance->GetLogger()->LogError(
              "Device group graphics queues do not match");
          continue;
        }
        if (async_compute_queue_index && (async_compute_queue_indices[i] !=
                                          async_compute_queue_indices[0])) {
          instance->GetLogger()->LogError(
              "Device group async queues do not match");
          continue;
        }
        if (sparse_binding_queue_index && (sparse_binding_queue_indices[i] !=
                                           sparse_binding_queue_indices[0])) {
          instance->GetLogger()->LogError(
              "Device group sparse queues do not match");
          continue;
        }
      }
    }

    const char* forced_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    containers::vector<const char*> enabled_extensions(allocator);
    for (auto ext : forced_extensions) {
      enabled_extensions.push_back(ext);
    }
    for (auto ext : extensions) {
      enabled_extensions.push_back(ext);
    }

    containers::vector<VkDeviceQueueCreateInfo> raw_queue_infos(allocator);
    raw_queue_infos.reserve(4);
    for (const auto& qi : queue_create_infos) {
      raw_queue_infos.emplace_back(qi.GetVkDeviceQueueCreateInfo());
    }
    // For now we create 1 or 2 devices, more can be done in the future, but
    // we should plumb down the number of devices.
    VkDeviceGroupDeviceCreateInfo device_group{
        VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO, device_next,
        group.physicalDeviceCount > 2 ? 2 : group.physicalDeviceCount,
        &group.physicalDevices[0]};

    VkDeviceCreateInfo info{
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,           // stype
        &device_group,                                  // pNext
        0,                                              // flags
        static_cast<uint32_t>(raw_queue_infos.size()),  // queueCreateInfoCount
        raw_queue_infos.data(),                         // pQueueCreateInfos
        0,                                              // enabledLayerCount
        nullptr,                                        // ppEnabledLayerNames
        static_cast<uint32_t>(
            enabled_extensions.size()),  // enabledExtensionCount
        enabled_extensions.data(),       // ppEnabledExtensionNames
        &features                        // ppEnabledFeatures
    };

    ::VkDevice raw_device;
    LOG_ASSERT(==, instance->GetLogger(),
               (*instance)->vkCreateDevice(group.physicalDevices[0], &info,
                                           nullptr, &raw_device),
               VK_SUCCESS);

    instance->GetLogger()->LogInfo("Enabled Device Extensions: ");
    for (auto& extension : enabled_extensions) {
      instance->GetLogger()->LogInfo("    ", extension);
    }

    *present_queue_index = present_queue_indices[0];
    *graphics_queue_index = graphics_queue_indices[0];
    if (async_compute_queue_index) {
      *async_compute_queue_index = async_compute_queue_indices[0];
    }
    if (sparse_binding_queue_index) {
      *sparse_binding_queue_index = sparse_binding_queue_indices[0];
    }

    return vulkan::VkDevice(allocator, raw_device, nullptr, instance, nullptr,
                            group.physicalDevices[0],
                            group.physicalDeviceCount);
  }
  instance->GetLogger()->LogError(
      "Could not find physical device or queue that can present");

  VkPhysicalDeviceProperties throwaway_properties;
  return vulkan::VkDevice(allocator, VK_NULL_HANDLE, nullptr, instance,
                          &throwaway_properties, VK_NULL_HANDLE);
}

VkCommandPool CreateDefaultCommandPool(containers::Allocator* allocator,
                                       VkDevice& device,
                                       bool use_protected_memory,
                                       uint32_t queueFamilyIndex) {
  VkCommandPoolCreateInfo info = {
      /* sType = */ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
          (use_protected_memory ? VK_COMMAND_POOL_CREATE_PROTECTED_BIT : 0u),
      // TODO(antiagainst): use a graphics + compute queue family.
      /* queueFamilyIndex = */ queueFamilyIndex,
  };

  ::VkCommandPool raw_command_pool = VK_NULL_HANDLE;
  if (device.is_valid()) {
    LOG_ASSERT(
        ==, device.GetLogger(),
        device->vkCreateCommandPool(device, &info, nullptr, &raw_command_pool),
        VK_SUCCESS);
  }
  return vulkan::VkCommandPool(raw_command_pool, nullptr, &device);
}

VkSurfaceKHR CreateDefaultSurface(VkInstance* instance,
                                  const entry::EntryData* data) {
  ::VkSurfaceKHR surface;
#if defined __ANDROID__
  VkAndroidSurfaceCreateInfoKHR create_info{
      VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, 0, 0,
      data->native_window_handle()};

  (*instance)->vkCreateAndroidSurfaceKHR(*instance, &create_info, nullptr,
                                         &surface);
#elif defined __ggp__
  VkStreamDescriptorSurfaceCreateInfoGGP create_info {
    VK_STRUCTURE_TYPE_STREAM_DESCRIPTOR_SURFACE_CREATE_INFO_GGP,
    nullptr, 0, 1
  } ;
  (*instance)->vkCreateStreamDescriptorSurfaceGGP(*instance, &create_info, nullptr,
                                         &surface);
#elif defined __linux__
  VkXcbSurfaceCreateInfoKHR create_info{
      VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR, 0, 0,
      data->native_connection(), data->native_window_handle()};

  (*instance)->vkCreateXcbSurfaceKHR(*instance, &create_info, nullptr,
                                     &surface);
#elif defined _WIN32
  VkWin32SurfaceCreateInfoKHR create_info{
      VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, 0, 0,
      data->native_hinstance(), data->native_window_handle()};

  (*instance)->vkCreateWin32SurfaceKHR(*instance, &create_info, nullptr,
                                       &surface);
#elif defined __APPLE__
  VkMacOSSurfaceCreateInfoMVK create_info{
      VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK, nullptr, 0,
      data->native_window_handle()};
  (*instance)->vkCreateMacOSSurfaceMVK(*instance, &create_info, nullptr,
                                       &surface);
#endif

  return VkSurfaceKHR(surface, nullptr, instance);
}

VkCommandBuffer CreateDefaultCommandBuffer(VkCommandPool* pool,
                                           VkDevice* device) {
  return CreateCommandBuffer(pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, device);
}

VkCommandBuffer CreateCommandBuffer(VkCommandPool* pool,
                                    VkCommandBufferLevel level,
                                    VkDevice* device) {
  VkCommandBufferAllocateInfo info = {
      /* sType = */ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      /* pNext = */ nullptr,
      /* commandPool = */ pool->get_raw_object(),
      /* level = */ level,
      /* commandBufferCount = */ 1,
  };
  ::VkCommandBuffer raw_command_buffer;
  LOG_ASSERT(
      ==, device->GetLogger(),
      (*device)->vkAllocateCommandBuffers(*device, &info, &raw_command_buffer),
      VK_SUCCESS);
  return vulkan::VkCommandBuffer(raw_command_buffer, pool, device);
}

VkSwapchainKHR CreateDefaultSwapchain(
    VkInstance* instance, VkDevice* device, VkSurfaceKHR* surface,
    containers::Allocator* allocator, uint32_t graphics_queue_index,
    uint32_t present_queue_index, const entry::EntryData* data,
    VkColorSpaceKHR swapchain_color_space, bool use_shared_presentation,
    VkSwapchainCreateFlagsKHR flags, bool use_10bit_hdr,
    const void* extensions) {
  ::VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  VkExtent2D image_extent = {0, 0};
  containers::vector<VkSurfaceFormatKHR> surface_formats(allocator);
  surface_formats.resize(1);

  if (device->is_valid()) {
    const bool has_multiple_queues =
        present_queue_index != graphics_queue_index;
    const uint32_t queues[2] = {graphics_queue_index, present_queue_index};
    VkSurfaceCapabilitiesKHR surface_caps;
    LOG_ASSERT(==, instance->GetLogger(),
               (*instance)->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                   device->physical_device(), *surface, &surface_caps),
               VK_SUCCESS);

    uint32_t num_formats = 0;
    LOG_ASSERT(==, instance->GetLogger(),
               (*instance)->vkGetPhysicalDeviceSurfaceFormatsKHR(
                   device->physical_device(), *surface, &num_formats, nullptr),
               VK_SUCCESS);

    surface_formats.resize(num_formats);
    LOG_ASSERT(==, instance->GetLogger(),
               (*instance)->vkGetPhysicalDeviceSurfaceFormatsKHR(
                   device->physical_device(), *surface, &num_formats,
                   surface_formats.data()),
               VK_SUCCESS);

	VkSurfaceFormatKHR surface_format = surface_formats[0];

	if (swapchain_color_space != 0) {
      containers::vector<VkSurfaceFormat2KHR> surface_formats2(
          allocator);
      surface_formats.resize(1);

      VkPhysicalDeviceSurfaceInfo2KHR surface_info{
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
          nullptr,  // pNext
          *surface  // surface
      };

      LOG_ASSERT(
          ==, instance->GetLogger(),
          (*instance)->vkGetPhysicalDeviceSurfaceFormats2KHR(
              device->physical_device(), &surface_info, &num_formats, nullptr),
          VK_SUCCESS);

      surface_formats2.resize(num_formats);
      LOG_ASSERT(==, instance->GetLogger(),
                 (*instance)->vkGetPhysicalDeviceSurfaceFormats2KHR(
                     device->physical_device(), &surface_info, &num_formats,
                     surface_formats2.data()),
                 VK_SUCCESS);

      for (size_t i = 0; i < surface_formats2.size(); ++i) {
        instance->GetLogger()->LogInfo(
            unsigned(surface_formats2[i].surfaceFormat.colorSpace));
        if (surface_formats2[i].surfaceFormat.colorSpace ==
            swapchain_color_space) {
          surface_format = surface_formats2[i].surfaceFormat;
        }
      }
	}

    uint32_t num_present_modes = 0;
    LOG_ASSERT(
        ==, instance->GetLogger(),
        (*instance)->vkGetPhysicalDeviceSurfacePresentModesKHR(
            device->physical_device(), *surface, &num_present_modes, nullptr),
        VK_SUCCESS);
    containers::vector<VkPresentModeKHR> present_modes(allocator);
    present_modes.resize(num_present_modes);

    LOG_ASSERT(==, instance->GetLogger(),
               (*instance)->vkGetPhysicalDeviceSurfacePresentModesKHR(
                   device->physical_device(), *surface, &num_present_modes,
                   present_modes.data()),
               VK_SUCCESS);

    uint32_t chosenAlpha =
        static_cast<uint32_t>(surface_caps.supportedCompositeAlpha);
    LOG_ASSERT(!=, instance->GetLogger(), 0,
               surface_caps.supportedCompositeAlpha);

    // Time to get the LSB of chosenAlpha;

    chosenAlpha = GetLSB(chosenAlpha);

    image_extent = surface_caps.currentExtent;
    if (image_extent.width == 0xFFFFFFFF) {
      image_extent = VkExtent2D{data->width(), data->height()};
    }

    uint32_t maxSwapchains =
        std::max(surface_caps.maxImageCount, surface_caps.minImageCount + 1);

    if (use_10bit_hdr) {
      surface_formats[0].format = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
      surface_formats[0].colorSpace = VK_COLOR_SPACE_HDR10_ST2084_EXT;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo{
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,  // sType
        extensions,                                   // pNext
        flags,                                        // flags
        *surface,                                     // surface
        std::min(surface_caps.minImageCount + 1,
                 maxSwapchains),        // minImageCount
        surface_formats[0].format,      // surfaceFormat
        surface_formats[0].colorSpace,  // colorSpace
        image_extent,                   // imageExtent
        1,                              // imageArrayLayers
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT,  // imageUsage
        has_multiple_queues ? VK_SHARING_MODE_CONCURRENT
                            : VK_SHARING_MODE_EXCLUSIVE,  // sharingMode
        has_multiple_queues ? 2u : 0u,
        has_multiple_queues ? queues : nullptr,  // pQueueFamilyIndices
        surface_caps.currentTransform,           // preTransform,
        static_cast<VkCompositeAlphaFlagBitsKHR>(
            chosenAlpha),  // compositeAlpha
        use_shared_presentation ? VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR
                                : present_modes.front(),  // presentModes
        false,                                            // clipped
        VK_NULL_HANDLE                                    // oldSwapchain
    };

    LOG_ASSERT(==, instance->GetLogger(),
               (*device)->vkCreateSwapchainKHR(*device, &swapchainCreateInfo,
                                               nullptr, &swapchain),
               VK_SUCCESS);
  }

  return VkSwapchainKHR(swapchain, nullptr, device, image_extent.width,
                        image_extent.height, 1u, surface_formats[0].format);
}

VkImage CreateDefault2DColorImage(VkDevice* device, uint32_t width,
                                  uint32_t height) {
  VkImageCreateInfo info{
      /* sType = */ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* imageType = */ VK_IMAGE_TYPE_2D,
      /* format = */ VK_FORMAT_R8G8B8A8_UNORM,
      /* extent = */
      {
          /* width = */ width,
          /* height = */ height,
          /* depth = */ 1,
      },
      /* mipLevels = */ 1,
      /* arrayLayers = */ 1,
      /* samples = */ VK_SAMPLE_COUNT_1_BIT,
      /* tiling = */ VK_IMAGE_TILING_OPTIMAL,
      /* usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      /* sharingMode = */ VK_SHARING_MODE_EXCLUSIVE,
      /* queueFamilyIndexCount = */ 0,
      /* pQueueFamilyIndices = */ nullptr,
      /* initialLayout = */ VK_IMAGE_LAYOUT_UNDEFINED,
  };
  ::VkImage raw_image;
  LOG_ASSERT(==, device->GetLogger(),
             (*device)->vkCreateImage(*device, &info, nullptr, &raw_image),
             VK_SUCCESS);
  return vulkan::VkImage(raw_image, nullptr, device);
}

VkSampler CreateDefaultSampler(VkDevice* device) {
  VkSamplerCreateInfo info{
      /* sType = */ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* magFilter = */ VK_FILTER_NEAREST,
      /* minFilter = */ VK_FILTER_NEAREST,
      /* mipmapMode = */ VK_SAMPLER_MIPMAP_MODE_NEAREST,
      /* addressModeU = */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      /* addressModeV = */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      /* addressModeW = */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      /* mipLodBias = */ 0.f,
      /* anisotropyEnable = */ false,
      /* maxAnisotropy = */ 1.f,
      /* compareEnable = */ false,
      /* compareOp = */ VK_COMPARE_OP_NEVER,
      /* minLod = */ 0.f,
      /* maxLod = */ 0.f,
      /* borderColor = */ VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
      /* unnormalizedCoordinates = */ false,
  };
  ::VkSampler raw_sampler;
  LOG_ASSERT(==, device->GetLogger(),
             (*device)->vkCreateSampler(*device, &info, nullptr, &raw_sampler),
             VK_SUCCESS);
  return vulkan::VkSampler(raw_sampler, nullptr, device);
}

VkSampler CreateSampler(VkDevice* device, VkFilter minFilter,
                        VkFilter magFilter, VkSamplerAddressMode addressModeU,
                        VkSamplerAddressMode addressModeV,
                        VkSamplerAddressMode addressModeW, void* extension) {
  VkSamplerCreateInfo info{
      /* sType = */ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      /* pNext = */ extension,
      /* flags = */ 0,
      /* magFilter = */ minFilter,
      /* minFilter = */ magFilter,
      /* mipmapMode = */ VK_SAMPLER_MIPMAP_MODE_NEAREST,
      /* addressModeU = */ addressModeU,
      /* addressModeV = */ addressModeV,
      /* addressModeW = */ addressModeW,
      /* mipLodBias = */ 0.f,
      /* anisotropyEnable = */ false,
      /* maxAnisotropy = */ 1.f,
      /* compareEnable = */ false,
      /* compareOp = */ VK_COMPARE_OP_NEVER,
      /* minLod = */ 0.f,
      /* maxLod = */ 0.f,
      /* borderColor = */ VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
      /* unnormalizedCoordinates = */ false,
  };
  ::VkSampler raw_sampler;
  LOG_ASSERT(==, device->GetLogger(),
             (*device)->vkCreateSampler(*device, &info, nullptr, &raw_sampler),
             VK_SUCCESS);
  return vulkan::VkSampler(raw_sampler, nullptr, device);
}

VkDescriptorSetLayout CreateDescriptorSetLayout(
    containers::Allocator* allocator, VkDevice* device,
    std::initializer_list<VkDescriptorSetLayoutBinding> bindings,
    VkDescriptorSetLayoutCreateFlags flags) {
  containers::vector<VkDescriptorSetLayoutBinding> contiguous_bindings(
      allocator);
  contiguous_bindings.resize(bindings.size());
  auto binding = bindings.begin();
  for (size_t i = 0; i < bindings.size(); ++i, ++binding) {
    contiguous_bindings[i] = *binding;
  }

  VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, flags,
      static_cast<uint32_t>(contiguous_bindings.size()),
      contiguous_bindings.data()};

  ::VkDescriptorSetLayout layout;
  LOG_ASSERT(
      ==, device->GetLogger(), VK_SUCCESS,
      (*device)->vkCreateDescriptorSetLayout(
          *device, &descriptor_set_layout_create_info, nullptr, &layout));
  return VkDescriptorSetLayout(layout, nullptr, device);
}

// Creates a default pipeline cache, it does not load anything from disk.
VkPipelineCache CreateDefaultPipelineCache(VkDevice* device) {
  ::VkPipelineCache cache = VK_NULL_HANDLE;

  VkPipelineCacheCreateInfo create_info{
      VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,  // sType
      nullptr,                                       // pNext
      0,                                             // flags
      0,                                             // initialDataSize
      nullptr                                        // pInitialData
  };
  if (device->is_valid()) {
    LOG_ASSERT(==, device->GetLogger(), VK_SUCCESS,
               (*device)->vkCreatePipelineCache(*device, &create_info, nullptr,
                                                &cache));
  }
  return VkPipelineCache(cache, nullptr, device);
}

VkQueryPool CreateQueryPool(VkDevice* device,
                            const VkQueryPoolCreateInfo& create_info) {
  ::VkQueryPool query_pool = VK_NULL_HANDLE;
  if (device->is_valid()) {
    LOG_ASSERT(==, device->GetLogger(), VK_SUCCESS,
               (*device)->vkCreateQueryPool(*device, &create_info, nullptr,
                                            &query_pool));
  }
  return VkQueryPool(query_pool, nullptr, device);
}

VkDescriptorPool CreateDescriptorPool(VkDevice* device, uint32_t num_pool_size,
                                      const VkDescriptorPoolSize* pool_sizes,
                                      uint32_t max_sets) {
  VkDescriptorPoolCreateInfo info{
      /* sType = */ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      /* maxSets = */ max_sets,
      /* poolSizeCount = */ num_pool_size,
      /* pPoolSizes = */ pool_sizes};

  ::VkDescriptorPool raw_pool;
  LOG_ASSERT(
      ==, device->GetLogger(),
      (*device)->vkCreateDescriptorPool(*device, &info, nullptr, &raw_pool),
      VK_SUCCESS);
  return vulkan::VkDescriptorPool(raw_pool, nullptr, device);
}

VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice* device,
                                                ::VkDescriptorType type,
                                                uint32_t count) {
  VkDescriptorSetLayoutBinding binding{
      /* binding = */ 0,
      /* descriptorType = */ type,
      /* descriptorCount = */ count,
      /* stageFlags = */ VK_SHADER_STAGE_ALL,
      /* pImmutableSamplers = */ nullptr,
  };
  VkDescriptorSetLayoutCreateInfo info{
      /* sType = */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      /* pNext = */ nullptr,
      /* flags = */ 0,
      /* bindingCount = */ 1,
      /* pBindings = */ &binding,
  };

  ::VkDescriptorSetLayout raw_layout;
  LOG_ASSERT(==, device->GetLogger(),
             (*device)->vkCreateDescriptorSetLayout(*device, &info, nullptr,
                                                    &raw_layout),
             VK_SUCCESS);
  return vulkan::VkDescriptorSetLayout(raw_layout, nullptr, device);
}

VkDescriptorSet AllocateDescriptorSet(VkDevice* device, ::VkDescriptorPool pool,
                                      ::VkDescriptorSetLayout layout) {
  VkDescriptorSetAllocateInfo alloc_info{
      /* sType = */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      /* pNext = */ nullptr,
      /* descriptorPool = */ pool,
      /* descriptorSetCount = */ 1,
      /* pSetLayouts = */ &layout,
  };
  ::VkDescriptorSet raw_set;
  LOG_ASSERT(
      ==, device->GetLogger(),
      (*device)->vkAllocateDescriptorSets(*device, &alloc_info, &raw_set),
      VK_SUCCESS);
  return vulkan::VkDescriptorSet(raw_set, pool, device);
}

VkDeviceMemory AllocateDeviceMemory(VkDevice* device,
                                    uint32_t memory_type_index,
                                    ::VkDeviceSize size) {
  const ::VkMemoryAllocateInfo alloc_info = {
      /* sType = */ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      /* pNext = */ nullptr,
      /* allocationSize = */ size,
      /* memoryTypeIndex = */ memory_type_index,
  };
  ::VkDeviceMemory raw_memory;
  LOG_ASSERT(
      ==, device->GetLogger(),
      (*device)->vkAllocateMemory(*device, &alloc_info, nullptr, &raw_memory),
      VK_SUCCESS);
  return vulkan::VkDeviceMemory(raw_memory, nullptr, device);
}

void RecordImageLayoutTransition(
    ::VkImage image, const VkImageSubresourceRange& subresource_range,
    VkImageLayout old_layout, VkAccessFlags src_access_mask,
    VkImageLayout new_layout, VkAccessFlags dst_access_mask,
    VkCommandBuffer* cmd_buffer) {
  // As we are changing the image memory layout, we should have a image memory
  // barrier.
  VkImageMemoryBarrier image_memory_barrier = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // sType
      NULL,                                    // pNext
      src_access_mask,                         // srcAccessMask
      dst_access_mask,          // dstAccessMask: Set it in below.
      old_layout,               // oldLayout
      new_layout,               // newLayout
      VK_QUEUE_FAMILY_IGNORED,  // srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,  // dstQueueFamilyIndex
      image,                    // image
      subresource_range,        // subresourceRange
  };
  (*cmd_buffer)
      ->vkCmdPipelineBarrier(
          *cmd_buffer,                         // commandBuffer
          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  // dstStageMask
          0,                                   // dependencyFlags
          0,                                   // memoryBarrierCount
          nullptr,                             // pMemoryBarriers
          0,                                   // bufferMemoryBarrierCount
          nullptr,                             // pBufferMemoryBarriers
          1,                                   // imageMemoryBarrierCount
          &image_memory_barrier                // pImageMemoryBarriers
      );
}

namespace {
// A helper function that returns the round-up result of unsigned integer
// division.  Returns 0 if the given divisor value is 0.
uint32_t RoundUpTo(uint32_t dividend, uint32_t divisor) {
  if (divisor == 0) return 0;
  return (dividend + divisor - 1) / divisor;
}
}  // namespace

std::tuple<uint32_t, uint32_t, uint32_t> GetElementAndTexelBlockSize(
    VkFormat format) {
  switch (format) {
    case VK_FORMAT_R8_UNORM:
      return std::make_tuple(
          1, 1, 1);  // element size, texel block width, texel height
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_R16_UNORM:
      return std::make_tuple(2, 1, 1);
    case VK_FORMAT_R8G8B8_UNORM:
      return std::make_tuple(3, 1, 1);
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
      return std::make_tuple(4, 1, 1);
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
      return std::make_tuple(5, 1, 1);
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
      return std::make_tuple(16, 4, 4);
    case VK_FORMAT_R16G16B16A16_SFLOAT:
      return std::make_tuple(8, 1, 1);
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_R32G32B32A32_UINT:
      return std::make_tuple(16, 1, 1);
    default:
      break;
  }
  return std::make_tuple(0, 0, 0);
}

size_t GetImageExtentSizeInBytes(const VkExtent3D& extent, VkFormat format) {
  auto element_texel_block_sizes = GetElementAndTexelBlockSize(format);
  uint32_t element_size = size_t(std::get<0>(element_texel_block_sizes));
  uint32_t tb_width_size = std::get<1>(element_texel_block_sizes);
  uint32_t tb_height_size = std::get<2>(element_texel_block_sizes);
  if (element_size == 0 || tb_width_size == 0 || tb_height_size == 0) {
    return 0;
  }
  size_t w = size_t(RoundUpTo(extent.width, tb_width_size));
  size_t h = size_t(RoundUpTo(extent.height, tb_height_size));
  return w * h * element_size;
}
}  // namespace vulkan