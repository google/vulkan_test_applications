# vkAllocateDescriptorSets / vkFreeDescriptorSets

## Signatures
```c++
VkResult vkAllocateDescriptorSets(
    VkDevice                                    device,
    const VkDescriptorSetAllocateInfo*          pAllocateInfo,
    VkDescriptorSet*                            pDescriptorSets);

VkResult vkFreeDescriptorSets(
    VkDevice                                    device,
    VkDescriptorPool                            descriptorPool,
    uint32_t                                    descriptorSetCount,
    const VkDescriptorSet*                      pDescriptorSets);
```

# VkDescriptorSetAllocateInfo
```c++
typedef struct VkDescriptorSetAllocateInfo {
    VkStructureType                 sType;
    const void*                     pNext;
    VkDescriptorPool                descriptorPool;
    uint32_t                        descriptorSetCount;
    const VkDescriptorSetLayout*    pSetLayouts;
} VkDescriptorSetAllocateInfo;
```

# Valid usage

For `vkAllocateDescriptorSets`, according to the Vulkan spec:
- `pDescriptorSets` **must** be a pointer to an array of `descriptorSetCount`
  `VkDescriptorSet` handles
- `descriptorSetCount` **must** be greater than 0
- `descriptorSetCount` **must** not be greater than the number of sets that
  are currently available for allocation in `descriptorPool`
- `pSetLayouts` **must** be a pointer to an array of `descriptorSetCount`
  valid `VkDescriptorSetLayout` handles

For `vkFreeDescriptorSets`, according to the Vulkan spec:
- `descriptorPool` **must** be a valid `VkDescriptorPool` handle
- `descriptorPool` **must** have been created with the
  `VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT` flag
- `descriptorSetCount` **must** be greater than 0
- `pDescriptorSets` **must** be a pointer to an array of `descriptorSetCount`
  `VkDescriptorSet` handles, each element of which **must** either be a valid
   handle or `VK_NULL_HANDLE`

# Tests

For `vkAllocateDescriptorSets`, these tests should test the following cases:
- `descriptorSetCount` of valid values:
  - [x] 1
  - [x] 3
- `pSetLayouts` of valid values
  - [x] pointer to 1 `VkDescriptorSetLayout` handle
  - [x] pointer to 3 `VkDescriptorSetLayout` handles
- `pDescriptorSets` of valid values
  - [x] pointer to 1 element
  - [x] pointer to 3 elements

For `vkFreeDescriptorSets`, these tests should test the following cases:
- `descriptorSetCount` of valid values
  - [x] 1
  - [x] 3
- `pDescriptorSets` contains `VK_NULL_HANDLE`
  - [x] yes
  - [x] no
