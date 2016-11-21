#include <stdio.h>
#include <stdint.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <driver_functions.h>

#include "CycleTimer.h"

#define INDEX(row, col, width) (((row) * (width)) + (col))

extern float toBW(int bytes, float sec);

__global__ void
energy_kernel(uint8_t *r, uint8_t *g, uint8_t *b, double *energy, int width, int height, int blocksWide, int blocksHigh) {

    // This is index of block
    int blockX = blockIdx.x;
    int blockY = blockIdx.y;

    // This is the dimension of the block we're in
    int blockWidth = blockDim.x;
    int blockHeight = blockDim.y;

    // These are the ranges of pixels for this block
    int startX = (blockX * blockWidth);
    int endX = startX + blockWidth;
    int startY = (blockY * blockHeight);
    int endY = startY + blockHeight;

    // Get the row and col that this thread is handling.
    int col = startX + threadIdx.x;
    int row = startY + threadIdx.y;
    // printf("row:%d\n", row);

    // If we're on the border, we'll just set the energy to 1 for simplicity
    // if (startY != 16)
        // printf("here\n");
    if (row == 0 || 
        row == (height - 1) ||
        col == 0 || 
        col == (width - 1)) {
        energy[INDEX(row, col, width)] = 1;
        // printf("blockWidth: %d, blockHeight: %d, blockX: %d, blockY: %d, startX: %d, startY: %d, row: %d col: %d\n", blockWidth, blockHeight, blockX, blockY, startX, startY, row, col);
        return;
    }

    if (row > (height - 1) || col > (width - 1)) 
        return;

    uint8_t rDown = r[INDEX(row+1, col, width)];
    uint8_t gDown = g[INDEX(row+1, col, width)];
    uint8_t bDown = b[INDEX(row+1, col, width)];

    uint8_t rUp = r[INDEX(row-1, col, width)];
    uint8_t gUp = g[INDEX(row-1, col, width)];
    uint8_t bUp = b[INDEX(row-1, col, width)];

    uint8_t rLeft = r[INDEX(row, col-1, width)];
    uint8_t gLeft = g[INDEX(row, col-1, width)];
    uint8_t bLeft = b[INDEX(row, col-1, width)];

    uint8_t rRight = r[INDEX(row, col+1, width)];
    uint8_t gRight = g[INDEX(row, col+1, width)];
    uint8_t bRight = b[INDEX(row, col+1, width)];
    // printf("%d\n", rRight);

    uint8_t rdy = (rUp > rDown) ? rUp - rDown : rDown - rUp;
    uint8_t gdy = (gUp > gDown) ? gUp - gDown : gDown - gUp;
    uint8_t bdy = (bUp > bDown) ? bUp - bDown : bDown - bUp;

    uint8_t rdx = (rRight > rLeft) ? rRight - rLeft : rLeft - rRight;
    uint8_t gdx = (gRight > gLeft) ? gRight - gLeft : gLeft - gRight;
    uint8_t bdx = (bRight > bLeft) ? bRight - bLeft : bLeft - bRight;

    uint16_t rDelta = ((uint16_t)rdy) + ((uint16_t)rdx);
    uint16_t gDelta = ((uint16_t)gdy) + ((uint16_t)gdx);
    uint16_t bDelta = ((uint16_t)bdy) + ((uint16_t)bdx);

   // The maximum delta is 3 * (255 + 255)
   // which is 1530
    uint16_t delta = rDelta + gDelta + bDelta;
    double energyValue = (((double)delta) / ((double)1530));
    energy[INDEX(row, col, width)] = energyValue;


}

double *
energyCuda(uint8_t *r, uint8_t *g, uint8_t *b, int width, int height) {

    uint8_t* device_r;
    uint8_t* device_g;
    uint8_t* device_b;

    double* device_energy;

    //
    // Allocate our r g b matrices for CUDA
    //
    cudaMalloc(&device_r, sizeof(uint8_t) * width * height);
    cudaMalloc(&device_g, sizeof(uint8_t) * width * height);
    cudaMalloc(&device_b, sizeof(uint8_t) * width * height);
    cudaMalloc(&device_energy, sizeof(double) * width * height);


    // // start timing after allocation of device memory
    double startTime = CycleTimer::currentSeconds();



    cudaMemcpy(device_r, r, sizeof(uint8_t) * width * height, cudaMemcpyHostToDevice);
    cudaMemcpy(device_g, g, sizeof(uint8_t) * width * height, cudaMemcpyHostToDevice);
    cudaMemcpy(device_b, b, sizeof(uint8_t) * width * height, cudaMemcpyHostToDevice);
    cudaMemset(device_energy, 0, sizeof(double) * width * height);


    // // run kernel

    int blockWidth = 16;
    int blockHeight = 16;
    dim3 blockDim(blockWidth, blockHeight);

    // Our block grid will be based on our blockWidth and blockHeight
    int blocksHigh = (int)(ceil((double)height / (double)blockHeight));
    int blocksWide = (int)(ceil((double)width / (double)blockWidth));
    dim3 gridDim(((blocksWide)), ((blocksHigh)));

    printf("Width: %d\n", width);
    printf("Height: %d\n", height);
    printf("blockWidth: %d\n", blockWidth);
    printf("blockHeight: %d\n", blockHeight);
    printf("blocksWide: %d\n", blocksWide);
    printf("blocksHigh: %d\n", blocksHigh);

    double startKernelTime = CycleTimer::currentSeconds();
    energy_kernel<<<gridDim, blockDim>>>(device_r, device_g, device_b, device_energy, width, height, blocksWide, blocksHigh);
    cudaThreadSynchronize();
    double endKernelTime = CycleTimer::currentSeconds();
    double *energy_result = (double*)malloc(sizeof(double) * width * height);
    cudaMemcpy(energy_result, device_energy, sizeof(double) * width * height, cudaMemcpyDeviceToHost);

    cudaFree(device_r);
    cudaFree(device_g);
    cudaFree(device_b);
    cudaFree(device_energy);

    double endTime = CycleTimer::currentSeconds();
    double overallDuration = endTime - startTime;
    printf("Overall: %.3f ms\n", 1000.f * overallDuration);


    return energy_result;
}

void
printCudaInfo() {

    // for fun, just print out some stats on the machine

    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);

    printf("---------------------------------------------------------\n");
    printf("Found %d CUDA devices\n", deviceCount);

    for (int i=0; i<deviceCount; i++) {
        cudaDeviceProp deviceProps;
        cudaGetDeviceProperties(&deviceProps, i);
        printf("Device %d: %s\n", i, deviceProps.name);
        printf("   SMs:        %d\n", deviceProps.multiProcessorCount);
        printf("   Global mem: %.0f MB\n",
               static_cast<float>(deviceProps.totalGlobalMem) / (1024 * 1024));
        printf("   CUDA Cap:   %d.%d\n", deviceProps.major, deviceProps.minor);
    }
    printf("---------------------------------------------------------\n");
}
