/*****************************************************************************
 * dct.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: dct-c.c,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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
#include <stdlib.h>
#include <stdarg.h>

#include "x264.h"

#include "common/dct.h"
#include "dct.h"


#if 0
#define MMX_ZERO( MMZ ) \
    asm volatile( "pxor " #MMZ ", " #MMZ "\n" :: )

/* MMP : diff,  MMT: temp */
#define MMX_LOAD_DIFF_4P( MMP, MMT, MMZ, pix1, pix2 ) \
    asm volatile( "movd (%0), " #MMP "\n" \
                  "punpcklbw  " #MMZ ", " #MMP "\n" \
                  "movd (%1), " #MMT "\n" \
                  "punpcklbw  " #MMZ ", " #MMT "\n" \
                  "psubw      " #MMT ", " #MMP "\n" : : "r"(pix1), "r"(pix2) )

/* in: out: mma=mma+mmb, mmb=mmb-mma */
#define MMX_SUMSUB_BA( MMA, MMB ) \
    asm volatile( "paddw " #MMB ", " #MMA "\n"\
                  "paddw " #MMB ", " #MMB "\n"\
                  "psubw " #MMA ", " #MMB "\n" :: )

#define MMX_SUMSUB_BADC( MMA, MMB, MMC, MMD ) \
    asm volatile( "paddw " #MMB ", " #MMA "\n"\
                  "paddw " #MMD ", " #MMC "\n"\
                  "paddw " #MMB ", " #MMB "\n"\
                  "paddw " #MMD ", " #MMD "\n"\
                  "psubw " #MMA ", " #MMB "\n"\
                  "psubw " #MMC ", " #MMD "\n" :: )

/* inputs MMA, MMB output MMA MMT */
#define MMX_SUMSUB2_AB( MMA, MMB, MMT ) \
    asm volatile( "movq  " #MMA ", " #MMT "\n" \
                  "paddw " #MMA ", " #MMA "\n" \
                  "paddw " #MMB ", " #MMA "\n" \
                  "psubw " #MMB ", " #MMT "\n" \
                  "psubw " #MMB ", " #MMT "\n" :: )

/* inputs MMA, MMB output MMA MMS */
#define MMX_SUMSUBD2_AB( MMA, MMB, MMT, MMS ) \
    asm volatile( "movq  " #MMA ", " #MMS "\n" \
                  "movq  " #MMB ", " #MMT "\n" \
                  "psraw   $1    , " #MMB "\n"       \
                  "psraw   $1    , " #MMS "\n"       \
                  "paddw " #MMB ", " #MMA "\n" \
                  "psubw " #MMT ", " #MMS "\n" :: )

#define SBUTTERFLYwd(a,b,t )\
    asm volatile( "movq " #a ", " #t "        \n\t" \
                  "punpcklwd " #b ", " #a "   \n\t" \
                  "punpckhwd " #b ", " #t "   \n\t" :: )

#define SBUTTERFLYdq(a,b,t )\
    asm volatile( "movq " #a ", " #t "        \n\t" \
                  "punpckldq " #b ", " #a "   \n\t" \
                  "punpckhdq " #b ", " #t "   \n\t" :: )

/* input ABCD output ADTC */
#define MMX_TRANSPOSE( MMA, MMB, MMC, MMD, MMT ) \
        SBUTTERFLYwd( MMA, MMB, MMT ); \
        SBUTTERFLYwd( MMC, MMD, MMB ); \
        SBUTTERFLYdq( MMA, MMC, MMD ); \
        SBUTTERFLYdq( MMT, MMB, MMC )

#define MMX_STORE_DIFF_4P( MMP, MMT, MM32, MMZ, dst ) \
    asm volatile( "paddw     " #MM32 "," #MMP "\n" \
                  "psraw       $6,     " #MMP "\n" \
                  "movd        (%0),   " #MMT "\n" \
                  "punpcklbw " #MMZ ", " #MMT "\n" \
                  "paddsw    " #MMT ", " #MMP "\n" \
                  "packuswb  " #MMZ ", " #MMP "\n" \
                  "movd      " #MMP ",   (%0)\n" :: "r"(dst) )

#define UNUSED_LONGLONG( foo ) \
    static const unsigned long long foo __asm__ (#foo)  __attribute__((unused)) __attribute__((aligned(16)))

UNUSED_LONGLONG( x264_mmx_32 ) = 0x0020002000200020ULL;
UNUSED_LONGLONG( x264_mmx_1 ) = 0x0001000100010001ULL;


/*
 * XXX For all dct dc : input could be equal to output so ...
 */
void x264_dct4x4dc_mmxext( int16_t d[4][4] )
{
    /* load DCT */
    asm volatile(
        "movq   (%0), %%mm0\n"
        "movq  8(%0), %%mm1\n"
        "movq 16(%0), %%mm2\n"
        "movq 24(%0), %%mm3\n" :: "r"(d) );

    MMX_SUMSUB_BADC( %%mm1, %%mm0, %%mm3, %%mm2 );  /* mm1=s01  mm0=d01  mm3=s23  mm2=d23 */
    MMX_SUMSUB_BADC( %%mm3, %%mm1, %%mm2, %%mm0 );  /* mm3=s01+s23  mm1=s01-s23  mm2=d01+d23  mm0=d01-d23 */

    /* in: mm3, mm1, mm0, mm2  out: mm3, mm2, mm4, mm0 */
    MMX_TRANSPOSE  ( %%mm3, %%mm1, %%mm0, %%mm2, %%mm4 );

    MMX_SUMSUB_BADC( %%mm2, %%mm3, %%mm0, %%mm4 );  /* mm2=s01  mm3=d01  mm0=s23  mm4=d23 */
    MMX_SUMSUB_BADC( %%mm0, %%mm2, %%mm4, %%mm3 );  /* mm0=s01+s23  mm2=s01-s23  mm4=d01+d23  mm3=d01-d23 */

    /* in: mm0, mm2, mm3, mm4  out: mm0, mm4, mm1, mm3 */
    MMX_TRANSPOSE  ( %%mm0, %%mm2, %%mm3, %%mm4, %%mm1 );


    asm volatile( "movq x264_mmx_1, %%mm6" :: );

    /* Store back */
    asm volatile(
        "paddw %%mm6, %%mm0\n"
        "paddw %%mm6, %%mm4\n"

        "psraw $1,    %%mm0\n"
        "movq  %%mm0,   (%0)\n"
        "psraw $1,    %%mm4\n"
        "movq  %%mm4,  8(%0)\n"

        "paddw %%mm6, %%mm1\n"
        "paddw %%mm6, %%mm3\n"

        "psraw $1,    %%mm1\n"
        "movq  %%mm1, 16(%0)\n"
        "psraw $1,    %%mm3\n"
        "movq  %%mm3, 24(%0)\n" :: "r"(d) );
}

void x264_idct4x4dc_mmxext( int16_t d[4][4] )
{
    /* load DCT */
    asm volatile(
        "movq   (%0), %%mm0\n"
        "movq  8(%0), %%mm1\n"
        "movq 16(%0), %%mm2\n" 
        "movq 24(%0), %%mm3\n" :: "r"(d) );

    MMX_SUMSUB_BADC( %%mm1, %%mm0, %%mm3, %%mm2 );  /* mm1=s01  mm0=d01  mm3=s23  mm2=d23 */
    MMX_SUMSUB_BADC( %%mm3, %%mm1, %%mm2, %%mm0 );  /* mm3=s01+s23 mm1=s01-s23 mm2=d01+d23 mm0=d01-d23 */

    /* in: mm3, mm1, mm0, mm2  out: mm3, mm2, mm4, mm0 */
    MMX_TRANSPOSE( %%mm3, %%mm1, %%mm0, %%mm2, %%mm4 );

    MMX_SUMSUB_BADC( %%mm2, %%mm3, %%mm0, %%mm4 );  /* mm2=s01  mm3=d01  mm0=s23  mm4=d23 */
    MMX_SUMSUB_BADC( %%mm0, %%mm2, %%mm4, %%mm3 );  /* mm0=s01+s23 mm2=s01-s23 mm4=d01+d23 mm3=d01-d23 */

    /* in: mm0, mm2, mm3, mm4  out: mm0, mm4, mm1, mm3 */
    MMX_TRANSPOSE( %%mm0, %%mm2, %%mm3, %%mm4, %%mm1 );

    /* Store back */
    asm volatile(
        "movq %%mm0,   (%0)\n"
        "movq %%mm4,  8(%0)\n"
        "movq %%mm1, 16(%0)\n" 
        "movq %%mm3, 24(%0)\n" :: "r"(d) );
}

/****************************************************************************
 * subXxX_dct:
 ****************************************************************************/
inline void x264_sub4x4_dct_mmxext( int16_t dct[4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    /* Reset mm7 */
    MMX_ZERO( %%mm7 );

    /* Load 4 lines */
    MMX_LOAD_DIFF_4P( %%mm0, %%mm6, %%mm7, &pix1[0*i_pix1], &pix2[0*i_pix2] );
    MMX_LOAD_DIFF_4P( %%mm1, %%mm6, %%mm7, &pix1[1*i_pix1], &pix2[1*i_pix2] );
    MMX_LOAD_DIFF_4P( %%mm2, %%mm6, %%mm7, &pix1[2*i_pix1], &pix2[2*i_pix2] );
    MMX_LOAD_DIFF_4P( %%mm3, %%mm6, %%mm7, &pix1[3*i_pix1], &pix2[3*i_pix2] );

    MMX_SUMSUB_BADC( %%mm3, %%mm0, %%mm2, %%mm1 );  /* mm3=s03  mm0=d03  mm2=s12  mm1=d12 */

    MMX_SUMSUB_BA(  %%mm2, %%mm3 );                 /* mm2=s03+s12      mm3=s03-s12 */
    MMX_SUMSUB2_AB( %%mm0, %%mm1, %%mm4 );          /* mm0=2.d03+d12    mm4=d03-2.d12 */

    /* transpose in: mm2, mm0, mm3, mm4, out: mm2, mm4, mm1, mm3 */
    MMX_TRANSPOSE( %%mm2, %%mm0, %%mm3, %%mm4, %%mm1 );

    MMX_SUMSUB_BADC( %%mm3, %%mm2, %%mm1, %%mm4 );  /* mm3=s03  mm2=d03  mm1=s12  mm4=d12 */

    MMX_SUMSUB_BA(  %%mm1, %%mm3 );                 /* mm1=s03+s12      mm3=s03-s12 */
    MMX_SUMSUB2_AB( %%mm2, %%mm4, %%mm0 );          /* mm2=2.d03+d12    mm0=d03-2.d12 */

    /* transpose in: mm1, mm2, mm3, mm0, out: mm1, mm0, mm4, mm3 */
    MMX_TRANSPOSE( %%mm1, %%mm2, %%mm3, %%mm0, %%mm4 );

    /* Store back */
    asm volatile(
        "movq %%mm1, (%0)\n"
        "movq %%mm0, 8(%0)\n"
        "movq %%mm4, 16(%0)\n"
        "movq %%mm3, 24(%0)\n" :: "r"(dct) );
}
#endif

void x264_sub8x8_dct_mmxext( int16_t dct[4][4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    x264_sub4x4_dct_mmxext( dct[0], &pix1[0], i_pix1, &pix2[0], i_pix2 );
    x264_sub4x4_dct_mmxext( dct[1], &pix1[4], i_pix1, &pix2[4], i_pix2 );
    x264_sub4x4_dct_mmxext( dct[2], &pix1[4*i_pix1+0], i_pix1, &pix2[4*i_pix2+0], i_pix2 );
    x264_sub4x4_dct_mmxext( dct[3], &pix1[4*i_pix1+4], i_pix1, &pix2[4*i_pix2+4], i_pix2 );
}

void x264_sub16x16_dct_mmxext( int16_t dct[16][4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    x264_sub8x8_dct_mmxext( &dct[ 0], &pix1[0], i_pix1, &pix2[0], i_pix2 );
    x264_sub8x8_dct_mmxext( &dct[ 4], &pix1[8], i_pix1, &pix2[8], i_pix2 );
    x264_sub8x8_dct_mmxext( &dct[ 8], &pix1[8*i_pix1], i_pix1, &pix2[8*i_pix2], i_pix2 );
    x264_sub8x8_dct_mmxext( &dct[12], &pix1[8*i_pix1+8], i_pix1, &pix2[8*i_pix2+8], i_pix2 );
}



/****************************************************************************
 * addXxX_idct:
 ****************************************************************************/
#if 0
inline void x264_add4x4_idct_mmxext( uint8_t *p_dst, int i_dst, int16_t dct[4][4] )
{
    /* Load dct coeffs */
    asm volatile(
        "movq   (%0), %%mm0\n"
        "movq  8(%0), %%mm1\n"
        "movq 16(%0), %%mm2\n"
        "movq 24(%0), %%mm3\n" :: "r"(dct) );

    MMX_SUMSUB_BA  ( %%mm2, %%mm0 );                /* mm2=s02  mm0=d02 */
    MMX_SUMSUBD2_AB( %%mm1, %%mm3, %%mm5, %%mm4 );  /* mm1=s13  mm4=d13 ( well 1 + 3>>1 and 1>>1 + 3) */

    MMX_SUMSUB_BADC( %%mm1, %%mm2, %%mm4, %%mm0 );  /* mm1=s02+s13  mm2=s02-s13  mm4=d02+d13  mm0=d02-d13 */

    /* in: mm1, mm4, mm0, mm2  out: mm1, mm2, mm3, mm0 */
    MMX_TRANSPOSE  ( %%mm1, %%mm4, %%mm0, %%mm2, %%mm3 );

    MMX_SUMSUB_BA  ( %%mm3, %%mm1 );                /* mm3=s02  mm1=d02 */
    MMX_SUMSUBD2_AB( %%mm2, %%mm0, %%mm5, %%mm4 );  /* mm2=s13  mm4=d13 ( well 1 + 3>>1 and 1>>1 + 3) */

    MMX_SUMSUB_BADC( %%mm2, %%mm3, %%mm4, %%mm1 );  /* mm2=s02+s13  mm3=s02-s13  mm4=d02+d13  mm1=d02-d13 */

    /* in: mm2, mm4, mm1, mm3  out: mm2, mm3, mm0, mm1 */
    MMX_TRANSPOSE  ( %%mm2, %%mm4, %%mm1, %%mm3, %%mm0 );

    MMX_ZERO( %%mm7 );
    asm volatile( "movq x264_mmx_32, %%mm6\n" :: );

    MMX_STORE_DIFF_4P( %%mm2, %%mm4, %%mm6, %%mm7, &p_dst[0*i_dst] );
    MMX_STORE_DIFF_4P( %%mm3, %%mm4, %%mm6, %%mm7, &p_dst[1*i_dst] );
    MMX_STORE_DIFF_4P( %%mm0, %%mm4, %%mm6, %%mm7, &p_dst[2*i_dst] );
    MMX_STORE_DIFF_4P( %%mm1, %%mm4, %%mm6, %%mm7, &p_dst[3*i_dst] );
}
#endif

void x264_add8x8_idct_mmxext( uint8_t *p_dst, int i_dst, int16_t dct[4][4][4] )
{
    x264_add4x4_idct_mmxext( p_dst, i_dst,             dct[0] );
    x264_add4x4_idct_mmxext( &p_dst[4], i_dst,         dct[1] );
    x264_add4x4_idct_mmxext( &p_dst[4*i_dst+0], i_dst, dct[2] );
    x264_add4x4_idct_mmxext( &p_dst[4*i_dst+4], i_dst, dct[3] );
}

void x264_add16x16_idct_mmxext( uint8_t *p_dst, int i_dst, int16_t dct[16][4][4] )
{
    x264_add8x8_idct_mmxext( &p_dst[0], i_dst, &dct[0] );
    x264_add8x8_idct_mmxext( &p_dst[8], i_dst, &dct[4] );
    x264_add8x8_idct_mmxext( &p_dst[8*i_dst], i_dst, &dct[8] );
    x264_add8x8_idct_mmxext( &p_dst[8*i_dst+8], i_dst, &dct[12] );
}
