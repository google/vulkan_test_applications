# vkCmdBindIndexBuffer

## Signatures
```c++
void vkCmdBindIndexBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    VkIndexType                                 indexType);
```

According to the Vulkan spec:
- `indexType` **must** be `VK_INDEX_TYPE_UINT16` or `VK_INDEX_TYPE_UINT32`

These tests should test the following cases:
- [x] `indexType` == `VK_INDEX_TYPE_UINT16`
- [x] `indexType` == ``VK_INDEX_TYPE_UINT32`
- [x] `offset` == 0
- [x] `offset` != 0
