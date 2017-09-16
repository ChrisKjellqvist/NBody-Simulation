
#include <SFML/Graphics.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <OpenCL/opencl.h>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <thread>
#include "SolarSystem.hpp"
#include "test.cl.h"
#define WINDOW_SIZE 1000
#define WAIT 0
#define KILL 1
int status = WAIT;
std::condition_variable cv;
std::mutex mut;

int nofB = 512;
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
void enqueueOnDevice(float* pX, float* pY, float* vX, float* vY, float* masses, int nofB){
    char name[128];
    dispatch_queue_t queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_GPU, NULL);
    if (queue == NULL) {
        queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_CPU, NULL);
    }
    cl_device_id gpu = gcl_get_device_id_with_dispatch_queue(queue);
    clGetDeviceInfo(gpu, CL_DEVICE_NAME, 128, name, NULL);
    fprintf(stdout, "Created a dispatch queue using the %s\n", name);
    void* cl_pX = gcl_malloc(sizeof(cl_float) * nofB, pX, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_pY = gcl_malloc(sizeof(cl_float) * nofB, pY, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_vX = gcl_malloc(sizeof(cl_float) * nofB, vX, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_vY = gcl_malloc(sizeof(cl_float) * nofB, vY, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_masses = gcl_malloc(sizeof(cl_float) * nofB, masses, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_nofB = gcl_malloc(sizeof(cl_int), &nofB, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    dispatch_sync(queue, ^{
        size_t wgs;
        gcl_get_kernel_block_workgroup_info(iteratePositionsVectors_kernel, CL_KERNEL_WORK_GROUP_SIZE, sizeof(wgs), &wgs, NULL);
        cl_ndrange range = {1, {0, 0, 0}, {(size_t)nofB, 0, 0}, {wgs, 0, 0}};
        for (int j = 0; j < 2000 && !status; j++) {
            for (int i = 0; i < 50; i++) {
                iterateVelocityVectors_kernel(&range, (cl_float*)cl_pX, (cl_float*)cl_pY, (cl_float*)cl_vX, (cl_float*)cl_vY, (cl_float*)cl_masses, (cl_int*)cl_nofB);
                iteratePositionsVectors_kernel(&range, (cl_float*)cl_pX, (cl_float*)cl_pY, (cl_float*)cl_vY, (cl_float*)cl_vX);
            }
            std::lock_guard<std::mutex> lck(mut);
            gcl_memcpy(pX, cl_pX, sizeof(cl_float) * nofB);
            gcl_memcpy(pY, cl_pY, sizeof(cl_float) * nofB);
            cv.notify_all();
            if (j%200==0) {
                printf("%d\n", j);
            }
        }
    });
    printf("exited");
    status = KILL;
    gcl_free(cl_pX);
    gcl_free(cl_pY);
    gcl_free(cl_vX);
    gcl_free(cl_vY);
    gcl_free(cl_masses);
    dispatch_release(queue);
}
int main(int, char const**) {
    sf::RenderWindow window(sf::VideoMode(WINDOW_SIZE, WINDOW_SIZE), "SFML window");
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
        for (int p = 0; p < nofB; ++p) {
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
