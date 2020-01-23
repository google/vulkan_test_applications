# SPIR-V 1.4

This sample renders a rotating cube with color output calculated by a function
that tests `VK_KHR_shader_float_controls` as part of `VK_KHR_spirv_1_4`. Shader float controls are enabled by
requesting the `VK_KHR_get_physical_device_properties2` instance extension and
the device extension `VK_KHR_spirv_1_4`. The various shader float
controls are enabled in the SPIR-V assembly shaders.