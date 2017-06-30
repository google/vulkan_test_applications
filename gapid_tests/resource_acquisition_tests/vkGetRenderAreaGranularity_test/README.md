# vkGetRenderAreaGranularity

## Signatures
```c++
void vkGetRenderAreaGranularity(
    VkDevice                                    device,
    VkRenderPass                                renderPass,
    VkExtent2D*                                 pGranularity);
```

According to the Vulkan spec:
- The conditions leading to an optimal `renderArea` (in `VkRenderPassBeginInfo`)
  are:
  - the `offset.x` member in `renderArea` is a multiple of the `width` member
  of the returned `VkExtent2D` pointed by `pGranularity`.
  - the `offset.y` member in `renderArea` is a multiple of the `height` member
  of the returned `VkExtent2D` pointed by `pGranularity`.
  - either the `offset.width` member in `renderArea` is a multiple of the
  horizontal granularity or `offset.x+offset.width` is equal to the `width` of
  the `framebuffer` in the `VkRenderPassBeginInfo`.
  - either the `offset.height` member in `renderArea` is a multiple of the
  vertical granularity or `offset.y+offset.height` is equal to the height of
  the `framebuffer` in the `VkRenderPassBeginInfo`.
- `pGranularity` **must** be a pointer to a `VkExtent2D` structure

These tests should test the following cases:
- [x] `renderPass` of a valid renderpass
- [x] `pGranularity` of a valid pointer to `VkExtent2D`
