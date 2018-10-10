#include "../include/muxer.h"
#include "../include/tables.h"


void prepareImage(OpenCLContext &ctx)
{
    checkImg(ctx);
    writeHeader(ctx.output_buffer);
    writeQuantTable(ctx, ctx.output_buffer);
    writeSof(ctx.output_buffer, ctx.width, ctx.height);
    writeHuffmanTable(ctx, ctx.output_buffer);
    writeSoS(ctx.output_buffer);
}

void writeHeader(std::vector<char> &output_buf)
{
    for(size_t i = 0; i < 20; ++i) {
        output_buf.push_back(JPEG_HEADER[i]);
    }
}

void writeByte(std::vector<char> &output_buf, int value)
{
    output_buf.push_back((char)value);
}

void writeMarker(std::vector<char> &output_buf, int value)
{
    writeByte(output_buf, 0xFF);
    writeByte(output_buf, value);
}


void writeQuantTable(OpenCLContext &ctx, std::vector<char> &output_buf)
{
    for (int ind = 0; ind < 2; ind++) {
        quantification_table_t *qtblptr;
        size_t i;
        qtblptr = &ctx.m_quant_tbls[ind];

        writeMarker(output_buf, 0xDB);
        writeByte(output_buf, 0);
        writeByte(output_buf, 67);
        writeByte(output_buf, ind);
        for(i = 0; i < 64; ++i) {
            unsigned int qval = qtblptr->value[jpeg_natural_order[i]];
            writeByte(output_buf, (int)(qval & 0xFF));
        }
    }
}


void writeSof(std::vector<char> &output_buf, size_t w, size_t h)
{
    writeMarker(output_buf, 0xC0);

    size_t tmp[17] = {0, 17, 8, (h >> 0x8) & 0xFF, h & 0xFF, (w >> 0x8) & 0xFF, w & 0xFF, 3, 1, 34, 0, 2, 17, 1, 3, 17, 1,};
    for (int i = 0; i < 17; i++) {
        output_buf.push_back((char)tmp[i]);
    }
}

void writeHuffmanTable(OpenCLContext &ctx, std::vector<char> &output_buf)
{
    for (int i = 0; i < 1; i++)
        for (int j = 0; j < 1; j++)
             writeHuffTbl(ctx, output_buf, i, j);
}

void writeHuffTbl(OpenCLContext &ctx, std::vector<char> &output_buf, int index, unsigned char is_ac)
{
    huffman_table *htblptr;
    size_t length, i;

    if(is_ac) {
        htblptr = &ctx.m_ac_huff_tbls[index];
        index += 0x10;
    } else {
        htblptr = &ctx.m_dc_huff_tbls[index];
    }

    writeMarker(output_buf, 0xC4);
    length = 0;
    for(i = 1; i < 0x11; ++i)
        length += htblptr->bits[i];
    writeByte(output_buf, ((length + 2 + 1 + 0x10) >> 0x8) & 0xFF);
    writeByte(output_buf, (length + 2 + 1 + 0x10) & 0xFF);
    writeByte(output_buf, index);
    for(i = 1; i < 0x11; ++i)
        writeByte(output_buf, htblptr->bits[i]);
    for(i = 0; i < length; ++i)
        writeByte(output_buf, htblptr->value[i]);
}

void writeSoS(std::vector<char> &output_buf)
{
    for (int i = 0; i < 14; i++) {
        output_buf.push_back(SOS_MARKER[i]);
    }
}

void writeTrailer(OpenCLContext &ctx)
{
    writeMarker(ctx.output_buffer, 0xD9);

    (void)fwrite(ctx.output_buffer.data(), sizeof(char),  ctx.output_buffer.size(), ctx.fp);
    fclose(ctx.fp);
}

void checkImg(OpenCLContext &ctx)
{
    ctx.fp = fopen(ctx.file, "wb");
    if(ctx.fp == NULL) {
        printf("Error. Input file can not be opened");
    }
}
