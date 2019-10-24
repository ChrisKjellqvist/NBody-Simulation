# NBody-Simulation
This is a simple implementation of an N-Body simulation, accelerated using OpenACC.
An N-Body simulation is one that simulates the movement of celestial bodies.
The idea is that since our own solar system started as a ball of randomly dispersed gas
and eventually formed a star at the center with orbiting bodies, a randomly generated
system with similar constants should model that same process.

## Design
For designing random bodies to place in the system, I used the mass and velocity of the
Earth and generate a random body to have characteristics within an order of magnitude.
The scope of the simulation is roughly that of our solar system.

## Compiling
This project requires the PGI group compilers, since they work well with OpenACC/MP/etc...
Other compilers can handle OpenACC but you'll have to switch around some of the Makefile
variables to get that to work.

make all - builds for gpu

make gpu - same as above

make serial - builds the project as a serial implementation. Useful for debugging

make var - also useful for debugging to find a good timestep. Since bodies may start
very far away from each other, it may take hours for them to reach each other. Having
a variable timestep allows them to travel fast when far away, but maintains most of the
accuracy of the integration when they are close. This requires a serial implementation.
This option is also not very well tested, and shouldn't be used for nofB (number of bodies) > 2.

## Constants.hpp

NOFB_INIT - the initial number of starting bodies

ITERATIONS - uncomment this line and specify the number of iterations desired to run for
some number of iterations

## Future work
I currently use OpenACC to accelerate on the GPU. I could swap this to OpenMP offloading in the future.
