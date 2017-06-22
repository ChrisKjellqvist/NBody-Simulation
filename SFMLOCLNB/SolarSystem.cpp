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

Body* body_new_random() {
    Body* b = (Body*)malloc(sizeof(Body));
    b->mass = ((double)(rand())/RAND_MAX) * (double)(5e24);

    b->positionX = ((double)(rand())/RAND_MAX) * SOLAR_SYSTEM_DIAMETER *.5+SOLAR_SYSTEM_DIAMETER*.25;
    b->positionY = ((double)(rand())/RAND_MAX) * SOLAR_SYSTEM_DIAMETER *.5+SOLAR_SYSTEM_DIAMETER*.25;
    b->velocityX = 10000*((double)(rand())/RAND_MAX);
    b->velocityY = ((double)(rand())/RAND_MAX)*1000;
        return b;
}
Body* body_new_with_params(float vX, float vY, float pX, float pY, float mass){
    Body* b = (Body*)malloc(sizeof(Body));
    b->mass = mass;
    b->positionY = pY;
    b->positionX = pX;
    b->velocityX = vX;
    b->velocityY = vY;
    return b;
}
int getScreenPosX(float ratio, Body* body){
    return body->positionX * ratio;
}
int getScreenPosY(float ratio, Body* body){
    return body->positionY * ratio;
}

