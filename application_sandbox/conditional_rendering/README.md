# conditional_rendering

This sample alternates between 2 rendering states using conditional rendering:
1) 2 colorful cubes with a blue background
2) 1 black cube with a pink background

Conditional rendering is enabled by requesting the `VK_EXT_conditional_rendering` device extension.
Conditional rendering is used to enable/disable draws, displatches and attachments clears.

Condition 1 is fulfilled if the buffer value is 1. Condition 2 is using VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT flag, so it will be the opposite of condition 1.

CommandBuffer for each frame:
	- Pipeline barriers
	- ConditionalBegin on condition 1
		- Binding
		- Dispatch: updates buffer value for alpha channel to 1.0 to produce a colourful cube in the later draw.
	- Barriers
	- RenderPass
		- Binding
		- ConditionalBegin on condition 1
			- Clear attachment to blue
			- Draw 2 cubes
		- ConditionalBegin on condition 2
			- Clear attachment to pink
			- Draw a single cube
