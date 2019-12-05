# Multiplanar Image Disjoint

This sample renders a rotating cube with a disjoint multiplanar texture that is
sampled with a VkSamplerYcbcrConversion. Sampler Y’CBCR conversion and disjoint
multiplanar image memory is enabled by requesting the
`VK_KHR_get_physical_device_properties2` instance extension and the device
extensions `VK_KHR_maintenance1`, `VK_KHR_get_memory_requirements2`,
`VK_KHR_bind_memory2`, and `VK_KHR_sampler_ycbcr_conversion`. The multiplanar
image is allocated with disjoint memory when the `VK_IMAGE_CREATE_DISJOINT_BIT`
flag is passed in a texture's `InitializeData` function.
