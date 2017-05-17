# vkCreatePipelineLayout / vkDestroyPipelineLayout

## Signatures
```c++
VkResult vkCreatePipelineLayout(
    VkDevice                                    device,
    const VkPipelineLayoutCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkPipelineLayout*                           pPipelineLayout);

void vkDestroyPipelineLayout(
    VkDevice                                    device,
    VkPipelineLayout                            pipelineLayout,
    const VkAllocationCallbacks*                pAllocator);
```

## VkPipelineLayoutCreateInfo
```c++
typedef struct VkPipelineLayoutCreateInfo {
    VkStructureType                 sType;
    const void*                     pNext;
    VkPipelineLayoutCreateFlags     flags;
    uint32_t                        setLayoutCount;
    const VkDescriptorSetLayout*    pSetLayouts;
    uint32_t                        pushConstantRangeCount;
    const VkPushConstantRange*      pPushConstantRanges;
} VkPipelineLayoutCreateInfo;
```

According to the Vulkan spec:
- `device` **must** be vald
- `pCreateInfo` **must** point to a valid `VkPipelineLayoutCreateInfo` structure.
- `sType` **must** be `VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO`
- `pNext` **must** be null
- `flags` **must** be 0
-  `pSetLayouts` is a pointer to an array of `setLayoutCount`
        `VkDescriptorSetLayout` structures.
- `pPushConstantRanges` is a pointer to an array of `pushConstantRangeCount`
        `VkPushConstantRange` structures.


These tests should test the following cases:
- `vkCreatePipelineLayout`
 - [x] `pSetLayouts` == 0
 - [x] `pSetLayouts` == 1
 - [x] `pSetLayouts` > 1
 - [x] `pPushConstantRanges` == 0
 - [ ] `pPushConstantRanges` == 1
 - [ ] `pPushConstantRanges` > 1
- `vkDestroyPipelineLayout`
 - [x] `pipelineLayout` == `VK_NULL_HANDLE`
 - [x] `pipelineLayout` != `VK_NULL_HANDE`
