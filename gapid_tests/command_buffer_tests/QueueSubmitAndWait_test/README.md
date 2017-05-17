# vkQueueSubmit

## Signatures
```c++
VkResult vkQueueSubmit(
    VkQueue                                     queue,
    uint32_t                                    submitCount,
    const VkSubmitInfo*                         pSubmits,
    VkFence                                     fence);

VkResult vkQueueWaitIdle(
    VkQueue                                     queue);
```

# VkSubmitInfo
```c++
typedef struct VkSubmitInfo {
    VkStructureType                sType;
    const void*                    pNext;
    uint32_t                       waitSemaphoreCount;
    const VkSemaphore*             pWaitSemaphores;
    const VkPipelineStageFlags*    pWaitDstStageMask;
    uint32_t                       commandBufferCount;
    const VkCommandBuffer*         pCommandBuffers;
    uint32_t                       signalSemaphoreCount;
    const VkSemaphore*             pSignalSemaphores;
} VkSubmitInfo;
```

According to the Vulkan spec:
- `pSubmits` points to `submitCount` `VkSubmitInfo` structures
- `pFence` can be `VK_NULL_HANDLE` or a valid handle
- `waitSemaphoreCount` can be 0 or > 0
- `pWaitDstStageMask` is a pointer to `pWaitDstStageMask` `VkPipelineStageFlags`
- `pCommandBuffers` is a pointer to `commandBufferCount` `VkCommandBuffer`s
- `pSignalSemaphores` is a pointer to `signalSemaphoreCount` `VkSemaphore`s

These tests should test the following cases:
- [x] `submitCount` == 0
- [x] `submitCount` > 0
- [x] `fence` == `VK_NULL_HANDLE`
- [ ] `fence` != `VK_NULL_HANDLE`
- [x] `waitSemaphoreCount` == 0
- [ ] `waitSemaphoreCount` == 1
- [ ] `waitSemaphoreCount` > 1
- [x] `commandBufferCount` == 0
- [x] `commandBufferCount` == 1
- [x] `commandBufferCount` > 1
- [x] `signalSemaphoreCount` == 0
- [ ] `signalSemaphoreCount` == 1
- [ ] `signalSemaphoreCount` > 1