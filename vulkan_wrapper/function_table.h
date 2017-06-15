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

#ifndef VULKAN_WRAPPER_FUNCTION_TABLE_H_
#define VULKAN_WRAPPER_FUNCTION_TABLE_H_

#include "vulkan_wrapper/lazy_function.h"

namespace vulkan {

class InstanceFunctions;
template <typename T>
using LazyInstanceFunction = LazyFunction<T, ::VkInstance, InstanceFunctions>;

// InstanceFunctions contains a list of lazily resolved Vulkan instance
// functions. All the lazily resolved functions are implemented through
// LazyFunction template, GetLogger() and getProcAddr() methods are required to
// conform the LazyFunction template. As this class is the source of lazily
// resolved Vulkan functions, the instance of this class is non-movable and
// non-copyable.
class InstanceFunctions {
 public:
  InstanceFunctions(const InstanceFunctions& other) = delete;
  InstanceFunctions(InstanceFunctions&& other) = delete;
  InstanceFunctions& operator=(const InstanceFunctions& other) = delete;
  InstanceFunctions& operator=(InstanceFunctions&& other) = delete;

  InstanceFunctions(::VkInstance instance,
                    PFN_vkGetInstanceProcAddr get_proc_addr_func,
                    logging::Logger* log)
      : log_(log),
        vkGetInstanceProcAddr_(get_proc_addr_func),
#define CONSTRUCT_LAZY_FUNCTION(function) function(instance, #function, this)
        CONSTRUCT_LAZY_FUNCTION(vkDestroyInstance),
        CONSTRUCT_LAZY_FUNCTION(vkEnumeratePhysicalDevices),
        CONSTRUCT_LAZY_FUNCTION(vkCreateDevice),
        CONSTRUCT_LAZY_FUNCTION(vkEnumerateDeviceExtensionProperties),
        CONSTRUCT_LAZY_FUNCTION(vkEnumerateDeviceLayerProperties),
        CONSTRUCT_LAZY_FUNCTION(vkGetPhysicalDeviceFeatures),
        CONSTRUCT_LAZY_FUNCTION(vkGetPhysicalDeviceMemoryProperties),
        CONSTRUCT_LAZY_FUNCTION(vkGetPhysicalDeviceProperties),
        CONSTRUCT_LAZY_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties),
        CONSTRUCT_LAZY_FUNCTION(vkGetPhysicalDeviceFormatProperties),
        CONSTRUCT_LAZY_FUNCTION(vkGetPhysicalDeviceImageFormatProperties),
        CONSTRUCT_LAZY_FUNCTION(vkGetPhysicalDeviceSparseImageFormatProperties),
        CONSTRUCT_LAZY_FUNCTION(vkDestroySurfaceKHR),
        CONSTRUCT_LAZY_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR),
        CONSTRUCT_LAZY_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR),
        CONSTRUCT_LAZY_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR),
        CONSTRUCT_LAZY_FUNCTION(vkGetPhysicalDeviceSurfacePresentModesKHR)
#if defined __ANDROID__
        ,
        CONSTRUCT_LAZY_FUNCTION(vkCreateAndroidSurfaceKHR)
#elif defined __linux__
        ,
        CONSTRUCT_LAZY_FUNCTION(vkCreateXcbSurfaceKHR)
#elif defined _WIN32
        ,
        CONSTRUCT_LAZY_FUNCTION(vkCreateWin32SurfaceKHR)
#endif
#undef CONSTRUCT_LAZY_FUNCTION
  {
  }

 private:
  logging::Logger* log_;
  // The function pointer to Vulkan vkGetInstanceProcAddr().
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_;

 public:
  // Returns the logger. This is required to conform LazyFunction template.
  logging::Logger* GetLogger() { return log_; }
  // Resolves an instance function with the given name. This is required to
  // conform LazyFunction template.
  PFN_vkVoidFunction getProcAddr(::VkInstance instance, const char* function) {
    return vkGetInstanceProcAddr_(instance, function);
  }

#define LAZY_FUNCTION(function) LazyInstanceFunction<PFN_##function> function;
  LAZY_FUNCTION(vkDestroyInstance);
  LAZY_FUNCTION(vkEnumeratePhysicalDevices);
  LAZY_FUNCTION(vkCreateDevice);
  LAZY_FUNCTION(vkEnumerateDeviceExtensionProperties);
  LAZY_FUNCTION(vkEnumerateDeviceLayerProperties);
  LAZY_FUNCTION(vkGetPhysicalDeviceFeatures);
  LAZY_FUNCTION(vkGetPhysicalDeviceMemoryProperties);
  LAZY_FUNCTION(vkGetPhysicalDeviceProperties);
  LAZY_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties);
  LAZY_FUNCTION(vkGetPhysicalDeviceFormatProperties);
  LAZY_FUNCTION(vkGetPhysicalDeviceImageFormatProperties);
  LAZY_FUNCTION(vkGetPhysicalDeviceSparseImageFormatProperties);
  LAZY_FUNCTION(vkDestroySurfaceKHR);
  LAZY_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR);
  LAZY_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
  LAZY_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR);
  LAZY_FUNCTION(vkGetPhysicalDeviceSurfacePresentModesKHR);
#if defined __ANDROID__
  LAZY_FUNCTION(vkCreateAndroidSurfaceKHR);
#elif defined __linux__
  LAZY_FUNCTION(vkCreateXcbSurfaceKHR);
#elif defined _WIN32
  LAZY_FUNCTION(vkCreateWin32SurfaceKHR);
#endif
#undef LAZY_FUNCTION
};

class DeviceFunctions;
template <typename T>
using LazyDeviceFunction = LazyFunction<T, ::VkDevice, DeviceFunctions>;

// CommandBufferFunctions stores a list of lazily resolved Vulkan Command
// buffer functions. The instance of this class should be owned and the
// functions listed inside should be resolved by DeviceFunctions. This class
// does not need to conform LazyFunction template as no function is resolved
// through it.
struct CommandBufferFunctions {
 public:
  CommandBufferFunctions(::VkDevice device, DeviceFunctions* device_functions)
      :
#define CONSTRUCT_LAZY_FUNCTION(function) \
  function(device, #function, device_functions)
        CONSTRUCT_LAZY_FUNCTION(vkBeginCommandBuffer),
        CONSTRUCT_LAZY_FUNCTION(vkEndCommandBuffer),
        CONSTRUCT_LAZY_FUNCTION(vkResetCommandBuffer),
        CONSTRUCT_LAZY_FUNCTION(vkCmdPipelineBarrier),
        CONSTRUCT_LAZY_FUNCTION(vkCmdCopyBufferToImage),
        CONSTRUCT_LAZY_FUNCTION(vkCmdCopyImageToBuffer),
        CONSTRUCT_LAZY_FUNCTION(vkCmdBeginRenderPass),
        CONSTRUCT_LAZY_FUNCTION(vkCmdEndRenderPass),
        CONSTRUCT_LAZY_FUNCTION(vkCmdNextSubpass),
        CONSTRUCT_LAZY_FUNCTION(vkCmdBindPipeline),
        CONSTRUCT_LAZY_FUNCTION(vkCmdSetLineWidth),
        CONSTRUCT_LAZY_FUNCTION(vkCmdSetBlendConstants),
        CONSTRUCT_LAZY_FUNCTION(vkCmdSetDepthBias),
        CONSTRUCT_LAZY_FUNCTION(vkCmdSetScissor),
        CONSTRUCT_LAZY_FUNCTION(vkCmdSetViewport),
        CONSTRUCT_LAZY_FUNCTION(vkCmdCopyBuffer),
        CONSTRUCT_LAZY_FUNCTION(vkCmdBindDescriptorSets),
        CONSTRUCT_LAZY_FUNCTION(vkCmdBindVertexBuffers),
        CONSTRUCT_LAZY_FUNCTION(vkCmdClearColorImage),
        CONSTRUCT_LAZY_FUNCTION(vkCmdClearDepthStencilImage),
        CONSTRUCT_LAZY_FUNCTION(vkCmdBindIndexBuffer),
        CONSTRUCT_LAZY_FUNCTION(vkCmdDraw),
        CONSTRUCT_LAZY_FUNCTION(vkCmdDrawIndexed),
        CONSTRUCT_LAZY_FUNCTION(vkCmdDrawIndirect),
        CONSTRUCT_LAZY_FUNCTION(vkCmdDrawIndexedIndirect),
        CONSTRUCT_LAZY_FUNCTION(vkCmdDispatch),
        CONSTRUCT_LAZY_FUNCTION(vkCmdDispatchIndirect),
        CONSTRUCT_LAZY_FUNCTION(vkCmdBlitImage),
        CONSTRUCT_LAZY_FUNCTION(vkCmdPushConstants),
        CONSTRUCT_LAZY_FUNCTION(vkCmdExecuteCommands),
        CONSTRUCT_LAZY_FUNCTION(vkCmdResolveImage),
        CONSTRUCT_LAZY_FUNCTION(vkCmdCopyImage),
        CONSTRUCT_LAZY_FUNCTION(vkCmdClearAttachments),
        CONSTRUCT_LAZY_FUNCTION(vkCmdUpdateBuffer),
        CONSTRUCT_LAZY_FUNCTION(vkCmdFillBuffer),
        CONSTRUCT_LAZY_FUNCTION(vkCmdResetQueryPool),
        CONSTRUCT_LAZY_FUNCTION(vkCmdBeginQuery),
        CONSTRUCT_LAZY_FUNCTION(vkCmdEndQuery),
        CONSTRUCT_LAZY_FUNCTION(vkCmdCopyQueryPoolResults),
        CONSTRUCT_LAZY_FUNCTION(vkCmdSetEvent),
        CONSTRUCT_LAZY_FUNCTION(vkCmdResetEvent),
        CONSTRUCT_LAZY_FUNCTION(vkCmdWaitEvents)
#undef CONSTRUCT_LAZY_FUNCTION
  {
  }

 public:
#define LAZY_FUNCTION(function) LazyDeviceFunction<PFN_##function> function;
  LAZY_FUNCTION(vkBeginCommandBuffer);
  LAZY_FUNCTION(vkEndCommandBuffer);
  LAZY_FUNCTION(vkResetCommandBuffer);
  LAZY_FUNCTION(vkCmdPipelineBarrier);
  LAZY_FUNCTION(vkCmdCopyBufferToImage);
  LAZY_FUNCTION(vkCmdCopyImageToBuffer);
  LAZY_FUNCTION(vkCmdBeginRenderPass);
  LAZY_FUNCTION(vkCmdEndRenderPass);
  LAZY_FUNCTION(vkCmdNextSubpass);
  LAZY_FUNCTION(vkCmdBindPipeline);
  LAZY_FUNCTION(vkCmdSetLineWidth);
  LAZY_FUNCTION(vkCmdSetBlendConstants);
  LAZY_FUNCTION(vkCmdSetDepthBias);
  LAZY_FUNCTION(vkCmdSetScissor);
  LAZY_FUNCTION(vkCmdSetViewport);
  LAZY_FUNCTION(vkCmdCopyBuffer);
  LAZY_FUNCTION(vkCmdBindDescriptorSets);
  LAZY_FUNCTION(vkCmdBindVertexBuffers);
  LAZY_FUNCTION(vkCmdClearColorImage);
  LAZY_FUNCTION(vkCmdClearDepthStencilImage);
  LAZY_FUNCTION(vkCmdBindIndexBuffer);
  LAZY_FUNCTION(vkCmdDraw);
  LAZY_FUNCTION(vkCmdDrawIndexed);
  LAZY_FUNCTION(vkCmdDrawIndirect);
  LAZY_FUNCTION(vkCmdDrawIndexedIndirect);
  LAZY_FUNCTION(vkCmdDispatch);
  LAZY_FUNCTION(vkCmdDispatchIndirect);
  LAZY_FUNCTION(vkCmdBlitImage);
  LAZY_FUNCTION(vkCmdPushConstants);
  LAZY_FUNCTION(vkCmdExecuteCommands);
  LAZY_FUNCTION(vkCmdResolveImage);
  LAZY_FUNCTION(vkCmdCopyImage);
  LAZY_FUNCTION(vkCmdClearAttachments);
  LAZY_FUNCTION(vkCmdUpdateBuffer);
  LAZY_FUNCTION(vkCmdFillBuffer);
  LAZY_FUNCTION(vkCmdResetQueryPool);
  LAZY_FUNCTION(vkCmdBeginQuery);
  LAZY_FUNCTION(vkCmdEndQuery);
  LAZY_FUNCTION(vkCmdCopyQueryPoolResults);
  LAZY_FUNCTION(vkCmdSetEvent);
  LAZY_FUNCTION(vkCmdResetEvent);
  LAZY_FUNCTION(vkCmdWaitEvents);
#undef LAZY_FUNCTION
};

struct QueueFunctions {
 public:
  QueueFunctions(::VkDevice device, DeviceFunctions* device_functions)
      :
#define CONSTRUCT_LAZY_FUNCTION(function) \
  function(device, #function, device_functions)
        CONSTRUCT_LAZY_FUNCTION(vkQueueSubmit),
        CONSTRUCT_LAZY_FUNCTION(vkQueueWaitIdle),
        CONSTRUCT_LAZY_FUNCTION(vkQueuePresentKHR)
#undef CONSTRUCT_LAZY_FUNCTION
  {
  }

 public:
#define LAZY_FUNCTION(function) LazyDeviceFunction<PFN_##function> function;
  LAZY_FUNCTION(vkQueueSubmit);
  LAZY_FUNCTION(vkQueueWaitIdle);
  LAZY_FUNCTION(vkQueuePresentKHR);
#undef LAZY_FUNCTION
};

// DeviceFunctions contains a list of lazily resolved Vulkan device functions
// and the functions of sub-device objects.All the lazily resolved functions
// are implemented through LazyFunction template, GetLogger() and getProcAddr()
// methods are required to conform the LazyFunction template. As this class is
// the source of lazily resolved Vulkan functions, the instance of this class
// is non-movable and non-copyable.
class DeviceFunctions {
 public:
  DeviceFunctions(const DeviceFunctions& other) = delete;
  DeviceFunctions(DeviceFunctions&& other) = delete;
  DeviceFunctions& operator=(const DeviceFunctions& other) = delete;
  DeviceFunctions& operator=(DeviceFunctions&& other) = delete;

  DeviceFunctions(::VkDevice device, PFN_vkGetDeviceProcAddr get_proc_addr_func,
                  logging::Logger* log)
      : log_(log),
        vkGetDeviceProcAddr_(get_proc_addr_func),
        command_buffer_functions_(device, this),
        queue_functions_(device, this),
#define CONSTRUCT_LAZY_FUNCTION(function) function(device, #function, this)
        CONSTRUCT_LAZY_FUNCTION(vkDestroyDevice),
        CONSTRUCT_LAZY_FUNCTION(vkCreateCommandPool),
        CONSTRUCT_LAZY_FUNCTION(vkResetCommandPool),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyCommandPool),
        CONSTRUCT_LAZY_FUNCTION(vkAllocateCommandBuffers),
        CONSTRUCT_LAZY_FUNCTION(vkFreeCommandBuffers),
        CONSTRUCT_LAZY_FUNCTION(vkGetDeviceQueue),
        CONSTRUCT_LAZY_FUNCTION(vkCreateSemaphore),
        CONSTRUCT_LAZY_FUNCTION(vkDestroySemaphore),
        CONSTRUCT_LAZY_FUNCTION(vkCreateImage),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyImage),
        CONSTRUCT_LAZY_FUNCTION(vkCreateSwapchainKHR),
        CONSTRUCT_LAZY_FUNCTION(vkDestroySwapchainKHR),
        CONSTRUCT_LAZY_FUNCTION(vkGetSwapchainImagesKHR),
        CONSTRUCT_LAZY_FUNCTION(vkGetImageMemoryRequirements),
        CONSTRUCT_LAZY_FUNCTION(vkGetImageSubresourceLayout),
        CONSTRUCT_LAZY_FUNCTION(vkCreateImageView),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyImageView),
        CONSTRUCT_LAZY_FUNCTION(vkCreateRenderPass),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyRenderPass),
        CONSTRUCT_LAZY_FUNCTION(vkCreatePipelineCache),
        CONSTRUCT_LAZY_FUNCTION(vkGetPipelineCacheData),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyPipelineCache),
        CONSTRUCT_LAZY_FUNCTION(vkCreateFramebuffer),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyFramebuffer),
        CONSTRUCT_LAZY_FUNCTION(vkAllocateMemory),
        CONSTRUCT_LAZY_FUNCTION(vkFreeMemory),
        CONSTRUCT_LAZY_FUNCTION(vkBindImageMemory),
        CONSTRUCT_LAZY_FUNCTION(vkCreateShaderModule),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyShaderModule),
        CONSTRUCT_LAZY_FUNCTION(vkCreateSampler),
        CONSTRUCT_LAZY_FUNCTION(vkDestroySampler),
        CONSTRUCT_LAZY_FUNCTION(vkCreateBuffer),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyBuffer),
        CONSTRUCT_LAZY_FUNCTION(vkCreateBufferView),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyBufferView),
        CONSTRUCT_LAZY_FUNCTION(vkGetBufferMemoryRequirements),
        CONSTRUCT_LAZY_FUNCTION(vkMapMemory),
        CONSTRUCT_LAZY_FUNCTION(vkUnmapMemory),
        CONSTRUCT_LAZY_FUNCTION(vkBindBufferMemory),
        CONSTRUCT_LAZY_FUNCTION(vkCreateDescriptorPool),
        CONSTRUCT_LAZY_FUNCTION(vkResetDescriptorPool),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyDescriptorPool),
        CONSTRUCT_LAZY_FUNCTION(vkCreateDescriptorSetLayout),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyDescriptorSetLayout),
        CONSTRUCT_LAZY_FUNCTION(vkFlushMappedMemoryRanges),
        CONSTRUCT_LAZY_FUNCTION(vkInvalidateMappedMemoryRanges),
        CONSTRUCT_LAZY_FUNCTION(vkCreatePipelineLayout),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyPipelineLayout),
        CONSTRUCT_LAZY_FUNCTION(vkCreateGraphicsPipelines),
        CONSTRUCT_LAZY_FUNCTION(vkCreateComputePipelines),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyPipeline),
        CONSTRUCT_LAZY_FUNCTION(vkAllocateDescriptorSets),
        CONSTRUCT_LAZY_FUNCTION(vkUpdateDescriptorSets),
        CONSTRUCT_LAZY_FUNCTION(vkFreeDescriptorSets),
        CONSTRUCT_LAZY_FUNCTION(vkCreateFence),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyFence),
        CONSTRUCT_LAZY_FUNCTION(vkResetFences),
        CONSTRUCT_LAZY_FUNCTION(vkWaitForFences),
        CONSTRUCT_LAZY_FUNCTION(vkGetFenceStatus),
        CONSTRUCT_LAZY_FUNCTION(vkAcquireNextImageKHR),
        CONSTRUCT_LAZY_FUNCTION(vkDeviceWaitIdle),
        CONSTRUCT_LAZY_FUNCTION(vkCreateQueryPool),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyQueryPool),
        CONSTRUCT_LAZY_FUNCTION(vkGetQueryPoolResults),
        CONSTRUCT_LAZY_FUNCTION(vkCreateEvent),
        CONSTRUCT_LAZY_FUNCTION(vkDestroyEvent),
        CONSTRUCT_LAZY_FUNCTION(vkGetEventStatus),
        CONSTRUCT_LAZY_FUNCTION(vkSetEvent),
        CONSTRUCT_LAZY_FUNCTION(vkResetEvent)
#undef CONSTRUCT_LAZY_FUNCTION
  {
  }

 private:
  logging::Logger* log_;
  // The function pointer to Vulkan vkGetDeviceProcAddr().
  PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr_;
  // Functions of sub device objects.
  CommandBufferFunctions command_buffer_functions_;
  QueueFunctions queue_functions_;

 public:
  // Returns the logger. This is required to conform LazyFunction template.
  logging::Logger* GetLogger() { return log_; }
  // Resolves a device function with the given name. This is required to
  // conform LazyFunction template.
  PFN_vkVoidFunction getProcAddr(::VkDevice device, const char* function) {
    return vkGetDeviceProcAddr_(device, function);
  }
  // Access the command buffer functions.
  CommandBufferFunctions* command_buffer_functions() {
    return &command_buffer_functions_;
  }
  QueueFunctions* queue_functions() { return &queue_functions_; }

#define LAZY_FUNCTION(function) LazyDeviceFunction<PFN_##function> function;
  LAZY_FUNCTION(vkDestroyDevice);
  LAZY_FUNCTION(vkCreateCommandPool);
  LAZY_FUNCTION(vkResetCommandPool);
  LAZY_FUNCTION(vkDestroyCommandPool);
  LAZY_FUNCTION(vkAllocateCommandBuffers);
  LAZY_FUNCTION(vkFreeCommandBuffers);
  LAZY_FUNCTION(vkGetDeviceQueue);
  LAZY_FUNCTION(vkCreateSemaphore);
  LAZY_FUNCTION(vkDestroySemaphore);
  LAZY_FUNCTION(vkCreateImage);
  LAZY_FUNCTION(vkDestroyImage);
  LAZY_FUNCTION(vkCreateSwapchainKHR);
  LAZY_FUNCTION(vkDestroySwapchainKHR);
  LAZY_FUNCTION(vkGetSwapchainImagesKHR);
  LAZY_FUNCTION(vkGetImageMemoryRequirements);
  LAZY_FUNCTION(vkGetImageSubresourceLayout);
  LAZY_FUNCTION(vkCreateImageView);
  LAZY_FUNCTION(vkDestroyImageView);
  LAZY_FUNCTION(vkCreateRenderPass);
  LAZY_FUNCTION(vkDestroyRenderPass);
  LAZY_FUNCTION(vkCreatePipelineCache);
  LAZY_FUNCTION(vkGetPipelineCacheData);
  LAZY_FUNCTION(vkDestroyPipelineCache);
  LAZY_FUNCTION(vkCreateFramebuffer);
  LAZY_FUNCTION(vkDestroyFramebuffer);
  LAZY_FUNCTION(vkAllocateMemory);
  LAZY_FUNCTION(vkFreeMemory);
  LAZY_FUNCTION(vkBindImageMemory);
  LAZY_FUNCTION(vkCreateShaderModule);
  LAZY_FUNCTION(vkDestroyShaderModule);
  LAZY_FUNCTION(vkCreateSampler);
  LAZY_FUNCTION(vkDestroySampler);
  LAZY_FUNCTION(vkCreateBuffer);
  LAZY_FUNCTION(vkDestroyBuffer);
  LAZY_FUNCTION(vkCreateBufferView);
  LAZY_FUNCTION(vkDestroyBufferView);
  LAZY_FUNCTION(vkGetBufferMemoryRequirements);
  LAZY_FUNCTION(vkMapMemory)
  LAZY_FUNCTION(vkUnmapMemory);
  LAZY_FUNCTION(vkBindBufferMemory);
  LAZY_FUNCTION(vkCreateDescriptorPool);
  LAZY_FUNCTION(vkResetDescriptorPool);
  LAZY_FUNCTION(vkDestroyDescriptorPool);
  LAZY_FUNCTION(vkCreateDescriptorSetLayout);
  LAZY_FUNCTION(vkDestroyDescriptorSetLayout);
  LAZY_FUNCTION(vkFlushMappedMemoryRanges);
  LAZY_FUNCTION(vkInvalidateMappedMemoryRanges);
  LAZY_FUNCTION(vkCreatePipelineLayout);
  LAZY_FUNCTION(vkDestroyPipelineLayout);
  LAZY_FUNCTION(vkCreateGraphicsPipelines);
  LAZY_FUNCTION(vkCreateComputePipelines);
  LAZY_FUNCTION(vkDestroyPipeline);
  LAZY_FUNCTION(vkAllocateDescriptorSets);
  LAZY_FUNCTION(vkUpdateDescriptorSets);
  LAZY_FUNCTION(vkFreeDescriptorSets);
  LAZY_FUNCTION(vkCreateFence);
  LAZY_FUNCTION(vkDestroyFence);
  LAZY_FUNCTION(vkWaitForFences);
  LAZY_FUNCTION(vkGetFenceStatus);
  LAZY_FUNCTION(vkResetFences);
  LAZY_FUNCTION(vkAcquireNextImageKHR);
  LAZY_FUNCTION(vkDeviceWaitIdle);
  LAZY_FUNCTION(vkCreateQueryPool);
  LAZY_FUNCTION(vkDestroyQueryPool);
  LAZY_FUNCTION(vkGetQueryPoolResults);
  LAZY_FUNCTION(vkCreateEvent);
  LAZY_FUNCTION(vkDestroyEvent);
  LAZY_FUNCTION(vkGetEventStatus);
  LAZY_FUNCTION(vkSetEvent);
  LAZY_FUNCTION(vkResetEvent);
#undef LAZY_FUNCTION
};

}  // namespace vulkan

#endif  // VULKAN_WRAPPER_FUNCTION_TABLE_H_
