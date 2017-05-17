# Vulkan Wrapper
This library is designed to simplify the loading of vulkan functions. This will
involve wrapping the dispatchable objects, as well as lazily loading function
pointers as necessary.

We lazily initialize the functions because we do not want to call
or even resolve any functions from the loader that we do not use. This will
let us more easily determine when a failure in a layer occurs.

NOTE: The goal of this library is not to be fast, but more to be both
easy to use and allow us to correctly handle a large variety of cases.