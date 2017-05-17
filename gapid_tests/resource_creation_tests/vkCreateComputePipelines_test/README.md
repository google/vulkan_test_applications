# vkCreateComputePipelines

## Signature

```c++
VkResult vkCreateComputePipelines(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkComputePipelineCreateInfo*          pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines);
```

## VkComputePipelineCreateInfo

```c++
typedef struct VkComputePipelineCreateInfo {
    VkStructureType                    sType;
    const void*                        pNext;
    VkPipelineCreateFlags              flags;
    VkPipelineShaderStageCreateInfo    stage;
    VkPipelineLayout                   layout;
    VkPipeline                         basePipelineHandle;
    int32_t                            basePipelineIndex;
} VkComputePipelineCreateInfo;
```

## VkPipelineShaderStageCreateInfo

```c++
typedef struct VkPipelineShaderStageCreateInfo {
    VkStructureType                     sType;
    const void*                         pNext;
    VkPipelineShaderStageCreateFlags    flags;
    VkShaderStageFlagBits               stage;
    VkShaderModule                      module;
    const char*                         pName;
    const VkSpecializationInfo*         pSpecializationInfo;
} VkPipelineShaderStageCreateInfo;
```

According to the Vulkan spec:
- If `flags` contains the `VK_PIPELINE_CREATE_DERIVATIVE_BIT` flag,
  and `basePipelineIndex` is not -1, `basePipelineHandle` **must** be
  `VK_NULL_HANDLE`
- If `flags` contains the `VK_PIPELINE_CREATE_DERIVATIVE_BIT` flag,
  and `basePipelineIndex` is not -1, it **must** be a valid index into the
  calling command’s `pCreateInfos` parameter
- If `flags` contains the `VK_PIPELINE_CREATE_DERIVATIVE_BIT` flag,
  and `basePipelineHandle` is not `VK_NULL_HANDLE`, `basePipelineIndex`
  **must** be -1
- If `flags` contains the `VK_PIPELINE_CREATE_DERIVATIVE_BIT` flag,
  and `basePipelineHandle` is not `VK_NULL_HANDLE`, `basePipelineHandle`
  **must** be a valid `VkPipeline` handle
- If `flags` contains the `VK_PIPELINE_CREATE_DERIVATIVE_BIT` flag,
  and `basePipelineHandle` is not `VK_NULL_HANDLE`, it **must** be a valid
  handle to a compute `VkPipeline`
- The `stage` member of `stage` **must** be `VK_SHADER_STAGE_COMPUTE_BIT`

## Tests

These tests should test the following cases:
- [x] `createInfoCount` == 1
- [ ] `createInfoCount` > 1
- [x] `flags` == 0
- [ ] `flags` == `VK_PIPELINE_CREATE_DERIVATIVE_BIT`
- [x] `basePipelineHandle` == `VK_NULL_HANDLE`
- [ ] `basePipelineHandle` != `VK_NULL_HANDLE`
- [x] `basePipelineIndex` == 0
- [ ] `basePipelineIndex` != 0
- [x] `pSpecializationInfo` == `nullptr`
- [ ] `pSpecializationInfo` != `nullptr`
