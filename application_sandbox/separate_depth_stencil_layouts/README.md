# separate_depth_stencil_layouts

Similar to depth_stencil_resolve, but vkCreatePhysicalDevice() has a
VkPhysicalDeviceFeatures2 passed as the pNext, with a
VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures set as the features' pNext.
We utilize this with two image barriers each renderpass that only affect the
depth/stencil image's depth aspect, to enable writing then reading a depth
image.
