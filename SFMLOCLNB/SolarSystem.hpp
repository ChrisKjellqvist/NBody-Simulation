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
#include <OpenCL/opencl.h>
typedef struct body {
    float velocityX;
    float velocityY;
    float positionX;
    float positionY;
    float mass;
} Body;

Body* body_new_random();
Body* body_new_with_params(float vX, float vY, float pX, float pY, float mass);
int getScreenPosX(float ratio, Body* body);
int getScreenPosY(float ratio, Body* body);

#endif /* SolarSystem_h */
