# Standard Uniform Buffer Layout

This sample renders a rotating cube on the screen and uses a uniform buffer with
a standard uniform buffer layout to store color information. The use of the
standard uniform buffer layout is enabled by requesting the instance extension
`VK_KHR_get_physical_device_properties2` and device extension
`VK_KHR_uniform_buffer_standard_layout`.