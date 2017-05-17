# Entry

The `entry` library is used to set up system agnostic entry-point routines.
This library creates a window that is suitable for Vulkan rendering. This
window is presented to the user, and passed to the `main_entry` in the
`entry_data` field as described below.

You must, somewhere in your program, define a function with the signature
`int main_entry(const entry_data* data);` This function will get called
by the entry point library.

# Command line options

Command line options that are supported by `entry` at this time are:
- `-w=N` This will set the width of the window to N
- `-h=N` This will set the height of the window to N
- `-output-frame=N` This will instruct any VulkanApplication to use a virtual
swapchain instead fo the real swapchain. This will output the contents of the
swapchain after frame `N` and instruct the application to exit. `-1` will
turn this off. `-1` is the default.
- `-output-file=filename` This will set the name of the file that
`-output-frame` writes to. The default is `output.ppm`
- `-separate-present` This prefers a separate presentation queue instead of the
default if possible.
- `-fixed` This will instruct the application to simulate a fixed framerate.
This is particularly useful when outputting frames, since the times should
be consistent.

# Cmake Configuration options
Each of the command-line arguments has a CMake build option that will
affect the default value.
- `OUTPUT_FRAME` Sets the default value of `-output-frame`. `-1` normally.
- `OUTPUT_FILE` Sets the default value of `-output-file`. `output.ppm` normally.
- `DEFUALT_WINDOW_WIDTH` Sets the default value of `-w=`. `100` normally.
- `DEFAULT_WINDOW_HEIGHT` Sets the default value of `-h=`. `100` normally.
- `FIXED_TIMESTEP` Turns on `-fixed` by default.
- `PREFER_SEPARATE_PRESENT` Turns on `-separate-present` by default.

# Android
Notes for Android, since there is no way of providing command-line arguments
to Android, only the CMake configuration options can be used to affect
behavior.