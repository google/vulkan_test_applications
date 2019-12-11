# Sample Locations

This sample renders a rotating cube on a black floor with the reflection of the
cube shown on the floor the same way as in the stencil application. The stencil
and depth buffer are multisampled with non-default sample locations using the
sample locations extension. The use of sample locations is enabled by requesting
the instance extension `VK_KHR_get_physical_device_properties2` and the device
extension `VK_EXT_sample_locations`. Sample locations are controlled by
extending the pipeline creation info with
`VkPipelineSampleLocationsStateCreateInfoEXT` when creating the pipeline and
extending the render pass begin info with
`VkRenderPassSampleLocationsBeginInfoEXT`.