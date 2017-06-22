# vkFlushMappedMemoryRanges / vkInvalidateMappedMemoryRanges

## Signatures
```c++
VkResult vkFlushMappedMemoryRanges(
    VkDevice                                    device,
    uint32_t                                    memoryRangeCount,
    const VkMappedMemoryRange*                  pMemoryRanges);

VkResult vkInvalidateMappedMemoryRanges(
    VkDevice                                    device,
    uint32_t                                    memoryRangeCount,
    const VkMappedMemoryRange*                  pMemoryRanges);
```

# VkMappedMemoryRange
```c++
typedef struct VkMappedMemoryRange {
    VkStructureType    sType;
    const void*        pNext;
    VkDeviceMemory     memory;
    VkDeviceSize       offset;
    VkDeviceSize       size;
} VkMappedMemoryRange;
```

According to the Vulkan spec:
- vkFlushMappedMemoryRanges / vkInvalidateMappedMemoryRanges
  - `device` **must** be a valid `VkDevice` handle
  - `pMemoryRanges` **must** be a pointer to an array of `memoryRangeCount`
  valid `VkMappedMemoryRange` structures
  - `memoryRangeCount` **must** be greater than 0
- VkMappedMemoryRange
  - `pNext` **must** be `NULL`
  - `memory` **must** be a valid `VkDeviceMemory` handle and **must** currently
  be mapped
  - If `size` is not equal to `VK_WHOLE_SIZE`, `offset` and `size` **must**
  specify a range contained within the current mapped range of `memory`
  - If `size` is equal to `VK_WHOLE_SIZE`, `offset` **must** be within the
  currently mapped range of `memory`
  - `offset` **must** be a multiple of
  `VkPhysicalDeviceLimits::nonCoherentAtomSize`
  - If `size` is not equal to `VK_WHOLE_SIZE`, `size` **must** be a multiple
  of `VkPhysicalDeviceLimits::nonCoherentAtomSize`


These tests should test the following cases:
- [x] `size` not equal to `VK_WHOLE_SIZE`
- [x] `size` eqaul to `VK_WHOLE_SIZE`
- [x] `offset` of value non-zero
- [x] `offset` of value 0
- [x] single VkMappedMemoryRange
- [ ] multiple VkMappedMemoryRange's
