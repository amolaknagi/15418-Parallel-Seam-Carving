Information on Amolak Nagi and James Mackaman's Seam Carving Submission.


There are a few directories in this one. I added a couple older versions
that we used for benchmarks:

- final_parallel_implementation
	THIS IS OUR ACTUAL SUBMISSION. This is our final code that we timed our results on.

- unoptimized_sequential
	This is the version that took roughly 122 seconds with our benchmark in our final
	report. This was a baseline sequential version with absolutely no optimizations made
	
- optimized sequential
	This is the version that took roughly 81 seconds and which we used as a benchmark for
	our parallelization. The main optimization here was in the ACM generation step. 

All of these directories are set up exactly the same way. The following instructions
to run our implementation apply to all 3 of these directories. 

THIS PROJECT IS MEANT TO BE RUN ON THE LATEDAYS CLUSTER WITH THE XEON PHI.

A couple notes:
- This was our OpenMP implementation, so we based it off of our
  Assignment 3 submission. For simplicity's sake with the Makefile, we kept the name of 
  the file holding our main algorithm to be wireroute.cpp. 

  INSTRUCTIONS TO RUN OUR IMPLEMENTATION:

- We had a lot of trouble getting our code to properly read a PNG as RGB into our program.
  We were able to do so, but the libPNG library that we used would not compile with the
  compiler that would offload our program onto the Xeon Phi. For this reason, we used a simple
  workaround of first turning an image into a text file of rgb, running our implementation
  which outputs a text file of rgb, and then turning that rgb text file back into a PNG file.
  I wrote a script (in each directory) that does the conversion of PNG to .txt, it should already 
  be compiled, but if you would like to recompile it you can run:

  $ g++ /usr/lib64/libpng.so png_rgb_file_conversion.cpp 

  Here is how you can properly run our implementation with a PNG file that you have:
  	1.) Compile our file conversion script using the command above
  	2.) Run the following to turn your PNG image into a text file
  			$ ./a.out i2txt YOUR_IMAGE_NAME.png
  		This will output a file called image_rgb.txt
  	3.) In order to make our implementation properly read the image_rgb.txt
  		the file must be in your HOME directory. You can do so with the following
  			$ cp image_rgb.txt ~/
  	4.) Now you can run the implementation. If you'd like to set the thread count,
  	    you can do so in jobs/batch_generate.sh. ONLY SET ONE THREADCOUNT PER SUBMISSION,
  	    DO NOT LIST MORE THAN ONE AND TRY TO SUBMIT. IT WILL CAUSE CONFLICTING WRITES
  	    TO THE OUTPUTTED IMAGE FILE AND NOT WORK.
  	5.) Run the following commands
  			$ make clean
  			$ make
  			$ make submit
  	6.) When the job is done, the timing and relevant print statements and timing will be 
  	    outputted in a file in the latedays directory. The outputted image will be called
  	    outputImage.txt IN YOUR HOME DIRECTORY.
  	7.) To convert the text file to PNG, run my script like this:
  			$ ./a.out txt2i ~/outputImage.txt
  		This will output an image in your directory called output.png with your result.

- For the sake of simplicity, the image I used for my final report is in the directory titled 
  as image1.png. I would've included the already generated image_rgb.txt file, but it wouldn't
  fit in our autolab submission.
- By default, the number of seams to be removed is 960 (half the width of a 1080p image which
  was our benchmark for our project). If you'd liek to change this, it is defined as #define SEAM_COUNT
  at the top of wireroute.cpp