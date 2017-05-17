# vkCreateDescriptorPool / vkResetDescriptorPool / vkDestroyDescriptorPool

## Signatures
```c++
VkResult vkCreateDescriptorPool(
    VkDevice                                    device,
    const VkDescriptorPoolCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDescriptorPool*                           pDescriptorPool);

VkResult vkResetDescriptorPool(
    VkDevice                                    device,
    VkDescriptorPool                            descriptorPool,
    VkDescriptorPoolResetFlags                  flags);

void vkDestroyDescriptorPool(
    VkDevice                                    device,
    VkDescriptorPool                            descriptorPool,
    const VkAllocationCallbacks*                pAllocator);
```

## Valid Usage

For `vkCreateDescriptorPool`, according to the Vulkan spec:
- `maxSets` **must** be greater than 0
- `poolSizeCount` **must** be greater than 0
- `descriptorCount` **must** be greater than 0

For `vkResetDescriptorPool`, according to the Vulkan spec:
- `descriptorPool` **must** be a valid `VkDescriptorPool` handle
- `flags` **must** be 0

# VkDescriptorPoolCreateInfo
```c++
typedef struct VkDescriptorPoolCreateInfo {
    VkStructureType                sType;
    const void*                    pNext;
    VkDescriptorPoolCreateFlags    flags;
    uint32_t                       maxSets;
    uint32_t                       poolSizeCount;
    const VkDescriptorPoolSize*    pPoolSizes;
} VkDescriptorPoolCreateInfo;

typedef struct VkDescriptorPoolSize {
    VkDescriptorType    type;
    uint32_t            descriptorCount;
} VkDescriptorPoolSize;
```

# Tests

These tests should test the following cases for `vkCreateDescriptorPool`:
- `flags` of valid `VkDescriptorPoolCreateFlagBits` values:
  - [x] no bits set
  - [x] `VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT`
- `maxSets` of valid values:
  - [x] 1
  - [x] 10
- `poolSizeCount` of valid values:
  - [x] 1
  - [x] 3
- `type` of valid `VkDescriptorType` values:
  - [x] `VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE`
  - [x] `VK_DESCRIPTOR_TYPE_STORAGE_IMAGE`
  - [x] `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
  - [x] `VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT`
- `descriptorCount` of valid values
  - [x] 1
  - [x] 5
  - [x] 8
  - [x] 42

These tests should test the following cases for `vkDestroyDescriptorPool`:
- [x] `descriptorPool` == null
- [x] `descriptorPool` != null
