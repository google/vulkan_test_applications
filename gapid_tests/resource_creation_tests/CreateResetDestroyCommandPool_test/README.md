# vkCreateCommandPool / vkDestroyCommandPool

## Signatures
```c++
VkResult vkCreateCommandPool(
    VkDevice                                    device,
    const VkCommandPoolCreateInfo*              pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkCommandPool*                              pCommandPool);

VkResult vkResetCommandPool(
    VkDevice                                    device,
    VkCommandPool                               commandPool,
    VkCommandPoolResetFlags                     flags);

void vkDestroyCommandPool(
    VkDevice                                    device,
    VkCommandPool                               commandPool,
    const VkAllocationCallbacks*                pAllocator);
```

# VkCommandPoolCreateInfo
```c++
typedef struct VkCommandPoolCreateInfo {
    VkStructureType             sType;
    const void*                 pNext;
    VkCommandPoolCreateFlags    flags;
    uint32_t                    queueFamilyIndex;
} VkCommandPoolCreateInfo;
```
According to the Vulkan spec:
- `vkResetCommandPool`
  - All `VkCommandBuffer` objects allocated from `commandPool` **must** not be
  in the *pending state*

These tests should test the following cases:
- vkCreateCommandPool
  - [x] VK\_COMMAND\_POOL\_CREATE\_TRANSIENT\_BIT
  - [x] VK\_COMMAND\_POOL\_CREATE\_RESET\_COMMAND\_BUFFER\_BIT
  - [x] VK\_COMMAND\_POOL\_CREATE\_TRANSIENT\_BIT | VK\_COMMAND\_POOL\_CREATE\_RESET\_COMMAND\_BUFFER\_BIT
  - [x] No bits set
  - [ ] Varying QueueFamilyIndices
- vkResetCommandPool
  - [x] `flags` of value 0
  - [x] `flags` of value `VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT`
