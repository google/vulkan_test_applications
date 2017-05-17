# vkCreateGraphicsPipelines / vkDestroyPipeline

## Signatures
```c++
VkResult vkCreateGraphicsPipelines(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkGraphicsPipelineCreateInfo*         pCreateInfos,
    const VkAllocationCallbacks*                pAllocator,
    VkPipeline*                                 pPipelines);

void vkDestroyPipeline(
    VkDevice                                    device,
    VkPipeline                                  pipeline,
    const VkAllocationCallbacks*                pAllocator);
```

## VkGraphicsPipelineCreateInfo
```c++
typedef struct VkGraphicsPipelineCreateInfo {
    VkStructureType                                  sType;
    const void*                                      pNext;
    VkPipelineCreateFlags                            flags;
    uint32_t                                         stageCount;
    const VkPipelineShaderStageCreateInfo*           pStages;
    const VkPipelineVertexInputStateCreateInfo*      pVertexInputState;
    const VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState;
    const VkPipelineTessellationStateCreateInfo*     pTessellationState;
    const VkPipelineViewportStateCreateInfo*         pViewportState;
    const VkPipelineRasterizationStateCreateInfo*    pRasterizationState;
    const VkPipelineMultisampleStateCreateInfo*      pMultisampleState;
    const VkPipelineDepthStencilStateCreateInfo*     pDepthStencilState;
    const VkPipelineColorBlendStateCreateInfo*       pColorBlendState;
    const VkPipelineDynamicStateCreateInfo*          pDynamicState;
    VkPipelineLayout                                 layout;
    VkRenderPass                                     renderPass;
    uint32_t                                         subpass;
    VkPipeline                                       basePipelineHandle;
    int32_t                                          basePipelineIndex;
} VkGraphicsPipelineCreateInfo;
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

## VkSpecializationInfo
```c++
typedef struct VkSpecializationInfo {
    uint32_t                           mapEntryCount;
    const VkSpecializationMapEntry*    pMapEntries;
    size_t                             dataSize;
    const void*                        pData;
} VkSpecializationInfo;
```

## VkSpecializationMapEntry
```c++
typedef struct VkSpecializationMapEntry {
    uint32_t    constantID;
    uint32_t    offset;
    size_t      size;
} VkSpecializationMapEntry;
```

## VkPipelineVertexInputStateCreateInfo
```c++
typedef struct VkPipelineVertexInputStateCreateInfo {
    VkStructureType                             sType;
    const void*                                 pNext;
    VkPipelineVertexInputStateCreateFlags       flags;
    uint32_t                                    vertexBindingDescriptionCount;
    const VkVertexInputBindingDescription*      pVertexBindingDescriptions;
    uint32_t                                    vertexAttributeDescriptionCount;
    const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions;
} VkPipelineVertexInputStateCreateInfo;
```

## VkVertexInputBindingDescription
```c++
typedef struct VkVertexInputBindingDescription {
    uint32_t             binding;
    uint32_t             stride;
    VkVertexInputRate    inputRate;
} VkVertexInputBindingDescription;
```

## VkVertexInputAttributeDescription
```c++
typedef struct VkVertexInputAttributeDescription {
    uint32_t    location;
    uint32_t    binding;
    VkFormat    format;
    uint32_t    offset;
} VkVertexInputAttributeDescription;
```

## VkPipelineInputAssemblyStateCreateInfo
```c++
typedef struct VkPipelineInputAssemblyStateCreateInfo {
    VkStructureType                            sType;
    const void*                                pNext;
    VkPipelineInputAssemblyStateCreateFlags    flags;
    VkPrimitiveTopology                        topology;
    VkBool32                                   primitiveRestartEnable;
} VkPipelineInputAssemblyStateCreateInfo;
```

## VkPipelineTessellationStateCreateInfo
```c++
typedef struct VkPipelineTessellationStateCreateInfo {
    VkStructureType                           sType;
    const void*                               pNext;
    VkPipelineTessellationStateCreateFlags    flags;
    uint32_t                                  patchControlPoints;
} VkPipelineTessellationStateCreateInfo;
```

## VkPipelineViewportStateCreateInfo
```c++
typedef struct VkPipelineViewportStateCreateInfo {
    VkStructureType                       sType;
    const void*                           pNext;
    VkPipelineViewportStateCreateFlags    flags;
    uint32_t                              viewportCount;
    const VkViewport*                     pViewports;
    uint32_t                              scissorCount;
    const VkRect2D*                       pScissors;
} VkPipelineViewportStateCreateInfo;
```

## VkViewport
```c++
typedef struct VkViewport {
    float    x;
    float    y;
    float    width;
    float    height;
    float    minDepth;
    float    maxDepth;
} VkViewport;
```

## VkPipelineRasterizationStateCreateInfo
```c++
typedef struct VkPipelineRasterizationStateCreateInfo {
    VkStructureType                            sType;
    const void*                                pNext;
    VkPipelineRasterizationStateCreateFlags    flags;
    VkBool32                                   depthClampEnable;
    VkBool32                                   rasterizerDiscardEnable;
    VkPolygonMode                              polygonMode;
    VkCullModeFlags                            cullMode;
    VkFrontFace                                frontFace;
    VkBool32                                   depthBiasEnable;
    float                                      depthBiasConstantFactor;
    float                                      depthBiasClamp;
    float                                      depthBiasSlopeFactor;
    float                                      lineWidth;
} VkPipelineRasterizationStateCreateInfo;
```

## VkPipelineMultisampleStateCreateInfo
```c++
typedef struct VkPipelineMultisampleStateCreateInfo {
    VkStructureType                          sType;
    const void*                              pNext;
    VkPipelineMultisampleStateCreateFlags    flags;
    VkSampleCountFlagBits                    rasterizationSamples;
    VkBool32                                 sampleShadingEnable;
    float                                    minSampleShading;
    const VkSampleMask*                      pSampleMask;
    VkBool32                                 alphaToCoverageEnable;
    VkBool32                                 alphaToOneEnable;
} VkPipelineMultisampleStateCreateInfo;
```

## VkPipelineDepthStencilStateCreateInfo
```c++
typedef struct VkPipelineDepthStencilStateCreateInfo {
    VkStructureType                           sType;
    const void*                               pNext;
    VkPipelineDepthStencilStateCreateFlags    flags;
    VkBool32                                  depthTestEnable;
    VkBool32                                  depthWriteEnable;
    VkCompareOp                               depthCompareOp;
    VkBool32                                  depthBoundsTestEnable;
    VkBool32                                  stencilTestEnable;
    VkStencilOpState                          front;
    VkStencilOpState                          back;
    float                                     minDepthBounds;
    float                                     maxDepthBounds;
} VkPipelineDepthStencilStateCreateInfo;
```

## VkPipelineColorBlendStateCreateInfo
```c++
typedef struct VkPipelineColorBlendStateCreateInfo {
    VkStructureType                               sType;
    const void*                                   pNext;
    VkPipelineColorBlendStateCreateFlags          flags;
    VkBool32                                      logicOpEnable;
    VkLogicOp                                     logicOp;
    uint32_t                                      attachmentCount;
    const VkPipelineColorBlendAttachmentState*    pAttachments;
    float                                         blendConstants[4];
} VkPipelineColorBlendStateCreateInfo;
```

## VkPipelineColorBlendAttachmentState
```c++
typedef struct VkPipelineColorBlendAttachmentState {
    VkBool32                 blendEnable;
    VkBlendFactor            srcColorBlendFactor;
    VkBlendFactor            dstColorBlendFactor;
    VkBlendOp                colorBlendOp;
    VkBlendFactor            srcAlphaBlendFactor;
    VkBlendFactor            dstAlphaBlendFactor;
    VkBlendOp                alphaBlendOp;
    VkColorComponentFlags    colorWriteMask;
} VkPipelineColorBlendAttachmentState;
```

## VkPipelineDynamicStateCreateInfo
```c++
typedef struct VkPipelineDynamicStateCreateInfo {
    VkStructureType                      sType;
    const void*                          pNext;
    VkPipelineDynamicStateCreateFlags    flags;
    uint32_t                             dynamicStateCount;
    const VkDynamicState*                pDynamicStates;
} VkPipelineDynamicStateCreateInfo;
```

According to the Vulkan spec:
- `flags` may be non-0
- `stages` may be 0 or > 0
- `pStages` must all be unique
- `pVertexInputState` **must** point to a valid
    `VkPipelineVertexInputStateCreateInfo`
- `pInputAssemblyState` **must** point to a valid
    `VkPipelineInputAssemblyStateCreateInfo`
- `pTessellationState` **may** be nullptr if the pipeline does not contain
    tessellation
- `pRasterizationState` **must** point to a valid
    `VkPipelineRasterizationStateCreateInfo`
- The following **must** be valid if rasterization is enabled, NULL otherwise
  - `pViewportState`
  - `pMultisampleState`
- `pDepthStencilState` **must** be valid if rasterization is enabled and the
    subpass uses depth/stencil. NULL otherwise.
- `pColorBlendState` **must** be valid if rasterization is enabled and the
    subpass uses color attachments, NULL otherwise.
- `pDynamicState` **can** be nullptr
- The other parameters must point to valid objects.
- `VkPipelineShaderStageCreateInfo`
  - `flags` **must** be 0
  - `stage` **must** have one bit set.
  - `pName` **must** be a null-terminated utf-8 string to a name in the given
    shader module
  - `pSpecializationInfo` **may** be null or a pointer to a specialization
    struct
  - `VkSpecializationInfo`
    - `pMapEntries` points to and array of `mapEntryCount`
        `VkSpecializationMapEntry` structures
    - `pData` points to `dataSize` bytes of data.
- `VkPipelineVertexInputStateCreateInfo`
  - `flags` **must** be 0
  - `pVertexBindingDescriptions` points to an array of
      `vertexBindingDescriptionCount` `VkVertexInputBindingDescription` structures
  - `pVertexAttributeDescriptions` points to an array of
     `vertexAttributeDescriptionCount` `VkVertexInputAttributeDescription` structures
    - `VkVertexInputBindingDescription`
      - All values must be valid
    - `VkVertexInputAttributeDescription`
      - All values must be valid
- `VkPipelineInputAssemblyStateCreateInfo`
  - `flags` **must** be 0
- `VkPipelineTessellationStateCreateInfo`
  - `flags` **must** be 0
- `VkPipelineViewportStateCreateInfo`
  - `flags` **must** be 0
  - `pViewports` points to an array of `viewportCount` `VkViewport` structures
  - `pScissors` points to an array of `scissorCount` `VkRect2D` structures
  - `viewportCount` **must** be >= 1 (> 1 only if multiViewport is enabled)
  - `scissorCount` **must** be >= 1 (> 1 only if multiViewport is enabled)
  - `scissorCount` must == `viewportCount`
  - `VkViewport`
    - ... something about fixed point?
    - `width, height` must be between 0 and
        `VkPhysicalDeviceLimits::maxViewportDimensions`
    - `x, y, x+width, y+width` must be in `viewportBoundsRange`
        // TODO(awoloszyn): WHAT IS VIEWPORTBOUNDSRANGE?
- `VkPipelineRasterizationStateCreateInfo`
  - `flags` **must** be 0
  - `depthClamp` must be VK_FALSE unless the `depthClamp` is enabled
  - `polygonMode` must be `VK_POLYGON_MODE_FILL` unless `fillModeNonSolid`
     is enabled
- `VkPipelineMultisampleStateCreateInfo`
  - `flags` **must** be 0
  - `pSampleMask` is either null or a pointer to `ceil(rasterizationSamples/32)`
     `VkSampleMask` structures
  - `sampleShadingEnable` **must** be `VK_FALSE` unless `sampleRateShading` is
    enabled
  - `alphaToOneEnable` **must** be `VK_FALSE` unless `alphaToOne` is enabled
- `VkPipelineDepthStencilStateCreateInfo`
  - `flags` **must** be 0
  - `depthBoundsTestEnable` **must** be 0 unless `depthBounds` is enabled
- `VkPipelineColorBlendStateCreateInfo`
  - `flags` **must** be 0
  - `pAttachments` points to an array of `attachmentCount`
        `VkPipelineColorBlendAttachmentState` structures
  - `VkPipelineColorBlendAttachmentState`
    - all parameters must be valid
    - If `dualSrcBlend` is not enabled then the following cannot be used
      - `VK_BLEND_FACTOR_SRC1_COLOR`
      - `VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR`
      - `VK_BLEND_FACTOR_SRC1_ALPHA`
      - `VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA`
- `VkPipelineDynamicStateCreateInfo`
  - `flags` **must** be 0
  - `pDynamicStates` points to an array of `dynamicStateCount` `VkDynamicState`
    values
  - `dynamicStateCount` **must** be `>0`

## Tests in their own README files
- [simple_vertex_fragment](simple_vertex_fragment.md)