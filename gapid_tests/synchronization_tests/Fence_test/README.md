# vkCmdPipelineBarrier

## Signatures
```c++
VkResult vkCreateFence(
    VkDevice                                    device,
    const VkFenceCreateInfo*                    pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkFence*                                    pFence);
void vkDestroyFence(
    VkDevice                                    device,
    VkFence                                     fence,
    const VkAllocationCallbacks*                pAllocator);
VkResult vkWaitForFences(
    VkDevice                                    device,
    uint32_t                                    fenceCount,
    const VkFence*                              pFences,
    VkBool32                                    waitAll,
    uint64_t                                    timeout);
VkResult vkResetFences(
    VkDevice                                    device,
    uint32_t                                    fenceCount,
    const VkFence*                              pFences);
```

# VkFenceCreateInfo
```c++
typedef struct VkFenceCreateInfo {
    VkStructureType       sType;
    const void*           pNext;
    VkFenceCreateFlags    flags;
} VkFenceCreateInfo;

```

According to the Vulkan spec:
- `device` **must** be valid
- `pCreateInfo` **must** be valid
- `flags` **may** be 0 or `VK_FENCE_CREATE_SIGNALED_BIT`
- `fenceCount` **must** be > 0
- `pFences` **must** point to `fenceCount` `VkFence` objects

These tests should test the following cases:
- [x] `flags` == 0
- [ ] `flags` == `VK_FENCE_CREATE_SIGNALED_BIT`
- [x] `fenceCount` == 1
- [ ] `fenceCount` > 1
- [x] `VkDestroyFence.fence` == Valid Fence
- [x] `VkDestroyFence.fence` == `VK_NULL_HANDLE`
- [x] `vkResetFence.fenceCount == 1`
- [ ] `vkResetFence.fenceCount > 1`