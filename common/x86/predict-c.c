/*****************************************************************************
 * predict.c: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *****************************************************************************/

#include "common/common.h"
#include "predict.h"
#include "pixel.h"

extern void predict_16x16_v_mmx( uint8_t *src );
extern void predict_16x16_h_mmxext( uint8_t *src );
extern void predict_16x16_h_ssse3( uint8_t *src );
extern void predict_16x16_dc_core_mmxext( uint8_t *src, int i_dc_left );
extern void predict_16x16_dc_left_core_mmxext( uint8_t *src, int i_dc_left );
extern void predict_16x16_dc_top_mmxext( uint8_t *src );
extern void predict_16x16_p_core_mmxext( uint8_t *src, int i00, int b, int c );
extern void predict_8x8c_p_core_mmxext( uint8_t *src, int i00, int b, int c );
extern void predict_8x8c_p_core_sse2( uint8_t *src, int i00, int b, int c );
extern void predict_8x8c_dc_core_mmxext( uint8_t *src, int s2, int s3 );
extern void predict_8x8c_dc_top_mmxext( uint8_t *src );
extern void predict_8x8c_v_mmx( uint8_t *src );
extern void predict_8x8c_h_mmxext( uint8_t *src );
extern void predict_8x8c_h_ssse3( uint8_t *src );
extern void predict_8x8_v_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_h_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_hd_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_hu_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_dc_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_dc_top_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_dc_left_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_ddl_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_ddr_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_ddl_sse2( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_ddr_sse2( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_vl_sse2( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_vr_sse2( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_hu_sse2( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_hd_sse2( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_vr_core_mmxext( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_hd_ssse3( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_hu_ssse3( uint8_t *src, uint8_t edge[33] );
extern void predict_8x8_filter_mmxext   ( uint8_t *src, uint8_t edge[33], int i_neighbor, int i_filters );
extern void predict_8x8_filter_ssse3   ( uint8_t *src, uint8_t edge[33], int i_neighbor, int i_filters );
extern void predict_4x4_ddl_mmxext( uint8_t *src );
extern void predict_4x4_ddr_mmxext( uint8_t *src );
extern void predict_4x4_vl_mmxext( uint8_t *src );
extern void predict_4x4_vr_mmxext( uint8_t *src );
extern void predict_4x4_vr_ssse3( uint8_t *src );
extern void predict_4x4_hd_mmxext( uint8_t *src );
extern void predict_4x4_hd_ssse3( uint8_t *src );
extern void predict_4x4_dc_mmxext( uint8_t *src );
extern void predict_4x4_ddr_ssse3( uint8_t *src );
extern void predict_4x4_hu_mmxext( uint8_t *src );
extern void predict_16x16_dc_top_sse2( uint8_t *src );
extern void predict_16x16_dc_core_sse2( uint8_t *src, int i_dc_left );
extern void predict_16x16_dc_left_core_sse2( uint8_t *src, int i_dc_left );
extern void predict_16x16_v_sse2( uint8_t *src );
extern void predict_16x16_p_core_sse2( uint8_t *src, int i00, int b, int c );

DECLARE_ALIGNED_8( static const int8_t pb_12345678[8] ) = {1,2,3,4,5,6,7,8};
DECLARE_ALIGNED_8( static const int8_t pb_m87654321[8] ) = {-8,-7,-6,-5,-4,-3,-2,-1};
DECLARE_ALIGNED_8( static const int8_t pb_m32101234[8] ) = {-3,-2,-1,0,1,2,3,4};

#define PREDICT_P_SUM(j,i)\
    H += i * ( src[j+i - FDEC_STRIDE ]  - src[j-i - FDEC_STRIDE ] );\
    V += i * ( src[(j+i)*FDEC_STRIDE -1] - src[(j-i)*FDEC_STRIDE -1] );\

#define PREDICT_16x16_P(name)\
static void predict_16x16_p_##name( uint8_t *src )\
{\
    int a, b, c;\
    int H = 0;\
    int V = 0;\
    int i00;\
    PREDICT_P_SUM(7,1) \
    PREDICT_P_SUM(7,2) \
    PREDICT_P_SUM(7,3) \
    PREDICT_P_SUM(7,4) \
    PREDICT_P_SUM(7,5) \
    PREDICT_P_SUM(7,6) \
    PREDICT_P_SUM(7,7) \
    PREDICT_P_SUM(7,8) \
    a = 16 * ( src[15*FDEC_STRIDE -1] + src[15 - FDEC_STRIDE] );\
    b = ( 5 * H + 32 ) >> 6;\
    c = ( 5 * V + 32 ) >> 6;\
    i00 = a - b * 7 - c * 7 + 16;\
    predict_16x16_p_core_##name( src, i00, b, c );\
}

#ifndef ARCH_X86_64
PREDICT_16x16_P( mmxext )
#endif
PREDICT_16x16_P( sse2   )

#ifdef __GNUC__
static void predict_16x16_p_ssse3( uint8_t *src )
{
    int a, b, c, i00;
    int H, V;
    asm (
        "movq           %1, %%mm1 \n"
        "movq         8+%1, %%mm0 \n"
        "palignr $7, -8+%1, %%mm1 \n"
        "pmaddubsw      %2, %%mm0 \n"
        "pmaddubsw      %3, %%mm1 \n"
        "paddw       %%mm1, %%mm0 \n"
        "pshufw $14, %%mm0, %%mm1 \n"
        "paddw       %%mm1, %%mm0 \n"
        "pshufw  $1, %%mm0, %%mm1 \n"
        "paddw       %%mm1, %%mm0 \n"
        "movd        %%mm0, %0    \n"
        "movsx         %w0, %0    \n"
        :"=r"(H)
        :"m"(src[-FDEC_STRIDE]), "m"(*pb_12345678), "m"(*pb_m87654321)
    );
    V = 8 * ( src[15*FDEC_STRIDE-1] - src[-1*FDEC_STRIDE-1] )
      + 7 * ( src[14*FDEC_STRIDE-1] - src[ 0*FDEC_STRIDE-1] )
      + 6 * ( src[13*FDEC_STRIDE-1] - src[ 1*FDEC_STRIDE-1] )
      + 5 * ( src[12*FDEC_STRIDE-1] - src[ 2*FDEC_STRIDE-1] )
      + 4 * ( src[11*FDEC_STRIDE-1] - src[ 3*FDEC_STRIDE-1] )
      + 3 * ( src[10*FDEC_STRIDE-1] - src[ 4*FDEC_STRIDE-1] )
      + 2 * ( src[ 9*FDEC_STRIDE-1] - src[ 5*FDEC_STRIDE-1] )
      + 1 * ( src[ 8*FDEC_STRIDE-1] - src[ 6*FDEC_STRIDE-1] );
    a = 16 * ( src[15*FDEC_STRIDE -1] + src[15 - FDEC_STRIDE] );
    b = ( 5 * H + 32 ) >> 6;
    c = ( 5 * V + 32 ) >> 6;
    i00 = a - b * 7 - c * 7 + 16;
    predict_16x16_p_core_sse2( src, i00, b, c );
}
#endif

#define PREDICT_8x8_P(name)\
static void predict_8x8c_p_##name( uint8_t *src )\
{\
    int a, b, c;\
    int H = 0;\
    int V = 0;\
    int i00;\
    PREDICT_P_SUM(3,1)\
    PREDICT_P_SUM(3,2)\
    PREDICT_P_SUM(3,3)\
    PREDICT_P_SUM(3,4)\
    a = 16 * ( src[7*FDEC_STRIDE -1] + src[7 - FDEC_STRIDE] );\
    b = ( 17 * H + 16 ) >> 5;\
    c = ( 17 * V + 16 ) >> 5;\
    i00 = a -3*b -3*c + 16;\
    predict_8x8c_p_core_##name( src, i00, b, c );\
}

#ifndef ARCH_X86_64
PREDICT_8x8_P( mmxext )
#endif
PREDICT_8x8_P( sse2   )

#ifdef __GNUC__
static void predict_8x8c_p_ssse3( uint8_t *src )
{
    int a, b, c, i00;
    int H, V;
    asm (
        "movq           %1, %%mm0 \n"
        "pmaddubsw      %2, %%mm0 \n"
        "pshufw $14, %%mm0, %%mm1 \n"
        "paddw       %%mm1, %%mm0 \n"
        "pshufw  $1, %%mm0, %%mm1 \n"
        "paddw       %%mm1, %%mm0 \n"
        "movd        %%mm0, %0    \n"
        "movsx         %w0, %0    \n"
        :"=r"(H)
        :"m"(src[-FDEC_STRIDE]), "m"(*pb_m32101234)
    );
    V = 1 * ( src[4*FDEC_STRIDE -1] - src[ 2*FDEC_STRIDE -1] )
      + 2 * ( src[5*FDEC_STRIDE -1] - src[ 1*FDEC_STRIDE -1] )
      + 3 * ( src[6*FDEC_STRIDE -1] - src[ 0*FDEC_STRIDE -1] )
      + 4 * ( src[7*FDEC_STRIDE -1] - src[-1*FDEC_STRIDE -1] );
    H += -4 * src[-1*FDEC_STRIDE -1];
    a = 16 * ( src[7*FDEC_STRIDE -1] + src[7 - FDEC_STRIDE] );
    b = ( 17 * H + 16 ) >> 5;
    c = ( 17 * V + 16 ) >> 5;
    i00 = a -3*b -3*c + 16;
    predict_8x8c_p_core_sse2( src, i00, b, c );
}
#endif

#define PREDICT_16x16_DC(name)\
static void predict_16x16_dc_##name( uint8_t *src )\
{\
    uint32_t dc=16;\
    int i;\
    for( i = 0; i < 16; i+=2 )\
    {\
        dc += src[-1 + i * FDEC_STRIDE];\
        dc += src[-1 + (i+1) * FDEC_STRIDE];\
    }\
    predict_16x16_dc_core_##name( src, dc );\
}

PREDICT_16x16_DC( mmxext )
PREDICT_16x16_DC( sse2 )

#define PREDICT_16x16_DC_LEFT(name)\
static void predict_16x16_dc_left_##name( uint8_t *src )\
{\
    uint32_t dc=8;\
    int i;\
    for( i = 0; i < 16; i+=2 )\
    {\
        dc += src[-1 + i * FDEC_STRIDE];\
        dc += src[-1 + (i+1) * FDEC_STRIDE];\
    }\
    predict_16x16_dc_left_core_##name( src, dc>>4 );\
}

PREDICT_16x16_DC_LEFT( mmxext )
PREDICT_16x16_DC_LEFT( sse2 )

static void predict_8x8c_dc_mmxext( uint8_t *src )
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

#ifndef ARCH_X86_64
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
#endif

#define SUMSUB(a,b,c,d,e,f,g,h)\
    t=a; a+=b; b-=t;\
    t=c; c+=d; d-=t;\
    t=e; e+=f; f-=t;\
    t=g; g+=h; h-=t;

#define INTRA_SA8D_X3(cpu) \
void x264_intra_sa8d_x3_8x8_##cpu( uint8_t *fenc, uint8_t edge[33], int res[3] )\
{\
    PREDICT_8x8_LOAD_TOP\
    PREDICT_8x8_LOAD_LEFT\
    int t;\
    DECLARE_ALIGNED_16( int16_t sa8d_1d[2][8] );\
    SUMSUB(l0,l4,l1,l5,l2,l6,l3,l7);\
    SUMSUB(l0,l2,l1,l3,l4,l6,l5,l7);\
    SUMSUB(l0,l1,l2,l3,l4,l5,l6,l7);\
    sa8d_1d[0][0] = l0;\
    sa8d_1d[0][1] = l1;\
    sa8d_1d[0][2] = l2;\
    sa8d_1d[0][3] = l3;\
    sa8d_1d[0][4] = l4;\
    sa8d_1d[0][5] = l5;\
    sa8d_1d[0][6] = l6;\
    sa8d_1d[0][7] = l7;\
    SUMSUB(t0,t4,t1,t5,t2,t6,t3,t7);\
    SUMSUB(t0,t2,t1,t3,t4,t6,t5,t7);\
    SUMSUB(t0,t1,t2,t3,t4,t5,t6,t7);\
    sa8d_1d[1][0] = t0;\
    sa8d_1d[1][1] = t1;\
    sa8d_1d[1][2] = t2;\
    sa8d_1d[1][3] = t3;\
    sa8d_1d[1][4] = t4;\
    sa8d_1d[1][5] = t5;\
    sa8d_1d[1][6] = t6;\
    sa8d_1d[1][7] = t7;\
    x264_intra_sa8d_x3_8x8_core_##cpu( fenc, sa8d_1d, res );\
}

#ifdef ARCH_X86_64
INTRA_SA8D_X3(sse2)
INTRA_SA8D_X3(ssse3)
#else
INTRA_SA8D_X3(mmxext)
#endif

/****************************************************************************
 * Exported functions:
 ****************************************************************************/
void x264_predict_16x16_init_mmx( int cpu, x264_predict_t pf[7] )
{
    if( !(cpu&X264_CPU_MMX) )
        return;
    pf[I_PRED_16x16_V]       = predict_16x16_v_mmx;
    if( !(cpu&X264_CPU_MMXEXT) )
        return;
    pf[I_PRED_16x16_DC]      = predict_16x16_dc_mmxext;
    pf[I_PRED_16x16_DC_TOP]  = predict_16x16_dc_top_mmxext;
    pf[I_PRED_16x16_DC_LEFT] = predict_16x16_dc_left_mmxext;
#ifndef ARCH_X86_64
    pf[I_PRED_16x16_P]       = predict_16x16_p_mmxext;
#endif
    pf[I_PRED_16x16_H]       = predict_16x16_h_mmxext;
    if( !(cpu&X264_CPU_SSE2) )
        return;
    pf[I_PRED_16x16_DC]     = predict_16x16_dc_sse2;
    pf[I_PRED_16x16_V]      = predict_16x16_v_sse2;
    if( cpu&X264_CPU_SSE2_IS_SLOW )
        return;
    pf[I_PRED_16x16_DC_TOP] = predict_16x16_dc_top_sse2;
    pf[I_PRED_16x16_DC_LEFT] = predict_16x16_dc_left_sse2;
    pf[I_PRED_16x16_P]      = predict_16x16_p_sse2;
    if( !(cpu&X264_CPU_SSSE3) )
        return;
    pf[I_PRED_16x16_H]      = predict_16x16_h_ssse3;
#ifdef __GNUC__
    pf[I_PRED_16x16_P]      = predict_16x16_p_ssse3;
#endif
}

void x264_predict_8x8c_init_mmx( int cpu, x264_predict_t pf[7] )
{
    if( !(cpu&X264_CPU_MMX) )
        return;
#ifdef ARCH_X86_64
    pf[I_PRED_CHROMA_DC_LEFT] = predict_8x8c_dc_left;
#endif
    pf[I_PRED_CHROMA_V]       = predict_8x8c_v_mmx;
    if( !(cpu&X264_CPU_MMXEXT) )
        return;
    pf[I_PRED_CHROMA_DC_TOP]  = predict_8x8c_dc_top_mmxext;
    pf[I_PRED_CHROMA_H]       = predict_8x8c_h_mmxext;
#ifndef ARCH_X86_64
    pf[I_PRED_CHROMA_P]       = predict_8x8c_p_mmxext;
#endif
    pf[I_PRED_CHROMA_DC]      = predict_8x8c_dc_mmxext;
    if( !(cpu&X264_CPU_SSE2) )
        return;
    pf[I_PRED_CHROMA_P]       = predict_8x8c_p_sse2;
    if( !(cpu&X264_CPU_SSSE3) )
        return;
    pf[I_PRED_CHROMA_H]       = predict_8x8c_h_ssse3;
#ifdef __GNUC__
    pf[I_PRED_CHROMA_P]       = predict_8x8c_p_ssse3;
#endif
}

void x264_predict_8x8_init_mmx( int cpu, x264_predict8x8_t pf[12], x264_predict_8x8_filter_t *predict_8x8_filter )
{
    if( !(cpu&X264_CPU_MMXEXT) )
        return;
    pf[I_PRED_8x8_V]   = predict_8x8_v_mmxext;
    pf[I_PRED_8x8_H]   = predict_8x8_h_mmxext;
    pf[I_PRED_8x8_DC]  = predict_8x8_dc_mmxext;
    pf[I_PRED_8x8_DC_TOP] = predict_8x8_dc_top_mmxext;
    pf[I_PRED_8x8_DC_LEFT]= predict_8x8_dc_left_mmxext;
    pf[I_PRED_8x8_HD]   = predict_8x8_hd_mmxext;
    *predict_8x8_filter = predict_8x8_filter_mmxext;
#ifdef ARCH_X86
    pf[I_PRED_8x8_DDL] = predict_8x8_ddl_mmxext;
    pf[I_PRED_8x8_DDR] = predict_8x8_ddr_mmxext;
    pf[I_PRED_8x8_VR]  = predict_8x8_vr_mmxext;
    pf[I_PRED_8x8_HU]   = predict_8x8_hu_mmxext;
#endif
    if( !(cpu&X264_CPU_SSE2) )
        return;
    pf[I_PRED_8x8_DDL] = predict_8x8_ddl_sse2;
    pf[I_PRED_8x8_VL]  = predict_8x8_vl_sse2;
    pf[I_PRED_8x8_VR]  = predict_8x8_vr_sse2;
    pf[I_PRED_8x8_DDR] = predict_8x8_ddr_sse2;
    pf[I_PRED_8x8_HD]   = predict_8x8_hd_sse2;
    pf[I_PRED_8x8_HU]   = predict_8x8_hu_sse2;
    if( !(cpu&X264_CPU_SSSE3) )
        return;
    pf[I_PRED_8x8_HD]   = predict_8x8_hd_ssse3;
    pf[I_PRED_8x8_HU]   = predict_8x8_hu_ssse3;
    *predict_8x8_filter = predict_8x8_filter_ssse3;
}

void x264_predict_4x4_init_mmx( int cpu, x264_predict_t pf[12] )
{
    if( !(cpu&X264_CPU_MMXEXT) )
        return;
    pf[I_PRED_4x4_VR]  = predict_4x4_vr_mmxext;
    pf[I_PRED_4x4_DDL] = predict_4x4_ddl_mmxext;
    pf[I_PRED_4x4_VL]  = predict_4x4_vl_mmxext;
    pf[I_PRED_4x4_DC]  = predict_4x4_dc_mmxext;
    pf[I_PRED_4x4_DDR] = predict_4x4_ddr_mmxext;
    pf[I_PRED_4x4_HD]  = predict_4x4_hd_mmxext;
    pf[I_PRED_4x4_HU]  = predict_4x4_hu_mmxext;
    if( !(cpu&X264_CPU_SSSE3) )
        return;
    pf[I_PRED_4x4_DDR] = predict_4x4_ddr_ssse3;
    pf[I_PRED_4x4_VR]  = predict_4x4_vr_ssse3;
    pf[I_PRED_4x4_HD]  = predict_4x4_hd_ssse3;
}
