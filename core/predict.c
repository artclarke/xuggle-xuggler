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

#include "x264.h"
#include "predict.h"

#ifdef _MSC_VER
#undef HAVE_MMXEXT  /* not finished now */
#endif
#ifdef HAVE_MMXEXT
#   include "i386/predict.h"
#endif

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
    int dc = 0;
    int i, j;

    /* calculate DC value */
    for( i = 0; i < 16; i++ )
    {
        dc += src[-1 + i * i_stride];
        dc += src[i - i_stride];
    }
    dc = ( dc + 16 ) >> 5;

    for( i = 0; i < 16; i++ )
    {
        for( j = 0; j < 16; j++ )
        {
            src[j] = dc;
        }
        src += i_stride;
    }
}
static void predict_16x16_dc_left( uint8_t *src, int i_stride )
{
    int dc = 0;
    int i,j;

    for( i = 0; i < 16; i++ )
    {
        dc += src[-1 + i * i_stride];
    }
    dc = ( dc + 8 ) >> 4;

    for( i = 0; i < 16; i++ )
    {
        for( j = 0; j < 16; j++ )
        {
            src[j] = dc;
        }
        src += i_stride;
    }
}
static void predict_16x16_dc_top( uint8_t *src, int i_stride )
{
    int dc = 0;
    int i,j;

    for( i = 0; i < 16; i++ )
    {
        dc += src[i - i_stride];
    }
    dc = ( dc + 8 ) >> 4;

    for( i = 0; i < 16; i++ )
    {
        for( j = 0; j < 16; j++ )
        {
            src[j] = dc;
        }
        src += i_stride;
    }
}
static void predict_16x16_dc_128( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 16; i++ )
    {
        for( j = 0; j < 16; j++ )
        {
            src[j] = 128;
        }
        src += i_stride;
    }
}
static void predict_16x16_h( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 16; i++ )
    {
        uint8_t v;

        v = src[-1];
        for( j = 0; j < 16; j++ )
        {
            src[j] = v;
        }
        src += i_stride;

    }
}
static void predict_16x16_v( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 16; i++ )
    {
        for( j = 0; j < 16; j++ )
        {
            src[i * i_stride +j] = src[j - i_stride];
        }
    }
}
static void predict_16x16_p( uint8_t *src, int i_stride )
{
    int x, y, i;
    int a, b, c;
    int H = 0;
    int V = 0;
    int i00;

    /* calcule H and V */
    for( i = 0; i <= 7; i++ )
    {
        H += ( i + 1 ) * ( src[ 8 + i - i_stride ] - src[6 -i -i_stride] );
        V += ( i + 1 ) * ( src[-1 + (8+i)*i_stride] - src[-1 + (6-i)*i_stride] );
    }

    a = 16 * ( src[-1 + 15*i_stride] + src[15 - i_stride] );
    b = ( 5 * H + 32 ) >> 6;
    c = ( 5 * V + 32 ) >> 6;

    i00 = a - b * 7 - c * 7 + 16;

    for( y = 0; y < 16; y++ )
    {
        for( x = 0; x < 16; x++ )
        {
            int pix;

            pix = (i00+b*x)>>5;

            src[x] = clip_uint8( pix );
        }
        src += i_stride;
        i00 += c;
    }
}


/****************************************************************************
 * 8x8 prediction for intra chroma block DC, H, V, P
 ****************************************************************************/
static void predict_8x8_dc_128( uint8_t *src, int i_stride )
{
    int x,y;

    for( y = 0; y < 8; y++ )
    {
        for( x = 0; x < 8; x++ )
        {
            src[x] = 128;
        }
        src += i_stride;
    }
}
static void predict_8x8_dc_left( uint8_t *src, int i_stride )
{
    int x,y;
    int dc0 = 0, dc1 = 0;

    for( y = 0; y < 4; y++ )
    {
        dc0 += src[y * i_stride     - 1];
        dc1 += src[(y+4) * i_stride - 1];
    }
    dc0 = ( dc0 + 2 ) >> 2;
    dc1 = ( dc1 + 2 ) >> 2;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 8; x++ )
        {
            src[           x] = dc0;
            src[4*i_stride+x] = dc1;
        }
        src += i_stride;
    }
}
static void predict_8x8_dc_top( uint8_t *src, int i_stride )
{
    int x,y;
    int dc0 = 0, dc1 = 0;

    for( x = 0; x < 4; x++ )
    {
        dc0 += src[x     - i_stride];
        dc1 += src[x + 4 - i_stride];
    }
    dc0 = ( dc0 + 2 ) >> 2;
    dc1 = ( dc1 + 2 ) >> 2;

    for( y = 0; y < 8; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            src[x    ] = dc0;
            src[x + 4] = dc1;
        }
        src += i_stride;
    }
}
static void predict_8x8_dc( uint8_t *src, int i_stride )
{
    int x,y;
    int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    int dc0, dc1, dc2, dc3;
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
    dc0 = ( s0 + s2 + 4 ) >> 3;
    dc1 = ( s1 + 2 ) >> 2;
    dc2 = ( s3 + 2 ) >> 2;
    dc3 = ( s1 + s3 + 4 ) >> 3;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            src[             x    ] = dc0;
            src[             x + 4] = dc1;
            src[4*i_stride + x    ] = dc2;
            src[4*i_stride + x + 4] = dc3;
        }
        src += i_stride;
    }
}

static void predict_8x8_h( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 8; i++ )
    {
        uint8_t v;

        v = src[-1];

        for( j = 0; j < 8; j++ )
        {
            src[j] = v;
        }
        src += i_stride;
    }
}
static void predict_8x8_v( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 8; i++ )
    {
        for( j = 0; j < 8; j++ )
        {
            src[i * i_stride +j] = src[j - i_stride];
        }
    }
}

static void predict_8x8_p( uint8_t *src, int i_stride )
{
    int i;
    int x,y;
    int a, b, c;
    int H = 0;
    int V = 0;
    int i00;

    for( i = 0; i < 4; i++ )
    {
        H += ( i + 1 ) * ( src[4+i - i_stride] - src[2 - i -i_stride] );
        V += ( i + 1 ) * ( src[-1 +(i+4)*i_stride] - src[-1+(2-i)*i_stride] );
    }

    a = 16 * ( src[-1+7*i_stride] + src[7 - i_stride] );
    b = ( 17 * H + 16 ) >> 5;
    c = ( 17 * V + 16 ) >> 5;
    i00 = a -3*b -3*c + 16;

    for( y = 0; y < 8; y++ )
    {
        for( x = 0; x < 8; x++ )
        {
            int pix;

            pix = (i00 +b*x) >> 5;
            src[x] = clip_uint8( pix );
        }
        src += i_stride;
        i00 += c;
    }
}

/****************************************************************************
 * 4x4 prediction for intra luma block DC, H, V, P
 ****************************************************************************/
static void predict_4x4_dc_128( uint8_t *src, int i_stride )
{
    int x,y;
    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            src[x] = 128;
        }
        src += i_stride;
    }
}
static void predict_4x4_dc_left( uint8_t *src, int i_stride )
{
    int x,y;
    int dc = ( src[-1+0*i_stride] + src[-1+i_stride]+
               src[-1+2*i_stride] + src[-1+3*i_stride] + 2 ) >> 2;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            src[x] = dc;
        }
        src += i_stride;
    }
}
static void predict_4x4_dc_top( uint8_t *src, int i_stride )
{
    int x,y;
    int dc = ( src[0 - i_stride] + src[1 - i_stride] +
               src[2 - i_stride] + src[3 - i_stride] + 2 ) >> 2;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            src[x] = dc;
        }
        src += i_stride;
    }
}
static void predict_4x4_dc( uint8_t *src, int i_stride )
{
    int x,y;
    int dc = ( src[-1+0*i_stride] + src[-1+i_stride]+
               src[-1+2*i_stride] + src[-1+3*i_stride] +
               src[0 - i_stride]  + src[1 - i_stride] +
               src[2 - i_stride]  + src[3 - i_stride] + 4 ) >> 3;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            src[x] = dc;
        }
        src += i_stride;
    }
}
static void predict_4x4_h( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 4; i++ )
    {
        uint8_t v;

        v = src[-1];

        for( j = 0; j < 4; j++ )
        {
            src[j] = v;
        }
        src += i_stride;
    }
}
static void predict_4x4_v( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 4; i++ )
    {
        for( j = 0; j < 4; j++ )
        {
            src[i * i_stride +j] = src[j - i_stride];
        }
    }
}

#define PREDICT_4x4_LOAD_LEFT \
    const int l0 = src[-1+0*i_stride];   \
    const int l1 = src[-1+1*i_stride];   \
    const int l2 = src[-1+2*i_stride];   \
    const int l3 = src[-1+3*i_stride];

#define PREDICT_4x4_LOAD_TOP \
    const int t0 = src[0-1*i_stride];   \
    const int t1 = src[1-1*i_stride];   \
    const int t2 = src[2-1*i_stride];   \
    const int t3 = src[3-1*i_stride];

#define PREDICT_4x4_LOAD_TOP_RIGHT \
    const int t4 = src[4-1*i_stride];   \
    const int t5 = src[5-1*i_stride];   \
    const int t6 = src[6-1*i_stride];   \
    const int t7 = src[7-1*i_stride];


static void predict_4x4_ddl( uint8_t *src, int i_stride )
{
    PREDICT_4x4_LOAD_TOP
    PREDICT_4x4_LOAD_TOP_RIGHT

    src[0*i_stride+0] = ( t0 + 2*t1+ t2 + 2 ) >> 2;

    src[0*i_stride+1] =
    src[1*i_stride+0] = ( t1 + 2*t2+ t3 + 2 ) >> 2;

    src[0*i_stride+2] =
    src[1*i_stride+1] =
    src[2*i_stride+0] = ( t2 + 2*t3+ t4 + 2 ) >> 2;

    src[0*i_stride+3] =
    src[1*i_stride+2] =
    src[2*i_stride+1] =
    src[3*i_stride+0] = ( t3 + 2*t4+ t5 + 2 ) >> 2;

    src[1*i_stride+3] =
    src[2*i_stride+2] =
    src[3*i_stride+1] = ( t4 + 2*t5+ t6 + 2 ) >> 2;

    src[2*i_stride+3] =
    src[3*i_stride+2] = ( t5 + 2*t6+ t7 + 2 ) >> 2;

    src[3*i_stride+3] = ( t6 + 3 * t7 + 2 ) >> 2;
}
static void predict_4x4_ddr( uint8_t *src, int i_stride )
{
    const int lt = src[-1-i_stride];
    PREDICT_4x4_LOAD_LEFT
    PREDICT_4x4_LOAD_TOP

    src[0*i_stride+0] =
    src[1*i_stride+1] =
    src[2*i_stride+2] =
    src[3*i_stride+3] = ( t0 + 2*lt +l0 + 2 ) >> 2;

    src[0*i_stride+1] =
    src[1*i_stride+2] =
    src[2*i_stride+3] = ( lt + 2 * t0 + t1 + 2 ) >> 2;

    src[0*i_stride+2] =
    src[1*i_stride+3] = ( t0 + 2 * t1 + t2 + 2 ) >> 2;

    src[0*i_stride+3] = ( t1 + 2 * t2 + t3 + 2 ) >> 2;

    src[1*i_stride+0] =
    src[2*i_stride+1] =
    src[3*i_stride+2] = ( lt + 2 * l0 + l1 + 2 ) >> 2;

    src[2*i_stride+0] =
    src[3*i_stride+1] = ( l0 + 2 * l1 + l2 + 2 ) >> 2;

    src[3*i_stride+0] = ( l1 + 2 * l2 + l3 + 2 ) >> 2;
}

static void predict_4x4_vr( uint8_t *src, int i_stride )
{
    const int lt = src[-1-i_stride];
    PREDICT_4x4_LOAD_LEFT
    PREDICT_4x4_LOAD_TOP
    /* produce warning as l3 is unused */

    src[0*i_stride+0]=
    src[2*i_stride+1]= ( lt + t0 + 1 ) >> 1;

    src[0*i_stride+1]=
    src[2*i_stride+2]= ( t0 + t1 + 1 ) >> 1;

    src[0*i_stride+2]=
    src[2*i_stride+3]= ( t1 + t2 + 1 ) >> 1;

    src[0*i_stride+3]= ( t2 + t3 + 1 ) >> 1;

    src[1*i_stride+0]=
    src[3*i_stride+1]= ( l0 + 2 * lt + t0 + 2 ) >> 2;

    src[1*i_stride+1]=
    src[3*i_stride+2]= ( lt + 2 * t0 + t1 + 2 ) >> 2;

    src[1*i_stride+2]=
    src[3*i_stride+3]= ( t0 + 2 * t1 + t2 + 2) >> 2;

    src[1*i_stride+3]= ( t1 + 2 * t2 + t3 + 2 ) >> 2;
    src[2*i_stride+0]= ( lt + 2 * l0 + l1 + 2 ) >> 2;
    src[3*i_stride+0]= ( l0 + 2 * l1 + l2 + 2 ) >> 2;
}

static void predict_4x4_hd( uint8_t *src, int i_stride )
{
    const int lt= src[-1-1*i_stride];
    PREDICT_4x4_LOAD_LEFT
    PREDICT_4x4_LOAD_TOP
    /* produce warning as t3 is unused */

    src[0*i_stride+0]=
    src[1*i_stride+2]= ( lt + l0 + 1 ) >> 1;
    src[0*i_stride+1]=
    src[1*i_stride+3]= ( l0 + 2 * lt + t0 + 2 ) >> 2;
    src[0*i_stride+2]= ( lt + 2 * t0 + t1 + 2 ) >> 2;
    src[0*i_stride+3]= ( t0 + 2 * t1 + t2 + 2 ) >> 2;
    src[1*i_stride+0]=
    src[2*i_stride+2]= ( l0 + l1 + 1 ) >> 1;
    src[1*i_stride+1]=
    src[2*i_stride+3]= ( lt + 2 * l0 + l1 + 2 ) >> 2;
    src[2*i_stride+0]=
    src[3*i_stride+2]= ( l1 + l2+ 1 ) >> 1;
    src[2*i_stride+1]=
    src[3*i_stride+3]= ( l0 + 2 * l1 + l2 + 2 ) >> 2;
    src[3*i_stride+0]= ( l2 + l3 + 1 ) >> 1;
    src[3*i_stride+1]= ( l1 + 2 * l2 + l3 + 2 ) >> 2;
}

static void predict_4x4_vl( uint8_t *src, int i_stride )
{
    PREDICT_4x4_LOAD_TOP
    PREDICT_4x4_LOAD_TOP_RIGHT
    /* produce warning as t7 is unused */

    src[0*i_stride+0]= ( t0 + t1 + 1 ) >> 1;
    src[0*i_stride+1]=
    src[2*i_stride+0]= ( t1 + t2 + 1 ) >> 1;
    src[0*i_stride+2]=
    src[2*i_stride+1]= ( t2 + t3 + 1 ) >> 1;
    src[0*i_stride+3]=
    src[2*i_stride+2]= ( t3 + t4+ 1 ) >> 1;
    src[2*i_stride+3]= ( t4 + t5+ 1 ) >> 1;
    src[1*i_stride+0]= ( t0 + 2 * t1 + t2 + 2 ) >> 2;
    src[1*i_stride+1]=
    src[3*i_stride+0]= ( t1 + 2 * t2 + t3 + 2 ) >> 2;
    src[1*i_stride+2]=
    src[3*i_stride+1]= ( t2 + 2 * t3 + t4 + 2 ) >> 2;
    src[1*i_stride+3]=
    src[3*i_stride+2]= ( t3 + 2 * t4 + t5 + 2 ) >> 2;
    src[3*i_stride+3]= ( t4 + 2 * t5 + t6 + 2 ) >> 2;
}

static void predict_4x4_hu( uint8_t *src, int i_stride )
{
    PREDICT_4x4_LOAD_LEFT

    src[0*i_stride+0]= ( l0 + l1 + 1 ) >> 1;
    src[0*i_stride+1]= ( l0 + 2 * l1 + l2 + 2 ) >> 2;

    src[0*i_stride+2]=
    src[1*i_stride+0]= ( l1 + l2 + 1 ) >> 1;

    src[0*i_stride+3]=
    src[1*i_stride+1]= ( l1 + 2*l2 + l3 + 2 ) >> 2;

    src[1*i_stride+2]=
    src[2*i_stride+0]= ( l2 + l3 + 1 ) >> 1;

    src[1*i_stride+3]=
    src[2*i_stride+1]= ( l2 + 2 * l3 + l3 + 2 ) >> 2;

    src[2*i_stride+3]=
    src[3*i_stride+1]=
    src[3*i_stride+0]=
    src[2*i_stride+2]=
    src[3*i_stride+2]=
    src[3*i_stride+3]= l3;
}

/****************************************************************************
 * Exported functions:
 ****************************************************************************/
void x264_predict_16x16_init( int cpu, x264_predict_t pf[7] )
{
    pf[I_PRED_16x16_V ]     = predict_16x16_v;
    pf[I_PRED_16x16_H ]     = predict_16x16_h;
    pf[I_PRED_16x16_DC]     = predict_16x16_dc;
    pf[I_PRED_16x16_P ]     = predict_16x16_p;
    pf[I_PRED_16x16_DC_LEFT]= predict_16x16_dc_left;
    pf[I_PRED_16x16_DC_TOP ]= predict_16x16_dc_top;
    pf[I_PRED_16x16_DC_128 ]= predict_16x16_dc_128;

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT )
    {
        x264_predict_16x16_init_mmxext( pf );
    }
#endif
}

void x264_predict_8x8_init( int cpu, x264_predict_t pf[7] )
{
    pf[I_PRED_CHROMA_V ]     = predict_8x8_v;
    pf[I_PRED_CHROMA_H ]     = predict_8x8_h;
    pf[I_PRED_CHROMA_DC]     = predict_8x8_dc;
    pf[I_PRED_CHROMA_P ]     = predict_8x8_p;
    pf[I_PRED_CHROMA_DC_LEFT]= predict_8x8_dc_left;
    pf[I_PRED_CHROMA_DC_TOP ]= predict_8x8_dc_top;
    pf[I_PRED_CHROMA_DC_128 ]= predict_8x8_dc_128;

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT )
    {
        x264_predict_8x8_init_mmxext( pf );
    }
#endif
}

void x264_predict_4x4_init( int cpu, x264_predict_t pf[12] )
{
    pf[I_PRED_4x4_V]      = predict_4x4_v;
    pf[I_PRED_4x4_H]      = predict_4x4_h;
    pf[I_PRED_4x4_DC]     = predict_4x4_dc;
    pf[I_PRED_4x4_DDL]    = predict_4x4_ddl;
    pf[I_PRED_4x4_DDR]    = predict_4x4_ddr;
    pf[I_PRED_4x4_VR]     = predict_4x4_vr;
    pf[I_PRED_4x4_HD]     = predict_4x4_hd;
    pf[I_PRED_4x4_VL]     = predict_4x4_vl;
    pf[I_PRED_4x4_HU]     = predict_4x4_hu;
    pf[I_PRED_4x4_DC_LEFT]= predict_4x4_dc_left;
    pf[I_PRED_4x4_DC_TOP] = predict_4x4_dc_top;
    pf[I_PRED_4x4_DC_128] = predict_4x4_dc_128;

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT )
    {
        x264_predict_4x4_init_mmxext( pf );
    }
#endif
}

