//
//  SolarSystem.h
//  openCLTest
//
//  Created by Chris Kjellqvist on 2017-06-20.
//  Copyright © 2017 Chris Kjellqvist. All rights reserved.
//

#ifndef SolarSystem_hpp
#define SolarSystem_hpp
#define SOLAR_SYSTEM_DIAMETER 30000000000

#include <stdio.h>

class Body {
  public:
    float velocityX;
    float velocityY;
    float velocityZ;
    float positionX;
    float positionY;
    float positionZ;
    float mass;
    Body();
};

#endif /* SolarSystem_h */
