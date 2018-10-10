#ifndef OPENCL_ENCODE_H
#define OPENCL_ENCODE_H

#include <CL/cl.hpp>
#include "tables.h"
#include "opencl.h"


void do_opencl_encode(OpenCLContext &ctx);

void rgb2yuv(OpenCLContext &ctx);

void downsample(OpenCLContext &ctx);

void dctquant(OpenCLContext &ctx);

void delUnusedPart(OpenCLContext &ctx);

void cpBackToHost(OpenCLContext &ctx);
#endif // OPENCL_ENCODE_H
