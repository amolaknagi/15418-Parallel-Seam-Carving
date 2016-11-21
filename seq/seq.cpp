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
#define INDEX(row, col, width)  ((width * (row)) + col)

#define SEAM_COUNT 500


using namespace cv;
using namespace std;

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

bool contains(int *seamStartCols, int col) {
     for (int i = 0; i < SEAM_COUNT; i++) {
          if (seamStartCols[i] == col) return true;
     }
     return false;
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

     long double *energy = (long double*)calloc(rows * cols, sizeof(long double));
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
               long double energyValue = (((long double)delta) / ((long double)1530));
               energy[INDEX(row, col, cols)] = energyValue;
          }
     }


     // Create our accumulated cost matrix. Make it a copy of our energy matrix
     // except for borders which will have max energy
     long double *acm;
     // Allocate a mask for a seam
     bool *seam = (bool*)calloc(rows * cols, sizeof(bool));

     // The energy array will always keep the same dimensions of the original image

     // Generate SEAM_COUNT seams
     for (int s = 0; s < SEAM_COUNT; s++) {

          // First, let's clear up our acm matrix
          acm = (long double*)calloc(rows * cols, sizeof(long double));
          for (int row = 0; row < rows; row++) {
               for (int col = 0; col < cols; col++) {
                    int index = INDEX(row,col,cols);
                    if (row == 0 || row == rows - 1 || col == 0 || col == cols - 1) {
                         acm[index] = 1;
                    } else {
                         acm[index] = energy[INDEX(row,col,originalCols)];
                    }
               }
          }

          // Let's also reset our seam matrix
          for (int i = 0; i < rows * cols; i++) {
               seam[i] = false;
          }


          // Now let's generate our acm matrix
          for (int row = 2; row < rows; row++) {
               for (int col = 2; col < cols - 1; col++) {
                    long double upLeft = acm[INDEX(row-1, col-1, cols)];
                    long double up     = acm[INDEX(row-1, col,   cols)];
                    long double upRight= acm[INDEX(row-1, col+1, cols)];

                    acm[INDEX(row,col,cols)] = acm[INDEX(row,col,cols)] + min(upLeft,up,upRight);
                    // printf("value:%Lf\n", acm[INDEX(row,col,cols)]);
               }
          }



          // Find the smallest element in the last row
          long double smallestVal = -1;
          int smallestCol = -1;
          for (int col = 2; col < cols - 2; col++) {
               // long double thisVal = acm[INDEX(rows-2, col, cols)];
               long double thisVal = acm[INDEX(2, col, cols)];
               if (thisVal < smallestVal || smallestCol == -1) {
                    smallestVal = thisVal;
                    smallestCol = col;
               }
          }
          printf("seam:%d smallest:%d\n", s, smallestCol);


          // Iterate upwards to generate this seam
          // for (int row = rows - 2; row >= 0; row--) {
          for (int row = 1; row < rows; row++) {
               seam[INDEX(row, smallestCol, cols)] = true;
               if (row == 0) {
                    continue;
               }
               long double upLeft = acm[INDEX(row-1, smallestCol-1, cols)];
               long double up     = acm[INDEX(row-1, smallestCol,   cols)];
               long double upRight= acm[INDEX(row-1, smallestCol+1, cols)];

               long double smallest = min(upLeft, up, upRight);
               if (smallest == upLeft) {
                    smallestCol--;
               } else if (smallest == upRight) {
                    smallestCol++;
               }
               // printf("smallest:%d\n", smallestCol);
          }

          // Shrink our image width by 1 pixel
          for (int row = 0; row < rows; row++) {
               bool passedTrue = false;
               for (int col = 0; col < cols - 1; col++) {
                    bool isSeam = seam[INDEX(row, col, cols)];
                    if (isSeam) {
                         passedTrue = true;
                    }

                    if (passedTrue && col < cols-1) {
                         energy[INDEX(row,col,originalCols)] = energy[INDEX(row,col+1,originalCols)];
                    }

                    if (col == cols - 1) {
                         energy[INDEX(row,col,originalCols)] = 2;
                    }
               }
          }

          cols--;
     }
     








     // Draw energy to our image output
     for (int row = 0; row < rows; row++) {
          for (int col = 0; col < cols; col++) {
               int newRVal = energy[INDEX(row, col, originalCols)] * ((long double)255);

               input[RINDEX(row, col, originalCols)] = newRVal;
               input[GINDEX(row, col, originalCols)] = newRVal;
               input[BINDEX(row, col, originalCols)] = newRVal;
          }
     }



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
