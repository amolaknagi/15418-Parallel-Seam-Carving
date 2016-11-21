#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <stdint.h>
#include <string>
#include "libpng.cpp"
#include "CycleTimer.h"

double *energyCuda(uint8_t *r, uint8_t *g, uint8_t *b, int width, int height);
void printCudaInfo();

#define INDEX(row, col, width) (((row) * (width)) + (col))


// return GB/s
float toBW(int bytes, float sec) {
  return static_cast<float>(bytes) / (1024. * 1024. * 1024.) / sec;
}


void usage(const char* progname) {
    printf("Usage: %s [options]\n", progname);
    printf("Program Options:\n");
    printf("  -n  --arraysize <INT>  Number of elements in arrays\n");
    printf("  -?  --help             This message\n");
}

double *energySeq(uint8_t *r, uint8_t *g, uint8_t *b, int width, int height) {
    double *energy = (double *)calloc(width * height, sizeof(double));

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {

            // For simplicity, if we're on a border, just set the energy to 1.
            if (row == 0 ||
                row == height - 1 ||
                col == 0 ||
                col == width - 1) {
                energy[INDEX(row, col, width)] = 1;
                continue;
            }

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
    }

    return energy;
}


int main(int argc, char** argv)
{
    image_t inputImage = png_to_rgb("MyPic.png");

    printCudaInfo();


    // First perform our implementation with CUDA, timing is done within this method.
    double *energy_matrix = energyCuda(inputImage.r, inputImage.g, inputImage.b, inputImage.width, inputImage.height);

    double startTime = CycleTimer::currentSeconds();
    double *energy_matrix_seq = energySeq(inputImage.r, inputImage.g, inputImage.b, inputImage.width, inputImage.height);
    double endTime = CycleTimer::currentSeconds();
    double seqDuration = endTime - startTime;
    printf("Seq Duration: %.3f ms\n", 1000.f * seqDuration);


    // Perform a quick check to see that our sequential implementation matches
    // our parallel implementation.
    // Note that we do plan on looking at a faster implementation that is an
    // approximation, but for now we're looking for exact matching.
    bool parallelEquiv = true;
    for (int i = 0; i < inputImage.width * inputImage.height; i++) {
        if (energy_matrix[i] != energy_matrix_seq[i]) {
            parallelEquiv = false;
            break;
        }
    }
    if (parallelEquiv) {
        printf("Sequential matches parallel!\n");
    } else {
        printf("Sequential does not match parallel!\n");
    }


    // Output our image to a PNG image called "output.png"
    image_t outputImage;
    outputImage.width = inputImage.width;
    outputImage.height = inputImage.height;

    uint8_t *outputR = (uint8_t*)calloc(inputImage.width * inputImage.height, sizeof(uint8_t));
    uint8_t *outputG = (uint8_t*)calloc(inputImage.width * inputImage.height, sizeof(uint8_t));
    uint8_t *outputB = (uint8_t*)calloc(inputImage.width * inputImage.height, sizeof(uint8_t));

    for (int i = 0; i < inputImage.width * inputImage.height; i++) {
        double thisEnergyValue = energy_matrix[i];
        uint8_t pixelValue = (uint8_t)(thisEnergyValue * ((double)(255)));
        outputR[i] = pixelValue;
        outputG[i] = pixelValue;
        outputB[i] = pixelValue;
    }
    outputImage.r = outputR;
    outputImage.g = outputG;
    outputImage.b = outputB;
    rgb_to_image(outputImage);


    // Free all memory
    free(outputR);
    free(outputG);
    free(outputB);
    free(energy_matrix);
    free(energy_matrix_seq);

    return 0;
}