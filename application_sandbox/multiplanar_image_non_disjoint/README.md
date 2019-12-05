# Multiplanar Image Non-Disjoint

This sample renders a rotating cube with a multiplanar texture that is
sampled with a VkSamplerYcbcrConversion. The sampler Y’CBCR conversion
feature is enabled by requesting the `VK_KHR_get_physical_device_properties2`
instance extension and the device extensions `VK_KHR_maintenance1`,
`VK_KHR_get_memory_requirements2`, and `VK_KHR_sampler_ycbcr_conversion`.
