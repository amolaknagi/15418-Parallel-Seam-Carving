/**
 * Seam Carving Parallelization in OpenMP on Xeon Phi
 * Amolak Nagi and James Mackaman
 * 
 * This is an optimized sequential implementation of the seam-carving algorithm
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

void writePixelsToOutput(pixel *pixels, int width, int height) {

}


// Helper function to calculate the energy of each pixel in a
// provided image. Takes in an array of pixels for which to calculate the
// image and writes the output to the provided energy array
// with corresponding indices.
void calculateEnergy(pixel *pixels, double *energy, int width, int height) {

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
               double energyValue = (((double)delta) / ((double)765));
               energy[INDEX(row, col, width)] = energyValue;
          }
     }
}

// Perform the ACM generation step of our algorithm. 
void calculateACM(double *acm, int width, int height) {
     for (int row = 2; row < height; row++) {

          double rowAbove[width];
          rowAbove[0] = DBL_MAX;
          for (int col = 1; col < (width - 1); col++) {
            rowAbove[col] = acm[INDEX(row-1, col, width)];
          }
          rowAbove[(width-1)] = DBL_MAX;


          for (int col = 1; col < width - 1; col++) {
               double upLeft = rowAbove[col-1];
              double up = rowAbove[col];
              double upRight = rowAbove[col+1];

              acm[INDEX(row, col, width)] = acm[INDEX(row, col, width)] + min(upLeft, up, upRight);

          }
     }
}

void generateSeam(double *acm, bool *seam, int cols, int rows) {

     // Now let's iterate through last row, find smallest col value
     double smallestVal = -1;
     double smallestCol = -1;
     for (int col = 1; col < cols - 1; col++) {
          double thisVal = acm[INDEX(rows-1, col, cols)];
          if (thisVal < smallestVal || smallestCol == -1) {
               smallestCol = col;
               smallestVal = thisVal;
          }
     }

     int upwardCol = smallestCol;

     // Start in the bottom row, iterate upwards
     for (int row = rows - 1; row >= 0; row--) {

          // Set the boolean value for this column in this row to be true
          seam[INDEX(row, upwardCol, cols)] = true;

          // Don't keep looking upward if in the top row
          if (row == 0) {
               continue;
          }

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

          double smallest = min(upLeft, up, upRight);

          if (smallest == upLeft) {
               upwardCol--;
          } else if (smallest == upRight) {
               upwardCol++;
          }
     }
}



// This is the step that removes the seam from our image given a helper
// temp_pixels array. This step is parallelized across rows.
void removeSeam(pixel *pixels, pixel *temp_pixels, bool *seam, int iterationWidth, int height) {

      // Now that we have teh seam, we should remove it from our image
      for (int row = 0; row < height; row++) {
        bool passedSeam = false;
        for (int col = 0; col < iterationWidth; col++) {
            if (seam[INDEX(row, col, iterationWidth)] == true) {
                passedSeam = true;
            } else {
                if (!passedSeam) {
                    temp_pixels[INDEX(row, col, (iterationWidth-1))].r = pixels[INDEX(row, col, iterationWidth)].r;
                    temp_pixels[INDEX(row, col, (iterationWidth-1))].g = pixels[INDEX(row, col, iterationWidth)].g;
                    temp_pixels[INDEX(row, col, (iterationWidth-1))].b = pixels[INDEX(row, col, iterationWidth)].b;
                } else {
                    temp_pixels[INDEX(row, (col-1), (iterationWidth-1))].r = pixels[INDEX(row, col, iterationWidth)].r;
                    temp_pixels[INDEX(row, (col-1), (iterationWidth-1))].g = pixels[INDEX(row, col, iterationWidth)].g;
                    temp_pixels[INDEX(row, (col-1), (iterationWidth-1))].b = pixels[INDEX(row, col, iterationWidth)].b;
                }
            }
        }
      }

        // We put our new image in the temp, let's copy the temp back to the pixels array
      iterationWidth--;

      memcpy(pixels, temp_pixels, sizeof(pixel) * iterationWidth * height);
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

  int width, height;
  // The first line of the image will be the dimensions.
  fscanf(input, "%d %d\n", &width, &height);

  pixel *pixels = (pixel *)calloc(width * height, sizeof(pixel));

  double *energy = (double *)calloc(width * height, sizeof(double));

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

  calculateEnergy(pixels, energy, width, height);

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

    omp_set_num_threads(num_of_threads);

    int iterationWidth = width;

    // Generate a general energy array
    double *energy = (double *)calloc(width * height, sizeof(double));

    // Generate a bool matrix for the seam. 
    bool *seam = (bool *)calloc(width * height, sizeof(bool));

    // Allocate some space for a temporary image, necessary to remove each seam
    pixel *temp_pixels = (pixel *)calloc(width * height, sizeof(pixel));

    // Create some timing measures
    double energy_time = 0;
    double acm_time = 0;
    double generate_time = 0;
    double remove_time = 0;

    for (int s = 0; s < SEAM_COUNT; s++) {

      auto energy_start = Clock::now();
      // First let's get the energy for the image
      calculateEnergy(pixels, energy, iterationWidth, height);
      energy_time += duration_cast<dsec>(Clock::now() - energy_start).count();


      auto acm_start = Clock::now();
      // Now let's get the ACM of this array
      calculateACM(energy, iterationWidth, height);
      acm_time += duration_cast<dsec>(Clock::now() - acm_start).count();
      

      auto generate_start = Clock::now();
      // Now that we have the ACM, let's generate the seam.
      generateSeam(energy, seam, iterationWidth, height);
      generate_time += duration_cast<dsec>(Clock::now() - generate_start).count();
      


      auto remove_start = Clock::now();
      // Now that we have the seam, we should remove it from our image
      removeSeam(pixels, temp_pixels, seam, iterationWidth, height);
      remove_time += duration_cast<dsec>(Clock::now() - remove_start).count();
      

      // Decrement our width, we have one less seam now
      iterationWidth--;

      // Reset seam to zero
      memset(seam, 0, sizeof(bool) * iterationWidth * height);
    }

    // Print our timing results
    printf("Energy Time: %lf.\n", energy_time);
    printf("ACM Time: %lf.\n", acm_time);
    printf("Generate Time: %lf.\n", generate_time);
    printf("Remove Time: %lf.\n", remove_time);

    //Free all our memory
    free(energy);
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
