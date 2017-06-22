
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

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <OpenCL/opencl.h>
#include "SolarSystem.hpp"
#include "test.cl.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 1000

// Here is a small helper for you! Have a look.
#include "ResourcePath.hpp"
int getXPosition(float realX, float sizeX, int width){
    int q = (int)((realX/sizeX)*width);
    if (q < 0)
        return 0;
    if (q > width) {
        return width-1;
    }
    printf("%d\n", q);
    return 0;
}
int getYPosition(float realY, float sizeY, int height){
    int q = (int)((realY/sizeY)*height);
    if (q < 0)
        return 0;
    if (q > height) {
        return height-1;
    }
    return 0;

}
int main(int, char const**)
{
    // Create the main window
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SFML window");

    // Set the Icon
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
    
    // Here we hardcode some test data.
    // Normally, when this application is running for real, data would come from
    // some REAL source, such as a camera, a sensor, or some compiled collection
    // of statistics—it just depends on the problem you want to solve.
    
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
    // Once the computation using CL is done, will have to read the results
    // back into our application's memory space.  Allocate some space for that.
    //    float* test_out = (float*)m /alloc(sizeof(cl_float) * NUM_VALUES);
    
    // The test kernel takes two parameters: an input float array and an
    // output float array.  We can't send the application's buffers above, since
    // our CL device operates on its own memory space.  Therefore, we allocate
    // OpenCL memory for doing the work.  Notice that for the input array,
    // we specify CL_MEM_COPY_HOST_PTR and provide the fake input data we
    // created above.  This tells OpenCL to copy the data into its memory
    // space before it executes the kernel.                               // 3
    void* cl_pX = gcl_malloc(sizeof(cl_float) * nofB, pX, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_pY = gcl_malloc(sizeof(cl_float) * nofB, pY, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_vX = gcl_malloc(sizeof(cl_float) * nofB, vX, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_vY = gcl_malloc(sizeof(cl_float) * nofB, vY, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_masses = gcl_malloc(sizeof(cl_float) * nofB, masses, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_nofB = gcl_malloc(sizeof(cl_int), &nofB, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    
    // Dispatch the kernel block using one of the dispatch_ commands and the
    // queue created earlier.                                            // 5
    dispatch_async(queue, ^{
        // Although we could pass NULL as the workgroup size, which would tell
        // OpenCL to pick the one it thinks is best, we can also ask
        // OpenCL for the suggested size, and pass it ourselves.
        size_t wgs;
        gcl_get_kernel_block_workgroup_info(iteratePositionsVectors_kernel,
                                            CL_KERNEL_WORK_GROUP_SIZE,
                                            sizeof(wgs), &wgs, NULL);
        
        // The N-Dimensional Range over which we'd like to execute our
        // kernel.  In this case, we're operating on a 1D buffer, so
        // it makes sense that the range is 1D.
        cl_ndrange range = {                                              // 6
            1,                     // The number of dimensions to use.
            
            {0, 0, 0},
            // The offset in each dimension.  To specify
            // that all the data is processed, this is 0
            // in the test case.                   // 7
            
            {(size_t)nofB, 0, 0},
            {wgs, 0, 0}
        };
//        for (int j = 0; j < 500; j++) {
            for (int i = 0; i < 10000; i++) {
                iterateVelocityVectors_kernel(&range, (cl_float*)cl_pX, (cl_float*)cl_pY, (cl_float*)cl_vX, (cl_float*)cl_vY, (cl_float*)cl_masses, (cl_int*)cl_nofB);
                iteratePositionsVectors_kernel(&range, (cl_float*)cl_pX, (cl_float*)cl_pY, (cl_float*)cl_vY, (cl_float*)cl_vX);
                
            }
            gcl_memcpy(pX, cl_pX, sizeof(cl_float) * nofB);
            gcl_memcpy(pY, cl_pY, sizeof(cl_float) * nofB);
//        }
        printf("done\n");
        
    });
    sf::Image image;

    sf::Texture texture;

    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();
        }
        
        // clear the window with black color
        window.clear(sf::Color::Black);
        image.create(WINDOW_WIDTH, WINDOW_HEIGHT);
        for (int p = 0; p < nofB; p++) {
            image.setPixel(getXPosition(pX[p], SOLAR_SYSTEM_DIAMETER, WINDOW_WIDTH), getYPosition(pY[p], SOLAR_SYSTEM_DIAMETER, WINDOW_HEIGHT), sf::Color::White);
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
    return EXIT_SUCCESS;
}
