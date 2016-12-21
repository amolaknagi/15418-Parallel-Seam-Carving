/*
 * Hey James,
 * This is just a simple file that has two functions.
 * One reads a PNG file and turns it into this image_t struct that I defined.
 * The other does vice versa, outputting a new PNG file (by default the output is
 * called output.png).
 *
 * By default, the main function will just read a PNG file you give in the command
 * line with ./a.out [FILENAME], read it in as RGB, and then rewrite it out
 * as "output.png". It's just meant to exemplify the two methods I mentioned above.
 *
 * We can use this on GHC36 no sweat. Just run g++ /usr/lib64/libpng.so test.cpp (the name of this file)
 * No worries about header paths since the libpng library I'm using is already
 * installed.
 *
 * Let's spend today (Sunday) working this into the CUDA code so we can 
 * at least parallelize the matrix operations.
 *
 * Also quick note, I wrote this rather quickly so I may have forgotten to free
 * memory in a couple places, but I'll clean that up before we post our code.
 */

#include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include <stdint.h>

typedef struct image_pixels {
     int width;
     int height;
     uint8_t *r;
     uint8_t *g;
     uint8_t *b;
} image_t;



image_t png_to_rgb(char *filename) {
  FILE *fp = fopen(filename, "rb");

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png) abort();

  png_infop info = png_create_info_struct(png);
  if(!info) abort();

  if(setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  png_read_info(png, info);

  int width      = png_get_image_width(png, info);
  int height     = png_get_image_height(png, info);
  png_byte color_type = png_get_color_type(png, info);
  png_byte bit_depth  = png_get_bit_depth(png, info);
  (void)color_type;
  (void)bit_depth;

  printf("width:%d\n", width);
  printf("height:%d\n", height);

  uint8_t *r = (uint8_t*)calloc(width * height, sizeof(uint8_t));
  uint8_t *g = (uint8_t*)calloc(width * height, sizeof(uint8_t));
  uint8_t *b = (uint8_t*)calloc(width * height, sizeof(uint8_t));

  // png_read_update_info(png, info);
  png_bytep row_pointers[height];

  // row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }

  png_read_image(png, row_pointers);

  for (int row = 0; row < height; row++) {

     // Get the pointer for this row
     png_byte *thisRowPointer = (png_byte*)row_pointers[row];

     for (int col = 0; col < width; col++) {
          int colIndex = 3 * col;
          r[(row * width) + col] = thisRowPointer[colIndex];
          g[(row * width) + col] = thisRowPointer[colIndex + 1];
          b[(row * width) + col] = thisRowPointer[colIndex + 2];
     }
  }

  for(int y = 0; y < height; y++) {
    free(row_pointers[y]);// = (png_byte*)malloc(png_get_rowbytes(png,info));
  }

  fclose(fp);

  image_t pngImage;
  pngImage.width = width;
  pngImage.height = height;
  pngImage.r = r;
  pngImage.g = g;
  pngImage.b = b;

  return pngImage;
}

void rgb_to_image(image_t image) {
     // First initialize our png struct, info, and rows
     png_structp png = NULL;
     png_infop info = NULL;
     png_byte *rowBytes;//[image.width * 3];
     FILE *fp = NULL;

     fp = fopen("output.png", "wb");
     if (fp == NULL) {
          printf("Error writing PNG image\n");
          return;
     }


     png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
     if (png == NULL) {
          printf("Error writing PNG image\n");
          return;
     }

     info = png_create_info_struct(png);
     if (info == NULL) {
          printf("Error writing PNG image\n");
          return;
     }

     png_init_io(png, fp);

     png_set_IHDR(png, info, image.width, image.height,
               8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

     png_text title_text;
     title_text.compression = PNG_TEXT_COMPRESSION_NONE;
     char key[] = "Title";
     char text[] = "output.png";
     title_text.key = key;
     title_text.text = text;
     png_set_text(png, info, &title_text, 1);

     png_write_info(png, info);


     rowBytes = (png_byte*)malloc(3 * image.width * sizeof(png_byte));

     for (int row = 0; row < image.height; row++) {
          for (int col = 0; col < image.width; col++) {
               // int colIndex = col * 3;
               rowBytes[(col * 3)]   = (png_byte)image.r[(image.width * row) + col];
               rowBytes[(col * 3) + 1] = (png_byte)image.g[(image.width * row) + col];
               rowBytes[(col * 3) + 2] = (png_byte)image.b[(image.width * row) + col];
          }
          png_write_row(png, (png_bytep)rowBytes);
     }

     png_write_end(png, NULL);
}
