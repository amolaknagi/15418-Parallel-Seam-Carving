//#include "opencv2/highgui/highgui.hpp"

// This is my command line, though for now it's pretty 
// g++ /usr/local/lib/libopencv_core.dylib /usr/local/lib/libopencv_ml.dylib /usr/local/lib/libopencv_video.dylib /usr/local/lib/libopencv_highgui.dylib test.cpp
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <cstdio>

#define RINDEX(row, col, width) ((width * (row) * 3) + (3 * (col)) + 2)
#define GINDEX(row, col, width) ((width * (row) * 3) + (3 * (col)) + 1)
#define BINDEX(row, col, width) ((width * (row) * 3) + (3 * (col)) + 0)
#define INDEX(row, col, width)  ((width * (row)) + (col))

#define SEAM_COUNT 500


using namespace cv;
using namespace std;

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

bool contains(int *seamStartCols, int col) {
     for (int i = 0; i < SEAM_COUNT; i++) {
          if (seamStartCols[i] == col) return true;
     }
     return false;
}

void calculateEnergy(unsigned char *input, double *energy, int cols, int rows) {
     for (int row = 0; row < rows; row++) {
          for (int col = 0; col < cols; col++) {

               if (row == 0 || row == rows - 1 ||
                    col == 0 || col == cols - 1) {
                    energy[INDEX(row,col,cols)] = 1;
                    continue;                    
               }

               int rDown = input[RINDEX(row+1, col, cols)];
               int gDown = input[GINDEX(row+1, col, cols)];
               int bDown = input[BINDEX(row+1, col, cols)];

               int rUp = input[RINDEX(row-1, col, cols)];
               int gUp = input[GINDEX(row-1, col, cols)];
               int bUp = input[BINDEX(row-1, col, cols)];

               int rLeft = input[RINDEX(row, col-1, cols)];
               int gLeft = input[GINDEX(row, col-1, cols)];
               int bLeft = input[BINDEX(row, col-1, cols)];

               int rRight = input[RINDEX(row, col+1, cols)];
               int gRight = input[GINDEX(row, col+1, cols)];
               int bRight = input[BINDEX(row, col+1, cols)];

               int rdy = abs(rUp - rDown);
               int gdy = abs(gUp - gDown);
               int bdy = abs(bUp - bDown);

               int rdx = abs(rRight - rLeft);
               int gdx = abs(gRight - gLeft);
               int bdx = abs(bRight - bLeft);

               int rDelta = rdy + rdx;
               int gDelta = gdy + gdx;
               int bDelta = bdy + bdx;

               // The maximum delta is 3 * (255 + 255)
               // which is 1530
               int delta = rDelta + gDelta + bDelta;
               double energyValue = (((double)delta) / ((double)1530));
               energy[INDEX(row, col, cols)] = energyValue;
          }
     }
}

void calculateACM(double *acm, int cols, int rows) {
     for (int row = 2; row < rows; row++) {
          for (int col = 1; col < cols - 1; col++) {

               // If we're in the leftmost column, disregard upLeft
               double upLeft;
               if (col == 1) {
                    upLeft = DBL_MAX;
               } else {
                    upLeft = acm[INDEX(row-1, col-1, cols)];
               }

               // Get the value right above
               double up = acm[INDEX(row-1, col, cols)];


               // If we're in the rightmost column, disregard upright
               double upRight;// = acm[INDEX(row-1, col+1, width)];
               if (col == (cols - 2)) {
                    upRight = DBL_MAX;
               } else {
                    upRight = acm[INDEX(row-1, col+1, cols)];
               }

               acm[INDEX(row, col, cols)] = acm[INDEX(row, col, cols)] + min(upLeft, up, upRight);
          }
     }
}

void generateSeam(double *acm, bool *seam, int cols, int rows) {

     // Now let's iterate through last row, find smallest acm value
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



int main( int argc, const char** argv )
{
     Mat img = imread("MyPic.JPG", CV_LOAD_IMAGE_UNCHANGED); //read the image data in the file "MyPic.JPG" and store it in 'img'


     int rows = img.rows;
     int cols = img.cols;
     int originalCols = cols;
     printf("Image width: %d\n", cols);
     printf("Image height: %d\n", rows);
     printf("Image step: %d\n", (int)img.step);

     // long double *energy = (long double*)calloc(rows * cols, sizeof(long double));
     int *seamStartCols = (int*)calloc(SEAM_COUNT, sizeof(int));
     for (int i = 0; i < SEAM_COUNT; i++) {
          seamStartCols[i] = -1;
     }


     // This is the data of all the pixels in the array.
     // Each pixel has 3 bytes, b, g, r, in consecutive order.
     // So the size of this input is 3 * width * height.
     unsigned char *input = (unsigned char*)(img.data);

     // Let's try our energy function. For simplicity sake, let's just
     // disregard the border pixels.
     // unsigned char *output = (unsigned char*)calloc(3 * rows * cols, sizeof(unsigned char));
     

     // Create our accumulated cost matrix. Make it a copy of our energy matrix
     // except for borders which will have max energy
     double *acm;
     // Allocate a mask for a seam

     double *energy;

     // The energy array will always keep the same dimensions of the original image



     // Generate SEAM_COUNT seams
     for (int s = 0; s < SEAM_COUNT; s++) {
          printf("seam:%d\n", s);
          // First, with our current width, calculate the energy of this image.
          energy = (double *)malloc(rows * cols * sizeof(double));
          calculateEnergy(input, energy, cols, rows);

          // Now, let's calculate our acm matrix.
          calculateACM(energy, cols, rows);

          // With our ACM, let's calculate a boolean matrix of our seam
          // The true values indicate a seam.
          bool *seam = (bool*)calloc(rows * cols, sizeof(bool));
          generateSeam(energy, seam, cols, rows);

          // Now that we have this seam, we should remove it from our image
          // Let's create a new image and update our input
          unsigned char *newImage = (unsigned char*)calloc(3 * rows * (cols - 1), sizeof(unsigned char));
          for (int row = 0; row < rows; row++) {
               bool passedSeam = false;
               for (int col = 0; col < cols; col++) {
                    // If this index is a seam, mark it as passed
                    if (seam[INDEX(row, col, cols)] == true) {
                         passedSeam = true;
                    // If not...
                    } else {
                         if (!passedSeam) {
                              newImage[RINDEX(row, col, (cols-1))] = input[RINDEX(row, col, cols)];
                              newImage[GINDEX(row, col, (cols-1))] = input[GINDEX(row, col, cols)];
                              newImage[BINDEX(row, col, (cols-1))] = input[BINDEX(row, col, cols)];
                         } else {
                              newImage[RINDEX(row, (col-1), (cols-1))] = input[RINDEX(row, col, cols)];
                              newImage[GINDEX(row, (col-1), (cols-1))] = input[GINDEX(row, col, cols)];
                              newImage[BINDEX(row, (col-1), (cols-1))] = input[BINDEX(row, col, cols)];
                         }
                    }
               }
          }

          free(energy);
          if (s != 0)
               free(input);
          free(seam);
          input = newImage;
          cols--;
     }
     




     img.cols = cols;
     img.data = input;



     // // Draw energy to our image output
     // for (int row = 0; row < rows; row++) {
     //      for (int col = 0; col < cols; col++) {
     //           int newRVal = energy[INDEX(row, col, originalCols)] * ((long double)255);

     //           input[RINDEX(row, col, originalCols)] = newRVal;
     //           input[GINDEX(row, col, originalCols)] = newRVal;
     //           input[BINDEX(row, col, originalCols)] = newRVal;
     //      }
     // }



     if (img.empty()) //check whether the image is loaded or not
     {
          cout << "Error : Image cannot be loaded..!!" << endl;
          //system("pause"); //wait for a key press
          return -1;
     }

     namedWindow("MyWindow", CV_WINDOW_AUTOSIZE); //create a window with the name "MyWindow"
     imshow("MyWindow", img); //display the image which is stored in the 'img' in the "MyWindow" window

     waitKey(0); //wait infinite time for a keypress

     destroyWindow("MyWindow"); //destroy the window with the name, "MyWindow"

     return 0;
}
