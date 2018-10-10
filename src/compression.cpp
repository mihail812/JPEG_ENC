#include "../include/compression.h"
#include <cstdio>

void compressData(OpenCLContext &ctx)
{
    short (*y_blocks)[4][64] = (short (*)[4][64])ctx.y_buffer;
    short (*cb_blocks)[64] = (short (*)[64])ctx.cb_buffer;
    short (*cr_blocks)[64] = (short (*)[64])ctx.cr_buffer;
    size_t super_block_y = ctx.nsbh - 1;
    size_t super_block_id_base = (super_block_y * ctx.nsbw);
    for(size_t gx = 0; gx < ctx.nsbw; ++gx) {
        if ((super_block_y << 0x1) + 1 >= ctx.nbh) {
            size_t super_block_x = gx;
            size_t super_block_id = super_block_id_base + super_block_x;
            short value = y_blocks[super_block_id][1][0];
            y_blocks[super_block_id][2][0] = value;
            y_blocks[super_block_id][3][0] = value;
        }
    }
    size_t wg = (ctx.nsbw * ctx.nsbh);
    short *mcu_buffer[6];
    entropy_state_t state;
    memset(state.last_dc_val, 0, sizeof(state.last_dc_val));
    state.bits = 0;
    for(size_t i = 0; i < wg; ++i)
    {
        mcu_buffer[0] = y_blocks[i][0];
        mcu_buffer[1] = y_blocks[i][1];
        mcu_buffer[2] = y_blocks[i][2];
        mcu_buffer[3] = y_blocks[i][3];
        mcu_buffer[4] = cb_blocks[i];
        mcu_buffer[5] = cr_blocks[i];
        do_entropy(ctx, mcu_buffer, ctx.output_buffer, state);
    }
}

void do_entropy(OpenCLContext &ctx, short *mcu_buffer[], std::vector<char> &outputbuf, entropy_state_t &state)
{
    const static unsigned char mcu_membership[0x6] = {0x0, 0x0, 0x0, 0x0, 0x1, 0x2};
    const static unsigned char table_index[0x6] = {0x0, 0x0, 0x0, 0x0, 0x1, 0x1};
    size_t i, ci;
    for(i = 0; i < 6; i++) {
        ci = mcu_membership[i];
        entropy_single_data(ctx, mcu_buffer[i], table_index[i], state.last_dc_val[ci], outputbuf, state);
        state.last_dc_val[ci] = mcu_buffer[i][0];
    }
}

static int get_nbits(int i) {
    if (i <= 2) return i;
    if (i == 3) return 2;
    if (i >= 4 && i < 8) return 3;
    if (i >= 8 && i < 16) return 4;
    if (i >= 16 && i < 32) return 5;
    if (i >= 32 && i < 64) return 6;
    if (i >= 64 && i < 128) return 7;
    if (i >= 128 && i < 256) return 8;
    if (i >= 256 && i < 512) return 9;
    if (i >= 512 && i < 1024) return 10;
    if (i >= 1024 && i < 2048) return 11;
    if (i >= 2048 && i < 4096) return 12;
    if (i >= 4096 && i < 8192) return 13;
    if (i >= 8192 && i < 16384) return 14;
    if (i >= 16384 && i < 32768) return 15;
    return 16;
}

#define kloop(k) {  \
    if ((temp = block[k]) == 0) { \
        r++; \
    } else { \
        temp2 = temp; \
        temp3 = temp >> (8 * sizeof(int) - 1); \
        temp ^= temp3; \
        temp -= temp3; \
        temp2 += temp3; \
        nbits = get_nbits(temp); \
        /* if run length > 15, must emit special run-length-16 codes (0xF0) */ \
        while (r > 15) { \
            EMIT_BITS(code_0xf0, size_0xf0) \
            r -= 16; \
        } \
        /* Emit Huffman symbol for run length / number of bits */ \
        temp3 = (r << 4) + nbits;  \
        code = acd->code[temp3]; \
        size = acd->length[temp3]; \
        EMIT_CODE(code, size) \
        r = 0;  \
    } \
}

void entropy_single_data(OpenCLContext &ctx, short *block, int table_index, int last_dc_val, std::vector<char> &outputbuf, entropy_state_t &state)
{
    int temp, temp2, temp3, r, code, size, put_bits, nbits, code_0xf0, size_0xf0;
    size_t put_buffer;
    derived_huffman_table *dcd;
    derived_huffman_table *acd;

    /* init values */
    dcd = &ctx.m_dc_derived_tbls[table_index];
    acd = &ctx.m_ac_derived_tbls[table_index];
    code_0xf0 = acd->code[240];
    size_0xf0 = acd->length[240];

    put_buffer = state.buffer;
    put_bits = state.bits;


    temp = temp2 = block[0] - last_dc_val;
    temp3 = temp >> (8 * sizeof(int) - 1);
    temp ^= temp3;
    temp -= temp3;

    temp2 += temp3;
    nbits = get_nbits(temp);

    code = dcd->code[nbits];
    size = dcd->length[nbits];
    EMIT_BITS(code, size)

    temp2 &= (((long) 1) << nbits) - 1;
    EMIT_BITS(temp2, nbits)

    /* run length encoding */
    r = 0;


    /* Do run length encoding in zig zag pattern */
    kloop(1);   kloop(8);   kloop(16);  kloop(9);   kloop(2);   kloop(3);
    kloop(10);  kloop(17);  kloop(24);  kloop(32);  kloop(25);  kloop(18);
    kloop(11);  kloop(4);   kloop(5);   kloop(12);  kloop(19);  kloop(26);
    kloop(33);  kloop(40);  kloop(48);  kloop(41);  kloop(34);  kloop(27);
    kloop(20);  kloop(13);  kloop(6);   kloop(7);   kloop(14);  kloop(21);
    kloop(28);  kloop(35);  kloop(42);  kloop(49);  kloop(56);  kloop(57);
    kloop(50);  kloop(43);  kloop(36);  kloop(29);  kloop(22);  kloop(15);
    kloop(23);  kloop(30);  kloop(37);  kloop(44);  kloop(51);  kloop(58);
    kloop(59);  kloop(52);  kloop(45);  kloop(38);  kloop(31);  kloop(39);
    kloop(46);  kloop(53);  kloop(60);  kloop(61);  kloop(54);  kloop(47);
    kloop(55);  kloop(62);  kloop(63);

    if(r > 0) {
        code = acd->code[0];
        size = acd->length[0];
        EMIT_BITS(code, size);
    }

    /* Store the current state back in the global one */
    state.bits = put_bits;
    state.buffer = put_buffer;
}
