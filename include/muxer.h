#ifndef JPEG_H
#define JPEG_H

#include <cstdio>
#include <vector>

#include <CL/cl.hpp>
#include "tables.h"
#include "opencl.h"

void prepareImage(OpenCLContext &ctx);

void checkImg(OpenCLContext &ctx);

void writeHeader(std::vector<char>& output_buf);

void writeByte(std::vector<char>& output_buf, int value);

void writeMarker(std::vector<char>& output_buf, int value);

void writeQuantTable(OpenCLContext &ctx, std::vector<char>& output_buf);

void writeSof(std::vector<char>& output_buf, size_t w, size_t h);

void writeHuffmanTable(OpenCLContext &ctx, std::vector<char> &output_buf);

void writeHuffTbl(OpenCLContext &ctx, std::vector<char>& output_buf, int index, unsigned char is_ac);

void writeSoS(std::vector<char> &output_buf);

void writeTrailer(OpenCLContext &ctx);
#endif // JPEG_H
