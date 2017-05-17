# vkCreatePipelineCache / vkDestroyPipelineCache

## Signatures
```c++
VkResult vkCreatePipelineCache(
    VkDevice                                    device,
    const VkPipelineCacheCreateInfo*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipelineCache*                            pPipelineCache);
void vkDestroyPipelineCache(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    const VkAllocationCallbacks*                pAllocator);
```

## VkPipelineCacheCreateInfo
```c++
typedef struct VkPipelineCacheCreateInfo {
    VkStructureType               sType;
    const void*                   pNext;
    VkPipelineCacheCreateFlags    flags;
    size_t                        initialDataSize;
    const void*                   pInitialData;
} VkPipelineCacheCreateInfo;
```
According to the Vulkan spec:
- `device` **must** be a valid VkDevice
- `pCreateInfo` **must** point to a vliad VkPipelineCacheCreateInfo
- `sType` **must** be `VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO`
- `pNext` **must** be nullptr
- `flags` **must** be 0
- `initialDataSize` **may** be 0 or >0
- `pInitialData` is a pointer to `initialDataSize` bytes.


These tests should test the following cases:
- [x] `initialDataSize` == 0
- [ ] `initialDataSize` > 0
