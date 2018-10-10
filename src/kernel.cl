#define LEFT_SHIFT(a, b) ((int)((unsigned int)(a) << (b)))
#define DESCALE(x,n)  RIGHT_SHIFT((x) + (1 << ((n)-1)), n)
#define RIGHT_SHIFT(x,shft)     ((x) >> (shft))

__kernel void rgb2yuv(__global uint *cctbl,
                      __global uchar *img,
                      uint sz)
{
        size_t id = get_global_id(0)*3;

        uint tmp1 = (cctbl[img[id] * 3] + cctbl[768 + img[id + 1] * 3] + cctbl[1536 + img[id + 2] * 3]) >> 0x10;
        uint tmp2 = (cctbl[img[id] * 3 + 2] + cctbl[770 + img[id + 1] * 3] + cctbl[1538 + img[id + 2] * 3]) >> 0x10;
        uint tmp3 = (cctbl[img[id] * 3 + 1] + cctbl[769 + img[id + 1] * 3] + cctbl[1537 + img[id + 2] * 3]) >> 0x10;

        img[id + 0] = (uchar)(tmp1);
        img[id + 1] = (uchar)(tmp2);
        img[id + 2] = (uchar)(tmp3);
}


__kernel void downsample_full(__global short *buffer,
                              __global uchar *image,
                              uint nsbw,
                              uint nbw,
                              uint nbh,
                              uint width,
                              uint height)
{
        size_t id = get_global_id(0);

        size_t image_x = (((id >> 0x8) % nsbw) << 0x4) | ((((id & 0xFF) >> 0x6) & 0x1) << 0x3) | ((id & 0x3F) & 0x7);
        size_t image_y = (((id >> 0x8) / nsbw) << 0x4) | ((((id & 0xFF) >> 0x6) >> 0x1) << 0x3) | ((id & 0x3F) >> 0x3);

	if(image_x >= width) image_x = width - 1;
	if(image_y >= height) image_y = height - 1;

        buffer[id] = (short)image[(image_x + (image_y * width)) * 3] - (short)0x80;
}

__kernel void downsample_2v2(__global short *cb,
                             __global short *cr,
                             __global uchar *image,
                             uint nsbw,
                             uint nbw,
                             uint nbh,
                             uint width,
                             uint height)
{
        size_t id = get_global_id(0);

        /* compute id of super block */
        size_t super_block_id = id / 64;		/* divide by 64 */

        /* super sub block id and x and y position */
        size_t sub_block_x = (id & 0x7) > 0x3;
        size_t sub_block_y = (id & 0x3F) > 0x1F;

        /* compute in block index and x and y index */
        size_t field_id = id & 0x3F;
        size_t field_x = (field_id & 0x7) << 0x1;
        size_t field_y = (field_id >> 0x3) << 0x1;

	/* Global x and y image position */
        size_t image_x = ((super_block_id % nsbw) << 0x4) | (sub_block_x << 0x3) | field_x;
        size_t image_y = ((super_block_id / nsbw) << 0x4) | (sub_block_y << 0x3) | field_y;

        /* Compute pixels x and y values */
        size_t pixel_x0 = image_x;
        size_t pixel_x1 = image_x + 1;
        size_t pixel_y0 = image_y;
        size_t pixel_y1 = image_y + 1;

	/* Clamp */
        if(pixel_x0 >= width) pixel_x0 = width - 1;
        if(pixel_x1 >= width) pixel_x1 = width - 1;
        if(pixel_y0 >= height) pixel_y0 = height - 1;
        if(pixel_y1 >= height) pixel_y1 = height - 1;

	/* compute pixel ids */
	size_t pixel00 = (pixel_x0 + (pixel_y0 * width));
	size_t pixel10 = (pixel_x1 + (pixel_y0 * width));
	size_t pixel01 = (pixel_x0 + (pixel_y1 * width));
	size_t pixel11 = (pixel_x1 + (pixel_y1 * width));

	/* Sum up the components */
	long cb_sum = 0;
	long cr_sum = 0;

	size_t pixel = pixel00 * 3;
	cb_sum += (long)image[pixel + 1];
	cr_sum += (long)image[pixel + 2];

	pixel = pixel10 * 3;
	cb_sum += (long)image[pixel + 1];
	cr_sum += (long)image[pixel + 2];

	pixel = pixel01 * 3;
	cb_sum += (long)image[pixel + 1];
	cr_sum += (long)image[pixel + 2];

	pixel = pixel11 * 3;
	cb_sum += (long)image[pixel + 1];
	cr_sum += (long)image[pixel + 2];

        int bias = 0x1 << (id & 0x1);
	cb_sum += bias;
	cr_sum += bias;

	/* Store the result */
        cb[id] = (short)(cb_sum >> 0x2) - (short)0x80;
        cr[id] = (short)(cr_sum >> 0x2) - (short)0x80;
}


__kernel void dct_quant(__global short *block, __global short *divisors, unsigned int divisor_offset,
                                                __global short *multiplier, __global int *sign, __global int *indices,
                                                __global char *descaler, __global short *descaler_offset)
{
	unsigned int product;
	unsigned short recip, corr;
	short ioffset, moffset, soffset, doffset;
	short t0, t1, t2, t3, res, neg;
	int value;
	__local short *dataptr;
	int shift;

        size_t global_id = get_global_id(0);
	size_t lx = get_local_id(0);

	short row = lx >> 0x3;
	short row_offset = (row) << 0x3;
	short column = lx & 0x7;

	__local short lblock[0x40];
        lblock[lx] = block[global_id];
	barrier(CLK_LOCAL_MEM_FENCE);
	dataptr = &lblock[row_offset];

	/* Pass 1: process rows. */
	ioffset = column << 0x3;
	moffset = column << 0x2;
	soffset = column << 0x1;
	doffset = column << 0x1;
	t0 = dataptr[indices[ioffset + 0]] + (dataptr[indices[ioffset + 1]] * sign[soffset + 0]);
	t1 = dataptr[indices[ioffset + 2]] + (dataptr[indices[ioffset + 3]] * sign[soffset + 0]);
	t2 = dataptr[indices[ioffset + 4]] + (dataptr[indices[ioffset + 5]] * sign[soffset + 0]);
	t3 = dataptr[indices[ioffset + 6]] + (dataptr[indices[ioffset + 7]] * sign[soffset + 0]);
	value = t0 * multiplier[moffset + 0] + (t1 + t0) * multiplier[moffset + 1] + (t2 + t0)
			   * multiplier[moffset + 2] + ((t0 + t1) + ((t2 + t3) * sign[soffset + 1])) * multiplier[moffset + 3];
	res = (short)DESCALE(value, 0xB) * descaler[doffset + 0] +	LEFT_SHIFT(value, 0x2) * descaler[doffset + 1];

	/* Wait for all rows in the local execution to complete */
	barrier(CLK_LOCAL_MEM_FENCE);
	lblock[lx] = res;
	barrier(CLK_LOCAL_MEM_FENCE);

	/* Pass 2: process columns */
	dataptr = &lblock[column];

	ioffset = row << 0x3;
	moffset = row << 0x2;
	soffset = row << 0x1;
	t0 = dataptr[indices[ioffset + 0] << 0x3] + (dataptr[indices[ioffset + 1] << 0x3] * sign[soffset + 0]);
	t1 = dataptr[indices[ioffset + 2] << 0x3] + (dataptr[indices[ioffset + 3] << 0x3] * sign[soffset + 0]);
	t2 = dataptr[indices[ioffset + 4] << 0x3] + (dataptr[indices[ioffset + 5] << 0x3] * sign[soffset + 0]);
	t3 = dataptr[indices[ioffset + 6] << 0x3] + (dataptr[indices[ioffset + 7] << 0x3] * sign[soffset + 0]);
	value = t0 * multiplier[moffset + 0] + (t1 + t0) * multiplier[moffset + 1] + (t2 + t0)
			   * multiplier[moffset + 2] + ((t0 + t1) + ((t2 + t3) * sign[soffset + 1])) * multiplier[moffset + 3];
	res = DESCALE(value, 0x2 + descaler_offset[row]);

	/* Pass 3: quantize */
	recip = divisors[divisor_offset + lx + 0x40 * 0];
	corr = divisors[divisor_offset + lx + 0x40 * 1];
	shift = divisors[divisor_offset + lx + 0x40 * 3];
	neg = res < 0 ? -1 : 1;
	res *= neg;
	product = (unsigned int) (res + corr) * recip;
	product >>= shift + sizeof(short) * 8;
	res = (short) product;
	res *= neg;
        block[global_id] = (short)res;
}

