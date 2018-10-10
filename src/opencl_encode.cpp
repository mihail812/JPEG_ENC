#include "../include/opencl_encode.h"

void do_opencl_encode(OpenCLContext &ctx)
{
    rgb2yuv(ctx);
    downsample(ctx);
    dctquant(ctx);
    cpBackToHost(ctx);
}

void rgb2yuv(OpenCLContext &ctx) {
    cl_int cle;
    cl_int wg;

    ctx.image_buffer = cl::Buffer(ctx.context, CL_MEM_READ_WRITE, sizeof(unsigned char) * 3 * ctx.width * ctx.height);
    ctx.queue.enqueueWriteBuffer(ctx.image_buffer, true, 0, sizeof(unsigned char) * 3 * ctx.width * ctx.height, ctx.image);

    ctx.kernel = cl::Kernel(ctx.prog, "rgb2yuv");
    ctx.queue.enqueueWriteBuffer(ctx.cctbl, false, 0, sizeof(color_conversion_table), color_conversion_table);

    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 0, ctx.cctbl);
    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 1, ctx.image_buffer);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 2, ctx.width * ctx.height);
    wg = (((ctx.width * ctx.height) + 0x3F) >> 0x6) << 0x6;
    ctx.queue.enqueueNDRangeKernel(ctx.kernel, 0, wg, 0x40);
}

void downsample(OpenCLContext &ctx)
{
    cl_int cle;
    cl_int wg;

    ctx.nbw = (ctx.width + 0x7) >> 0x3;
    ctx.nbh = (ctx.height + 0x7) >> 0x3;
    ctx.nsbw = (ctx.width + 0xF) >> 0x4;
    ctx.nsbh = (ctx.height + 0xF) >> 0x4;

    wg = (ctx.nsbw * ctx.nsbh) << 0x8;

    ctx.planeY = cl::Buffer(ctx.context, CL_MEM_READ_WRITE, wg * sizeof(cl_short));

    ctx.kernel = cl::Kernel(ctx.prog, "downsample_full");
    /* Set the kernel arguments */
    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 0, ctx.planeY);
    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 1, ctx.image_buffer);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 2, ctx.nsbw);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 3, ctx.nbw);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 4, ctx.nbh);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 5, ctx.width);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 6, ctx.height);

    ctx.queue.enqueueNDRangeKernel(ctx.kernel, 0, wg, 0x40);


    wg = (ctx.nsbw * ctx.nsbh) << 0x6;

    ctx.planeCb = cl::Buffer(ctx.context, CL_MEM_READ_WRITE, wg * sizeof(cl_short));
    ctx.planeCr = cl::Buffer(ctx.context, CL_MEM_READ_WRITE, wg * sizeof(cl_short));

    ctx.kernel = cl::Kernel(ctx.prog, "downsample_2v2");
    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 0, ctx.planeCb);
    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 1, ctx.planeCr);
    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 2, ctx.image_buffer);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 3, ctx.nsbw);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 4, ctx.nbw);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 5, ctx.nbh);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 6, ctx.width);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 7, ctx.height);

    ctx.queue.enqueueNDRangeKernel(ctx.kernel, 0, wg, 0x40);
}

void dctquant(OpenCLContext &ctx)
{
    cl_int cle;
    cl_int wg;

    ctx.kernel = cl::Kernel(ctx.prog, "dct_quant");
    ctx.queue.enqueueWriteBuffer(ctx.dctqnt_divs, false, 0, sizeof(ctx.m_fdct_divisors), &ctx.m_fdct_divisors);
    ctx.queue.enqueueWriteBuffer(ctx.dctqnt_mlt, false, 0, sizeof(MULTIPLIER), &MULTIPLIER);
    ctx.queue.enqueueWriteBuffer(ctx.dctqnt_sign, false, 0, sizeof(SIGN), &SIGN);
    ctx.queue.enqueueWriteBuffer(ctx.dctqnt_ids, false, 0, sizeof(IDS), &IDS);
    ctx.queue.enqueueWriteBuffer(ctx.dctqnt_desc, false, 0, sizeof(DESCALER), &DESCALER);
    ctx.queue.enqueueWriteBuffer(ctx.dctqnt_bias, false, 0, sizeof(DESCALER_OFFSET), &DESCALER_OFFSET);

    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 0, ctx.planeY);
    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 1, ctx.dctqnt_divs);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 2, 0);
    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 3, ctx.dctqnt_mlt);
    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 4, ctx.dctqnt_sign);
    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 5, ctx.dctqnt_ids);
    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 6, ctx.dctqnt_desc);
    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 7, ctx.dctqnt_bias);

    wg = (ctx.nsbw * ctx.nsbh) << 0x8;
    ctx.queue.enqueueNDRangeKernel(ctx.kernel, 0x0, wg, 0x40);

    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 0, ctx.planeCb);
    CL_SET_KERNEL_ARG(ctx.kernel, cl_uint, 2, 0x100);
    wg = (ctx.nsbw * ctx.nsbh) << 0x6;
    ctx.queue.enqueueNDRangeKernel(ctx.kernel, 0x0, wg, 0x40);

    CL_SET_KERNEL_ARG(ctx.kernel, cl::Buffer, 0, ctx.planeCr);
    ctx.queue.enqueueNDRangeKernel(ctx.kernel, 0x0, wg, 0x40);
}

void cpBackToHost(OpenCLContext &ctx)
{
    ctx.y_buffer = (short*)malloc(sizeof(short) * (ctx.nsbw * ctx.nsbh) << 0x8);
    ctx.cb_buffer = (short*)malloc(sizeof(short) * (ctx.nsbw * ctx.nsbh) << 0x6);
    ctx.cr_buffer = (short*)malloc(sizeof(short) * (ctx.nsbw * ctx.nsbh) << 0x6);
    ctx.queue.enqueueReadBuffer(ctx.planeY, true, 0, sizeof(short) * (ctx.nsbw * ctx.nsbh) << 0x8, ctx.y_buffer);
    ctx.queue.enqueueReadBuffer(ctx.planeCb, true, 0, sizeof(short) * (ctx.nsbw * ctx.nsbh) << 0x6, ctx.cb_buffer);
    ctx.queue.enqueueReadBuffer(ctx.planeCr, true, 0, sizeof(short) * (ctx.nsbw * ctx.nsbh) << 0x6, ctx.cr_buffer);
}
