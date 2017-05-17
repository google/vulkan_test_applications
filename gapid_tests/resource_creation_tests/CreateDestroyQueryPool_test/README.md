# vkCreateQueryPool / vkDestroyQueryPool

## Signatures
```c++
VkResult vkCreateQueryPool(
    VkDevice                                    device,
    const VkQueryPoolCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkQueryPool*                                pQueryPool);

void vkDestroyQueryPool(
    VkDevice                                    device,
    VkQueryPool                                 queryPool,
    const VkAllocationCallbacks*                pAllocator);
```

# VkQueryPoolCreateInfo
```c++
typedef struct VkQueryPoolCreateInfo {
    VkStructureType                  sType;
    const void*                      pNext;
    VkQueryPoolCreateFlags           flags;
    VkQueryType                      queryType;
    uint32_t                         queryCount;
    VkQueryPipelineStatisticFlags    pipelineStatistics;
} VkQueryPoolCreateInfo;
```

# VkQueryType
```c++
typedef enum VkQueryType {
    VK_QUERY_TYPE_OCCLUSION = 0,
    VK_QUERY_TYPE_PIPELINE_STATISTICS = 1,
    VK_QUERY_TYPE_TIMESTAMP = 2,
} VkQueryType;
```

According to the Vulkan spec:
- `sType` **must** be `VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO`
- `pNext` **must** be `NULL`
- `flags` **must** be 0
- If the pipeline statistics queries feature is not enabled, `queryType`
  **must not** be `VK_QUERY_TYPE_PIPELINE_STATISTICS`
- If `queryType` is `VK_QUERY_TYPE_PIPELINE_STATISTICS`, `pipelineStatistics`
  **must** be a valid combination of `VkQueryPipelineStatisticFlagBits` values

These tests should test the following cases:
- [x] `queryCount` of value 1
- [x] `queryCount` of value other than 1
- [x] `queryType` of value `VK_QUERY_TYPE_OCCLUSION`
- [x] `queryType` of value `VK_QUERY_TYPE_PIPELINE_STATISTICS`, with
  `pipelineStatistics` of value
  `VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT`
- [x] `queryType` of value `VK_QUERY_TYPE_TIMESTAMP`
