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
#include "pixel.h"

extern void predict_16x16_v_mmx( uint8_t *src );
extern void predict_16x16_dc_core_mmxext( uint8_t *src, int i_dc_left );
extern void predict_16x16_dc_top_mmxext( uint8_t *src );
extern void predict_16x16_p_core_mmxext( uint8_t *src, int i00, int b, int c );
extern void predict_8x8c_p_core_mmxext( uint8_t *src, int i00, int b, int c );
extern void predict_8x8c_dc_core_mmxext( uint8_t *src, int s2, int s3 );
extern void predict_8x8c_v_mmx( uint8_t *src );
extern void predict_8x8_v_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_dc_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_dc_top_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_dc_left_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_ddl_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_ddr_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_ddl_sse2( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_ddr_sse2( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_vl_sse2( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_vr_core_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_4x4_ddl_mmxext( uint8_t *src );
extern void predict_4x4_vl_mmxext( uint8_t *src );

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

    predict_16x16_p_core_mmxext( src, i00, b, c );
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

    predict_8x8c_p_core_mmxext( src, i00, b, c );
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

/* Diagonals */

#define PREDICT_4x4_LOAD_LEFT \
    const int l0 = src[-1+0*FDEC_STRIDE];   \
    const int l1 = src[-1+1*FDEC_STRIDE];   \
    const int l2 = src[-1+2*FDEC_STRIDE];   \
    UNUSED const int l3 = src[-1+3*FDEC_STRIDE];

#define PREDICT_4x4_LOAD_TOP \
    const int t0 = src[0-1*FDEC_STRIDE];   \
    const int t1 = src[1-1*FDEC_STRIDE];   \
    const int t2 = src[2-1*FDEC_STRIDE];   \
    UNUSED const int t3 = src[3-1*FDEC_STRIDE];

#define PREDICT_4x4_LOAD_TOP_RIGHT \
    const int t4 = src[4-1*FDEC_STRIDE];   \
    const int t5 = src[5-1*FDEC_STRIDE];   \
    const int t6 = src[6-1*FDEC_STRIDE];   \
    UNUSED const int t7 = src[7-1*FDEC_STRIDE];

#define F1(a,b)   (((a)+(b)+1)>>1)
#define F2(a,b,c) (((a)+2*(b)+(c)+2)>>2)

#ifdef ARCH_X86_64 // slower on x86
#if 0
static void predict_4x4_ddl( uint8_t *src )
{
    PREDICT_4x4_LOAD_TOP
    PREDICT_4x4_LOAD_TOP_RIGHT
    uint32_t vec = (F2(t3,t4,t5)<< 0)
                 + (F2(t4,t5,t6)<< 8)
                 + (F2(t5,t6,t7)<<16)
                 + (F2(t6,t7,t7)<<24);
    *(uint32_t*)&src[3*FDEC_STRIDE] = vec;
    *(uint32_t*)&src[2*FDEC_STRIDE] = vec = (vec<<8) + F2(t2,t3,t4);
    *(uint32_t*)&src[1*FDEC_STRIDE] = vec = (vec<<8) + F2(t1,t2,t3);
    *(uint32_t*)&src[0*FDEC_STRIDE] = vec = (vec<<8) + F2(t0,t1,t2);
}
#endif

static void predict_4x4_ddr( uint8_t *src )
{
    const int lt = src[-1-FDEC_STRIDE];
    PREDICT_4x4_LOAD_LEFT
    PREDICT_4x4_LOAD_TOP
    uint32_t vec = (F2(l0,lt,t0)<< 0)
                 + (F2(lt,t0,t1)<< 8)
                 + (F2(t0,t1,t2)<<16)
                 + (F2(t1,t2,t3)<<24);
    *(uint32_t*)&src[0*FDEC_STRIDE] = vec;
    *(uint32_t*)&src[1*FDEC_STRIDE] = vec = (vec<<8) + F2(l1,l0,lt);
    *(uint32_t*)&src[2*FDEC_STRIDE] = vec = (vec<<8) + F2(l2,l1,l0);
    *(uint32_t*)&src[3*FDEC_STRIDE] = vec = (vec<<8) + F2(l3,l2,l1);
}

static void predict_4x4_vr( uint8_t *src )
{
    const int lt = src[-1-FDEC_STRIDE];
    PREDICT_4x4_LOAD_LEFT
    PREDICT_4x4_LOAD_TOP
    const int ltt0 = lt + t0 + 1;
    const int t0t1 = t0 + t1 + 1;
    const int t1t2 = t1 + t2 + 1;
    const int t2t3 = t2 + t3 + 1;
    const int l0lt = l0 + lt + 1;
    const int l1l0 = l1 + l0 + 1;
    const int l2l1 = l2 + l1 + 1;

    src[0*FDEC_STRIDE+0]=
    src[2*FDEC_STRIDE+1]= ltt0 >> 1;

    src[0*FDEC_STRIDE+1]=
    src[2*FDEC_STRIDE+2]= t0t1 >> 1;

    src[0*FDEC_STRIDE+2]=
    src[2*FDEC_STRIDE+3]= t1t2 >> 1;

    src[0*FDEC_STRIDE+3]= t2t3 >> 1;

    src[1*FDEC_STRIDE+0]=
    src[3*FDEC_STRIDE+1]= (l0lt + ltt0) >> 2;

    src[1*FDEC_STRIDE+1]=
    src[3*FDEC_STRIDE+2]= (ltt0 + t0t1) >> 2;

    src[1*FDEC_STRIDE+2]=
    src[3*FDEC_STRIDE+3]= (t0t1 + t1t2) >> 2;

    src[1*FDEC_STRIDE+3]= (t1t2 + t2t3) >> 2;
    src[2*FDEC_STRIDE+0]= (l1l0 + l0lt) >> 2;
    src[3*FDEC_STRIDE+0]= (l2l1 + l1l0) >> 2;
}

static void predict_4x4_hd( uint8_t *src )
{
    const int lt= src[-1-1*FDEC_STRIDE];
    PREDICT_4x4_LOAD_LEFT
    PREDICT_4x4_LOAD_TOP
    const int ltt0 = lt + t0 + 1;
    const int t0t1 = t0 + t1 + 1;
    const int t1t2 = t1 + t2 + 1;
    const int l0lt = l0 + lt + 1;
    const int l1l0 = l1 + l0 + 1;
    const int l2l1 = l2 + l1 + 1;
    const int l3l2 = l3 + l2 + 1;

    src[0*FDEC_STRIDE+0]=
    src[1*FDEC_STRIDE+2]= l0lt >> 1;
    src[0*FDEC_STRIDE+1]=
    src[1*FDEC_STRIDE+3]= (l0lt + ltt0) >> 2;
    src[0*FDEC_STRIDE+2]= (ltt0 + t0t1) >> 2;
    src[0*FDEC_STRIDE+3]= (t0t1 + t1t2) >> 2;
    src[1*FDEC_STRIDE+0]=
    src[2*FDEC_STRIDE+2]= l1l0 >> 1;
    src[1*FDEC_STRIDE+1]=
    src[2*FDEC_STRIDE+3]= (l0lt + l1l0) >> 2;
    src[2*FDEC_STRIDE+0]=
    src[3*FDEC_STRIDE+2]= l2l1 >> 1;
    src[2*FDEC_STRIDE+1]=
    src[3*FDEC_STRIDE+3]= (l1l0 + l2l1) >> 2;
    src[3*FDEC_STRIDE+0]= l3l2 >> 1;
    src[3*FDEC_STRIDE+1]= (l2l1 + l3l2) >> 2;
}

#if 0
static void predict_4x4_vl( uint8_t *src )
{
    PREDICT_4x4_LOAD_TOP
    PREDICT_4x4_LOAD_TOP_RIGHT
    const int t0t1 = t0 + t1 + 1;
    const int t1t2 = t1 + t2 + 1;
    const int t2t3 = t2 + t3 + 1;
    const int t3t4 = t3 + t4 + 1;
    const int t4t5 = t4 + t5 + 1;
    const int t5t6 = t5 + t6 + 1;

    src[0*FDEC_STRIDE+0]= t0t1 >> 1;
    src[0*FDEC_STRIDE+1]=
    src[2*FDEC_STRIDE+0]= t1t2 >> 1;
    src[0*FDEC_STRIDE+2]=
    src[2*FDEC_STRIDE+1]= t2t3 >> 1;
    src[0*FDEC_STRIDE+3]=
    src[2*FDEC_STRIDE+2]= t3t4 >> 1;
    src[2*FDEC_STRIDE+3]= t4t5 >> 1;
    src[1*FDEC_STRIDE+0]= (t0t1 + t1t2) >> 2;
    src[1*FDEC_STRIDE+1]=
    src[3*FDEC_STRIDE+0]= (t1t2 + t2t3) >> 2;
    src[1*FDEC_STRIDE+2]=
    src[3*FDEC_STRIDE+1]= (t2t3 + t3t4) >> 2;
    src[1*FDEC_STRIDE+3]=
    src[3*FDEC_STRIDE+2]= (t3t4 + t4t5) >> 2;
    src[3*FDEC_STRIDE+3]= (t4t5 + t5t6) >> 2;
}
#endif

static void predict_4x4_hu( uint8_t *src )
{
    PREDICT_4x4_LOAD_LEFT
    const int l1l0 = l1 + l0 + 1;
    const int l2l1 = l2 + l1 + 1;
    const int l3l2 = l3 + l2 + 1;

    src[0*FDEC_STRIDE+0]= l1l0 >> 1;
    src[0*FDEC_STRIDE+1]= (l1l0 + l2l1) >> 2;

    src[0*FDEC_STRIDE+2]=
    src[1*FDEC_STRIDE+0]= l2l1 >> 1;

    src[0*FDEC_STRIDE+3]=
    src[1*FDEC_STRIDE+1]= (l2l1 + l3l2) >> 2;

    src[1*FDEC_STRIDE+2]=
    src[2*FDEC_STRIDE+0]= l3l2 >> 1;

    src[1*FDEC_STRIDE+3]=
    src[2*FDEC_STRIDE+1]= (l2 + 3*l3 + 2) >> 2;

    src[2*FDEC_STRIDE+3]=
    src[3*FDEC_STRIDE+1]=
    src[3*FDEC_STRIDE+0]=
    src[2*FDEC_STRIDE+2]=
    src[3*FDEC_STRIDE+2]=
    src[3*FDEC_STRIDE+3]= l3;
}
#endif

/****************************************************************************
 * 8x8 prediction for intra luma block
 ****************************************************************************/

#define PL(y) \
    UNUSED int l##y = edge[14-y];
#define PT(x) \
    UNUSED int t##x = edge[16+x];
#define PREDICT_8x8_LOAD_TOPLEFT \
    int lt = edge[15];
#define PREDICT_8x8_LOAD_LEFT \
    PL(0) PL(1) PL(2) PL(3) PL(4) PL(5) PL(6) PL(7)
#define PREDICT_8x8_LOAD_TOP \
    PT(0) PT(1) PT(2) PT(3) PT(4) PT(5) PT(6) PT(7)

#define PREDICT_8x8_DC(v) \
    int y; \
    for( y = 0; y < 8; y++ ) { \
        ((uint32_t*)src)[0] = \
        ((uint32_t*)src)[1] = v; \
        src += FDEC_STRIDE; \
    }

#define SRC(x,y) src[(x)+(y)*FDEC_STRIDE]

static void predict_8x8_vr_mmxext( uint8_t *src, uint8_t edge[33] )
{
    predict_8x8_vr_core_mmxext( src, edge );
    {
        PREDICT_8x8_LOAD_TOPLEFT
        PREDICT_8x8_LOAD_LEFT
        SRC(0,2)=SRC(1,4)=SRC(2,6)= (l1 + 2*l0 + lt + 2) >> 2;
        SRC(0,3)=SRC(1,5)=SRC(2,7)= (l2 + 2*l1 + l0 + 2) >> 2;
        SRC(0,4)=SRC(1,6)= (l3 + 2*l2 + l1 + 2) >> 2;
        SRC(0,5)=SRC(1,7)= (l4 + 2*l3 + l2 + 2) >> 2;
        SRC(0,6)= (l5 + 2*l4 + l3 + 2) >> 2;
        SRC(0,7)= (l6 + 2*l5 + l4 + 2) >> 2;
    }
}

#define SUMSUB(a,b,c,d,e,f,g,h)\
    t=a; a+=b; b-=t;\
    t=c; c+=d; d-=t;\
    t=e; e+=f; f-=t;\
    t=g; g+=h; h-=t;

#ifdef ARCH_X86_64
void x264_intra_sa8d_x3_8x8_sse2( uint8_t *fenc, uint8_t edge[33], int res[3] )
#else
void x264_intra_sa8d_x3_8x8_mmxext( uint8_t *fenc, uint8_t edge[33], int res[3] )
#endif
{
    PREDICT_8x8_LOAD_TOP
    PREDICT_8x8_LOAD_LEFT
    int t;
    DECLARE_ALIGNED( int16_t, sa8d_1d[2][8], 16 );
    SUMSUB(l0,l4,l1,l5,l2,l6,l3,l7);
    SUMSUB(l0,l2,l1,l3,l4,l6,l5,l7);
    SUMSUB(l0,l1,l2,l3,l4,l5,l6,l7);
    sa8d_1d[0][0] = l0;
    sa8d_1d[0][1] = l1;
    sa8d_1d[0][2] = l2;
    sa8d_1d[0][3] = l3;
    sa8d_1d[0][4] = l4;
    sa8d_1d[0][5] = l5;
    sa8d_1d[0][6] = l6;
    sa8d_1d[0][7] = l7;
    SUMSUB(t0,t4,t1,t5,t2,t6,t3,t7);
    SUMSUB(t0,t2,t1,t3,t4,t6,t5,t7);
    SUMSUB(t0,t1,t2,t3,t4,t5,t6,t7);
    sa8d_1d[1][0] = t0;
    sa8d_1d[1][1] = t1;
    sa8d_1d[1][2] = t2;
    sa8d_1d[1][3] = t3;
    sa8d_1d[1][4] = t4;
    sa8d_1d[1][5] = t5;
    sa8d_1d[1][6] = t6;
    sa8d_1d[1][7] = t7;
#ifdef ARCH_X86_64
    x264_intra_sa8d_x3_8x8_core_sse2( fenc, sa8d_1d, res );
#else
    x264_intra_sa8d_x3_8x8_core_mmxext( fenc, sa8d_1d, res );
#endif
}

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
    pf[I_PRED_8x8_V]   = predict_8x8_v_mmxext;
    pf[I_PRED_8x8_DC]  = predict_8x8_dc_mmxext;
    pf[I_PRED_8x8_DC_TOP] = predict_8x8_dc_top_mmxext;
    pf[I_PRED_8x8_DC_LEFT]= predict_8x8_dc_left_mmxext;
    pf[I_PRED_8x8_DDL] = predict_8x8_ddl_mmxext;
    pf[I_PRED_8x8_VR]  = predict_8x8_vr_mmxext;
#ifdef ARCH_X86
    pf[I_PRED_8x8_DDR] = predict_8x8_ddr_mmxext;
#endif
}

void x264_predict_8x8_init_sse2( x264_predict8x8_t pf[12] )
{
#ifdef ARCH_X86_64 // x86 not written yet
    pf[I_PRED_8x8_DDL] = predict_8x8_ddl_sse2;
    pf[I_PRED_8x8_DDR] = predict_8x8_ddr_sse2;
    pf[I_PRED_8x8_VL]  = predict_8x8_vl_sse2;
#endif
}

void x264_predict_4x4_init_mmxext( x264_predict_t pf[12] )
{
#ifdef ARCH_X86_64 // x86 not written yet
    pf[I_PRED_4x4_DDL] = predict_4x4_ddl_mmxext;
    pf[I_PRED_4x4_VL]  = predict_4x4_vl_mmxext;
#endif
#ifdef ARCH_X86_64 // slower on x86
    pf[I_PRED_4x4_DDR] = predict_4x4_ddr;
    pf[I_PRED_4x4_VR]  = predict_4x4_vr;
    pf[I_PRED_4x4_HD]  = predict_4x4_hd;
    pf[I_PRED_4x4_HU]  = predict_4x4_hu;
#endif
}

