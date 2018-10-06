#include <stdio.h>
#include <signal.h>
#include <ctime>
#include <stdlib.h>
#include <math.h>
#include "SolarSystem.hpp"
#define TIME_STEP 1
#define GRAV_CONST (float)6.67408e-11
#ifndef NOFB_INIT
#define NOFB_INIT 512
#endif
#ifndef rsqrt
#define rsqrt 1.0/sqrtf
#endif
// 2^NOFB_BIT_SIZE options of bodies per bit
#define NOFB_BIT_SIZE 4

#if NOFB_INIT < 2
#error "NOFB_INIT must be defined as 2 or more"
#endif

int nofB = NOFB_INIT;
float pX[NOFB_INIT];
float pY[NOFB_INIT];
float pZ[NOFB_INIT];
float vX[NOFB_INIT];
float vY[NOFB_INIT];
float vZ[NOFB_INIT];
float masses[NOFB_INIT];
int collisions[NOFB_INIT];

int sig = 0;
void handle_sigs(int signo){
  sig = signo == SIGINT;
}
#pragma acc routine seq
extern float sqrtf(float f);
int main() {
  srand(time(0));

  for (int i = 0; i < nofB; i++) {
    Body t;
    vX[i] = t.velocityX;
    vY[i] = t.velocityY;
    vZ[i] = t.velocityZ;
    masses[i] = t.mass;
  }
  collisions[0] = collisions[1] = 0;
  int cnt = 0;
#pragma acc data copyin(pX, pY, pZ, vX, vY, vZ, masses, collisions, nofB)
  while(cnt++ < 100 && !sig){
    if (cnt % 1000 == 0){
      printf("%d\n", cnt);
    }
#pragma acc data copyout(pX, pY, pZ)
    for(int i = 0; i < 100; ++i){
      int curr;
#pragma acc kernels private(curr) async(2)
      for(int my_planet = 0; my_planet < nofB; ++my_planet){
	float deltaX =0, deltaY =0, deltaZ =0;
	int collided_with = 0;
	for(curr = 0; curr < my_planet; ++curr){
	  float dX = pX[my_planet]-pX[curr];
	  float dY = pY[my_planet]-pY[curr];
	  float dZ = pZ[my_planet]-pZ[curr];
	  // TODO: replace rsqrt macro definition with CUDA builtin rsqrt, it'll
	  // get rid of a division which could be big
	  float recip_dist = rsqrt(dX*dX+dY*dY+dZ*dZ);
	  // want to get rid of all conditionals
	  // replace
	  /*
	     if (recip_dist < 100000)
	       collisions[my_planet] = curr;
	  */
	  // we can get pretty close. We give the range that curr is in by marking
	  // a bit flag. Let the CPU figure out which one it wants to merge
	  // also enables us to do more than one collision
	  collided_with |= (recip_dist > .000001) << (curr >> NOFB_BIT_SIZE);
	  recip_dist *= masses[my_planet] * masses[curr] * GRAV_CONST;
	  deltaX += dX * recip_dist;
	  deltaY += dY * recip_dist;
	  deltaZ += dZ * recip_dist;
	}
	vX[my_planet] += deltaX;
	vY[my_planet] += deltaY;
	vZ[my_planet] += deltaZ;
	collisions[my_planet] = collided_with;
      }
#pragma acc kernels async(1) wait(2)
      for(curr = 0; curr < nofB; ++curr){
	pX[curr] += vX[curr] * TIME_STEP;
	pY[curr] += vY[curr] * TIME_STEP;
	pZ[curr] += vZ[curr] * TIME_STEP;
      }
    }
    int num_shiftdowns = 0;
#pragma acc update host(collisions)
    for (int i = 0; i < nofB; ++i){
      while (collisions[i] != 0){
#pragma acc update host(vX, vY, vZ, pX, pY, pZ)
	int p1 = i;
	int p2;
	float minLength = 2e100;
	for (int j = 0; j < nofB; ++j){
	  if ((j >> NOFB_BIT_SIZE) & collisions[i] == 0)
	    continue;
	  collisions[i] ^= 1 << (j >> NOFB_BIT_SIZE);
	  float dX = pX[i] - pX[j];
	  float dY = pY[i] - pY[j];
	  float dZ = pZ[i] - pZ[j];
	  float dist;
	  if ((dist = sqrt(dX*dX + dY*dY + dZ*dZ)) < minLength){
	    p2 = j;
	    minLength = dist;
	  }
	}
	int newmass = masses[p1]+masses[p2];
	vX[p1] = (vX[p1]*masses[p1]+vX[p2]*masses[p2])/newmass;
	vY[p1] = (vY[p1]*masses[p1]+vY[p2]*masses[p2])/newmass;
	vZ[p1] = (vZ[p1]*masses[p1]+vZ[p2]*masses[p2])/newmass;
	masses[p1] = newmass;
	for(int j = p2; j < nofB-1; ++j){
	  pX[j] = pX[j+1];
	  pY[j] = pY[j+1];
	  pZ[j] = pZ[j+1];
	  vX[j] = vX[j+1];
	  vY[j] = vY[j+1];
	  vZ[j] = vZ[j+1];
	  masses[j] = pX[j+1];
	}
	collisions[i] = 0;
        --nofB;
      }
#pragma acc wait(1)
#pragma acc update device(nofB, pX, pY, pZ, vX, vY, vZ, masses)
    }
#pragma acc wait(1)
    // this is where you might write some display stuff
    // for now it just prints out 
    //
  }
  printf("%d\n", nofB);
}
