/*****************************************************************************
 * predict.c: h264 encoder
 *****************************************************************************
 * Copyright (C) 2006 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110 USA
 *****************************************************************************/

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif

#include "common/predict.h"
#include "common/i386/predict.h"

extern void predict_16x16_v_mmx( uint8_t *src, int i_stride );
extern void predict_8x8c_v_mmx( uint8_t *src, int i_stride );

/****************************************************************************
 * 16x16 prediction for intra luma block
 ****************************************************************************/

#define PREDICT_16x16_DC(v) \
    for( i = 0; i < 16; i++ )\
    {\
        uint64_t *p = (uint64_t*)src;\
        *p++ = v;\
        *p++ = v;\
        src += i_stride;\
    }

static void predict_16x16_dc( uint8_t *src, int i_stride )
{
    uint32_t s = 0;
    uint64_t dc;
    int i;

    /* calculate DC value */
    for( i = 0; i < 16; i++ )
    {
        s += src[-1 + i * i_stride];
        s += src[i - i_stride];
    }
    dc = (( s + 16 ) >> 5) * 0x0101010101010101ULL;

    PREDICT_16x16_DC(dc);
}
static void predict_16x16_dc_left( uint8_t *src, int i_stride )
{
    uint32_t s = 0;
    uint64_t dc;
    int i;

    for( i = 0; i < 16; i++ )
    {
        s += src[-1 + i * i_stride];
    }
    dc = (( s + 8 ) >> 4) * 0x0101010101010101ULL;

    PREDICT_16x16_DC(dc);
}
static void predict_16x16_h( uint8_t *src, int i_stride )
{
    int i;
    for( i = 0; i < 16; i++ )
    {
        const uint64_t v = 0x0101010101010101ULL * src[-1];
        uint64_t *p = (uint64_t*)src;
        *p++ = v;
        *p++ = v;
        src += i_stride;
    }
}


/****************************************************************************
 * 8x8 prediction for intra chroma block
 ****************************************************************************/

static void predict_8x8c_dc_left( uint8_t *src, int i_stride )
{
    int y;
    uint32_t s0 = 0, s1 = 0;
    uint64_t dc0, dc1;

    for( y = 0; y < 4; y++ )
    {
        s0 += src[y * i_stride     - 1];
        s1 += src[(y+4) * i_stride - 1];
    }
    dc0 = (( s0 + 2 ) >> 2)*0x0101010101010101ULL;
    dc1 = (( s1 + 2 ) >> 2)*0x0101010101010101ULL;

    for( y = 0; y < 4; y++ )
    {
        *(uint64_t*)src = dc0;
        src += i_stride;
    }
    for( y = 0; y < 4; y++ )
    {
        *(uint64_t*)src = dc1;
        src += i_stride;
    }

}
static void predict_8x8c_dc_top( uint8_t *src, int i_stride )
{
    int y, x;
    uint32_t s0 = 0, s1 = 0;
    uint64_t dc;

    for( x = 0; x < 4; x++ )
    {
        s0 += src[x     - i_stride];
        s1 += src[x + 4 - i_stride];
    }
    dc = (( s0 + 2 ) >> 2)*0x01010101
       + (( s1 + 2 ) >> 2)*0x0101010100000000ULL;

    for( y = 0; y < 8; y++ )
    {
        *(uint64_t*)src = dc;
        src += i_stride;
    }
}
static void predict_8x8c_h( uint8_t *src, int i_stride )
{
    int i;
    for( i = 0; i < 8; i++ )
    {
        *(uint64_t*)src = 0x0101010101010101ULL * src[-1];
        src += i_stride;
    }
}


/****************************************************************************
 * Exported functions:
 ****************************************************************************/
void x264_predict_16x16_init_mmxext( x264_predict_t pf[7] )
{
    pf[I_PRED_16x16_V ]     = predict_16x16_v_mmx;
    pf[I_PRED_16x16_H ]     = predict_16x16_h;
    pf[I_PRED_16x16_DC]     = predict_16x16_dc;
    pf[I_PRED_16x16_DC_LEFT]= predict_16x16_dc_left;
}

void x264_predict_8x8c_init_mmxext( x264_predict_t pf[7] )
{
    pf[I_PRED_CHROMA_V ]     = predict_8x8c_v_mmx;
    pf[I_PRED_CHROMA_H ]     = predict_8x8c_h;
    pf[I_PRED_CHROMA_DC_LEFT]= predict_8x8c_dc_left;
    pf[I_PRED_CHROMA_DC_TOP ]= predict_8x8c_dc_top;
}

void x264_predict_8x8_init_mmxext( x264_predict8x8_t pf[12] )
{
}

