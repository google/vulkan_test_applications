# vkBindBufferMemory / vkMapMemory / vkUnmapMemory

## Signatures
```c++
VkResult vkBindBufferMemory(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    VkDeviceMemory                              memory,
    VkDeviceSize                                memoryOffset);

VkResult vkMapMemory(
    VkDevice                                    device,
    VkDeviceMemory                              memory,
    VkDeviceSize                                offset,
    VkDeviceSize                                size,
    VkMemoryMapFlags                            flags,
    void**                                      ppData);

void vkUnmapMemory(
    VkDevice                                    device,
    VkDeviceMemory                              memory);
```

According to the Vulkan spec:
- `vkBindBufferMemory`
  - `device` **must** be valid
  - `buffer` **must** be valid
  - `memory` **must** be valid
  - `memoryOffset` **must** be < the size of `memory` - the size retured by
    `vkGetBufferMemoryRequirements`
  - `memoryOffset` **must** be a multiple `alignment` from
    `vkGetBufferMemoryRequirements`
- `vkMapMemory`
  - `device` **must** be valid
  - `memory` **must** be valid
  - `offset` **must** be <= than the size of the memory
  - If `size` is not `VK_WHOLE_SIZE` then `size + offset` **must** be <= the
    size of the memory
  - `flags` **must** be 0
  - `ppData` **must** be a valid pointer to a `void*`
- `vkUnmapMemory`
  - `device` **must** be valid
  - `memory` **must** be valid

These tests should test the following cases:
- [x] `memoryOffset` == 0
- [ ] `memoryOffset` != 0
- [x] `size` == `VK_WHOLE_SIZE`
- [ ] `size` != `VK_WHOLE_SIZE`
- [x] `vkMapMemory.memoryOffset` == 0
- [ ] `vkMapMemory.memoryOffset` != 0