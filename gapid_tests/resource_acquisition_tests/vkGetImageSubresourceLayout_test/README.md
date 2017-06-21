# vkGetImageSubresourceLayout

## Signatures
```c++
void vkGetImageSubresourceLayout(
    VkDevice                                    device,
    VkImage                                     image,
    const VkImageSubresource*                   pSubresource,
    VkSubresourceLayout*                        pLayout);
```

According to the Vulkan spec:
- `image` **must** have been created with `tiling` equal to `VK_IMAGE_TILING_LINEAR`
- The `aspectMask` member of `pSubresource` **must** only have a single bit set

These tests should test the following cases:
- [x] `pSubresource` and `pLayout` of pointers to valid `VkImageSubresource`
  and `VkSubresourceLayout` struct
