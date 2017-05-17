# vkDeviceWaitIdle

## Signatures
```c++
VkResult vkDeviceWaitIdle(
    VkDevice                                    device);
```

According to the Vulkan spec:
- `device` **must** be a valid `VkDevice` handle
- `vkDeviceWaitIdle` is equivalent to calling `vkQueueWaitIdle` for all queues
  owned by `device`.

These tests should test the following cases:
- [x] Wait for queue
