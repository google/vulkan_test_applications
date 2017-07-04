# vkCreatePipelineCache / vkDestroyPipelineCache / vkMergePipelineCaches

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
VkResult vkMergePipelineCaches(
    VkDevice                                    device,
    VkPipelineCache                             dstCache,
    uint32_t                                    srcCacheCount,
    const VkPipelineCache*                      pSrcCaches);
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
- `dstCache` **must** NOT appear in the list of source caches (`pSrcCaches`)
- `srcCacheCount` **must** be greater than `0`
- `dstCache` and each element of `pSrcCaches` **must** have been created,
  allocated, or retrieved from `device`


These tests should test the following cases:
- [x] `initialDataSize` == 0
- [ ] `initialDataSize` > 0
- [x] `dstCache` of valid value
- [x] `srcCacheCount` > 0
- [x] `pSrcCaches` of valid value
