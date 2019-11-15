# Swapchain Colorspace

This sample renders a rotating cube on the screen with swapchain that uses an
extended colorspace if one is available. The use of extended swapchain
colorspaces is enabled by requesting the instance extensions
`VK_EXT_swapchain_colorspace` and `VK_KHR_get_surface_capabilities2`. The option
of using an extended colorspace is controlled by setting the extended swapchain
colorspace flag in `SampleOptions` with the function
`EnableExtendedSwapchainColorSpace`.