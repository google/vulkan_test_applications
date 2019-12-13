# Display Timing

This sample renders a rotating cube on the screen and adjusts the image
presentation duration when images are presented too early or too late. The use
of diplay timing is enabled by requesting the device extension
`VK_GOOGLE_display_timing`. Display timing is controlled through `SampleOptions`
by calling the function `EnableDisplayTiming()`.