# JPEG_ENC

OpenCL-based encoder from PPM(P6) to JPG

# How to compile

- Change PATHTOLIB variable in [Makefile](https://github.com/nafarya/JPEG_ENC/blob/master/Makefile) to your path to OpenCL library

- make

# How to use:

Encoder accepts 3 arguments: inputImage(PPM-P6), quality(0-100), outputImage.

- ./encode input.ppm quality output.jpg
