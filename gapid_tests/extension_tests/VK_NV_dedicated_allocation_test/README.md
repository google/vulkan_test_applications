# VK_NV_dedicated_allocation

## VkDedicatedAllocationImageCreateInfoNV
```c++
typedef struct VkDedicatedAllocationImageCreateInfoNV {
    VkStructureType    sType;
    const void*        pNext;
    VkBool32           dedicatedAllocation;
} VkDedicatedAllocationImageCreateInfoNV;
```

## VkDedicatedAllocationBufferCreateInfoNV
```c++
typedef struct VkDedicatedAllocationBufferCreateInfoNV {
    VkStructureType    sType;
    const void*        pNext;
    VkBool32           dedicatedAllocation;
} VkDedicatedAllocationBufferCreateInfoNV;
```

## VkDedicatedAllocationMemoryAllocateInfoNV
```c++
typedef struct VkDedicatedAllocationMemoryAllocateInfoNV {
    VkStructureType    sType;
    const void*        pNext;
    VkImage            image;
    VkBuffer           buffer;
} VkDedicatedAllocationMemoryAllocateInfoNV;
```

According to the Vulkan spec:
- If `VkDedicatedAllocationBufferCreateInfoNV` is set
  - bufferCreateInfo.flags must not contain
        `VK_BUFFER_CREATE_SPARSE_BINDING_BIT`
        `VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT` or
        `VK_BUFFER_CREATE_SPARSE_ALIASED_BIT`
- If `VkDedicatedAllocationMemoryAllocateInfoNV` is set
  - VkImage `must` be an image created with
    `VkDedicatedAllocationImageCreateInfoNV` and `dedicatedAllocation` = `true`
  - or
  - VkBuffer `must` be a buffer created with
    `VkDedicatedAllocationBufferCreateInfoNV` and `dedicatedAllocation` = `true`
  - This memory can only be used with vkBind* on the VkImage or VBuffer it was
    created with


These tests should test the following cases:
- [x] `VkDedicatedAllocationBufferCreateInfoNV` with dedicatedAllocation = `True`
- [x] `VkDedicatedAllocationBufferCreateInfoNV` with dedicatedAllocation = `False`
- [x] `VkDedicatedAllocationImageCreateInfoNV` with dedicatedAllocation = `True`
- [x] `VkDedicatedAllocationImageCreateInfoNV` with dedicatedAllocation = `False`
- [x] `VkDedicatedAllocationMemoryAllocateInfoNV` with `image` set
- [x] `VkDedicatedAllocationMemoryAllocateInfoNV` with `buffer` set
