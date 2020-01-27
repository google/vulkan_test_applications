# Shader Float Controls

This sample renders a rotating cube with color output calculated by a function
that tests `VK_KHR_shader_float_controls`. Shader float controls are enabled by
requesting the `VK_KHR_get_physical_device_properties2` instance extension and
the device extension `VK_KHR_shader_float_controls`. The various shader float
controls are enabled in the SPIR-V assembly shaders.