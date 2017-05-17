# vkAllocateCommandBuffer / vkResetCommandBuffer / vkFreeCommandBuffer

## Signatures
```c++
VkResult vkAllocateCommandBuffers(
    VkDevice                                    device,
    const VkCommandBufferAllocateInfo*          pAllocateInfo,
    VkCommandBuffer*                            pCommandBuffers);

VkResult vkResetCommandBuffer(
    VkCommandBuffer                             commandBuffer,
    VkCommandBufferResetFlags                   flags);

void vkFreeCommandBuffers(
    VkDevice                                    device,
    VkCommandPool                               commandPool,
    uint32_t                                    commandBufferCount,
    const VkCommandBuffer*                      pCommandBuffers);
```

# VkCommandBufferAllocateInfo
```c++
typedef struct VkCommandBufferAllocateInfo {
    VkStructureType         sType;
    const void*             pNext;
    VkCommandPool           commandPool;
    VkCommandBufferLevel    level;
    uint32_t                commandBufferCount;
} VkCommandBufferAllocateInfo;
```

These tests should test the following cases:
- [x] all possible `VkCommandBufferLevel` values
- [x] all possible `VkCommandBufferResetFlags` values
- [x] `commandBufferCount` == 0, == 1, > 1
  - [x] `vkAllocateCommandBuffers`
  - [x] `vkFreeCommandBuffers`
- [x] combinations of the above
