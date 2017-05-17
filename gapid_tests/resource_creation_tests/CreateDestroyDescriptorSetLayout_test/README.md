# vkCreateDescriptorSetLayout / vkDestroyDescriptorSetLayout

## Signatures
```c++
VkResult vkCreateDescriptorSetLayout(
    VkDevice                                    device,
    const VkDescriptorSetLayoutCreateInfo*      pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDescriptorSetLayout*                      pSetLayout);

void vkDestroyDescriptorSetLayout(
    VkDevice                                    device,
    VkDescriptorSetLayout                       descriptorSetLayout,
    const VkAllocationCallbacks*                pAllocator);
```

# VkDescriptorSetLayoutCreateInfo
```c++
typedef struct VkDescriptorSetLayoutCreateInfo {
    VkStructureType                        sType;
    const void*                            pNext;
    VkDescriptorSetLayoutCreateFlags       flags;
    uint32_t                               bindingCount;
    const VkDescriptorSetLayoutBinding*    pBindings;
} VkDescriptorSetLayoutCreateInfo;

typedef struct VkDescriptorSetLayoutBinding {
    uint32_t              binding;
    VkDescriptorType      descriptorType;
    uint32_t              descriptorCount;
    VkShaderStageFlags    stageFlags;
    const VkSampler*      pImmutableSamplers;
} VkDescriptorSetLayoutBinding;
```

According to the Vulkan spec:
- `flags` **must** be 0
- If `bindingCount` is not 0, `pBindings` **must** be a pointer to an array of
  `bindingCount` valid `VkDescriptorSetLayoutBinding` structures
- If `descriptorType` is `VK_DESCRIPTOR_TYPE_SAMPLER` or
  `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`, and `descriptorCount` is not 0
  and `pImmutableSamplers` is not `NULL`, `pImmutableSamplers` **must** be a
  pointer to an array of `descriptorCount` valid `VkSampler` handles
- If `descriptorCount` is not 0, `stageFlags` **must** be a valid combination of
  `VkShaderStageFlagBits` values

For `vkCreateDescriptorSetLayout`, these tests should test the following cases:
- `bindingCount` of valid values:
  - [x] 0
  - [x] 2
  - [x] 3
- `pBindings` of valid values:
  - [x] == `nullptr`
  - [x] != `nullptr`
- `bounding` of valid values:
  - [x] 0
  - [x] 2
  - [x] 5
  - [x] 7
- `descriptorType` of valid `VkDescriptorType` values:
  - [x] `VK_DESCRIPTOR_TYPE_SAMPLER`
  - [x] `VK_DESCRIPTOR_TYPE_STORAGE_IMAGE`
  - [x] `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
  - [x] `VK_DESCRIPTOR_TYPE_STORAGE_BUFFER`
- `descriptorCount` of valid values:
  - [x] 0
  - [x] 1
  - [x] 3
  - [x] 6
- `stageFlags` of values:
  - [x] `VK_SHADER_STAGE_VERTEX_BIT`
  - [x] `VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT`
  - [x] `VK_SHADER_STAGE_ALL`
  - [x] `0xdeadbeef`
- `pImmutableSamplers` of valid values:
  - [x] == `nullptr`
  - [x] != `nullptr`

For `vkDestroyDescriptorSetLayout`, these tests should test the following cases:
- [x] `descriptorSetLayout` == `nullptr`
- [x] `descriptorSetLayout` != `nullptr`
