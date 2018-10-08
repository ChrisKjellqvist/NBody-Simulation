#ifndef CONSTANTS_NBODY_HPP
#define CONSTANTS_NBODY_HPP

// number of initial bodies
#define NOFB_INIT 512
// number of iterations (comment/uncomment to enable/disable)
#define ITERATIONS 600
// define time_step if we're not doing a variable time_step
#ifndef VARIABLE_TS
#define TIME_STEP 1.0
#endif
// gravitational constant
#define GRAV_CONST (double)6.67408e-11
// TODO: replace with CUDA rsqrt. Inverse sqrt
#ifndef rsqrt
#define rsqrt 1.0/sqrt
#endif

// 2^NOFB_BIT_SIZE options of bodies per bit
#define NOFB_BIT_SIZE 4

// make sure that number of initial bodies > 2
#if NOFB_INIT < 2
#error "NOFB_INIT must be defined as 2 or more"
#endif


#endif
