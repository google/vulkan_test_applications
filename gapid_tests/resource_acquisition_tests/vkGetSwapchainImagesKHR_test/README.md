# vkGetDeviceQueue

## Signatures
```c++
VkResult vkGetSwapchainImagesKHR(
    VkDevice                                    device,
    VkSwapchainKHR                              swapchain,
    uint32_t*                                   pSwapchainImageCount,
    VkImage*                                    pSwapchainImages);
```

According to the Vulkan spec:
- `device` **must** be a valid vkDevice
- `swapchain` **must** a valid VkSwapchainKHR
- `pSwapchainImages` **may** be null
- `pSwapchainImageCount` **must** be a valid pointer

These tests should test the following cases:
- [x] `pSwapchainImageCount` == 0
- [x] `pSwapchainImageCount` > max returned
- [x]  0 < `pSwapchainImageCount`< max returned
