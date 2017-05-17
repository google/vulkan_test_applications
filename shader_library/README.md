# Shader Library

This contains a set of standard shaders that can be included in other
applications.

Of particular interest is models/model_setup.glsl. The vulkan model
class that is present in `vulkan_helpers/vulkan_model.h` have a particular
attribute layout. To use these models, and to future-proof any changes
to that layout, the functions in `vulkan_model.h` can be used
as they will be updated if the format changes.

