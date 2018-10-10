#include "../include/preprocessing.h"

void preprocess(OpenCLContext &ctx, unsigned char quality)
{
    createQuantTbl(ctx, quality);
    createHuffmanTables(ctx);
    createDCTdivTbl(ctx);
    createDHuffTbl(ctx);
}

void createQuantTbl(OpenCLContext &ctx, unsigned char quality)
{
    unsigned char q;
    if(quality <= 0) {
        q = 1;
    } else if(quality > 100) {
        q = 100;
    } else if(quality < 50) {
        q = 5000 / quality;
    } else {
        q = 200 - (quality << 0x1);
    }

    createQTbl(ctx, 0, q, std_luminance_quant_tbl);
    createQTbl(ctx, 1, q, std_chrominance_quant_tbl);
}

void createQTbl(OpenCLContext &ctx, int table_idx, unsigned char scale, const unsigned int *base_table) {
    quantification_table_t *tblptr;
    size_t i;
    long temp;

    tblptr = &ctx.m_quant_tbls[table_idx];
    for(i = 0; i < 64; ++i)
    {
        temp = ((long)base_table[i] * scale + 50) / 100;
        if(temp <= 0) {
            temp = 1;
        }
        else if(temp > 0xFF) {
            temp = 0xFF;
        }
        tblptr->value[i] = (unsigned char)temp;
    }
}

void createHuffmanTables(OpenCLContext &ctx)
{
    addHuffTbl(ctx, &ctx.m_dc_huff_tbls[0], bits_dc_luminance, value_dc_luminance);
    addHuffTbl(ctx, &ctx.m_ac_huff_tbls[0], bits_ac_luminance, value_ac_luminance);
    addHuffTbl(ctx, &ctx.m_dc_huff_tbls[1], bits_dc_chrominance, value_dc_chrominance);
    addHuffTbl(ctx, &ctx.m_ac_huff_tbls[1], bits_ac_chrominance, value_ac_chrominance);
}

void addHuffTbl(OpenCLContext &ctx, huffman_table *tblptr, const unsigned char *bits, const unsigned char *values)
{
    size_t len;
    int n = 0;
    memcpy(tblptr->bits, bits, sizeof(tblptr->bits));
    for(len = 0; len < 0x11; ++len)
        n += bits[len];
    memset(tblptr->value, 0, sizeof(tblptr->value));
    memcpy(tblptr->value, values, n * sizeof(unsigned char));
}

static int get_nbits(int i) {
    if (i <= 2)                  return i;
    if (i == 3)                  return 2;
    if (i >= 4     && i < 8)     return 3;
    if (i >= 8     && i < 16)    return 4;
    if (i >= 16    && i < 32)    return 5;
    if (i >= 32    && i < 64)    return 6;
    if (i >= 64    && i < 128)   return 7;
    if (i >= 128   && i < 256)   return 8;
    if (i >= 256   && i < 512)   return 9;
    if (i >= 512   && i < 1024)  return 10;
    if (i >= 1024  && i < 2048)  return 11;
    if (i >= 2048  && i < 4096)  return 12;
    if (i >= 4096  && i < 8192)  return 13;
    if (i >= 8192  && i < 16384) return 14;
    if (i >= 16384 && i < 32768) return 15;
    return 16;
}

static int compute_reciprocal (unsigned short divisor, short *dtbl)
{
    unsigned int fq, fr;
    unsigned short c;
    int b, r;

    if (divisor == 1) {
        dtbl[0x40 * 0] = (short) 1;
        dtbl[0x40 * 1] = (short) 0;
        dtbl[0x40 * 2] = (short) 1;
        dtbl[0x40 * 3] = -(short) (sizeof(short) * 8);
        return 0;
    }

    b = get_nbits(divisor) - 1;
    r = sizeof(short) * 8 + b;
    fq = ((unsigned int)1 << r) / divisor;
    fr = ((unsigned int)1 << r) % divisor;
    c = divisor >> 0x1;
    if (fr == 0) {
        fq >>= 1;
        r--;
    } else if (fr <= (divisor / 2U)) {
        c++;
    } else {
        fq++;
    }

    dtbl[0x40 * 0] = (short) fq;
    dtbl[0x40 * 1] = (short) c;
    dtbl[0x40 * 2] = (short) (1 << (sizeof(short)*8*2 - r));
    dtbl[0x40 * 3] = (short) r - sizeof(short)*8;

    return r <= 16 ? 0 : 1;
}

void createDCTdivTbl(OpenCLContext &ctx)
{
    size_t i;
    quantification_table_t *qtblptr;
    short *dtblptr;
    qtblptr = &ctx.m_quant_tbls[0];
    dtblptr = ctx.m_fdct_divisors[0];
    for(i = 0; i < 0x40; ++i) {
        compute_reciprocal(qtblptr->value[i] << 0x3, &dtblptr[i]);
    }
    qtblptr = &ctx.m_quant_tbls[1];
    dtblptr = ctx.m_fdct_divisors[1];
    for(i = 0; i < 0x40; ++i) {
        compute_reciprocal(qtblptr->value[i] << 0x3, &dtblptr[i]);
    }
}

void createDHuffTbl(OpenCLContext &ctx)
{
    DHuffTbl(&ctx.m_dc_huff_tbls[0], &ctx.m_dc_derived_tbls[0]);
    DHuffTbl(&ctx.m_ac_huff_tbls[0], &ctx.m_ac_derived_tbls[0]);
    DHuffTbl(&ctx.m_dc_huff_tbls[1], &ctx.m_dc_derived_tbls[1]);
    DHuffTbl(&ctx.m_ac_huff_tbls[1], &ctx.m_ac_derived_tbls[1]);
}

void DHuffTbl(huffman_table *htblptr, derived_huffman_table *dhtblptr)
{
    int p, i, l, lastp, si;
    char huffsize[0x101];
    unsigned int huffcode[0x101];
    unsigned int code;
    p = 0;
    for (l = 1; l < 0x11; l++)
    {
        i = (int) htblptr->bits[l];
        while (i--)
        {
            huffsize[p++] = (char) l;
        }
    }
    huffsize[p] = 0;
    lastp = p;
    code = 0;
    si = huffsize[0];
    p = 0;
    while (huffsize[p])
    {
        while (((int) huffsize[p]) == si)
        {
            huffcode[p++] = code;
            code++;
        }
        code <<= 1;
        si++;
    }
    memset(dhtblptr->length, 0, sizeof(dhtblptr->length));
    for (p = 0; p < lastp; ++p)
    {
        i = htblptr->value[p];
        dhtblptr->code[i] = huffcode[p];
        dhtblptr->length[i] = huffsize[p];
    }
}
