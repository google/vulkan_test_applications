# Mutable Swapchain Format

This sample renders a rotating cube on the screen with a swapchain that is
created with a UNORM format and image views with a SRGB format. To enable the
creation of a swapchain with a mutable format the device extensions
`VK_KHR_maintenance2`, `VK_KHR_image_format_list`, and
`VK_KHR_swapchain_mutable_format` are requested. The creation of a swapchain
with a mutable format is controlled by providing a
`VkImageFormatListCreateInfoKHR` and setting the swapchain create info flag
`VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR`.