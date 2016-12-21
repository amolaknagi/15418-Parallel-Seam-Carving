//g++ /usr/lib64/libpng.so png_rgb_file_conversion.cpp 

#include "libpng.cpp"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>


int main(int argc, const char *argv[])
{


  if (!strcmp(argv[1], "i2txt")) {
      // IMAGE TO TXT
    //char imageName[] = "MyPic.png";
    image_t inputImage = png_to_rgb((char*)argv[2]);

    int width = inputImage.width;
    int height = inputImage.height;

    // Let's write this data out to a file called image_rgb.txt
    FILE *outputImageFile = fopen("image_rgb.txt", "w");
    if (!outputImageFile) return -1;

    fprintf(outputImageFile,"%d %d\n", width, height);
    for (int i = 0; i < width * height; i++) {
     fprintf(outputImageFile,"%d %d %d\n", inputImage.r[i], inputImage.g[i], inputImage.b[i]);
    }
  } else if (!strcmp(argv[1], "txt2i")) {
    char imageName[] = "~/outputImage.txt";
    FILE *inputImageFile = fopen(argv[2], "r");
    int width, height;
    fscanf(inputImageFile, "%d %d\n", &width, &height);

    printf("Width: %d Height: %d\n", width, height);

    uint8_t *r = (uint8_t *)calloc(width * height, sizeof(uint8_t));
    uint8_t *g = (uint8_t *)calloc(width * height, sizeof(uint8_t));
    uint8_t *b = (uint8_t *)calloc(width * height, sizeof(uint8_t));
    int rval, gval, bval;
    int i = 0;
    while (fscanf(inputImageFile, "%d %d %d\n", &rval, &gval, &bval) != EOF) {
      r[i] = (uint8_t)rval;
      g[i] = (uint8_t)gval;
      b[i] = (uint8_t)bval;
      i++;
    }

    image_t image;
    image.r = r;
    image.g = g;
    image.b = b;
    image.width = width;
    image.height = height;

    rgb_to_image(image);
  } 


  // TXT TO IMAGE
  // Let's read the text file
  

  // fprintf(outputCostsFile, "%d %d\n", dim_x, dim_y);
  // for (int row = 0; row < dim_y; row++) {
  //   for (int col = 0; col < dim_x; col++) {
  //     int thisIndex = costArrayIndexFromXY(col, row, dim_x);
  //     fprintf(outputCostsFile, "%d ", (int)costs[thisIndex]);
  //   }
  //   fprintf(outputCostsFile, "\n");
  // }
  // fclose(outputImageFile);

  return true;
	return 0;
}
