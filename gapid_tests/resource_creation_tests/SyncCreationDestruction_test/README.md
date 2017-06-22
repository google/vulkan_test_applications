# vkCreateSemaphore / vkDestroySemaphore

## Signatures
```c++
VkResult vkCreateSemaphore(
    VkDevice                                    device,
    const VkSemaphoreCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSemaphore*                                pSemaphore);

void vkDestroySemaphore(
    VkDevice                                    device,
    VkSemaphore                                 semaphore,
    const VkAllocationCallbacks*                pAllocator);
```

# VkSemaphoreCreateInfo
```c++
typedef struct VkSemaphoreCreateInfo {
    VkStructureType           sType;
    const void*               pNext;
    VkSemaphoreCreateFlags    flags;
} VkSemaphoreCreateInfo;
```


According to the Vulkan spec:
- `pCreateInfo` **must** be a pointer to a valid `VkSemaphoreCreateInfo` structure
- `flags` **must** be 0
- `pNext` **must** be 0


These tests should test the following cases:
- [x] Valid vkCreateSemaphore
- [x] vkDestroySemaphore is `VK_NULL_HANDLE`
