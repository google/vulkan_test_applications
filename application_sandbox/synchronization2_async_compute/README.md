# Async Compute

This sample creates a secondary queue that runs compute operations. If no
dedicated compute queue family is found, then a second queue in the main
graphics/compute family will be used. If this also cannot be found, then
the application will terminate.

These are run on a separate thread, and shuttled back to the main thread
in a mailbox. The main thread will take whatever the most up-to-date
simulation is, and render that as fast as possible.

The actual simulation in question is an N-Body simulation of 64k particles.
Each simulation frame, each particle is attracted to 1/128 of the other
particles. The set of particles that each particle attracts to is rotated
once per simulation frame.