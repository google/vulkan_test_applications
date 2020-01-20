# Storage 8bit

This sample renders a rotating cube on the screen with color data stored in 8bit
integers. The use of 8bit storage is enabled by requesting the instance
extension `VK_KHR_get_physical_device_properties2` and the device extensions
`VK_KHR_storage_buffer_storage_class` and `VK_KHR_8bit_storage`.