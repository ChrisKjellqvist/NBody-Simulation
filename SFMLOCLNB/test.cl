#ifdef cl_khr_fp64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#elif defined(cl_amd_fp64)
#pragma OPENCL EXTENSION cl_amd_fp64 : enable
#else
#warning "Double precision floating point not supported by OpenCL implementation."
#endif
#ifndef time_scalar
#define time_scalar .01
#endif

kernel void iterateVelocityVectors(global float* positionsX, global float* positionsY, global float* velocitiesX, global float* velocitiesY, global float* masses , global int* size, global int* flags){
    size_t i = get_global_id(0);
    if (flags[i]==0)
        return;
    for (int j = 0; j < i; j++ ){
        float q = positionsX[j] - positionsX[i];
        float r = positionsY[j] - positionsY[i];
        if (fabs(q)+fabs(r)<500000){
            flags[i] = -2;
        }
        float rsqrtdist = rsqrt(q*q + r*r);
        float dV = rsqrtdist * (float)6.67408e-11 * masses[j];
        velocitiesY[i] = velocitiesY[i] + r * rsqrtdist * time_scalar;
        velocitiesX[i] = velocitiesX[i] + q * rsqrtdist * time_scalar;

    }
    for (int j = i+1; (flags[j+1]) & (j < *size); j++){
        float q = positionsX[j] - positionsX[i];
        float r = positionsY[j] - positionsY[i];
        if (fabs(q)+fabs(r)<500000){
            flags[i] = -2;
        }
        float rsqrtdist = rsqrt(q*q + r*r);
        float dV = rsqrtdist * (float)6.67408e-11 * masses[j] * time_scalar;
        velocitiesY[i] += r * rsqrtdist * dV;
        velocitiesX[i] += q * rsqrtdist * dV;
    }
}

kernel void iteratePositionsVectors(global float* positionsX, global float* positionsY, global float* velocitiesY, global float* velocitiesX, global int* flags){
    size_t i = get_global_id(0);
    int m = (flags[i] | (flags[i] >> 1));
    float a = (velocitiesX[i] * time_scalar);
    int temp = *(int *) ((void *) &a) & m;
    a = *(float *) (&temp);
    positionsX[i] += a;
    
    a = (velocitiesY[i] * time_scalar);
    temp = *(int *) ((void *) &a) & m;
    a = *(float *) (&temp);
    positionsY[i] += a;
//    positionsX[i] += velocitiesX[i] * time_scalar;
//    positionsY[i] += velocitiesY[i] * time_scalar;
}
