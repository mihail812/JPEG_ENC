#include "../include/encoder.h"
#include "../include/opencl_encode.h"
#include "../include/compression.h"
#include "../include/muxer.h"
#include "../include/preprocessing.h"

static cl::Program build_from_file(cl::Context &context, cl::Device &device, const char* file)
{
    std::ifstream kernel(file);
	std::string str;

    kernel.seekg(0, std::ios::end);
    str.reserve(kernel.tellg());
    kernel.seekg(0, std::ios::beg);
    str.assign((std::istreambuf_iterator<char>(kernel)), std::istreambuf_iterator<char>());
	cl::Program ret(context, str);
    ret.build({device});
	return ret;
}

static void initOpenCL(OpenCLContext &ctx, RGBImage &image, const char *outputfile) {
    ctx.image = (unsigned char*)image.pixel;
    ctx.height = image.h;
    ctx.width = image.w;
    ctx.file = outputfile;

    ctx.context = cl::Context(CL_DEVICE_TYPE_ALL);
    ctx.device = cl::Device(ctx.context.getInfo<CL_CONTEXT_DEVICES>()[0]);
    ctx.queue = cl::CommandQueue(ctx.context, ctx.device, CL_QUEUE_PROFILING_ENABLE);
    ctx.prog = cl::Program(build_from_file(ctx.context, ctx.device, "src/kernel.cl"));

    CL_CREATE_READONLY_BUFFER(ctx.cctbl, ctx.context, color_conversion_table);
    CL_CREATE_READONLY_BUFFER(ctx.dctqnt_divs, ctx.context, ctx.m_fdct_divisors);
    CL_CREATE_READONLY_BUFFER(ctx.dctqnt_mlt,  ctx.context, MULTIPLIER);
    CL_CREATE_READONLY_BUFFER(ctx.dctqnt_sign, ctx.context, SIGN);
    CL_CREATE_READONLY_BUFFER(ctx.dctqnt_ids,  ctx.context, IDS);
    CL_CREATE_READONLY_BUFFER(ctx.dctqnt_desc, ctx.context, DESCALER);
    CL_CREATE_READONLY_BUFFER(ctx.dctqnt_bias, ctx.context, DESCALER_OFFSET);
}

int encodeImage(RGBImage &image, unsigned char quality, const char *file)
{
    OpenCLContext ctx;
    preprocess(ctx, quality);
    initOpenCL(ctx, image, file);
    prepareImage(ctx);
    do_opencl_encode(ctx);
    compressData(ctx);
    writeTrailer(ctx);
    return 0;
}

