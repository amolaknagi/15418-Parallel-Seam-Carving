/**
 * Seam Carving Parallelization in OpenMP on Xeon Phi
 * Amolak Nagi and James Mackaman
 * 
 * This is a parallelized implementation of the seam-carving algorithm
 * meant to run on the Latedays cluster on the Xeon Phi. Information
 * on the implementation details are in the project report.
 *
 * For simplicity, we chose to make the energy values of the edges of the
 * image 1 and just disregard them when generating seams. This helped to 
 * simplify or parallelization and minimize edge cases.
 *
 * Information regarding how to run this code is available in the README.
 *
 * If you would like to change the number of seams to remove from the
 * provided image, you can set the #define SEAM_COUNT below
 */

#include "wireroute.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <assert.h>
#include <algorithm>
#include <cfloat>
#include <omp.h>
#include "mic.h"

#define INDEX(row, col, width)  ((width * (row)) + (col))

// You can set this variable to be however many seams you'd like
// to remove from your algorithm.
#define SEAM_COUNT 960

// Simple data structure to contain a pixel in our image.
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} pixel;




// Simple helper function to find the minimum of 3 doubles
// Helpful for our ACM generation.
double min(double d1, double d2, double d3) {
     double max = d3;
     if (d2 < max) {
          max = d2;
     }
     if (d1 < max) {
          max = d1;
     }
     return max;
}


static int _argc;
static const char **_argv;


const char *get_option_string(const char *option_name,
			      const char *default_value)
{
  for (int i = _argc - 2; i >= 0; i -= 2)
    if (strcmp(_argv[i], option_name) == 0)
      return _argv[i + 1];
  return default_value;
}


int get_option_int(const char *option_name, int default_value)
{
  for (int i = _argc - 2; i >= 0; i -= 2)
    if (strcmp(_argv[i], option_name) == 0)
      return atoi(_argv[i + 1]);
  return default_value;
}



// Helper function to calculate the energy of each pixel in a
// provided image. Takes in an array of pixels for which to calculate the
// image and writes the output to the provided energy array
// with corresponding indices.
void calculateEnergy(pixel *pixels, double *energy, int width, int height) {

     #pragma omp parallel for
     for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {

            // For simplicity, make all edges 1
            if ((row == 0) || 
                (row == (height - 1)) ||
                (col == 0) || 
                (col == (width - 1))) {
                    energy[INDEX(row,col,width)] = 1;
                    continue;                    
            }

           // Determine the dx for each color channel
           int rLeft = pixels[INDEX(row, col-1, width)].r;
           int gLeft = pixels[INDEX(row, col-1, width)].g;
           int bLeft = pixels[INDEX(row, col-1, width)].b;

           int rRight = pixels[INDEX(row, col+1, width)].r;
           int gRight = pixels[INDEX(row, col+1, width)].g;
           int bRight = pixels[INDEX(row, col+1, width)].b;

           int rdx = abs(rRight - rLeft);
           int gdx = abs(gRight - gLeft);
           int bdx = abs(bRight - bLeft);

           // The maximum delta is 3 * (255)
           // which is 765
           int delta = rdx + gdx + bdx;

           // Compute and set our energy value.
           double energyValue = (((double)delta) / ((double)765));
           energy[INDEX(row, col, width)] = energyValue;
        }
    }
}



// Helper function to calculate the energy of the pixels along a seam in a
// provided image. Takes in an array of pixels for which to calculate the
// image and writes the output to the provided energy array
// with corresponding indices. Also takes in the seam along  which to compute energies
void calculateEnergyAlongSeam(pixel *pixels, double *energy, int *seam, int width, int height) {

    #pragma omp parallel for
    for (int row = 0; row < height; row++) {
        int colRemoved = seam[row];

        // We choose to recompute the 5 energies around
        // the column of the seam in this row
        int lowerBound = colRemoved - 2;
        int upperBound = colRemoved + 2;
        for (int col = lowerBound; col <= upperBound; col++) {

            // Disregard if we're out of bounds.
            if ((row < 0) || 
                (row > (height - 1)) ||
                (col < 0) || 
                (col > (width - 1))) {
                    continue;                    
            }

            // For simplicity, make all edges 1
            if ((row == 0) || 
                (row == (height - 1)) ||
                (col == 0) || 
                (col == (width - 1))) {
                    energy[INDEX(row,col,width)] = 1;
                    continue;                    
            }

            // Calculate the dx around this pixel for each channel
            int rLeft = pixels[INDEX(row, col-1, width)].r;
            int gLeft = pixels[INDEX(row, col-1, width)].g;
            int bLeft = pixels[INDEX(row, col-1, width)].b; 

            int rRight = pixels[INDEX(row, col+1, width)].r;
            int gRight = pixels[INDEX(row, col+1, width)].g;
            int bRight = pixels[INDEX(row, col+1, width)].b;

            int rdx = abs(rRight - rLeft);
            int gdx = abs(gRight - gLeft);
            int bdx = abs(bRight - bLeft);

            // The maximum delta is 3 * (255)
            // which is 765
            int delta = rdx + gdx + bdx;

            // Compute and set our energy value.
            double energyValue = (((double)delta) / ((double)765));
            energy[INDEX(row, col, width)] = energyValue;

        }
    }
}





// Given a region of pixels, calculate the ACM (accumulated cost matrix)
// for that region. This is used as a helper to be called by each thread for
// our spatial decomposition.
// low is inclusive, high is exclusive.
void calculateACMForRegion(double *acm, int width, int height, int rowLow, int rowHigh) {

    // Iterate through our region of rows
    for (int row = rowLow; row < rowHigh; row++) {

        // As a cache optimization, read all the values in the above row FIRST
        double rowAbove[width];
        rowAbove[0] = DBL_MAX;
        for (int col = 1; col < (width - 1); col++) {
            rowAbove[col] = acm[INDEX(row-1, col, width)];
        }
        rowAbove[(width-1)] = DBL_MAX;

        // Once we have our row above, perform the acm calculation
        // for this row. Disregard the edges of the image when looking
        // at columns.
        for (int col = 1; col < (width - 1); col++) {

            double upLeft = rowAbove[col-1];
            double up = rowAbove[col];
            double upRight = rowAbove[col+1];

            acm[INDEX(row, col, width)] = acm[INDEX(row, col, width)] + min(upLeft, up, upRight);
        }
    }
}


// Perform the ACM generation step of our algorithm. This is parallelized
// into tasks by OpenMP
void calculateACM(double *acm, int width, int height) {
    
    // Get our number of threads as well as the height of
    // each vertical region, called separationPoint.
    int maxThreads = omp_get_max_threads();
    int separationPoint = height / maxThreads;

    #pragma omp parallel num_threads(omp_get_max_threads())
    {
        // Get the index of this thread.
        int threadNum = omp_get_thread_num();

        // Calculate the region for which we want this thread to generate the ACM
        int low;
        int high;

        // If we're doing the top region, make sure low is row 2
        if (threadNum == 0) {
            low = 2;
            high = separationPoint;

        // If we're doing the bottom region, make sure high is our lowest row (exclusive)
        } else if (threadNum == (maxThreads - 1)) {
            low = threadNum * separationPoint;
            high = height;

        // Otherwise, just perform a multiplier calculation of our region based on 
        // our thread index.
        } else {
            low = threadNum * separationPoint;
            high = (threadNum + 1) * separationPoint;
        }

        calculateACMForRegion(acm, width, height, low, high);
    }
}


// Generates an integer array representing the cheapest seam we can remove.
// This is optimized by using an average of the lowest row of every vertical
// region (explained thoroughly in project report).
void generateSeam(double *acm, int *seam, int cols, int rows) {

    // Initialize an array representing the averages of all the 
    // bottom rows of each vertical region.
    double rowAverages[cols];
    int maxThreads = omp_get_max_threads();
    int separationPoint = rows / maxThreads;
    for (int i = 0; i < cols; i++) {
        rowAverages[i] = (double)0.0;
    }

    // Iterate through every vertical region
    for (int tid = 0; tid < maxThreads; tid++) {
        // Determine the row we want to look at based on our thread
        int separationRow = (tid == (maxThreads - 1)) ? (rows - 1)
                                                      : ((tid * separationPoint) - 1);
    
        // For our row, sum each column's corresponding ACM value
        for (int col = 1; col < cols - 1; col++) {
            rowAverages[col] += acm[INDEX(separationRow, col, cols)];
        }
    }

    // Now we can iterate through our columns and average out our sums based on our thread count.
    for (int col = 1; col < cols - 1; col++) {
        rowAverages[col] = (rowAverages[col] / ((double)maxThreads));
    }

    
    // We can now use rowAverages to determine the column root of our seam.
    double smallestVal = -1;
    double smallestCol = -1;
    for (int col = 1; col < cols - 1; col++) {
        double thisVal = rowAverages[col];
        if (thisVal < smallestVal || smallestCol == -1) {
            smallestCol = col;
            smallestVal = thisVal;
        }
    }

    int upwardCol = smallestCol;

    // Start in the bottom row, iterate upwards
    for (int row = rows - 1; row >= 0; row--) {

        // Keep track of the column we're at in this row
        seam[row] = upwardCol;

         // Don't keep looking upward if in the top row
        if (row == 0) {
            continue;
        }

        // Retrieve the ACM values upLeft, up, and upRight
        double upLeft;
        if (upwardCol == 1) {
            upLeft = DBL_MAX;
        } else {
            upLeft = acm[INDEX(row-1, upwardCol-1, cols)];
        }

        double up = acm[INDEX(row-1, upwardCol, cols)];

        double upRight;
        if (upwardCol == (cols - 2)) {
            upRight = DBL_MAX;
        } else {
            upRight = acm[INDEX(row-1, upwardCol+1, cols)];
        }


        // Retrieve the minimum of upLeft, up, and upRight.
        double smallest = min(upLeft, up, upRight);

        // Increase or decrease our column based on whicheer
        // pixel was the smallest.
        if (smallest == upLeft) {
            upwardCol--;
        } else if (smallest == upRight) {
            upwardCol++;
        }
     }
}


// This is the step that removes the seam from our image given a helper
// temp_pixels array. This step is parallelized across rows.
void removeSeam(pixel *pixels, pixel *temp_pixels, int *seam, int iterationWidth, int height) {

    // Now that we have the seam, we should remove it from our image
    #pragma omp parallel for 
    for (int row = 0; row < height; row++) {
        bool passedSeam = false;
        int colToRemove = seam[row];
        pixel rowPixels[iterationWidth];

        // Until we encounter the seam column, just copy our image.
        // Once we do, copy the pixels shifted over left by one.
        for (int col = 0; col < iterationWidth; col++) {
            if (col < colToRemove) {
                rowPixels[col].r = pixels[INDEX(row, col, iterationWidth)].r;
                rowPixels[col].g = pixels[INDEX(row, col, iterationWidth)].g;
                rowPixels[col].b = pixels[INDEX(row, col, iterationWidth)].b;
            } else if (col > colToRemove) {
                rowPixels[col-1].r = pixels[INDEX(row, col, iterationWidth)].r;
                rowPixels[col-1].g = pixels[INDEX(row, col, iterationWidth)].g;
                rowPixels[col-1].b = pixels[INDEX(row, col, iterationWidth)].b;
            }
        }

        memcpy(&temp_pixels[INDEX(row, 0, (iterationWidth-1))], rowPixels, (iterationWidth-1) * sizeof(pixel));
    }

    iterationWidth--;

    memcpy(pixels, temp_pixels, sizeof(pixel) * iterationWidth * height);
}


// This function is identical to that of the removeSeam function except that
// it removes the seam from our energy matrix rather than from the image.
void removeSeamFromEnergy(double *energy, double *temp_energy, int *seam, int iterationWidth, int height) {

    // Now that we have the seam, we should remove it from our image
    #pragma omp parallel for 
    for (int row = 0; row < height; row++) {
        bool passedSeam = false;
        int colToRemove = seam[row];

        // Until we encounter the seam column, just copy our image.
        // Once we do, copy the pixels shifted over left by one.
        for (int col = 0; col < iterationWidth; col++) {
            if (col < colToRemove) {
                temp_energy[INDEX(row, col, (iterationWidth-1))] = energy[INDEX(row, col, iterationWidth)];
            } else if (col > colToRemove) {
                temp_energy[INDEX(row, (col-1), (iterationWidth-1))] = energy[INDEX(row, col, iterationWidth)];
            }
        }
    }

    iterationWidth--;

    memcpy(energy, temp_energy, sizeof(double) * iterationWidth * height);
}





int main(int argc, const char *argv[])
{
  using namespace std::chrono;
  typedef std::chrono::high_resolution_clock Clock;
  typedef std::chrono::duration<double> dsec;

  auto init_start = Clock::now();
  double init_time = 0;
 
  _argc = argc - 1;
  _argv = argv + 1;

  const char *input_filename = get_option_string("-f", NULL);
  int num_of_threads = get_option_int("-n", 1);

  
  printf("Number of threads: %d\n", num_of_threads);
  printf("Input file: %s\n", input_filename);


  // Read each line of our provided image text file
  // line by line.
  FILE *input = fopen(input_filename, "r");

  if (!input) {
    printf("Unable to open file: image_rgb.txt.\n");
    return 1;
  }

  // The first line of the image will be the dimensions.
  int width, height;
  fscanf(input, "%d %d\n", &width, &height);

  pixel *pixels = (pixel *)calloc(width * height, sizeof(pixel));

  // The rest of the image will be the pixel values at each cooordinate.
  int rval, gval, bval;
  int i = 0;
  while (fscanf(input, "%d %d %d\n", &rval, &gval, &bval) != EOF) {
    pixels[i].r = (uint8_t)rval;
    pixels[i].g = (uint8_t)gval;
    pixels[i].b = (uint8_t)bval;
    i++;
  }

  fclose(input);
  
  printf("Width: %d, Height: %d, i: %d\n", width, height, i);



  init_time += duration_cast<dsec>(Clock::now() - init_start).count();
  printf("Initialization Time: %lf.\n", init_time);


  auto compute_start = Clock::now();
  double compute_time = 0;

  #ifdef RUN_MIC /* Use RUN_MIC to distinguish between the target of compilation */

  /* This pragma means we want the code in the following block be executed in 
   * Xeon Phi.
   */
#pragma offload target(mic) \
  inout(pixels: length(width * height) INOUT)
#endif
  {
    // Set our thread count.
    omp_set_num_threads(num_of_threads);

    int iterationWidth = width;

    // Generate a general energy array
    double *energy = (double *)calloc(width * height, sizeof(double));
    double *acm = (double *)calloc(width * height, sizeof(double));

    // Generate a bool matrix for the seam. 
    int *seam = (int *)calloc(height, sizeof(int));

    // Allocate some space for a temporary image, necessary to remove each seam
    pixel *temp_pixels = (pixel *)calloc(width * height, sizeof(pixel));
    double *temp_energy = (double *)calloc(width * height, sizeof(double));

    // Create some timing measures
    double acm_time = 0;
    double generate_time = 0;
    double remove_time = 0;

    // Let's generate the overall energy matrix first once
    // For this optimization, let's see what happens if we just 
    // calculate the overall energy once, use it, remove the seam
    // from the energy, and then just recalculate along the seam rather than the whole thing.
    calculateEnergy(pixels, energy, iterationWidth, height);

    for (int s = 0; s < SEAM_COUNT; s++) {

      // Copy our energy matrix to our ACM matrix and compute the ACM
      memcpy(acm, energy, sizeof(double) * iterationWidth * height);
      auto acm_start = Clock::now();
      // Now let's get the ACM of this array
      calculateACM(acm, iterationWidth, height);
      acm_time += duration_cast<dsec>(Clock::now() - acm_start).count();
      

      auto generate_start = Clock::now();
      // Now that we have the ACM, let's generate the seam.
      generateSeam(acm, seam, iterationWidth, height);
      generate_time += duration_cast<dsec>(Clock::now() - generate_start).count();
      


      auto remove_start = Clock::now();
      // Now that we have the seam, we should remove it from our image AND the energy matrix
      removeSeam(pixels, temp_pixels, seam, iterationWidth, height);
      removeSeamFromEnergy(energy, temp_energy, seam, iterationWidth, height);
      remove_time += duration_cast<dsec>(Clock::now() - remove_start).count();
      
      // Now we should calculate the energy only along the seam

      // Decrement our width, we have one less seam now
      iterationWidth--;

      calculateEnergyAlongSeam(pixels, energy, seam, iterationWidth, height);

    }

    // Print our timing results
    printf("ACM Time: %lf.\n", acm_time);
    printf("Generate Time: %lf.\n", generate_time);
    printf("Remove Time: %lf.\n", remove_time);

    //Free all our memory
    free(energy);
    free(temp_energy);
    free(acm);
    free(seam);
    free(temp_pixels);

  }



  compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
  printf("Computation Time: %lf.\n", compute_time);

  int newWidth = width - SEAM_COUNT;
  

  // Write a new outputImage.txt file with our resulting image.
  FILE *outputFile = fopen("outputImage.txt", "w");
  fprintf(outputFile, "%d %d\n", newWidth, height);
  for (int i = 0; i < newWidth * height; i++) {
    fprintf(outputFile, "%d %d %d\n", (int)pixels[i].r, (int)pixels[i].g, (int)pixels[i].b);
  }
  fclose(outputFile);

  free(pixels);
  return 0;
}
