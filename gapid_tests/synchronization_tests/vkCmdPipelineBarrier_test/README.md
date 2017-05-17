# vkCmdPipelineBarrier

## Signatures
```c++
void vkCmdPipelineBarrier(
    VkCommandBuffer                             commandBuffer,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        dstStageMask,
    VkDependencyFlags                           dependencyFlags,
    uint32_t                                    memoryBarrierCount,
    const VkMemoryBarrier*                      pMemoryBarriers,
    uint32_t                                    bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
    uint32_t                                    imageMemoryBarrierCount,
    const VkImageMemoryBarrier*                 pImageMemoryBarriers);
```

# VkMemoryBarrier
```c++
typedef struct VkMemoryBarrier {
    VkStructureType    sType;
    const void*        pNext;
    VkAccessFlags      srcAccessMask;
    VkAccessFlags      dstAccessMask;
} VkMemoryBarrier;
```

# VkBufferMemoryBarrier
```c++
typedef struct VkBufferMemoryBarrier {
    VkStructureType    sType;
    const void*        pNext;
    VkAccessFlags      srcAccessMask;
    VkAccessFlags      dstAccessMask;
    uint32_t           srcQueueFamilyIndex;
    uint32_t           dstQueueFamilyIndex;
    VkBuffer           buffer;
    VkDeviceSize       offset;
    VkDeviceSize       size;
} VkBufferMemoryBarrier;
```
# VkImageMemoryBarrier
```c++
typedef struct VkImageMemoryBarrier {
    VkStructureType            sType;
    const void*                pNext;
    VkAccessFlags              srcAccessMask;
    VkAccessFlags              dstAccessMask;
    VkImageLayout              oldLayout;
    VkImageLayout              newLayout;
    uint32_t                   srcQueueFamilyIndex;
    uint32_t                   dstQueueFamilyIndex;
    VkImage                    image;
    VkImageSubresourceRange    subresourceRange;
} VkImageMemoryBarrier;
```

According to the Vulkan spec:
- `commandBuffer` **must** be valid
- `srcStageMask` **must** be non-zero and a combination of
VkPipelineStageFlagBits
- `dstStageMask` **must** be non-zero and a combination of
VkPipelineStageFlagBits
- `dependencyFlags` **must** be a combination of VkDependencyFlagBits
- `memoryBarrierCount` **can** be 0 OR `pMemoryBarriers` points to an array of
`memoryBarrierCount` `VkMemoryBarrier` structures.
- `bufferMemoryBarrierCount` **can** be 0 OR `pBufferMemoryBarriers` points to
an array of `bufferMemoryBarrierCount` `VkBufferMemoryBarrier` structures.
- `imageMemoryBarrierCount` **can** be 0 OR `pImageMemoryBarriers` points to
an array of `imageMemoryBarrierCount` `VkImageMemoryBarrier` structures


These tests should test the following cases:
- [x] `srcStageMask` 1 or > 1 bits set
- [x] `dstStageMask` 1 or > 1 bits set
- [x] `dependencyFlags` ==  `VK_DEPENDENCY_BY_REGION_BIT` or 0
- [ ] `memoryBarrierCount` 0 or > 0
- [ ] `bufferMemoryBarrierCount` 0 or > 0
- [x] `imageMemoryBarrierCount` 0 or > 0
- [ ] `srcAccessMask` / `dstAccessMask` 1 or >1 bits set