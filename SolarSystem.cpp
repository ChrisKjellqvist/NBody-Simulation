//
//  SolarSystem.c
//  openCLTest
//
//  Created by Chris Kjellqvist on 2017-06-20.
//  Copyright © 2017 Chris Kjellqvist. All rights reserved.
//

#include "SolarSystem.hpp"
#include <stdlib.h>
#include <inttypes.h>

Body::Body() {
  mass = ((double)(rand())/RAND_MAX) * (double)(5e24);
  positionX = ((double)(rand())/RAND_MAX) * SOLAR_SYSTEM_DIAMETER;
  positionY = ((double)(rand())/RAND_MAX) * SOLAR_SYSTEM_DIAMETER;
  positionZ = ((double)(rand())/RAND_MAX) * SOLAR_SYSTEM_DIAMETER;
  velocityX = 100000*((double)(rand())/RAND_MAX)-50000;
  velocityY = 100000*((double)(rand())/RAND_MAX)-50000;
  velocityZ = 100000*((double)(rand())/RAND_MAX)-50000;
}
