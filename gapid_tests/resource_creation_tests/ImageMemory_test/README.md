# vkGetImageMemoryRequirements vkAllocateMemory vkFreeMemory

## Signatures
```c++
void vkGetImageMemoryRequirements(
    VkDevice                                    device,
    VkImage                                     image,
    VkMemoryRequirements*                       pMemoryRequirements);
VkResult vkAllocateMemory(
    VkDevice                                    device,
    const VkMemoryAllocateInfo*                 pAllocateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDeviceMemory*                             pMemory);
void vkFreeMemory(
    VkDevice                                    device,
    VkDeviceMemory                              memory,
    const VkAllocationCallbacks*                pAllocator);
VkResult vkBindImageMemory(
    VkDevice                                    device,
    VkImage                                     image,
    VkDeviceMemory                              memory,
    VkDeviceSize                                memoryOffset);
```

# VkMemoryRequirements
```c++
typedef struct VkMemoryRequirements {
    VkDeviceSize    size;
    VkDeviceSize    alignment;
    uint32_t        memoryTypeBits;
} VkMemoryRequirements;
typedef struct VkMemoryAllocateInfo {
    VkStructureType    sType;
    const void*        pNext;
    VkDeviceSize       allocationSize;
    uint32_t           memoryTypeIndex;
} VkMemoryAllocateInfo;
```

According to the Vulkan spec:
- `device` **must** be a valid device
- `image` **must** have come from device
- `VkMemoryRequirements` **must** be a pointer to a VkMemoryRequirements
structure
- `allocationSize` must be > 0
- `memoryOffset` must be a multiple of `alignment`


These tests should test the following cases:
- [x] Valid usage
- [x] Valid memory allocation
- [x] `memoryOffset` of 0
- [ ] `memoryOffset` > 0
