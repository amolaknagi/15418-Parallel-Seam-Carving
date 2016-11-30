#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <stdint.h>
#include <float.h>
#include <string>
#include "libpng.cpp"
#include "CycleTimer.h"

double *energyCuda(uint8_t *r, uint8_t *g, uint8_t *b, int width, int height);
double *acmCuda(double *energy, int width, int height);
void printCudaInfo();

#define INDEX(row, col, width) (((row) * (width)) + (col))

double min(long double d1, long double d2, long double d3) {
     double max = d3;
     if (d2 < max) {
          max = d2;
     }
     if (d1 < max) {
          max = d1;
     }
     return max;
}

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



double *acmSeq(double *energy, int width, int height) {

    // Initialize our acm matrix
    double *acm = (double *)calloc(width * height, sizeof(double));

    // Initialize it so that the borders have an energy of 1, but 
    // everything else will just be a copy of the energy array
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int index = INDEX(row,col,width);
            acm[index] = energy[index];
        }
    }

    // Now, let's perform the acm calculation

    for (int row = 2; row < height; row++) {
        for (int col = 1; col < width - 1; col++) {

            double upLeft;
            if (col == 1) {
                upLeft = DBL_MAX;
            } else {
                upLeft = acm[INDEX(row-1, col-1, width)];
            }
            // double upLeft = acm[INDEX(row-1, col-1, width)];
            double up = acm[INDEX(row-1, col, width)];


            double upRight;// = acm[INDEX(row-1, col+1, width)];
            if (col == width - 2) {
                upRight = DBL_MAX;
            } else {
                upRight = acm[INDEX(row-1, col+1, width)];
            }

            acm[INDEX(row, col, width)] = acm[INDEX(row,col,width)] + min(upLeft,up,upRight);
        }
    }

    return acm;
}


int main(int argc, char** argv)
{
    image_t inputImage = png_to_rgb("MyPic.png");

    printCudaInfo();


    // First perform our implementation with CUDA, timing is done within this method.
    double *energy_matrix = energyCuda(inputImage.r, inputImage.g, inputImage.b, inputImage.width, inputImage.height);
    

    double startTime = CycleTimer::currentSeconds();
    double *acm_seq = acmSeq(energy_matrix, inputImage.width, inputImage.height);
    double endTime = CycleTimer::currentSeconds();
    double seqDuration = endTime - startTime;
    printf("Seq Duration: %.3f ms\n", 1000.f * seqDuration);

    double *acm_pll = acmCuda(energy_matrix, inputImage.width, inputImage.height);

    // Output our image to a PNG image called "output.png"
    image_t outputImage;
    outputImage.width = inputImage.width;
    outputImage.height = inputImage.height;

    uint8_t *outputR = (uint8_t*)calloc(inputImage.width * inputImage.height, sizeof(uint8_t));
    uint8_t *outputG = (uint8_t*)calloc(inputImage.width * inputImage.height, sizeof(uint8_t));
    uint8_t *outputB = (uint8_t*)calloc(inputImage.width * inputImage.height, sizeof(uint8_t));

    for (int i = 0; i < inputImage.width * inputImage.height; i++) {
        double thisEnergyValue = acm_pll[i];
        uint8_t pixelValue = (uint8_t)(thisEnergyValue * ((double)(255)));
        outputR[i] = pixelValue;
        outputG[i] = pixelValue;
        outputB[i] = pixelValue;
    }

    for (int i = inputImage.height - 1; i > 0; i--) {
        double thisEnergyValue = acm_pll[INDEX()]
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

    return 0;
}