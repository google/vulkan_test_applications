# vkCmdUpdateBuffer

## Signatures
```c++
void vkCmdUpdateBuffer(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                dataSize,
    const void*                                 pData);
```

According to the Vulkan spec:
- `vkCmdUpdateBuffer` is only allowed outside of a render pass. This command is
  treated as "transfer" operation, for the purposes of synchronization barriers.
- The `VK_BUFFER_USAGE_TRANSFER_DST_BIT` **must** be specified in the `usage`
  of `VkBufferCreateInfo` in order for the buffer to be compatible with this
  command
- `dstOffset` **must** be less than the size of `dstBuffer`
- `dataSize` **must** be less than or equal to the size of `dstBuffer-dstOffset`
- `dstBuffer` **must** have been created with `VK_BUFFER_USAGE_TRANSFER_DST_BIT`
  usage flag
- If `dstBuffer` is non-sparse then it **must** be bound completely and
  contiguously to a single `VkDeviceMemory` object
- `dstOffset` **must** be a multiple of 4
- `dataSize` **must** be less than or equal to 65536
- `dstaSize` **must** be a multiple of 4
- `pData` **must** be a pointer to an array of `dataSize` bytes
- The `VkCommandPool` that `commandBuffer` was allocated from **must** support
- transfer, graphics or compute operations
- `dataSize` **must** be greater than 0
- Both of `commandBuffer`, and `dstBuffer` **must** have been created,
  allocated, or retrieved from the same `VkDevice`


These tests should test the following cases:
- [x] `dstOffset` of value 0
- [ ] `dstOffset` of value other than 0
- [x] `dataSize` of value 65536
- [ ] `dataSize` of value other than 65536
