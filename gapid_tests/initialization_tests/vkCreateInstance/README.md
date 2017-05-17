# vkCreateInstance / vkDestroyInstance

## Signatures
```c++
VkResult vkCreateInstance(
    const VkInstanceCreateInfo*                 pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkInstance*                                 pInstance);

void vkDestroyInstance(
    VkInstance                                  instance,
    const VkAllocationCallbacks*                pAllocator);
```

## VkInstanceCreateInfo
```c++
typedef struct VkInstanceCreateInfo {
    VkStructureType             sType;
    const void*                 pNext;
    VkInstanceCreateFlags       flags;
    const VkApplicationInfo*    pApplicationInfo;
    uint32_t                    enabledLayerCount;
    const char* const*          ppEnabledLayerNames;
    uint32_t                    enabledExtensionCount;
    const char* const*          ppEnabledExtensionNames;
} VkInstanceCreateInfo;
```

## VkApplicationInfo
```c++
typedef struct VkApplicationInfo {
    VkStructureType    sType;
    const void*        pNext;
    const char*        pApplicationName;
    uint32_t           applicationVersion;
    const char*        pEngineName;
    uint32_t           engineVersion;
    uint32_t           apiVersion;
} VkApplicationInfo;
```

These tests should test the following cases:
- [x] `pApplicationInfo` Null or not
- [ ] `enabledLayerCount` of 0 or not
- [ ] `enabledExtensionCount` of 0 or not
- [ ] `enabledLayerCount` of 0 with non-null ppEnabledLayerNames
- [ ] `enabledExtensionCount` of 0 with non-null ppEnabledLayerNames
- [ ] `pAllocator` set or not for vkCreateInstance
- [ ] `pAllocator` set or not set for vkDestroyInstance
- [ ] `pApplicationName` normal, UTF-8 or empty
- [ ] `pEngineName` normal, UTF-8 or empty
- [ ] `apiVersion` 0 or valid number