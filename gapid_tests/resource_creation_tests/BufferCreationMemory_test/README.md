# vkCreateBuffer / vkGetBufferMemoryRequirements / vkDestroyBuffer

## Signatures
```c++
VkResult vkCreateBuffer(
    VkDevice                                    device,
    const VkBufferCreateInfo*                   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkBuffer*                                   pBuffer);

void vkDestroyBuffer(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    const VkAllocationCallbacks*                pAllocator);

void vkGetBufferMemoryRequirements(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    VkMemoryRequirements*                       pMemoryRequirements);
```

# VkBufferCreateInfo
```c++
typedef struct VkBufferCreateInfo {
    VkStructureType        sType;
    const void*            pNext;
    VkBufferCreateFlags    flags;
    VkDeviceSize           size;
    VkBufferUsageFlags     usage;
    VkSharingMode          sharingMode;
    uint32_t               queueFamilyIndexCount;
    const uint32_t*        pQueueFamilyIndices;
} VkBufferCreateInfo;
```
# VkMemoryRequirements
```c++
typedef struct VkMemoryRequirements {
    VkDeviceSize    size;
    VkDeviceSize    alignment;
    uint32_t        memoryTypeBits;
} VkMemoryRequirements;
```

According to the Vulkan spec:
- `sType` **must** be `VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO`
- `pNext` **must** be `nullptr`
- `flags` is a bitmask it **may** be 0
- `size` **must** be > 0
- `usage` is a bitmask and **must** have at least one set
- if `sharingMode` is `VK_SHARING_MODE_EXCLUSIVE` pQueueFamilyIndices must be 0
- if `sharingMode` is `VK_SHARING_MODE_CONCURRENT` pQueueFamilyIndices must be > 1

These tests should test the following cases:
- [x] `flags` == 0
- [ ] `flags` != 0 #note: This requires
- [x] `usage` == 1 bit
- [ ] `usage` > 1 bit
- [x] `sharingMode` == `VK_SHARING_MODE_EXCLUSIVE`
- [ ] `sharingMode` == `VK_SHARING_MODE_CONCURRENT`
