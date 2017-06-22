# vkCmdWriteTimestamp

## Signatures
```c++
void vkCmdWriteTimestamp(
    VkCommandBuffer                             commandBuffer,
    VkPipelineStageFlagBits                     pipelineStage,
    VkQueryPool                                 queryPool,
    uint32_t                                    query);
```

According to the Vulkan spec:
- The number of valid bits in a timestamp value is determined by the
  `VkQueueFamilyProperties::timestampValidBits` property of the queue on which
  the timestamp is written. Timestamps are supported on any queue which reports
  a non-zero value for `timestampValidBits` via
  `vkGetPhysicalDeviceQueueFamilyProperties`
- If the `timestampComputeAndGraphics` limit is `VK_TRUE`, timestamps are
  supported by every queue family that supports either graphics or compute
  operations
- The nubmer of nanoseconds it takes for a timestamp value to be incremented
  by 1 can be obtained from `VkPhysicalDeviceLimits::timestampPeriod` after a
  call to `vkGetPhysicalDeviceProperties`
- The query identified by `queryPool` and `query` **must** be *unavailable*
- The command pool's queue family **must** support a non-zero
  `timestampValidBits`

These tests should test the following cases:
- [x] `pipelineStage` of valid values
- [x] `queryPool` of valid values
- [x] `query` of valid values
