# vkCmdBindVertexBuffers / vkCmdCopyBuffer

## Signatures
```c++
void vkCmdBindVertexBuffers(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    firstBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets);

void vkCmdCopyBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    srcBuffer,
    VkBuffer                                    dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferCopy*                         pRegions);
```

# VkBufferCopy
```c++
typedef struct VkBufferCopy {
    VkDeviceSize    srcOffset;
    VkDeviceSize    dstOffset;
    VkDeviceSize    size;
} VkBufferCopy;
```

According to the Vulkan spec:
- `pBuffers` **must** point to `bindingCount` `VkBuffer` structures
- `pOffsets` **must** point to `bindingCount` `VkDeviceSize` values
- `bindingCount` **must** be > 0
- `firstBinding` **must** be < `VkPhysicalDeviceLimits::maxVertexInputBindings`
- `firstBinding` + `bindingCount` **must** be <=
    `VkPhysicalDeviceLimits::maxVertexInputBindings`
- `pRegions` **must** point to `regionCount` `VkBufferCopy` structs
- `VkBufferCopy.size` **must** be > 0

These tests should test the following cases:
- [x] `bindingCount` == 1
- [ ] `bindingCount` > 1
- [x] `firstBinding` == 0
- [ ] `firstBinding` > 0
- [ ] `regionCount` == 0
- [x] `regionCount` == 1
- [ ] `regionCount` > 1
- [x] `srcOffset` == 0
- [ ] `srcOffset`> 0
- [x] `dstOffset` == 0
- [ ] `dstOffset`> 0
