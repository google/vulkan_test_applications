# Scalar Block Layout

This sample renders a rotating cube on the screen and uses a uniform buffer with
a scalar block layout to store color information. To enable the use of the
scalar block layout the instance extension
`VK_KHR_get_physical_device_properties2` and device extension
`VK_EXT_scalar_block_layout` are requested.