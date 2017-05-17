# vkCreateBufferView / vkDestroyBufferView

## Signatures
```c++
VkResult vkCreateBufferView(
    VkDevice                                    device,
    const VkBufferViewCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkBufferView*                               pView);

void vkDestroyBufferView(
    VkDevice                                    device,
    VkBufferView                                bufferView,
    const VkAllocationCallbacks*                pAllocator);
```

# VkBufferViewCreateInfo
```c++
typedef struct VkBufferViewCreateInfo {
    VkStructureType            sType;
    const void*                pNext;
    VkBufferViewCreateFlags    flags;
    VkBuffer                   buffer;
    VkFormat                   format;
    VkDeviceSize               offset;
    VkDeviceSize               range;
} VkBufferViewCreateInfo;
```

According to the Vulkan spec:
- `sType` **must** be `VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO`
- `pNext` **must** be `NULL`
- `flags` **must** be 0
- `buffer` **must** be a valid `VkBuffer` handle
- `format` **must** be a valid `VkFormat` value
- `offset` is an offset in bytes from the **base address of the buffer**.
  Accesses to the buffer view from shaders use addressing that is relative to
  this starting offset
- `offset` **must** be a multiple of
  `VkPhysicalDeviceLimits::minTexelBufferOffsetAlignment`
- `offset` **must** be less than the size of `buffer`
- `range` is a size in bytes of the buffer view. If `range` is equal to
  `VK_WHOLE_SIZE`, the range from `offset` to the end of the buffer is used.
  If `VK_WHOLE_SIZE` is used an the remaining size of the buffer is **not** a
  multiple of the element size of `format`, the nearest smaller multiple is
  used
- If `range` is not equal to `VK_WHOLE_SIZE`, `range` **must** be greater than
  0, **must** be a multiple of the element size of `format`, `range` divided by
  the element size of `format`, **must** be less than or equal to
  `VkPhysicalDeviceLimits::maxTexelBufferElements` and the sum of `offset` and
  `range` **must** be less than or equal to the size of `buffer`
- `buffer` **must** have been created with a `usage` value containing at least
  one of `VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT` or
  `VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT`
- If `buffer` was created with `usage` containing
  `VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT`, `format` **must** be supported
  for uniform texel buffers
- If `buffer` was created with `usage` containing
  `VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT`, `format` **must** be supported
  for storage texel buffers
- If `buffer` is non-sparse then it **must** be bound completely and
  contiguously to a single `VkDeviceMemory` object

These tests should test the following cases:
- [x] `buffer` created with usage bit `VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT`
- [x] `buffer` created with usage bit `VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT`
- [x] `offset` of value zero
- [x] `offset` of value non-zero
- [x] `range` of value `VK_WHOLE_SIZE`
- [x] `range` of value other than `VK_WHOLE_SIZE`
