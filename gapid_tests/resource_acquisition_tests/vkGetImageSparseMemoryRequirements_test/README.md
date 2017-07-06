# vkGetImageSparseMemoryRequirements

## Signatures
```c++
void vkGetImageSparseMemoryRequirements(
    VkDevice                                    device,
    VkImage                                     image,
    uint32_t*                                   pSparseMemoryRequirementCount,
    VkSparseImageMemoryRequirements*            pSparseMemoryRequirements);
```

# VkSparseImageMemoryRequirements
```c++
typedef struct VkSparseImageMemoryRequirements {
    VkSparseImageFormatProperties    formatProperties;
    uint32_t                         imageMipTailFirstLod;
    VkDeviceSize                     imageMipTailSize;
    VkDeviceSize                     imageMipTailOffset;
    VkDeviceSize                     imageMipTailStride;
} VkSparseImageMemoryRequirements;
```

According to the Vulkan spec:
- `device` **must** be a valid device
- `image` **must** have come from device
- `pSparseMemoryRequirementCount` is a pointer to an integer related to the
  number of sparse memory requirements available or queried, as described below.
- `pSparseMemoryRequirements` is either `NULL` or a pointer to an array of
  `VkSparseImageMemoryRequirements` structures.
- If `pSparseMemoryRequirements` is `NULL`, then the number of sparse memory
  requirements available is returned in `pSparseMemoryRequirementCount`.
  Otherwise, `pSparseMemoryRequirementCount` **must** point to a variable set
  by the user to the number of elements in the `pSparseMemoryRequirements`
  array, and on return the variable is overwritten with the number of
  structures actually written to `pSparseMemoryRequirements`. If
  `pSparseMemoryRequirementCount` is less than the number of sparse memory
  requirements available, at most `pSparseMemoryRequirementCount` structures
  will be written.

These tests should test the following cases:
- [x] `pSparseMemoryRequirements` == `NULL`
- [x] `pSparseMemoryRequirements` != `NULL`
- [x] `pSparseMemoryRequirementCount` points to a value equal to the actual
  number of sparse memory requirements
- [ ] `pSparseMemoryRequirementCount` points to a value less than the actual
  number of sparse memory requirements
- [ ] `pSparseMemoryRequirementCount` points to a value greater than the actual
  number of sparse memory requirements

