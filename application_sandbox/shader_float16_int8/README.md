# Shader Float16 Int8

This sample renders a rotating cube on the screen with color data stored in 8bit
integers and modified with float16 operations. The use of 16bit floats and 8bit
integers is enabled by requesting the instance extension
`VK_KHR_get_physical_device_properties2` and the device extensions
`VK_KHR_storage_buffer_storage_class`, `VK_KHR_8bit_storage` and
`VK_KHR_shader_float16_int8`.