#ifndef OPENCL_H
#define OPENCL_H

#define CL_SET_KERNEL_ARG(kernel, type, arg_num, arg)             \
    cle = kernel.setArg<type>(arg_num, arg);                      \
    if (cle != CL_SUCCESS) {                                      \
        printf("ERROR while setting kernel arg:%d", arg_num);     \
    }

#define CL_CREATE_READONLY_BUFFER(buffer, context, size) \
    buffer = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(size));

typedef struct derived_huffman_table
{
    unsigned int code[256];
    unsigned char length[256];
} derived_huffman_table;


typedef struct huffman_table
{
    unsigned char bits[17];
    unsigned char value[256];
} huffman_table;

struct entropy_state
{
    size_t buffer;
    int bits;
    int last_dc_val[0x3];
};
typedef struct entropy_state entropy_state_t;

struct quantification_table
{
    unsigned char value[0x40];
};
typedef struct quantification_table quantification_table_t;

struct OpenCLContext{
    FILE *fp;
    const char *file;
    cl_int width, height;
    unsigned char *image;

    std::vector<char> output_buffer;

    cl_uint nbw, nbh, nsbw, nsbh;

    short *y_buffer, *cb_buffer, *cr_buffer;

    quantification_table_t m_quant_tbls[2];
    short m_fdct_divisors[2][256];
    derived_huffman_table m_dc_derived_tbls[2];
    derived_huffman_table m_ac_derived_tbls[2];
    huffman_table m_dc_huff_tbls[2];
    huffman_table m_ac_huff_tbls[2];

    cl::Context context;
    cl::Device device;
    cl::CommandQueue queue;
    cl::Program prog;
    cl::Kernel kernel;

    cl::Buffer planeCb;
    cl::Buffer planeCr;
    cl::Buffer planeY;
    cl::Buffer image_buffer;
    cl::Buffer cctbl;
    cl::Buffer dctqnt_divs;
    cl::Buffer dctqnt_mlt;
    cl::Buffer dctqnt_sign;
    cl::Buffer dctqnt_ids;
    cl::Buffer dctqnt_desc;
    cl::Buffer dctqnt_bias;
};

#endif // OPENCL_H
