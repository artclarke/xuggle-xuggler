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

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif

#include "common/clip1.h"
#include "common/common.h"
#include "predict.h"

extern void predict_16x16_v_mmx( uint8_t *src, int i_stride );
extern void predict_16x16_dc_core_mmxext( uint8_t *src, int i_stride, int i_dc_left );
extern void predict_16x16_dc_top_mmxext( uint8_t *src, int i_stride );
extern void predict_16x16_p_core_mmx( uint8_t *src, int i_stride, int i00, int b, int c );
extern void predict_8x8c_p_core_mmx( uint8_t *src, int i_stride, int i00, int b, int c );
extern void predict_8x8c_dc_core_mmxext( uint8_t *src, int i_stride, int s2, int s3 );
extern void predict_8x8c_v_mmx( uint8_t *src, int i_stride );
extern void predict_8x8_v_mmxext( uint8_t *src, int i_stride, int i_neighbors );
extern void predict_8x8_dc_core_mmxext( uint8_t *src, int i_stride, int i_neighbors, uint8_t *pix_left );

static void predict_16x16_p( uint8_t *src, int i_stride )
{
    int a, b, c, i;
    int H = 0;
    int V = 0;
    int i00;

    for( i = 1; i <= 8; i++ )
    {
        H += i * ( src[7+i - i_stride ]  - src[7-i - i_stride ] );
        V += i * ( src[(7+i)*i_stride -1] - src[(7-i)*i_stride -1] );
    }

    a = 16 * ( src[15*i_stride -1] + src[15 - i_stride] );
    b = ( 5 * H + 32 ) >> 6;
    c = ( 5 * V + 32 ) >> 6;
    i00 = a - b * 7 - c * 7 + 16;

    predict_16x16_p_core_mmx( src, i_stride, i00, b, c );
}

static void predict_8x8c_p( uint8_t *src, int i_stride )
{
    int a, b, c, i;
    int H = 0;
    int V = 0;
    int i00;

    for( i = 1; i <= 4; i++ )
    {
        H += i * ( src[3+i - i_stride] - src[3-i - i_stride] );
        V += i * ( src[(3+i)*i_stride -1] - src[(3-i)*i_stride -1] );
    }

    a = 16 * ( src[7*i_stride -1] + src[7 - i_stride] );
    b = ( 17 * H + 16 ) >> 5;
    c = ( 17 * V + 16 ) >> 5;
    i00 = a -3*b -3*c + 16;

    predict_8x8c_p_core_mmx( src, i_stride, i00, b, c );
}

static void predict_16x16_dc( uint8_t *src, int i_stride )
{
    uint32_t dc=16;
    int i;

    for( i = 0; i < 16; i+=2 )
    {
        dc += src[-1 + i * i_stride];
        dc += src[-1 + (i+1) * i_stride];
    }

    predict_16x16_dc_core_mmxext( src, i_stride, dc );
}

static void predict_8x8c_dc( uint8_t *src, int i_stride )
{
    int s2 = 4
       + src[-1 + 0*i_stride]
       + src[-1 + 1*i_stride]
       + src[-1 + 2*i_stride]
       + src[-1 + 3*i_stride];

    int s3 = 2
       + src[-1 + 4*i_stride]
       + src[-1 + 5*i_stride]
       + src[-1 + 6*i_stride]
       + src[-1 + 7*i_stride];

    predict_8x8c_dc_core_mmxext( src, i_stride, s2, s3 );
}

#define SRC(x,y) src[(x)+(y)*i_stride]
static void predict_8x8_dc( uint8_t *src, int i_stride, int i_neighbor )
{
    uint8_t l[10];
    l[0] = i_neighbor&MB_TOPLEFT ? SRC(-1,-1) : SRC(-1,0);
    l[1] = SRC(-1,0);
    l[2] = SRC(-1,1);
    l[3] = SRC(-1,2);
    l[4] = SRC(-1,3);
    l[5] = SRC(-1,4);
    l[6] = SRC(-1,5);
    l[7] = SRC(-1,6);
    l[8] =
    l[9] = SRC(-1,7);

    predict_8x8_dc_core_mmxext( src, i_stride, i_neighbor, l+1 );
}

/****************************************************************************
 * Exported functions:
 ****************************************************************************/
void x264_predict_16x16_init_mmxext( x264_predict_t pf[7] )
{
    pf[I_PRED_16x16_V] = predict_16x16_v_mmx;
    pf[I_PRED_16x16_DC] = predict_16x16_dc;
    pf[I_PRED_16x16_DC_TOP] = predict_16x16_dc_top_mmxext;
    pf[I_PRED_16x16_P] = predict_16x16_p;
}

void x264_predict_8x8c_init_mmxext( x264_predict_t pf[7] )
{
    pf[I_PRED_CHROMA_V] = predict_8x8c_v_mmx;
    pf[I_PRED_CHROMA_P] = predict_8x8c_p;
    pf[I_PRED_CHROMA_DC] = predict_8x8c_dc;
}

void x264_predict_8x8_init_mmxext( x264_predict8x8_t pf[12] )
{
    pf[I_PRED_8x8_V] = predict_8x8_v_mmxext;
    pf[I_PRED_8x8_DC] = predict_8x8_dc;
}

