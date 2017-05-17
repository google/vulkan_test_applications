# vkCmdResetQueryPool

## Signatures
```c++
void vkCmdResetQueryPool(
    VkCommandBuffer                             commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    firstQuery,
    uint32_t                                    queryCount);
```

According to the Vulkan spec:
- `firstQuery` **must** be less than the number of queries in `queryPool`
- The sum of `firstQuery` and `queryCount` **must** be less than or equal to
  the number of queries in `queryPool`
- `vkCmdResetQueryPool` **must** only be called outside of a render pass
  instance
- The `VkCommandPool` that `commandBuffer` was allocated from **must** support
  graphics, or compute operations

These tests should test the following cases:
- [x] `firstQuery` of value 0
- [x] `firstQuery` of value other than 0
- [x] `queryCount` of value 1
- [x] `queryCount` of value other than 1
