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

#include "common/common.h"
#include "common/clip1.h"
#include "predict.h"

extern void predict_16x16_v_mmx( uint8_t *src );
extern void predict_16x16_dc_core_mmxext( uint8_t *src, int i_dc_left );
extern void predict_16x16_dc_top_mmxext( uint8_t *src );
extern void predict_16x16_p_core_mmx( uint8_t *src, int i00, int b, int c );
extern void predict_8x8c_p_core_mmx( uint8_t *src, int i00, int b, int c );
extern void predict_8x8c_dc_core_mmxext( uint8_t *src, int s2, int s3 );
extern void predict_8x8c_v_mmx( uint8_t *src );
extern void predict_8x8_v_mmxext( uint8_t *src, int i_neighbors );
extern void predict_8x8_dc_core_mmxext( uint8_t *src, int i_neighbors, uint8_t *pix_left );

static void predict_16x16_p( uint8_t *src )
{
    int a, b, c, i;
    int H = 0;
    int V = 0;
    int i00;

    for( i = 1; i <= 8; i++ )
    {
        H += i * ( src[7+i - FDEC_STRIDE ]  - src[7-i - FDEC_STRIDE ] );
        V += i * ( src[(7+i)*FDEC_STRIDE -1] - src[(7-i)*FDEC_STRIDE -1] );
    }

    a = 16 * ( src[15*FDEC_STRIDE -1] + src[15 - FDEC_STRIDE] );
    b = ( 5 * H + 32 ) >> 6;
    c = ( 5 * V + 32 ) >> 6;
    i00 = a - b * 7 - c * 7 + 16;

    predict_16x16_p_core_mmx( src, i00, b, c );
}

static void predict_8x8c_p( uint8_t *src )
{
    int a, b, c, i;
    int H = 0;
    int V = 0;
    int i00;

    for( i = 1; i <= 4; i++ )
    {
        H += i * ( src[3+i - FDEC_STRIDE] - src[3-i - FDEC_STRIDE] );
        V += i * ( src[(3+i)*FDEC_STRIDE -1] - src[(3-i)*FDEC_STRIDE -1] );
    }

    a = 16 * ( src[7*FDEC_STRIDE -1] + src[7 - FDEC_STRIDE] );
    b = ( 17 * H + 16 ) >> 5;
    c = ( 17 * V + 16 ) >> 5;
    i00 = a -3*b -3*c + 16;

    predict_8x8c_p_core_mmx( src, i00, b, c );
}

static void predict_16x16_dc( uint8_t *src )
{
    uint32_t dc=16;
    int i;

    for( i = 0; i < 16; i+=2 )
    {
        dc += src[-1 + i * FDEC_STRIDE];
        dc += src[-1 + (i+1) * FDEC_STRIDE];
    }

    predict_16x16_dc_core_mmxext( src, dc );
}

static void predict_8x8c_dc( uint8_t *src )
{
    int s2 = 4
       + src[-1 + 0*FDEC_STRIDE]
       + src[-1 + 1*FDEC_STRIDE]
       + src[-1 + 2*FDEC_STRIDE]
       + src[-1 + 3*FDEC_STRIDE];

    int s3 = 2
       + src[-1 + 4*FDEC_STRIDE]
       + src[-1 + 5*FDEC_STRIDE]
       + src[-1 + 6*FDEC_STRIDE]
       + src[-1 + 7*FDEC_STRIDE];

    predict_8x8c_dc_core_mmxext( src, s2, s3 );
}

#define SRC(x,y) src[(x)+(y)*FDEC_STRIDE]
static void predict_8x8_dc( uint8_t *src, int i_neighbor )
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

    predict_8x8_dc_core_mmxext( src, i_neighbor, l+1 );
}

#ifdef ARCH_X86_64
static void predict_16x16_h( uint8_t *src )
{
    int y;
    for( y = 0; y < 16; y++ )
    {
        const uint64_t v = 0x0101010101010101ULL * src[-1];
        uint64_t *p = (uint64_t*)src;
        p[0] = p[1] = v;
        src += FDEC_STRIDE;
    }
}

static void predict_8x8c_h( uint8_t *src )
{
    int y;
    for( y = 0; y < 8; y++ )
    {
        *(uint64_t*)src = 0x0101010101010101ULL * src[-1];
        src += FDEC_STRIDE;
    }
}

static void predict_16x16_dc_left( uint8_t *src )
{
    uint32_t s = 0;
    uint64_t dc; 
    int y;
    
    for( y = 0; y < 16; y++ )
    {
        s += src[-1 + y * FDEC_STRIDE];
    }   
    dc = (( s + 8 ) >> 4) * 0x0101010101010101ULL;
    
    for( y = 0; y < 16; y++ )
    {
        uint64_t *p = (uint64_t*)src;
        p[0] = p[1] = dc;
        src += FDEC_STRIDE;
    }
}

static void predict_8x8c_dc_left( uint8_t *src )
{
    int y;
    uint32_t s0 = 0, s1 = 0;
    uint64_t dc0, dc1;

    for( y = 0; y < 4; y++ )
    {
        s0 += src[y * FDEC_STRIDE     - 1];
        s1 += src[(y+4) * FDEC_STRIDE - 1];
    }
    dc0 = (( s0 + 2 ) >> 2) * 0x0101010101010101ULL;
    dc1 = (( s1 + 2 ) >> 2) * 0x0101010101010101ULL;

    for( y = 0; y < 4; y++ )
    {
        *(uint64_t*)src = dc0;
        src += FDEC_STRIDE;
    }
    for( y = 0; y < 4; y++ )
    {
        *(uint64_t*)src = dc1;
        src += FDEC_STRIDE;
    }

}

static void predict_8x8c_dc_top( uint8_t *src )
{
    int y, x;
    uint32_t s0 = 0, s1 = 0;
    uint64_t dc;

    for( x = 0; x < 4; x++ )
    {
        s0 += src[x     - FDEC_STRIDE];
        s1 += src[x + 4 - FDEC_STRIDE];
    }
    dc = (( s0 + 2 ) >> 2) * 0x01010101
       + (( s1 + 2 ) >> 2) * 0x0101010100000000ULL;

    for( y = 0; y < 8; y++ )
    {
        *(uint64_t*)src = dc;
        src += FDEC_STRIDE;
    }
}
#endif

/****************************************************************************
 * Exported functions:
 ****************************************************************************/
void x264_predict_16x16_init_mmxext( x264_predict_t pf[7] )
{
    pf[I_PRED_16x16_V]       = predict_16x16_v_mmx;
    pf[I_PRED_16x16_DC]      = predict_16x16_dc;
    pf[I_PRED_16x16_DC_TOP]  = predict_16x16_dc_top_mmxext;
    pf[I_PRED_16x16_P]       = predict_16x16_p;

#ifdef ARCH_X86_64
    pf[I_PRED_16x16_H]       = predict_16x16_h;
    pf[I_PRED_16x16_DC_LEFT] = predict_16x16_dc_left;
#endif
}

void x264_predict_8x8c_init_mmxext( x264_predict_t pf[7] )
{
    pf[I_PRED_CHROMA_V]       = predict_8x8c_v_mmx;
    pf[I_PRED_CHROMA_P]       = predict_8x8c_p;
    pf[I_PRED_CHROMA_DC]      = predict_8x8c_dc;

#ifdef ARCH_X86_64
    pf[I_PRED_CHROMA_H]       = predict_8x8c_h;
    pf[I_PRED_CHROMA_DC_LEFT] = predict_8x8c_dc_left;
    pf[I_PRED_CHROMA_DC_TOP]  = predict_8x8c_dc_top;
#endif
}

void x264_predict_8x8_init_mmxext( x264_predict8x8_t pf[12] )
{
    pf[I_PRED_8x8_V]  = predict_8x8_v_mmxext;
    pf[I_PRED_8x8_DC] = predict_8x8_dc;
}

