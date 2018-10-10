#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <CL/cl.hpp>
#include "tables.h"
#include "opencl.h"

void compressData(OpenCLContext &ctx);

void do_entropy(OpenCLContext &ctx, short *mcu_buffer[0x6], std::vector<char>& outputbuf, entropy_state_t& state);

void entropy_single_data(OpenCLContext &ctx, short *block, int table_index, int last_dc_val, std::vector<char>& outputbuf, entropy_state_t& state);

#endif // COMPRESSION_H
