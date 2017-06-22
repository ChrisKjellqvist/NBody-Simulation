
//
// Disclaimer:
// ----------
//
// This code will work only if you selected window, graphics and audio.
//
// Note that the "Run Script" build phase will copy the required frameworks
// or dylibs to your application bundle so you can execute it on any OS X
// computer.
//
// Your resource files (images, sounds, fonts, ...) are also copied to your
// application bundle. To get the path to these resources, use the helper
// function `resourcePath()` from ResourcePath.hpp
//

#include <SFML/Graphics.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <OpenCL/opencl.h>
#include "SolarSystem.hpp"
#include "test.cl.h"
#include "ResourcePath.hpp"
#define WINDOW_SIZE 1000

float scale = WINDOW_SIZE / SOLAR_SYSTEM_DIAMETER;

int getScreenPosition(float realX, float scale, int translation){
    return realX * scale + translation;
}
int main(int, char const**) {
    sf::RenderWindow window(sf::VideoMode(WINDOW_SIZE, WINDOW_SIZE), "SFML window");
    sf::Image icon;
    if (!icon.loadFromFile(resourcePath() + "icon.png")) {
        return EXIT_FAILURE;
    }
    window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    srand((uint32_t)time(0));
    int i;
    char name[128];
    
    int nofB = 512;
    Body** space = (Body**)malloc(sizeof(Body*)*nofB);
    for (int j = 0; j < nofB; j++) {
        space[j] = body_new_random();
    }
    
    dispatch_queue_t queue =
    gcl_create_dispatch_queue(CL_DEVICE_TYPE_GPU, NULL);
    if (queue == NULL) {
        queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_CPU, NULL);
    }
    cl_device_id gpu = gcl_get_device_id_with_dispatch_queue(queue);
    clGetDeviceInfo(gpu, CL_DEVICE_NAME, 128, name, NULL);
    fprintf(stdout, "Created a dispatch queue using the %s\n", name);
    
    float* pX = (float*)malloc(sizeof(float)*nofB);
    float* pY = (float*)malloc(sizeof(float)*nofB);
    float* vX = (float*)malloc(sizeof(float)*nofB);
    float* vY = (float*)malloc(sizeof(float)*nofB);
    float* masses = (float*)malloc(sizeof(float)*nofB);
    for (int i = 0; i < nofB; i++) {
        pX[i] = space[i]->positionX;
        pY[i] = space[i]->positionY;
        vX[i] = space[i]->velocityX;
        vY[i] = space[i]->velocityY;
        masses[i] = space[i]->mass;
    }
    printf("%f\n", pX[0]);
    void* cl_pX = gcl_malloc(sizeof(cl_float) * nofB, pX, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_pY = gcl_malloc(sizeof(cl_float) * nofB, pY, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_vX = gcl_malloc(sizeof(cl_float) * nofB, vX, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_vY = gcl_malloc(sizeof(cl_float) * nofB, vY, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_masses = gcl_malloc(sizeof(cl_float) * nofB, masses, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_nofB = gcl_malloc(sizeof(cl_int), &nofB, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    
    dispatch_async(queue, ^{
        size_t wgs;
        gcl_get_kernel_block_workgroup_info(iteratePositionsVectors_kernel,
                                            CL_KERNEL_WORK_GROUP_SIZE,
                                            sizeof(wgs), &wgs, NULL);
        
        cl_ndrange range = {
            1,
            {0, 0, 0},
            
            {(size_t)nofB, 0, 0},
            {wgs, 0, 0}
        };
        for (int j = 0; j < 1000; j++) {
            for (int i = 0; i < 100; i++) {
                iterateVelocityVectors_kernel(&range, (cl_float*)cl_pX, (cl_float*)cl_pY, (cl_float*)cl_vX, (cl_float*)cl_vY, (cl_float*)cl_masses, (cl_int*)cl_nofB);
                iteratePositionsVectors_kernel(&range, (cl_float*)cl_pX, (cl_float*)cl_pY, (cl_float*)cl_vY, (cl_float*)cl_vX);
                
            }
            gcl_memcpy(pX, cl_pX, sizeof(cl_float) * nofB);
            gcl_memcpy(pY, cl_pY, sizeof(cl_float) * nofB);
        }
        printf("done\n");
        
    });
    sf::Image image;

    sf::Texture texture;
//    image.create(WINDOW_WIDTH, WINDOW_HEIGHT);

    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed){
                switch (event.key.code) {
                    case sf::Keyboard::A:
                        <#statements#>
                        break;
                        
                    default:
                        break;
                }
            }
        }
        
        window.clear(sf::Color::Black);
        
        image.create(WINDOW_SIZE, WINDOW_SIZE);
        for (int p = 0; p < nofB; p++) {
            int x = getScreenPosition(pX[p], scale, 0);(pX[p], SOLAR_SYSTEM_DIAMETER, WINDOW_SIZE);
            int y = getYPosition(pY[p], SOLAR_SYSTEM_DIAMETER, WINDOW_SIZE);
            image.setPixel(x, y, sf::Color::White);
        }
        // drawing uses the same functions
        
        // set the shape color to green
        // get the target texture (where the stuff has been drawn)

        texture.loadFromImage(image);
        sf::Sprite bufferSprite(texture);
        window.draw(bufferSprite);

        // end the current frame
        window.display();
    }

    
    gcl_free(cl_pX);
    gcl_free(cl_pY);
    gcl_free(cl_vX);
    gcl_free(cl_vY);
    gcl_free(cl_masses);
    
    
    free(pX);
    free(pY);
    free(vX);
    free(vY);
    free(masses);
    dispatch_release(queue);
    
    printf("got to the end");
    return EXIT_SUCCESS;
}
