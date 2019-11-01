# conservative_rasterization

This sample renders a thin rotating stick that is rendered incorrectly without
the use of conservative rasterization. Conservative rasterization is enabled by
requesting the `VK_KHR_get_physical_device_properties2` instance extension and
the `VK_EXT_conservative_rasterization` device extension. The use of
conservative rasterization is controlled by creating a
`VkPipelineRasterizationConservativeStateCreateInfoEXT` structure and passing it
to the function `SetRasterizationExtension` on the graphics pipeline.
