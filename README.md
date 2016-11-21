# 15418-Parallel-Seam-Carving

Amolak Nagi and James Mackaman

This is the code repository for our sequential and parallel implementations of our seam carving algorithm. Each implementation has a respective directory.

Here's what each directory is:

/seq
    -This is our starting sequential code. As of now this uses OpenCV and was created just for the sake of figuring out how the algorithm works. It's a proof of concept.
/seq/libpng.cpp
    -We had a lot of trouble figuring out which library was appropriate to convert an image into readable RGB values in C++ code. We started with OpenCV to read JPEG, but for the sake of time we eventually experimented with a library known as libpng to read PNG files. When we get more time, we'll switch back to OpenCV so that we can try our implementation with live video.
/seq/seq.cpp
    - Our main seq implementation. Again, just a proof of concept.



/pll_energy
    - The directory for our parallel implementation of the energy calculation. We used libpng to read one PNG file and output a visual representation of the energy. We did this with CUDA. Our code compiled runs the sequential energy computation, the parallel energy computation, compares them for correctness, and then outputs an "output.png" file with the parallel visual representation of energy.
/pll_energy/CycleTimer.h
    - Header file used for timing. Taken from 15-418 Assignment 2
/pll_energy/libpng.cpp
    - File included in our main implementation that handles PNG -> RGB arrays and vice versa. Implemented with libpng library, available on GHC36 machine.
/pll_energy/main.cpp
    - Our main file. This file has the code for the sequential energy computation, calls our CUDA method, compares both implementation for correctness, and outputs an output.png file with the parallel visual representation.
/pll_energy/energy.cu
    - This file holds the kernel for our parallel implementation. Called from our main.cpp file.



