/*****************************************************************************
 * predict.c: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: predict.c,v 1.1 2004/06/03 19:27:07 fenrir Exp $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

/* XXX predict4x4 are inspired from ffmpeg h264 decoder
 */

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <stdarg.h>

#include "x264.h"   /* for keyword inline */
#include "common/predict.h"
#include "predict.h"

static inline int clip_uint8( int a )
{
    if (a&(~255))
        return (-a)>>31;
    else
        return a;
}

/****************************************************************************
 * 16x16 prediction for intra block DC, H, V, P
 ****************************************************************************/
static void predict_16x16_dc( uint8_t *src, int i_stride )
{
    uint32_t dc = 0;
    int i;

    /* calculate DC value */
    for( i = 0; i < 16; i++ )
    {
        dc += src[-1 + i * i_stride];
        dc += src[i - i_stride];
    }
    dc = (( dc + 16 ) >> 5) * 0x01010101;

    for( i = 0; i < 16; i++ )
    {
        uint32_t *p = (uint32_t*)src;

        *p++ = dc;
        *p++ = dc;
        *p++ = dc;
        *p++ = dc;

        src += i_stride;
    }
}
static void predict_16x16_dc_left( uint8_t *src, int i_stride )
{
    uint32_t dc = 0;
    int i;

    for( i = 0; i < 16; i++ )
    {
        dc += src[-1 + i * i_stride];
    }
    dc = (( dc + 8 ) >> 4) * 0x01010101;

    for( i = 0; i < 16; i++ )
    {
        uint32_t *p = (uint32_t*)src;

        *p++ = dc;
        *p++ = dc;
        *p++ = dc;
        *p++ = dc;

        src += i_stride;
    }
}
static void predict_16x16_dc_top( uint8_t *src, int i_stride )
{
    uint32_t dc = 0;
    int i;

    for( i = 0; i < 16; i++ )
    {
        dc += src[i - i_stride];
    }
    dc = (( dc + 8 ) >> 4) * 0x01010101;

    for( i = 0; i < 16; i++ )
    {
        uint32_t *p = (uint32_t*)src;

        *p++ = dc;
        *p++ = dc;
        *p++ = dc;
        *p++ = dc;

        src += i_stride;
    }
}
static void predict_16x16_dc_128( uint8_t *src, int i_stride )
{
    int i;

    for( i = 0; i < 16; i++ )
    {
        uint32_t *p = (uint32_t*)src;

        *p++ = 0x80808080;
        *p++ = 0x80808080;
        *p++ = 0x80808080;
        *p++ = 0x80808080;

        src += i_stride;
    }
}
static void predict_16x16_h( uint8_t *src, int i_stride )
{
    int i;

    for( i = 0; i < 16; i++ )
    {
        const uint32_t v = 0x01010101 * src[-1];
        uint32_t *p = (uint32_t*)src;

        *p++ = v;
        *p++ = v;
        *p++ = v;
        *p++ = v;

        src += i_stride;

    }
}

extern void predict_16x16_v_mmx( uint8_t *src, int i_stride );

#if 0
static void predict_16x16_v( uint8_t *src, int i_stride )
{
    int i;

    asm volatile(
        "movq  (%0), %%mm0\n"
        "movq 8(%0), %%mm1\n" :: "r"(&src[-i_stride]) );

    for( i = 0; i < 16; i++ )
    {
        asm volatile(
            "movq %%mm0,  (%0)\n"
            "movq %%mm1, 8(%0)\n" :: "r"(src) );
        src += i_stride;
    }
}
#endif

/****************************************************************************
 * 8x8 prediction for intra chroma block DC, H, V, P
 ****************************************************************************/
static void predict_8x8c_dc_128( uint8_t *src, int i_stride )
{
    int y;

    for( y = 0; y < 8; y++ )
    {
        uint32_t *p = (uint32_t*)src;

        *p++ = 0x80808080;
        *p++ = 0x80808080;

        src += i_stride;
    }
}
static void predict_8x8c_dc_left( uint8_t *src, int i_stride )
{
    int y;
    uint32_t dc0 = 0, dc1 = 0;

    for( y = 0; y < 4; y++ )
    {
        dc0 += src[y * i_stride     - 1];
        dc1 += src[(y+4) * i_stride - 1];
    }
    dc0 = (( dc0 + 2 ) >> 2)*0x01010101;
    dc1 = (( dc1 + 2 ) >> 2)*0x01010101;

    for( y = 0; y < 4; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc0;
        *p++ = dc0;

        src += i_stride;
    }
    for( y = 0; y < 4; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc1;
        *p++ = dc1;

        src += i_stride;
    }

}
static void predict_8x8c_dc_top( uint8_t *src, int i_stride )
{
    int y, x;
    uint32_t dc0 = 0, dc1 = 0;

    for( x = 0; x < 4; x++ )
    {
        dc0 += src[x     - i_stride];
        dc1 += src[x + 4 - i_stride];
    }
    dc0 = (( dc0 + 2 ) >> 2)*0x01010101;
    dc1 = (( dc1 + 2 ) >> 2)*0x01010101;

    for( y = 0; y < 8; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc0;
        *p++ = dc1;

        src += i_stride;
    }
}
static void predict_8x8c_dc( uint8_t *src, int i_stride )
{
    int y;
    int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    uint32_t dc0, dc1, dc2, dc3;
    int i;

    /* First do :
          s0 s1
       s2
       s3
    */
    for( i = 0; i < 4; i++ )
    {
        s0 += src[i - i_stride];
        s1 += src[i + 4 - i_stride];
        s2 += src[-1 + i * i_stride];
        s3 += src[-1 + (i+4)*i_stride];
    }
    /* now calculate
       dc0 dc1
       dc2 dc3
     */
    dc0 = (( s0 + s2 + 4 ) >> 3)*0x01010101;
    dc1 = (( s1 + 2 ) >> 2)*0x01010101;
    dc2 = (( s3 + 2 ) >> 2)*0x01010101;
    dc3 = (( s1 + s3 + 4 ) >> 3)*0x01010101;

    for( y = 0; y < 4; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc0;
        *p++ = dc1;

        src += i_stride;
    }

    for( y = 0; y < 4; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p++ = dc2;
        *p++ = dc3;

        src += i_stride;
    }
}

static void predict_8x8c_h( uint8_t *src, int i_stride )
{
    int i;

    for( i = 0; i < 8; i++ )
    {
        uint32_t v = 0x01010101 * src[-1];
        uint32_t *p = (uint32_t*)src;

        *p++ = v;
        *p++ = v;

        src += i_stride;
    }
}

extern void predict_8x8c_v_mmx( uint8_t *src, int i_stride );

#if 0
static void predict_8x8c_v( uint8_t *src, int i_stride )
{
    int i;

    asm volatile( "movq  (%0), %%mm0\n" :: "r"(&src[-i_stride]) );

    for( i = 0; i < 8; i++ )
    {
        asm volatile( "movq %%mm0,  (%0)\n" :: "r"(src) );
        src += i_stride;
    }
}
#endif


/****************************************************************************
 * 4x4 prediction for intra luma block
 ****************************************************************************/
static void predict_4x4_dc_128( uint8_t *src, int i_stride )
{
    int y;
    for( y = 0; y < 4; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p = 0x80808080;

        src += i_stride;
    }
}
static void predict_4x4_dc_left( uint8_t *src, int i_stride )
{
    int y;
    uint32_t dc = (( src[-1+0*i_stride] + src[-1+i_stride]+
                     src[-1+2*i_stride] + src[-1+3*i_stride] + 2 ) >> 2)*0x01010101;

    for( y = 0; y < 4; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p = dc;

        src += i_stride;
    }
}
static void predict_4x4_dc_top( uint8_t *src, int i_stride )
{
    int y;
    uint32_t dc = (( src[0 - i_stride] + src[1 - i_stride] +
                     src[2 - i_stride] + src[3 - i_stride] + 2 ) >> 2)*0x01010101;

    for( y = 0; y < 4; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p = dc;
        src += i_stride;
    }
}
static void predict_4x4_dc( uint8_t *src, int i_stride )
{
    int y;
    uint32_t dc = (( src[-1+0*i_stride] + src[-1+i_stride]+
                     src[-1+2*i_stride] + src[-1+3*i_stride] +
                     src[0 - i_stride]  + src[1 - i_stride] +
                     src[2 - i_stride]  + src[3 - i_stride] + 4 ) >> 3)*0x01010101;

    for( y = 0; y < 4; y++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p = dc;

        src += i_stride;
    }
}
static void predict_4x4_h( uint8_t *src, int i_stride )
{
    int i;

    for( i = 0; i < 4; i++ )
    {
        uint32_t *p = (uint32_t*)src;
        *p = 0x01010101*src[-1];

        src += i_stride;
    }
}
static void predict_4x4_v( uint8_t *src, int i_stride )
{
    uint32_t top = *((uint32_t*)&src[-i_stride]);
    int i;

    for( i = 0; i < 4; i++ )
    {
        uint32_t *p = (uint32_t*)src;

        *p = top;

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
    pf[I_PRED_16x16_DC_TOP ]= predict_16x16_dc_top;
    pf[I_PRED_16x16_DC_128 ]= predict_16x16_dc_128;
}

void x264_predict_8x8c_init_mmxext( x264_predict_t pf[7] )
{
    pf[I_PRED_CHROMA_V ]     = predict_8x8c_v_mmx;
    pf[I_PRED_CHROMA_H ]     = predict_8x8c_h;
    pf[I_PRED_CHROMA_DC]     = predict_8x8c_dc;
    pf[I_PRED_CHROMA_DC_LEFT]= predict_8x8c_dc_left;
    pf[I_PRED_CHROMA_DC_TOP ]= predict_8x8c_dc_top;
    pf[I_PRED_CHROMA_DC_128 ]= predict_8x8c_dc_128;
}

void x264_predict_4x4_init_mmxext( x264_predict_t pf[12] )
{
    pf[I_PRED_4x4_V]      = predict_4x4_v;
    pf[I_PRED_4x4_H]      = predict_4x4_h;
    pf[I_PRED_4x4_DC]     = predict_4x4_dc;
    pf[I_PRED_4x4_DC_LEFT]= predict_4x4_dc_left;
    pf[I_PRED_4x4_DC_TOP] = predict_4x4_dc_top;
    pf[I_PRED_4x4_DC_128] = predict_4x4_dc_128;
}

