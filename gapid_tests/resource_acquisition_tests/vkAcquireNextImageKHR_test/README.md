# vkAcquireNextImageKHR

## Signatures
```c++
VkResult vkAcquireNextImageKHR(
    VkDevice                                    device,
    VkSwapchainKHR                              swapchain,
    uint64_t                                    timeout,
    VkSemaphore                                 semaphore,
    VkFence                                     fence,
    uint32_t*                                   pImageIndex);
```

According to the Vulkan spec:
- `device` is the device associated with `swapchain`, it **must** be a valid
`VkDevice` handle
- `swapchain` **must** be a valid `VkSwapchainKHR` handle
- If `semaphore` is not `VK_NULL_HANDLE`, `semaphore` **must** be a valid
`VkSemaphore` handle that **must** have been created, allocated, or retrieved
from `device`
- If `fence` is not `VK_NULL_HANDLE`, `fence` **must** be a valid `VkFence`
handle that **must** have been created, allocated, or retrieved from `device`
- `pImageIndex` **must** be a pointer to a `uint32_t` value
- Host access to `swapchain`, `semaphore` and `fence` **must** be externally
synchronized


These tests should test the following cases:
- [ ] non-Null `semaphore` and non-Null `fence`
- [x] non-Null `semaphore` and Null `fence`
- [ ] Null `semaphore` and non-Null `fence`
