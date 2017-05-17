# vkQueuePresentKHR

## Signatures
```c++
VkResult vkQueuePresentKHR(
    VkQueue                                     queue,
    const VkPresentInfoKHR*                     pPresentInfo);
```

According to the Vulkan spec:
- vkQueuePresentKHR
  - `queue` is a queue that is capable of presentation to the target surface's
  platform on the ssame device as the image's swapchain. It **must** be a valid
  `VkQueue` handle
  - Any given element of `pSwapchains` member of `pPresentInfo` **must** be a
  swapchain that is created for a surface for which presentation is supported
  from `queue` as determined using a call to
  `vkGetPhysicalDeviceSurfaceSupportKHR`
  - Host access to `queue`, `pPresentInfo.pWaitSemaphores[]` and
  - `pPresentInfo.pSwapchains[]` **must** be externally synchronized


## VkPresentInfoKHR
```c++
typedef struct VkPresentInfoKHR {
    VkStructureType          sType;
    const void*              pNext;
    uint32_t                 waitSemaphoreCount;
    const VkSemaphore*       pWaitSemaphores;
    uint32_t                 swapchainCount;
    const VkSwapchainKHR*    pSwapchains;
    const uint32_t*          pImageIndices;
    VkResult*                pResults;
} VkPresentInfoKHR;
```

According to the Vulkan spec:
- VkPresentInfoKHR
  - `sType` **must** be `VK_STRUCTURE_TYPE_PRESENT_INFO_KHR`
  - `pNext` **must** be `NULL` or a pointer to a `VkDisplayPresentInfoKHR`
  - `waitSemaphoreCount` **may** be zero
  - `pWaitSemaphores` if not `NULL`, is an array of `VkSemaphore` objects with
  `waitSemaphoreCount` entries, and specifies the semaphores to wait for before
  issuing the present request
  - `swapchainCount` is the number of swapchains being presented to by this command
  - `pSwapchains` is an array of `VkSwapchainKHR` objects with `swapchainCount`
  entries. A given swapchain **must not** appear in this list more than once
  - `pImageIndices` is an array of indices into the array of each swapchain's
  presentable images, with `swapchainCount` entries. Each entry in this array
  identifies the image to the present on the corresponding entry in the
  `pSwapchains` array.
  - `pResults` is an array of `VkResult` typed elementes with `swapchainCount`
  entries. If the given `pResults` is `non-Null`, each entry in `pResults` will
  be set to the `VkResult` for presenting the swapchain corresponding to the
  same index in `pSwapchains`

These tests should test the following cases:
- [x] None waiting semaphores
- [ ] One waiting semaphores
- [ ] Multiple waiting semaphores
- [x] One swapchain
- [ ] Multiple swapchains
- [x] `pResults` of value `NULL`
- [ ] `pResults` points to a valid result array
