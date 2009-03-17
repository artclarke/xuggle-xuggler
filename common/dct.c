/*****************************************************************************
 * dct.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *          Laurent Aimar <fenrir@via.ecp.fr>
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

#include "common.h"
#ifdef HAVE_MMX
#   include "x86/dct.h"
#endif
#ifdef ARCH_PPC
#   include "ppc/dct.h"
#endif

int x264_dct4_weight2_zigzag[2][16];
int x264_dct8_weight2_zigzag[2][64];

/*
 * XXX For all dct dc : input could be equal to output so ...
 */

static void dct4x4dc( int16_t d[4][4] )
{
    int16_t tmp[4][4];
    int s01, s23;
    int d01, d23;
    int i;

    for( i = 0; i < 4; i++ )
    {
        s01 = d[i][0] + d[i][1];
        d01 = d[i][0] - d[i][1];
        s23 = d[i][2] + d[i][3];
        d23 = d[i][2] - d[i][3];

        tmp[0][i] = s01 + s23;
        tmp[1][i] = s01 - s23;
        tmp[2][i] = d01 - d23;
        tmp[3][i] = d01 + d23;
    }

    for( i = 0; i < 4; i++ )
    {
        s01 = tmp[i][0] + tmp[i][1];
        d01 = tmp[i][0] - tmp[i][1];
        s23 = tmp[i][2] + tmp[i][3];
        d23 = tmp[i][2] - tmp[i][3];

        d[i][0] = ( s01 + s23 + 1 ) >> 1;
        d[i][1] = ( s01 - s23 + 1 ) >> 1;
        d[i][2] = ( d01 - d23 + 1 ) >> 1;
        d[i][3] = ( d01 + d23 + 1 ) >> 1;
    }
}

static void idct4x4dc( int16_t d[4][4] )
{
    int16_t tmp[4][4];
    int s01, s23;
    int d01, d23;
    int i;

    for( i = 0; i < 4; i++ )
    {
        s01 = d[i][0] + d[i][1];
        d01 = d[i][0] - d[i][1];
        s23 = d[i][2] + d[i][3];
        d23 = d[i][2] - d[i][3];

        tmp[0][i] = s01 + s23;
        tmp[1][i] = s01 - s23;
        tmp[2][i] = d01 - d23;
        tmp[3][i] = d01 + d23;
    }

    for( i = 0; i < 4; i++ )
    {
        s01 = tmp[i][0] + tmp[i][1];
        d01 = tmp[i][0] - tmp[i][1];
        s23 = tmp[i][2] + tmp[i][3];
        d23 = tmp[i][2] - tmp[i][3];

        d[i][0] = s01 + s23;
        d[i][1] = s01 - s23;
        d[i][2] = d01 - d23;
        d[i][3] = d01 + d23;
    }
}

static inline void pixel_sub_wxh( int16_t *diff, int i_size,
                                  uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    int y, x;
    for( y = 0; y < i_size; y++ )
    {
        for( x = 0; x < i_size; x++ )
        {
            diff[x + y*i_size] = pix1[x] - pix2[x];
        }
        pix1 += i_pix1;
        pix2 += i_pix2;
    }
}

static void sub4x4_dct( int16_t dct[4][4], uint8_t *pix1, uint8_t *pix2 )
{
    int16_t d[4][4];
    int16_t tmp[4][4];
    int i;

    pixel_sub_wxh( (int16_t*)d, 4, pix1, FENC_STRIDE, pix2, FDEC_STRIDE );

    for( i = 0; i < 4; i++ )
    {
        const int s03 = d[i][0] + d[i][3];
        const int s12 = d[i][1] + d[i][2];
        const int d03 = d[i][0] - d[i][3];
        const int d12 = d[i][1] - d[i][2];

        tmp[0][i] =   s03 +   s12;
        tmp[1][i] = 2*d03 +   d12;
        tmp[2][i] =   s03 -   s12;
        tmp[3][i] =   d03 - 2*d12;
    }

    for( i = 0; i < 4; i++ )
    {
        const int s03 = tmp[i][0] + tmp[i][3];
        const int s12 = tmp[i][1] + tmp[i][2];
        const int d03 = tmp[i][0] - tmp[i][3];
        const int d12 = tmp[i][1] - tmp[i][2];

        dct[i][0] =   s03 +   s12;
        dct[i][1] = 2*d03 +   d12;
        dct[i][2] =   s03 -   s12;
        dct[i][3] =   d03 - 2*d12;
    }
}

static void sub8x8_dct( int16_t dct[4][4][4], uint8_t *pix1, uint8_t *pix2 )
{
    sub4x4_dct( dct[0], &pix1[0], &pix2[0] );
    sub4x4_dct( dct[1], &pix1[4], &pix2[4] );
    sub4x4_dct( dct[2], &pix1[4*FENC_STRIDE+0], &pix2[4*FDEC_STRIDE+0] );
    sub4x4_dct( dct[3], &pix1[4*FENC_STRIDE+4], &pix2[4*FDEC_STRIDE+4] );
}

static void sub16x16_dct( int16_t dct[16][4][4], uint8_t *pix1, uint8_t *pix2 )
{
    sub8x8_dct( &dct[ 0], &pix1[0], &pix2[0] );
    sub8x8_dct( &dct[ 4], &pix1[8], &pix2[8] );
    sub8x8_dct( &dct[ 8], &pix1[8*FENC_STRIDE+0], &pix2[8*FDEC_STRIDE+0] );
    sub8x8_dct( &dct[12], &pix1[8*FENC_STRIDE+8], &pix2[8*FDEC_STRIDE+8] );
}


static void add4x4_idct( uint8_t *p_dst, int16_t dct[4][4] )
{
    int16_t d[4][4];
    int16_t tmp[4][4];
    int x, y;
    int i;

    for( i = 0; i < 4; i++ )
    {
        const int s02 =  dct[0][i]     +  dct[2][i];
        const int d02 =  dct[0][i]     -  dct[2][i];
        const int s13 =  dct[1][i]     + (dct[3][i]>>1);
        const int d13 = (dct[1][i]>>1) -  dct[3][i];

        tmp[i][0] = s02 + s13;
        tmp[i][1] = d02 + d13;
        tmp[i][2] = d02 - d13;
        tmp[i][3] = s02 - s13;
    }

    for( i = 0; i < 4; i++ )
    {
        const int s02 =  tmp[0][i]     +  tmp[2][i];
        const int d02 =  tmp[0][i]     -  tmp[2][i];
        const int s13 =  tmp[1][i]     + (tmp[3][i]>>1);
        const int d13 = (tmp[1][i]>>1) -  tmp[3][i];

        d[0][i] = ( s02 + s13 + 32 ) >> 6;
        d[1][i] = ( d02 + d13 + 32 ) >> 6;
        d[2][i] = ( d02 - d13 + 32 ) >> 6;
        d[3][i] = ( s02 - s13 + 32 ) >> 6;
    }


    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            p_dst[x] = x264_clip_uint8( p_dst[x] + d[y][x] );
        }
        p_dst += FDEC_STRIDE;
    }
}

static void add8x8_idct( uint8_t *p_dst, int16_t dct[4][4][4] )
{
    add4x4_idct( &p_dst[0],               dct[0] );
    add4x4_idct( &p_dst[4],               dct[1] );
    add4x4_idct( &p_dst[4*FDEC_STRIDE+0], dct[2] );
    add4x4_idct( &p_dst[4*FDEC_STRIDE+4], dct[3] );
}

static void add16x16_idct( uint8_t *p_dst, int16_t dct[16][4][4] )
{
    add8x8_idct( &p_dst[0],               &dct[0] );
    add8x8_idct( &p_dst[8],               &dct[4] );
    add8x8_idct( &p_dst[8*FDEC_STRIDE+0], &dct[8] );
    add8x8_idct( &p_dst[8*FDEC_STRIDE+8], &dct[12] );
}

/****************************************************************************
 * 8x8 transform:
 ****************************************************************************/

#define DCT8_1D {\
    const int s07 = SRC(0) + SRC(7);\
    const int s16 = SRC(1) + SRC(6);\
    const int s25 = SRC(2) + SRC(5);\
    const int s34 = SRC(3) + SRC(4);\
    const int a0 = s07 + s34;\
    const int a1 = s16 + s25;\
    const int a2 = s07 - s34;\
    const int a3 = s16 - s25;\
    const int d07 = SRC(0) - SRC(7);\
    const int d16 = SRC(1) - SRC(6);\
    const int d25 = SRC(2) - SRC(5);\
    const int d34 = SRC(3) - SRC(4);\
    const int a4 = d16 + d25 + (d07 + (d07>>1));\
    const int a5 = d07 - d34 - (d25 + (d25>>1));\
    const int a6 = d07 + d34 - (d16 + (d16>>1));\
    const int a7 = d16 - d25 + (d34 + (d34>>1));\
    DST(0) =  a0 + a1     ;\
    DST(1) =  a4 + (a7>>2);\
    DST(2) =  a2 + (a3>>1);\
    DST(3) =  a5 + (a6>>2);\
    DST(4) =  a0 - a1     ;\
    DST(5) =  a6 - (a5>>2);\
    DST(6) = (a2>>1) - a3 ;\
    DST(7) = (a4>>2) - a7 ;\
}

static void sub8x8_dct8( int16_t dct[8][8], uint8_t *pix1, uint8_t *pix2 )
{
    int i;
    int16_t tmp[8][8];

    pixel_sub_wxh( (int16_t*)tmp, 8, pix1, FENC_STRIDE, pix2, FDEC_STRIDE );

#define SRC(x) tmp[x][i]
#define DST(x) tmp[x][i]
    for( i = 0; i < 8; i++ )
        DCT8_1D
#undef SRC
#undef DST

#define SRC(x) tmp[i][x]
#define DST(x) dct[x][i]
    for( i = 0; i < 8; i++ )
        DCT8_1D
#undef SRC
#undef DST
}

static void sub16x16_dct8( int16_t dct[4][8][8], uint8_t *pix1, uint8_t *pix2 )
{
    sub8x8_dct8( dct[0], &pix1[0],               &pix2[0] );
    sub8x8_dct8( dct[1], &pix1[8],               &pix2[8] );
    sub8x8_dct8( dct[2], &pix1[8*FENC_STRIDE+0], &pix2[8*FDEC_STRIDE+0] );
    sub8x8_dct8( dct[3], &pix1[8*FENC_STRIDE+8], &pix2[8*FDEC_STRIDE+8] );
}

#define IDCT8_1D {\
    const int a0 =  SRC(0) + SRC(4);\
    const int a2 =  SRC(0) - SRC(4);\
    const int a4 = (SRC(2)>>1) - SRC(6);\
    const int a6 = (SRC(6)>>1) + SRC(2);\
    const int b0 = a0 + a6;\
    const int b2 = a2 + a4;\
    const int b4 = a2 - a4;\
    const int b6 = a0 - a6;\
    const int a1 = -SRC(3) + SRC(5) - SRC(7) - (SRC(7)>>1);\
    const int a3 =  SRC(1) + SRC(7) - SRC(3) - (SRC(3)>>1);\
    const int a5 = -SRC(1) + SRC(7) + SRC(5) + (SRC(5)>>1);\
    const int a7 =  SRC(3) + SRC(5) + SRC(1) + (SRC(1)>>1);\
    const int b1 = (a7>>2) + a1;\
    const int b3 =  a3 + (a5>>2);\
    const int b5 = (a3>>2) - a5;\
    const int b7 =  a7 - (a1>>2);\
    DST(0, b0 + b7);\
    DST(1, b2 + b5);\
    DST(2, b4 + b3);\
    DST(3, b6 + b1);\
    DST(4, b6 - b1);\
    DST(5, b4 - b3);\
    DST(6, b2 - b5);\
    DST(7, b0 - b7);\
}

static void add8x8_idct8( uint8_t *dst, int16_t dct[8][8] )
{
    int i;

    dct[0][0] += 32; // rounding for the >>6 at the end

#define SRC(x)     dct[x][i]
#define DST(x,rhs) dct[x][i] = (rhs)
    for( i = 0; i < 8; i++ )
        IDCT8_1D
#undef SRC
#undef DST

#define SRC(x)     dct[i][x]
#define DST(x,rhs) dst[i + x*FDEC_STRIDE] = x264_clip_uint8( dst[i + x*FDEC_STRIDE] + ((rhs) >> 6) );
    for( i = 0; i < 8; i++ )
        IDCT8_1D
#undef SRC
#undef DST
}

static void add16x16_idct8( uint8_t *dst, int16_t dct[4][8][8] )
{
    add8x8_idct8( &dst[0],               dct[0] );
    add8x8_idct8( &dst[8],               dct[1] );
    add8x8_idct8( &dst[8*FDEC_STRIDE+0], dct[2] );
    add8x8_idct8( &dst[8*FDEC_STRIDE+8], dct[3] );
}

static void inline add4x4_idct_dc( uint8_t *p_dst, int16_t dc )
{
    int i;
    dc = (dc + 32) >> 6;
    for( i = 0; i < 4; i++, p_dst += FDEC_STRIDE )
    {
        p_dst[0] = x264_clip_uint8( p_dst[0] + dc );
        p_dst[1] = x264_clip_uint8( p_dst[1] + dc );
        p_dst[2] = x264_clip_uint8( p_dst[2] + dc );
        p_dst[3] = x264_clip_uint8( p_dst[3] + dc );
    }
}

static void add8x8_idct_dc( uint8_t *p_dst, int16_t dct[2][2] )
{
    add4x4_idct_dc( &p_dst[0],               dct[0][0] );
    add4x4_idct_dc( &p_dst[4],               dct[0][1] );
    add4x4_idct_dc( &p_dst[4*FDEC_STRIDE+0], dct[1][0] );
    add4x4_idct_dc( &p_dst[4*FDEC_STRIDE+4], dct[1][1] );
}

static void add16x16_idct_dc( uint8_t *p_dst, int16_t dct[4][4] )
{
    int i;
    for( i = 0; i < 4; i++, p_dst += 4*FDEC_STRIDE )
    {
        add4x4_idct_dc( &p_dst[ 0], dct[i][0] );
        add4x4_idct_dc( &p_dst[ 4], dct[i][1] );
        add4x4_idct_dc( &p_dst[ 8], dct[i][2] );
        add4x4_idct_dc( &p_dst[12], dct[i][3] );
    }
}


/****************************************************************************
 * x264_dct_init:
 ****************************************************************************/
void x264_dct_init( int cpu, x264_dct_function_t *dctf )
{
    dctf->sub4x4_dct    = sub4x4_dct;
    dctf->add4x4_idct   = add4x4_idct;

    dctf->sub8x8_dct    = sub8x8_dct;
    dctf->add8x8_idct   = add8x8_idct;
    dctf->add8x8_idct_dc = add8x8_idct_dc;

    dctf->sub16x16_dct  = sub16x16_dct;
    dctf->add16x16_idct = add16x16_idct;
    dctf->add16x16_idct_dc = add16x16_idct_dc;

    dctf->sub8x8_dct8   = sub8x8_dct8;
    dctf->add8x8_idct8  = add8x8_idct8;

    dctf->sub16x16_dct8  = sub16x16_dct8;
    dctf->add16x16_idct8 = add16x16_idct8;

    dctf->dct4x4dc  = dct4x4dc;
    dctf->idct4x4dc = idct4x4dc;

#ifdef HAVE_MMX
    if( cpu&X264_CPU_MMX )
    {
        dctf->sub4x4_dct    = x264_sub4x4_dct_mmx;
        dctf->add4x4_idct   = x264_add4x4_idct_mmx;
        dctf->add8x8_idct_dc = x264_add8x8_idct_dc_mmx;
        dctf->add16x16_idct_dc = x264_add16x16_idct_dc_mmx;
        dctf->dct4x4dc      = x264_dct4x4dc_mmx;
        dctf->idct4x4dc     = x264_idct4x4dc_mmx;

#ifndef ARCH_X86_64
        dctf->sub8x8_dct    = x264_sub8x8_dct_mmx;
        dctf->sub16x16_dct  = x264_sub16x16_dct_mmx;
        dctf->add8x8_idct   = x264_add8x8_idct_mmx;
        dctf->add16x16_idct = x264_add16x16_idct_mmx;

        dctf->sub8x8_dct8   = x264_sub8x8_dct8_mmx;
        dctf->sub16x16_dct8 = x264_sub16x16_dct8_mmx;
        dctf->add8x8_idct8  = x264_add8x8_idct8_mmx;
        dctf->add16x16_idct8= x264_add16x16_idct8_mmx;
#endif
    }

    if( cpu&X264_CPU_SSE2 )
    {
        dctf->sub8x8_dct8   = x264_sub8x8_dct8_sse2;
        dctf->sub16x16_dct8 = x264_sub16x16_dct8_sse2;
        dctf->add8x8_idct8  = x264_add8x8_idct8_sse2;
        dctf->add16x16_idct8= x264_add16x16_idct8_sse2;

        dctf->sub8x8_dct    = x264_sub8x8_dct_sse2;
        dctf->sub16x16_dct  = x264_sub16x16_dct_sse2;
        dctf->add8x8_idct   = x264_add8x8_idct_sse2;
        dctf->add16x16_idct = x264_add16x16_idct_sse2;
        dctf->add16x16_idct_dc = x264_add16x16_idct_dc_sse2;
    }

    if( cpu&X264_CPU_SSSE3 )
    {
        dctf->sub4x4_dct    = x264_sub4x4_dct_ssse3;
        dctf->sub8x8_dct    = x264_sub8x8_dct_ssse3;
        dctf->sub16x16_dct  = x264_sub16x16_dct_ssse3;
        dctf->sub8x8_dct8   = x264_sub8x8_dct8_ssse3;
        dctf->sub16x16_dct8 = x264_sub16x16_dct8_ssse3;
        dctf->add8x8_idct_dc = x264_add8x8_idct_dc_ssse3;
        dctf->add16x16_idct_dc = x264_add16x16_idct_dc_ssse3;
    }
#endif //HAVE_MMX

#ifdef ARCH_PPC
    if( cpu&X264_CPU_ALTIVEC )
    {
        dctf->sub4x4_dct    = x264_sub4x4_dct_altivec;
        dctf->sub8x8_dct    = x264_sub8x8_dct_altivec;
        dctf->sub16x16_dct  = x264_sub16x16_dct_altivec;

        dctf->add4x4_idct   = x264_add4x4_idct_altivec;
        dctf->add8x8_idct   = x264_add8x8_idct_altivec;
        dctf->add16x16_idct = x264_add16x16_idct_altivec;

        dctf->sub8x8_dct8   = x264_sub8x8_dct8_altivec;
        dctf->sub16x16_dct8 = x264_sub16x16_dct8_altivec;

        dctf->add8x8_idct8  = x264_add8x8_idct8_altivec;
        dctf->add16x16_idct8= x264_add16x16_idct8_altivec;
    }
#endif
}

void x264_dct_init_weights( void )
{
    int i, j;
    for( j=0; j<2; j++ )
    {
        for( i=0; i<16; i++ )
            x264_dct4_weight2_zigzag[j][i] = x264_dct4_weight2_tab[ x264_zigzag_scan4[j][i] ];
        for( i=0; i<64; i++ )
            x264_dct8_weight2_zigzag[j][i] = x264_dct8_weight2_tab[ x264_zigzag_scan8[j][i] ];
    }
}


// gcc pessimizes multi-dimensional arrays here, even with constant indices
#define ZIG(i,y,x) level[i] = dct[0][x*8+y];
#define ZIGZAG8_FRAME\
    ZIG( 0,0,0) ZIG( 1,0,1) ZIG( 2,1,0) ZIG( 3,2,0)\
    ZIG( 4,1,1) ZIG( 5,0,2) ZIG( 6,0,3) ZIG( 7,1,2)\
    ZIG( 8,2,1) ZIG( 9,3,0) ZIG(10,4,0) ZIG(11,3,1)\
    ZIG(12,2,2) ZIG(13,1,3) ZIG(14,0,4) ZIG(15,0,5)\
    ZIG(16,1,4) ZIG(17,2,3) ZIG(18,3,2) ZIG(19,4,1)\
    ZIG(20,5,0) ZIG(21,6,0) ZIG(22,5,1) ZIG(23,4,2)\
    ZIG(24,3,3) ZIG(25,2,4) ZIG(26,1,5) ZIG(27,0,6)\
    ZIG(28,0,7) ZIG(29,1,6) ZIG(30,2,5) ZIG(31,3,4)\
    ZIG(32,4,3) ZIG(33,5,2) ZIG(34,6,1) ZIG(35,7,0)\
    ZIG(36,7,1) ZIG(37,6,2) ZIG(38,5,3) ZIG(39,4,4)\
    ZIG(40,3,5) ZIG(41,2,6) ZIG(42,1,7) ZIG(43,2,7)\
    ZIG(44,3,6) ZIG(45,4,5) ZIG(46,5,4) ZIG(47,6,3)\
    ZIG(48,7,2) ZIG(49,7,3) ZIG(50,6,4) ZIG(51,5,5)\
    ZIG(52,4,6) ZIG(53,3,7) ZIG(54,4,7) ZIG(55,5,6)\
    ZIG(56,6,5) ZIG(57,7,4) ZIG(58,7,5) ZIG(59,6,6)\
    ZIG(60,5,7) ZIG(61,6,7) ZIG(62,7,6) ZIG(63,7,7)\

#define ZIGZAG8_FIELD\
    ZIG( 0,0,0) ZIG( 1,1,0) ZIG( 2,2,0) ZIG( 3,0,1)\
    ZIG( 4,1,1) ZIG( 5,3,0) ZIG( 6,4,0) ZIG( 7,2,1)\
    ZIG( 8,0,2) ZIG( 9,3,1) ZIG(10,5,0) ZIG(11,6,0)\
    ZIG(12,7,0) ZIG(13,4,1) ZIG(14,1,2) ZIG(15,0,3)\
    ZIG(16,2,2) ZIG(17,5,1) ZIG(18,6,1) ZIG(19,7,1)\
    ZIG(20,3,2) ZIG(21,1,3) ZIG(22,0,4) ZIG(23,2,3)\
    ZIG(24,4,2) ZIG(25,5,2) ZIG(26,6,2) ZIG(27,7,2)\
    ZIG(28,3,3) ZIG(29,1,4) ZIG(30,0,5) ZIG(31,2,4)\
    ZIG(32,4,3) ZIG(33,5,3) ZIG(34,6,3) ZIG(35,7,3)\
    ZIG(36,3,4) ZIG(37,1,5) ZIG(38,0,6) ZIG(39,2,5)\
    ZIG(40,4,4) ZIG(41,5,4) ZIG(42,6,4) ZIG(43,7,4)\
    ZIG(44,3,5) ZIG(45,1,6) ZIG(46,2,6) ZIG(47,4,5)\
    ZIG(48,5,5) ZIG(49,6,5) ZIG(50,7,5) ZIG(51,3,6)\
    ZIG(52,0,7) ZIG(53,1,7) ZIG(54,4,6) ZIG(55,5,6)\
    ZIG(56,6,6) ZIG(57,7,6) ZIG(58,2,7) ZIG(59,3,7)\
    ZIG(60,4,7) ZIG(61,5,7) ZIG(62,6,7) ZIG(63,7,7)

#define ZIGZAG4_FRAME\
    ZIG( 0,0,0) ZIG( 1,0,1) ZIG( 2,1,0) ZIG( 3,2,0)\
    ZIG( 4,1,1) ZIG( 5,0,2) ZIG( 6,0,3) ZIG( 7,1,2)\
    ZIG( 8,2,1) ZIG( 9,3,0) ZIG(10,3,1) ZIG(11,2,2)\
    ZIG(12,1,3) ZIG(13,2,3) ZIG(14,3,2) ZIG(15,3,3)

#define ZIGZAG4_FIELD\
    ZIG( 0,0,0) ZIG( 1,1,0) ZIG( 2,0,1) ZIG( 3,2,0)\
    ZIG( 4,3,0) ZIG( 5,1,1) ZIG( 6,2,1) ZIG( 7,3,1)\
    ZIG( 8,0,2) ZIG( 9,1,2) ZIG(10,2,2) ZIG(11,3,2)\
    ZIG(12,0,3) ZIG(13,1,3) ZIG(14,2,3) ZIG(15,3,3)

static void zigzag_scan_8x8_frame( int16_t level[64], int16_t dct[8][8] )
{
    ZIGZAG8_FRAME
}

static void zigzag_scan_8x8_field( int16_t level[64], int16_t dct[8][8] )
{
    ZIGZAG8_FIELD
}

#undef ZIG
#define ZIG(i,y,x) level[i] = dct[0][x*4+y];

static void zigzag_scan_4x4_frame( int16_t level[16], int16_t dct[4][4] )
{
    ZIGZAG4_FRAME
}

static void zigzag_scan_4x4_field( int16_t level[16], int16_t dct[4][4] )
{
    *(uint32_t*)level = *(uint32_t*)dct;
    ZIG(2,0,1) ZIG(3,2,0) ZIG(4,3,0) ZIG(5,1,1)
    *(uint32_t*)(level+6) = *(uint32_t*)(*dct+6);
    *(uint64_t*)(level+8) = *(uint64_t*)(*dct+8);
    *(uint64_t*)(level+12) = *(uint64_t*)(*dct+12);
}

#undef ZIG
#define ZIG(i,y,x) {\
    int oe = x+y*FENC_STRIDE;\
    int od = x+y*FDEC_STRIDE;\
    level[i] = p_src[oe] - p_dst[od];\
}
#define COPY4x4\
    *(uint32_t*)(p_dst+0*FDEC_STRIDE) = *(uint32_t*)(p_src+0*FENC_STRIDE);\
    *(uint32_t*)(p_dst+1*FDEC_STRIDE) = *(uint32_t*)(p_src+1*FENC_STRIDE);\
    *(uint32_t*)(p_dst+2*FDEC_STRIDE) = *(uint32_t*)(p_src+2*FENC_STRIDE);\
    *(uint32_t*)(p_dst+3*FDEC_STRIDE) = *(uint32_t*)(p_src+3*FENC_STRIDE);
#define COPY8x8\
    *(uint64_t*)(p_dst+0*FDEC_STRIDE) = *(uint64_t*)(p_src+0*FENC_STRIDE);\
    *(uint64_t*)(p_dst+1*FDEC_STRIDE) = *(uint64_t*)(p_src+1*FENC_STRIDE);\
    *(uint64_t*)(p_dst+2*FDEC_STRIDE) = *(uint64_t*)(p_src+2*FENC_STRIDE);\
    *(uint64_t*)(p_dst+3*FDEC_STRIDE) = *(uint64_t*)(p_src+3*FENC_STRIDE);\
    *(uint64_t*)(p_dst+4*FDEC_STRIDE) = *(uint64_t*)(p_src+4*FENC_STRIDE);\
    *(uint64_t*)(p_dst+5*FDEC_STRIDE) = *(uint64_t*)(p_src+5*FENC_STRIDE);\
    *(uint64_t*)(p_dst+6*FDEC_STRIDE) = *(uint64_t*)(p_src+6*FENC_STRIDE);\
    *(uint64_t*)(p_dst+7*FDEC_STRIDE) = *(uint64_t*)(p_src+7*FENC_STRIDE);

static void zigzag_sub_4x4_frame( int16_t level[16], const uint8_t *p_src, uint8_t *p_dst )
{
    ZIGZAG4_FRAME
    COPY4x4
}

static void zigzag_sub_4x4_field( int16_t level[16], const uint8_t *p_src, uint8_t *p_dst )
{
    ZIGZAG4_FIELD
    COPY4x4
}

static void zigzag_sub_8x8_frame( int16_t level[64], const uint8_t *p_src, uint8_t *p_dst )
{
    ZIGZAG8_FRAME
    COPY8x8
}
static void zigzag_sub_8x8_field( int16_t level[64], const uint8_t *p_src, uint8_t *p_dst )
{
    ZIGZAG8_FIELD
    COPY8x8
}

#undef ZIG
#undef COPY4x4

static void zigzag_interleave_8x8_cavlc( int16_t *dst, int16_t *src, uint8_t *nnz )
{
    int i,j;
    for( i=0; i<4; i++ )
    {
        int nz = 0;
        for( j=0; j<16; j++ )
        {
            nz |= src[i+j*4];
            dst[i*16+j] = src[i+j*4];
        }
        nnz[(i&1) + (i>>1)*8] = !!nz;
    }
}

void x264_zigzag_init( int cpu, x264_zigzag_function_t *pf, int b_interlaced )
{
    if( b_interlaced )
    {
        pf->scan_8x8   = zigzag_scan_8x8_field;
        pf->scan_4x4   = zigzag_scan_4x4_field;
        pf->sub_8x8    = zigzag_sub_8x8_field;
        pf->sub_4x4    = zigzag_sub_4x4_field;
#ifdef HAVE_MMX
        if( cpu&X264_CPU_MMXEXT )
            pf->scan_4x4 = x264_zigzag_scan_4x4_field_mmxext;
#endif

#ifdef ARCH_PPC
        if( cpu&X264_CPU_ALTIVEC )
            pf->scan_4x4   = x264_zigzag_scan_4x4_field_altivec;
#endif
    }
    else
    {
        pf->scan_8x8   = zigzag_scan_8x8_frame;
        pf->scan_4x4   = zigzag_scan_4x4_frame;
        pf->sub_8x8    = zigzag_sub_8x8_frame;
        pf->sub_4x4    = zigzag_sub_4x4_frame;
#ifdef HAVE_MMX
        if( cpu&X264_CPU_MMX )
            pf->scan_4x4 = x264_zigzag_scan_4x4_frame_mmx;
        if( cpu&X264_CPU_MMXEXT )
            pf->scan_8x8 = x264_zigzag_scan_8x8_frame_mmxext;
        if( cpu&X264_CPU_SSE2_IS_FAST )
            pf->scan_8x8 = x264_zigzag_scan_8x8_frame_sse2;
        if( cpu&X264_CPU_SSSE3 )
        {
            pf->sub_4x4  = x264_zigzag_sub_4x4_frame_ssse3;
            pf->scan_8x8 = x264_zigzag_scan_8x8_frame_ssse3;
            if( cpu&X264_CPU_SHUFFLE_IS_FAST )
                pf->scan_4x4 = x264_zigzag_scan_4x4_frame_ssse3;
        }
#endif

#ifdef ARCH_PPC
        if( cpu&X264_CPU_ALTIVEC )
            pf->scan_4x4   = x264_zigzag_scan_4x4_frame_altivec;
#endif
    }

    pf->interleave_8x8_cavlc = zigzag_interleave_8x8_cavlc;
#ifdef HAVE_MMX
    if( cpu&X264_CPU_MMX )
        pf->interleave_8x8_cavlc = x264_zigzag_interleave_8x8_cavlc_mmx;
    if( cpu&X264_CPU_SHUFFLE_IS_FAST )
        pf->interleave_8x8_cavlc = x264_zigzag_interleave_8x8_cavlc_sse2;
#endif
}
