# CMake Modifications

This folder contains all of the additional files for a cmake build.

`build_apk.cmake` contains all of the custom `add_executable` and
`add_library` functions that we use to create Android Studio projects as well
as pass through for non Android Studio builds.

`android_project_template` contains all of the boilerplate files
that are needed to build an android project.

# Custom CMake Functionality
Once `build_apk.cmake` is included in a project the following functionality
is exposed. These helpers should be used instead of standard `add_executable`,
`add_library`, etc. This will allow the tracking and creation of Android
apk files. Otherwise dependencies may be out of date, and the apk files will
not update as needed.

## `add_vulkan_executable`
`add_vulkan_executable` may be called. It takes the same arguments as
`add_executable` but adds the following options.
- `SHADERS` Defines a shader library that this executable depends on.
- `MODELS`  Defines a model library that this executable depends on.
- `TEXTURES` Defines a texture library that this executable depends on.
For all of the above, include paths will automatically be set up so that
you may include the build artefacts from these libraries.

The executable will be created with an entry point as described in
[entry](../support/entry/README.md). This executable be created with a default
window, as described by command-line options or build configuration.

## `add_vulkan_library`
`add_vulkan_library` may be called. It takes the same arguments as
`add_library` with an additional `TYPE` that defines shared or static.

## `add_shader_library`
`add_shader_library` may be called. It takes the following arguments.
This uses `glslc` to compile the glsl to appropriate spirv files.
- `SOURCES` A list of glsl sources with filename suffixes of
`.vert;.frag;.comp;.tesse;.tessc;.geom`. These will be compiled into
`.spv.h` files that can be included in applications.
- `SHADER_DEPS` A list of other shader libraries that can be included from
this shader library.

## `add_texture_library`
Functionally equivalent to `add_shader_library` except the input is `.png`
files. This uses a python script to convert the `.png` files to a
header that is includable in an application.

## `add_model_library`
Functionally equivalent to `add_shader_library` except the input is `.obj` files.
This uses a python script to convert the `.obj.` files to a header that is
includable in an application.

