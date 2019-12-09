# Shared Presentable Image

This sample renders a rotating cube on the screen using a swapchain with shared
presentable images. The use of shared presentable images are enabled by
requesting the instance extensions `VK_KHR_get_physical_device_properties2`,
`VK_KHR_get_surface_capabilities2`, and requesting the device extension
`VK_KHR_shared_presentable_image`.