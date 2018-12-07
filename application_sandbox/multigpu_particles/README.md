# MultiGPU Particles

This sample is WIP. In that it is not very well tested yet.
The intended behavior is that for a multi-gpu context, this
will simulate particles on one GPU, and render them on the
other.

Given that the simulation and rendering are independent
operations, this should provide some amount of benefit.

You can make tweaks to:
`particle_data_shared.h:TOTAL_PARTICLES`
`particle_velocity_update.comp:TOTAL_PARTICLES`

 in order to change the proportion of compute vs
 raster work.