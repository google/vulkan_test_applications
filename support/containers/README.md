# Containers

`containers` is a library that provides an allocator interface to memory
allocation.

Every piece of the framework uses the allocators for malloc/free operations.
At the program's creation a default allocator is created, and at the end
of a program, it is expected that the allocator contains no outstanding
allocations. This is checked for every application type.

Also provided are wrapped STL objects that all take allocators on creation.
This allows normal STL functions/objects to be used without missing out on
the usefulness provided by the allocator interface.

In the future, if more complicated applications are necessary, more interesting
and complex allocators can be created and slotted in at specific points.