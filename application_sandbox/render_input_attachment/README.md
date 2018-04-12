# Render Input Attachment

This sample renders the raw data to a quad image to a color attachment, then
use the color attachment as an input attachment to render the output. The
width and height of the rendering targets (the swapchain images) must be at
least as large as 400x400, i.e. run with argument `-w=400 -h=400` or larger
numbers.