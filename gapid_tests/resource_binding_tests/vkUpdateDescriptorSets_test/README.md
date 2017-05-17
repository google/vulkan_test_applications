# vkUpdateDescriptorSets

## Signatures
```c++
void vkUpdateDescriptorSets(
    VkDevice                                    device,
    uint32_t                                    descriptorWriteCount,
    const VkWriteDescriptorSet*                 pDescriptorWrites,
    uint32_t                                    descriptorCopyCount,
    const VkCopyDescriptorSet*                  pDescriptorCopies);
```

# VkWriteDescriptorSet
```c++
typedef struct VkWriteDescriptorSet {
    VkStructureType                  sType;
    const void*                      pNext;
    VkDescriptorSet                  dstSet;
    uint32_t                         dstBinding;
    uint32_t                         dstArrayElement;
    uint32_t                         descriptorCount;
    VkDescriptorType                 descriptorType;
    const VkDescriptorImageInfo*     pImageInfo;
    const VkDescriptorBufferInfo*    pBufferInfo;
    const VkBufferView*              pTexelBufferView;
} VkWriteDescriptorSet;

typedef struct VkDescriptorBufferInfo {
    VkBuffer        buffer;
    VkDeviceSize    offset;
    VkDeviceSize    range;
} VkDescriptorBufferInfo;

typedef struct VkDescriptorImageInfo {
    VkSampler        sampler;
    VkImageView      imageView;
    VkImageLayout    imageLayout;
} VkDescriptorImageInfo;

typedef enum VkDescriptorType {
    VK_DESCRIPTOR_TYPE_SAMPLER = 0,
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
    VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
    VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
} VkDescriptorType;
```

# VkCopyDescriptorSet
```c++
typedef struct VkCopyDescriptorSet {
    VkStructureType    sType;
    const void*        pNext;
    VkDescriptorSet    srcSet;
    uint32_t           srcBinding;
    uint32_t           srcArrayElement;
    VkDescriptorSet    dstSet;
    uint32_t           dstBinding;
    uint32_t           dstArrayElement;
    uint32_t           descriptorCount;
} VkCopyDescriptorSet;
```

# Valid usage

For `VkWriteDescriptorSet`, according to the Vulkan spec:
- `descriptorCount` must be greater than 0
- If `descriptorType` is `VK_DESCRIPTOR_TYPE_SAMPLER`,
  `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`,
  `VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE`, `VK_DESCRIPTOR_TYPE_STORAGE_IMAGE`,
  or `VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT`, `pImageInfo` must be a pointer
  to an array of `descriptorCount` valid `VkDescriptorImageInfo` structures
- If `descriptorType` is `VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER` or
  `VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER`, `pTexelBufferView` must be
  a pointer to an array of `descriptorCount` valid `VkBufferView` handles
- If `descriptorType` is `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`,
  `VK_DESCRIPTOR_TYPE_STORAGE_BUFFER`,
  `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`, or
  `VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC`, `pBufferInfo` must be
  a pointer to an array of `descriptorCount` valid `VkDescriptorBufferInfo`
  structures

For `VkCopyDescriptorSet`, according to the Vulkan spec:
- `srcSet` **must** be a valid `VkDescriptorSet` handle
- `dstSet` **must** be a valid `VkDescriptorSet` handle
- If `srcSet` is equal to `dstSet`, then the source and destination ranges
  of descriptors **must** not overlap, where the ranges may include array
  elements from consecutive bindings as described by consecutive binding
  updates

# Tests

For `vkUpdateDescriptorSets`, these tests should test the following cases:
- `descriptorWriteCount`
  - [x] == 0
  - [x] != 0
- `descriptorCopyCount`
  - [x] == 0
  - [ ] != 0
For `VkWriteDescriptorSet`, these tests should test the following cases:
- `dstArraryElement`
  - [x] == 0
  - [x] > 0
- `descriptorCount`
  - [x] == 1
  - [x] > 1
- `descriptorType`
  - [x] == `VK_DESCRIPTOR_TYPE_SAMPLER`
  - [ ] == `VK_DESCRIPTOR_TYPE_STORAGE_IMAGE`
  - [ ] == `VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER`
  - [x] == `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- `pImageInfo`
  - [x] == `nullptr`
  - [x] == pointer to meaningful data
  - [ ] == pointer to random data
- `pBufferInfo`
  - [x] == `nullptr`
  - [x] == pointer to meaningful data
  - [ ] == pointer to random data
- `pTexelBufferView`
  - [x] == `nullptr`
  - [ ] == pointer to meaningful data
  - [ ] == pointer to random data

For `VkDescriptorBufferInfo`, these tests should test the following cases:
- `range`
  - [ ] == `VK_WHOLE_SIZE`
  - [x] == some size

For `VkDescriptorImageInfo`, these tests should test the following cases:
- `sampler`
  - [x] used
  - [ ] not used
- `imageView`
  - [ ] used
  - [x] not used
- `imageLayout`
  - [ ] used
  - [x] not used

For `VkCopyDescriptorSet`, these tests should test the following cases:
- `srcArraryElement`
  - [x] == 0
  - [x] > 0
- `dstArraryElement`
  - [x] == 0
  - [x] > 0
- `descriptorCount`
  - [x] == 1
  - [x] > 1
