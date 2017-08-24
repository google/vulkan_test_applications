# Vulkan Test Applications

`Vulkan Test Applications` is a repository that contains several sets of
Vulkan applications.

The goals of this project are to maintain a repository of accessible test
applications for Vulkan tool developers to leverage, as well as expose
interesting or non-obvious implications of the API.


This is not an official Google product (experimental or otherwise), it is just
code that happens to be owned by Google. See the
[CONTRIBUTING.md](CONTRIBUTING.md) file for more information. See also the
[AUTHORS](AUTHORS) and [CONTRIBUTORS](CONTRIBUTORS) files.

# Test Types
## Sample applications

These are a set of sample applications that either uses the API in a way that is
interesting for tools or use some functionality of the API that has not been
exposed to other samples.
- [application_sandbox](application_sandbox/README.md)

## GAPID command tests

These tests are designed to test the functionality of
[GAPID](https://github.com/google/gapid) for Vulkan. They can also be
used to expose a variety of function call permutations for any layers. As a
group, they attempt to call all Vulkan functions with all permutations of
valid inputs. See [gapid_tests](gapid_tests/README.md) for more information.

## Checking out / Building
To clone:
git clone --recursive path/to/this/repository

This will ensure that you have all of the dependencies checked out.

To build for Windows.
```
cmake /path/to/source
open VulkanTestApplications.sln
```
or if you want to use Ninja
```
From a Visual Studio command-prompt
cmake -GNinja /path/to/source -DCMAKE_BUILD_TYPE=Release
ninja
```


To build for Linux.
```
cmake -GNinja /path/to/source
ninja
```

To build for Android.
```
cmake -GNinja {root} -DBUILD_APKS=ON -DANDROID_SDK=path/to/android/sdk
ninja
```

This assumes the Android ndk is installed in the default location of
path/to/android/sdk/ndk-bundle.

If it is installed elsewhere, use
```
cmake -GNinja {root} -DBUILD_APKS=ON -DANDROID_SDK=path/to/android/sdk -DANDROID_NDK=path/to/ndk -DCMAKE_GLSL_COMPILER=path/to/glslc
```

To build only for 32-bit ARM platform.
```
cmake -GNinja {root} -DBUILD_APKS=ON -DANDROID_SDK=path/to/android/sdk -DANDROID_ABIS=armeabi-v7a -DCMAKE_GLSL_COMPILER=path/to/glslc
```

`glslc` is required to compile GLSL shaders to SPIR-V. If it is not
on your path, its location should be specified through `-DCMAKE_GLSL_COMPILER`
option.

# Compilation Options
The only specific other compilation options control default behavior for all
applications. See [entry](support/entry/README.md) for more information
on these flags.

# Support Functionality
- [cmake](cmake/README.md)
- [support](support/README.md)
- [vulkan_wrapper](vulkan_wrapper/README.md)
- [vulkan_helpers](vulkan_helpers/README.md)

# Standard Assets
- [standard_images](standard_images/README.md)
- [standard_models](standard_models/README.md)
- [shader_library](shader_library/README.md)

# Dependencies
These should be checked out into `third_party`.
- [mathfu](https://github.com/google/mathfu)
- [vk_callback_swapchain](https://github.com/google/vk_callback_swapchain)
