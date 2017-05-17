# vkCreateAndroidSurfaceKHR / vkDestroySurfaceKHR

## Signatures
```c++
VkResult vkCreateAndroidSurfaceKHR(
    VkInstance                                  instance,
    const VkAndroidSurfaceCreateInfoKHR*        pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface);

void vkDestroySurfaceKHR(
    VkInstance                                  instance,
    VkSurfaceKHR                                surface,
    const VkAllocationCallbacks*                pAllocator);
```

# VkAndroidSurfaceCreateInfoKHR
```c++
typedef struct VkAndroidSurfaceCreateInfoKHR {
    VkStructureType                   sType;
    const void*                       pNext;
    VkAndroidSurfaceCreateFlagsKHR    flags;
    ANativeWindow*                    window;
} VkAndroidSurfaceCreateInfoKHR;
```


According to the Vulkan spec:
- `pCreateInfo` **must** be a pointer to a valid `VkAndroidSurfaceCreateInfoKHR`
structure
- `flags` **must** be 0
- `pNext` **must** be 0
- `window` **must** be a pointer to the `ANativeWindow`


These tests should test the following cases:
- [x] Valid `vkCreateAndroidSurfaceKHR`
- [x] `vkDestroySurfaceKHR` with `VK_NULL_HANDLE`