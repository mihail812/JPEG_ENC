#ifndef PREPROCESSING_H
#define PREPROCESSING_H

#include <CL/cl.hpp>
#include "tables.h"
#include "opencl.h"

void preprocess(OpenCLContext &ctx, unsigned char quality);

void createQuantTbl(OpenCLContext &ctx, unsigned char quality);

void createQTbl(OpenCLContext &ctx, int table_idx, unsigned char scale, const unsigned int *base_table);

void createHuffmanTables(OpenCLContext &ctx);

void addHuffTbl(OpenCLContext &ctx, huffman_table *tblptr, const unsigned char *bits, const unsigned char *values);

void createDCTdivTbl(OpenCLContext &ctx);

void createDHuffTbl(OpenCLContext &ctx);

void DHuffTbl(huffman_table *htblptr, derived_huffman_table *dhtblptr);
#endif // PREPROCESSING_H
