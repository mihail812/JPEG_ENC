#ifndef _JPEG_ENCODER_
#define _JPEG_ENCODER_

#include <CL/cl.hpp>
#include <string>
#include <fstream>
#include <streambuf>
#include <vector>
#include <cstdio>
#include "tables.h"
#include "opencl.h"
#include <iostream>


int encodeImage(RGBImage &image, unsigned char quality, const char *file);

#endif
