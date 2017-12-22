
#include <SFML/Graphics.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <OpenCL/opencl.h>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <thread>
#include "SolarSystem.hpp"
#include "test.cl.h"

#define WINDOW_SIZE 1500
#define WAIT 0
#define KILL 1
int status = WAIT;
std::condition_variable cv;
std::mutex mut;

int nofB = 512;
int lostBodies = 0;
/*
 * convert position in space to position on the screen. We do this by defining boundaries at
 * 0 and "realsize," and converting that to boundaries at 0 and screenDim. Whenever I get 
 * collisions happen, I can easily add a translation so you can look around with the arrow
 * keys, but that'll come later. i
 */
int getScreenPos(float realPos, float realSize, int screenDim){
    int q = (realPos / realSize)*screenDim;
    if (q < 0) {
        q = 0;
    }
    if (q > screenDim) {
        q = screenDim;
    }
    return q;
}

/*
 * Find a GPU device (if you don't have one, grab a CPU), and run the algorithm on it. If it
 * finishes, then it'll kill the drawing process, and if the drawing process is killed by,
 * for instance, exiting the window, it will also end this process.
 */
void enqueueOnDevice(float* pX, float* pY, float* vX, float* vY, float* masses, int nofB){
    char name[128];
    dispatch_queue_t queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_GPU, NULL);
    if (queue == NULL) {
        queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_CPU, NULL);
    }
    cl_device_id gpu = gcl_get_device_id_with_dispatch_queue(queue);
    clGetDeviceInfo(gpu, CL_DEVICE_NAME, 128, name, NULL);
    fprintf(stdout, "Created a dispatch queue using the %s\n", name);
    int* masks = (int*)malloc(sizeof(int)*nofB);
    for (int i = 0; i < nofB; i++) {
        masks[i] = -1;
    }
    cl_int* cl_nofB = (cl_int*)gcl_malloc(sizeof(cl_int), &nofB, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    dispatch_sync(queue, ^{
        cl_float* cl_pX = (cl_float*)gcl_malloc(sizeof(cl_float) * nofB, pX, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
        cl_float* cl_pY = (cl_float*)gcl_malloc(sizeof(cl_float) * nofB, pY, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
        cl_float* cl_vX = (cl_float*)gcl_malloc(sizeof(cl_float) * nofB, vX, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
        cl_float* cl_vY = (cl_float*)gcl_malloc(sizeof(cl_float) * nofB, vY, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
        cl_float* cl_masses = (cl_float*)gcl_malloc(sizeof(cl_float) * nofB, masses, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
        cl_int* cl_masks = (cl_int*)gcl_malloc(sizeof(cl_int) * nofB, masks, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
        size_t wgs;
        gcl_get_kernel_block_workgroup_info(iteratePositionsVectors_kernel, CL_KERNEL_WORK_GROUP_SIZE, sizeof(wgs), &wgs, NULL);
        cl_ndrange range = {1, {0, 0, 0}, {(size_t)nofB, 0, 0}, {wgs, 0, 0}};
        for (int j = 0; j < 10000 && !status; j++) {
            gcl_memcpy(pX, cl_pX, sizeof(cl_float) * nofB);
            gcl_memcpy(pY, cl_pY, sizeof(cl_float) * nofB);
            gcl_memcpy(masks, cl_masks, sizeof(cl_float)*nofB);
            for (int i = 0; i < 50; i++) {
                iterateVelocityVectors_kernel(&range, cl_pX, cl_pY, cl_vX, cl_vY, cl_masses, cl_nofB, cl_masks);
                iteratePositionsVectors_kernel(&range, cl_pX, cl_pY, cl_vY, cl_vX, cl_masks);
                int triggered = 0;
                for (int i = 0; i < nofB - lostBodies; i++) {
                    if (masks[i]==-2) {
                        triggered = 1;
                        int b2 = -1;
                        int b1 = i;
                        for (int j = i+1; j < nofB-lostBodies; j++) {
                            if (masks[j] == -2) {
                                b2 = j;
                                j = nofB-lostBodies;
                            }
                        }
                        if (b2 == -1) {
                            break;
                        }
                        printf("collision %d\n", nofB-lostBodies);
//                        masks[b1] = -1;
//                        masks[b2] = -1;
                        masses[b1] = masses[b1]+masses[b2];
                        vX[b1] = (vX[b1]*masses[b1]+vX[b2]*masses[b2])/masses[b1];
                        vY[b1] = (vY[b1]*masses[b1]+vY[b2]*masses[b2])/masses[b1];
                        lostBodies+=1;
                        for (int i = 0, j = 0; i < nofB; i++) {
                            if (i == b2)
                                continue;
                            pX[j] = pX[i];
                            pY[j] = pY[i];
                            vX[j] = vX[i];
                            vY[j] = vY[i];
                            masks[j] = masks[i];
                            masses[j] = masses[i];
                            ++j;
                        }
                        gcl_free(cl_pX);
                        gcl_free(cl_pY);
                        gcl_free(cl_vX);
                        gcl_free(cl_vY);
                        gcl_free(cl_masks);
                        gcl_free(cl_masses);
                        for (int j = nofB-lostBodies; j < nofB; j++) {
                            masks[j] = 0;
                        }
                        cl_pX = (cl_float*)gcl_malloc(sizeof(cl_float) * nofB, pX, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
                        cl_pY = (cl_float*)gcl_malloc(sizeof(cl_float) * nofB, pY, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
                        cl_vX = (cl_float*)gcl_malloc(sizeof(cl_float) * nofB, vX, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
                        cl_vY = (cl_float*)gcl_malloc(sizeof(cl_float) * nofB, vY, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
                        cl_masses = (cl_float*)gcl_malloc(sizeof(cl_float) * nofB, masses, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
                        cl_masks = (cl_int*)gcl_malloc(sizeof(cl_int) * nofB, masks, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
                    }
                }
                if (triggered) {
                    for (int i = 0; i < nofB; i++) {
                        masks[i] = masks[i] | (masks[i] >> 1);
                    }
                }
            }
            std::lock_guard<std::mutex> lck(mut);
//        }
            cv.notify_all();
        }
        printf("leaving gpu\n");
        cv.notify_all();
        gcl_free(cl_pX);
        gcl_free(cl_pY);
        gcl_free(cl_vX);
        gcl_free(cl_vY);
        gcl_free(cl_masses);
        gcl_free(cl_nofB);
    });
    status = KILL;
    printf("kill status %d\n", status == KILL);
    cv.notify_all();
    dispatch_release(queue);
}
int main(int, char const**) {
    sf::RenderWindow window(sf::VideoMode(WINDOW_SIZE, WINDOW_SIZE), "N Body");
    srand((uint32_t)time(0));
    int i;
    Body** space = (Body**)malloc(sizeof(Body*)*nofB);
    for (int j = 0; j < nofB; j++) {
        space[j] = body_new_random();
    }
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
    pX[nofB] = 0;
    std::thread gpuThread(enqueueOnDevice, pX, pY, vX, vY, masses, nofB);
    sf::Image image;
    sf::Texture texture;
    int go = 1;
    while (window.isOpen() | go) {
        sf::Event event;
        while (window.pollEvent(event) | (status && go)){
            if ((event.type == sf::Event::Closed) | status){
                window.close();
                go = 0;
                status = KILL;
            }
        }
        std::unique_lock<std::mutex> lock(mut);
        cv.wait(lock);
        window.clear(sf::Color::Black);
        image.create(WINDOW_SIZE, WINDOW_SIZE);
        for (int p = 0; (p < nofB - lostBodies) & !status; ++p) {
            int x = getScreenPos(pX[p], SOLAR_SYSTEM_DIAMETER, WINDOW_SIZE);
            int y = getScreenPos(pY[p], SOLAR_SYSTEM_DIAMETER, WINDOW_SIZE);
            image.setPixel(x, y, sf::Color::White);
        }
        lock.unlock();
        cv.notify_all();
        texture.loadFromImage(image);
        sf::Sprite bufferSprite(texture);
        window.draw(bufferSprite);
        window.display();
    }
    gpuThread.join();
    free(pX);
    free(pY);
    free(vX);
    free(vY);
    free(masses);
    return EXIT_SUCCESS;
}
