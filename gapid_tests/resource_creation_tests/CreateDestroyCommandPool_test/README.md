# vkCreateCommandPool / vkDestroyCommandPool

## Signatures
```c++
VkResult vkCreateCommandPool(
    VkDevice                                    device,
    const VkCommandPoolCreateInfo*              pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkCommandPool*                              pCommandPool);

void vkDestroyCommandPool(
    VkDevice                                    device,
    VkCommandPool                               commandPool,
    const VkAllocationCallbacks*                pAllocator);
```

# VkCommandPoolCreateInfo
```c++
typedef struct VkCommandPoolCreateInfo {
    VkStructureType             sType;
    const void*                 pNext;
    VkCommandPoolCreateFlags    flags;
    uint32_t                    queueFamilyIndex;
} VkCommandPoolCreateInfo;
```

These tests should test the following cases:
- [x] VK\_COMMAND\_POOL\_CREATE\_TRANSIENT\_BIT
- [x] VK\_COMMAND\_POOL\_CREATE\_RESET\_COMMAND\_BUFFER\_BIT
- [x] VK\_COMMAND\_POOL\_CREATE\_TRANSIENT\_BIT | VK\_COMMAND\_POOL\_CREATE\_RESET\_COMMAND\_BUFFER\_BIT
- [x] No bits set
- [ ] Varying QueueFamilyIndices