# Pipeline Creation Feedback

This sample renders a rotating cube on the screen and prints out the pipeline
creation feedback information to the console. The use of pipeline creation
feedback is enabled by requesting the `VK_EXT_pipeline_creation_feedback` device
extension. Pipeline creation feedback is then aquired by setting a
`VkPipelineCreationFeedbackCreateInfoEXT` as a pipline extension with the
function `SetPipelineExtensions`.