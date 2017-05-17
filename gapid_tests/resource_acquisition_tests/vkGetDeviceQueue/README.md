# vkGetDeviceQueue

## Signatures
```c++
void vkGetDeviceQueue(
    VkDevice                                    device,
    uint32_t                                    queueFamilyIndex,
    uint32_t                                    queueIndex,
    VkQueue*                                    pQueue);
```

According to the Vulkan spec:
- `queueFamilyIndex` **must** be one of the queue family indices specified when
  device was created, via the `VkDeviceQueueCreateInfo` structure
- `queueIndex` **must** be less than the number of queues created for the
  specified queue family index when device was created, via the `queueCount`
  member of the `VkDeviceQueueCreateInfo` structure

These tests should test the following cases:
- [ ] `queueFamilyIndex` == 0
- [ ] `queueFamilyIndex` > 0
- [x] `queueIndex` == 0
- [ ] `queueIndex` == `queueCount` - 1
