# vkCmdFillBuffer

## Signatures
```c++
void vkCmdFillBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                size,
    uint32_t                                    data);
```

According to the Vulkan spec:
- `vkCmdFillBuffer` is treated as "transfer" operation for the purposes of
  synchronization barriers. The `VK_BUFFER_USAGE_TRANSFER_DST_BIT` **must** be
  specified in `usage` of `VkBufferCreateInfo` in order for the buffer to be
  compatible with `vkCmdFillBuffer`
- `vkCmdFillBuffer` **must** only be called outside of a render pass instance
- `dstOffset` **must** be less than the size of `dstBuffer`
- `dstOffset` **must** be a multiple of 4
- `size` is the number of bytes to fill
- If `size` is not equal to `VK_WHOLE_SIZE`, `size` **must** be greater than 0
  , **must** be less than or equal to the size of `dstBuffer` minus
  `dstOffset`, and **must** be a multiple of 4
- `dstBuffer` **must** have been created with `VK_BUFFER_USAGE_TRANSFER_DST_BIT`
  usage flag
- If `dstBuffer` is non-sparse then it **must** be bound completely and
  contiguously to a single `VkDeviceMemory` object

These tests should test the following cases:
- [x] `dstOffset` of value 0
- [x] `dstOffset` of value other than 0
- [x] `size` of value VK_WHOLE_SIZE
- [x] `size` of value other than VK_WHOLE_SIZE
