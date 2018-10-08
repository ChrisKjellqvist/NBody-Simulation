#include <stdio.h>
#include <signal.h>
#include <ctime>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include "SolarSystem.hpp"
#include "constants.hpp"

int nofB = NOFB_INIT;
double pX[NOFB_INIT];
double pY[NOFB_INIT];
double pZ[NOFB_INIT];
double vX[NOFB_INIT];
double vY[NOFB_INIT];
double vZ[NOFB_INIT];
double masses[NOFB_INIT];
int collisions[NOFB_INIT];
#ifdef VARIABLE_TS
double TIME_STEP;
#endif
int sig = 0;
void handle_sigs(int signo){
  sig = signo == SIGINT;
}
//
#pragma acc routine seq
extern double sqrt(double f);
int main() {
  srand(509);

  // OpenACC doesn't like while(1), so we have this monstrosity instead
  struct sigaction sa;
  sa.sa_handler = handle_sigs;
  sigaction(SIGINT, &sa, NULL);

  //initialize vectors
  for (int i = 0; i < nofB; i++) {
    Body t;
    vX[i] = t.velocityX;
    vY[i] = t.velocityY;
    vZ[i] = t.velocityZ;
    pX[i] = t.positionX;
    pY[i] = t.positionY;
    pZ[i] = t.positionZ;
    masses[i] = t.mass;
    collisions[i] = 0;
  }
#ifdef ITERATIONS
  int cnt = 0;
#endif
#pragma acc data copyin(pX, pY, pZ, vX, vY, vZ, masses, collisions, nofB)
  while(
#ifdef ITERATIONS 
      cnt++ < ITERATIONS &&
#endif
      !sig){
#ifdef ITERATIONS
    if (cnt % 1000 == 0){
      printf("%d\n", cnt);
    }
#endif

// only need p{X,Y,Z} if we're sure there's a collision.
    for(int i = 0; i < 100; ++i){
      int curr;
#pragma acc kernels private(curr) async(2) wait(1)
      for(int my_planet = 0; my_planet < nofB; ++my_planet){
	double deltaX =0, deltaY =0, deltaZ =0;
	int collided_with = 0;
	for(curr = 0; curr < nofB; ++curr){
	  if (curr == my_planet)
	    continue;
	  double dX = pX[my_planet]-pX[curr];
	  double dY = pY[my_planet]-pY[curr];
	  double dZ = pZ[my_planet]-pZ[curr];
	  // TODO: replace rsqrt macro definition with CUDA builtin rsqrt, it'll
	  // get rid of a division which could be big
	  double recip_dist = sqrt(dX*dX+dY*dY+dZ*dZ);
#ifdef VARIABLE_TS
	  if (my_planet == 0 && curr == 1)
	    TIME_STEP = .001 + recip_dist / 1e9;
#endif
	  recip_dist = 1 / recip_dist;
	  // 1 / d

	  // want to get rid of all conditionals
	  // replace
	  /*
	     if (recip_dist < 100000)
	     collisions[my_planet] = curr;
	   */
	  // we can get pretty close. We give the range that curr is in by marking
	  // a bit flag. Let the CPU figure out which one it wants to merge
	  // also enables us to do more than one collision
	  collided_with |= (recip_dist > .000000001) << (curr >> NOFB_BIT_SIZE);

	  double ds = recip_dist*recip_dist*recip_dist * masses[curr] * GRAV_CONST;
	  // a = m1 * G / d^2
	  deltaX += dX * ds;
	  deltaY += dY * ds;
	  deltaZ += dZ * ds;
	}
	vX[my_planet] -= deltaX * TIME_STEP;
	vY[my_planet] -= deltaY * TIME_STEP;
	vZ[my_planet] -= deltaZ * TIME_STEP;
	collisions[my_planet] = collided_with;
      }
#pragma acc kernels async(1) wait(2)
      for(int curr = 0; curr < nofB; ++curr){
	pX[curr] += vX[curr] * TIME_STEP;
	pY[curr] += vY[curr] * TIME_STEP;
	pZ[curr] += vZ[curr] * TIME_STEP;
      }
    }

    //handle collisions
#pragma acc update host(collisions[0:nofB])
    int has_updated = 0;
    int bot = nofB;
    for (int i = 0; i < nofB; ++i){
      while (collisions[i] != 0){
	if (!has_updated){
#pragma acc update host(vX[0:nofB],\
    vY[0:nofB],\
    vZ[0:nofB],\
    pX[0:nofB],\
    pY[0:nofB],\
    pZ[0:nofB])
	  has_updated = 1;
	}
	int p1 = i;
	int p2;
	double minLength = 2e100;
	// find which body i collided with
	for (int j = 0; j < nofB; ++j){
	  if ((j >> NOFB_BIT_SIZE) & collisions[i] == 0)
	    continue;
	  collisions[i] ^= 1 << (j >> NOFB_BIT_SIZE);
	  double dX = pX[i] - pX[j];
	  double dY = pY[i] - pY[j];
	  double dZ = pZ[i] - pZ[j];
	  double dist;
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
	// shift everything and decrement nofB so we
	// never look at it anymore
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
	if (p1 < bot)
	  bot = p1;
	--nofB;
      }
    }
    if (has_updated){
#pragma acc update device(nofB, \
    pX[bot:nofB],\
    pY[bot:nofB],\
    pZ[bot:nofB],\
    vX[bot:nofB],\
    vY[bot:nofB],\
    vZ[bot:nofB],\
    masses[bot:nofB])
    }
  }
  std::cout << nofB<< "\n";
}
