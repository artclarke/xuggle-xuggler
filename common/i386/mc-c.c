/*****************************************************************************
 * mc.c: h264 encoder library (Motion Compensation)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: mc-c.c,v 1.5 2004/06/18 01:59:58 chenm001 Exp $
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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "x264.h"   /* DECLARE_ALIGNED */
#include "common/mc.h"
#include "common/clip1.h"
#include "mc.h"

/* NASM functions */
extern void x264_pixel_avg_w4_mmxext( uint8_t *,  int, uint8_t *, int, uint8_t *, int, int );
extern void x264_pixel_avg_w8_mmxext( uint8_t *,  int, uint8_t *, int, uint8_t *, int, int );
extern void x264_pixel_avg_w16_mmxext( uint8_t *,  int, uint8_t *, int, uint8_t *, int, int );
extern void x264_pixel_avg_w16_sse2( uint8_t *,  int, uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w4_mmxext( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w8_mmxext( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w16_mmxext( uint8_t *, int, uint8_t *, int, int );
extern void x264_mc_copy_w16_sse2( uint8_t *, int, uint8_t *, int, int );

#if 0

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define USED_UINT64(foo) \
    static const uint64_t foo __asm__ (#foo) __attribute__((used))
#else
#define USED_UINT64(foo) \
    static const uint64_t foo __asm__ (#foo) __attribute__((unused))
#endif

USED_UINT64( x264_w0x10 ) = 0x0010001000100010ULL;


#define MMX_ZERO( MMZ ) \
    asm volatile( "pxor " #MMZ ", " #MMZ "\n" :: )

#define MMX_INIT( MMV, NAME ) \
    asm volatile( "movq " #NAME ", " #MMV "\n" :: )

#define MMX_SAVE_4P( MMP, MMZ, dst ) \
    asm volatile( "packuswb " #MMZ  "," #MMP "\n" \
                  "movd " #MMP ", (%0)" :: "r"(dst) )

#define MMX_LOAD_4P( MMP, MMZ, pix ) \
    asm volatile( "movd (%0), " #MMP "\n" \
                  "punpcklbw  " #MMZ ", " #MMP "\n" : : "r"(pix) )

#define MMX_LOAD_4x4( MMP1, MMP2, MMP3, MMP4, MMZ, pix, i_pix )\
    MMX_LOAD_4P( MMP1, MMZ, &(pix)[0*(i_pix)] ); \
    MMX_LOAD_4P( MMP2, MMZ, &(pix)[1*(i_pix)] ); \
    MMX_LOAD_4P( MMP3, MMZ, &(pix)[2*(i_pix)] ); \
    MMX_LOAD_4P( MMP4, MMZ, &(pix)[3*(i_pix)] )

#define MMX_LOAD_2x4( MMP1, MMP2, MMZ, pix, i_pix )\
    MMX_LOAD_4P( MMP1, MMZ, &(pix)[0*(i_pix)] ); \
    MMX_LOAD_4P( MMP2, MMZ, &(pix)[1*(i_pix)] )

#define MMX_SAVEPACK_8P( MMP1, MMP2, MMZ, dst ) \
    asm volatile( "packuswb " #MMP2  "," #MMP1 "\n" \
                  "movq " #MMP1 ", (%0)\n" :: "r"(dst) )


#define MMX_LOAD_8P( MMP1, MMP2, MMZ, pix ) \
    asm volatile( "movq         (%0)   , " #MMP1 "\n" \
                  "movq       " #MMP1 ", " #MMP2 "\n" \
                  "punpcklbw  " #MMZ  ", " #MMP1 "\n" \
                  "punpckhbw  " #MMZ  ", " #MMP2 "\n" : : "r"(pix) )

#define MMX_LOAD_2x8( MMP1, MMP2, MMP3, MMP4, MMZ, pix, i_pix )\
    MMX_LOAD_8P( MMP1, MMP2, MMZ, &(pix)[0*(i_pix)] ); \
    MMX_LOAD_8P( MMP3, MMP4, MMZ, &(pix)[1*(i_pix)] )

#define SBUTTERFLYwd(a,b,t )\
    asm volatile( "movq " #a ", " #t "        \n\t" \
                  "punpcklwd " #b ", " #a "   \n\t" \
                  "punpckhwd " #b ", " #t "   \n\t" :: )

#define SBUTTERFLYdq(a,b,t )\
    asm volatile( "movq " #a ", " #t "        \n\t" \
                  "punpckldq " #b ", " #a "   \n\t" \
                  "punpckhdq " #b ", " #t "   \n\t" :: )

/* input ABCD output ADTC  ( or 0?31-2->0123 ) */
#define MMX_TRANSPOSE( MMA, MMB, MMC, MMD, MMT ) \
        SBUTTERFLYwd( MMA, MMB, MMT ); \
        SBUTTERFLYwd( MMC, MMD, MMB ); \
        SBUTTERFLYdq( MMA, MMC, MMD ); \
        SBUTTERFLYdq( MMT, MMB, MMC )

/* first pass MM0 = MM0 -5*MM1 */
#define MMX_FILTERTAP_P1( MMP0, MMP1 ) \
    asm volatile( "psubw    " #MMP1 "," #MMP0 "\n" \
                  "psllw      $2,     " #MMP1 "\n" \
                  "psubw    " #MMP1 "," #MMP0 "\n" :: )
                                                   \
/* second pass MM0 = MM0 + 20*(MM2+MM3) */
#define MMX_FILTERTAP_P2( MMP0, MMP2, MMP3 ) \
    asm volatile( "paddw    " #MMP3 "," #MMP2 "\n" \
                                                 \
                  "psllw      $2,     " #MMP2 "\n" \
                  "paddw    " #MMP2 "," #MMP0 "\n" \
                  "psllw      $2,     " #MMP2 "\n" \
                  "paddw    " #MMP2 "," #MMP0 "\n" :: )

/* last pass: MM0 = ( MM0 -5*MM1 + MM2 + MMV ) >> 5 */
#define MMX_FILTERTAP_P3( MMP0, MMP1, MMP2, MMV, MMZ ) \
    asm volatile( "psubw    " #MMP1 "," #MMP0 "\n" \
                  "psllw      $2,     " #MMP1 "\n" \
                  "psubw    " #MMP1 "," #MMP0 "\n" \
                                                   \
                  "paddw    " #MMP2 "," #MMP0 "\n" \
                  "paddw    " #MMV  "," #MMP0 "\n" \
                  "psraw      $5,     " #MMP0 "\n" :: )

#define MMX_FILTERTAP2_P1( MMP0, MMP1, MMP2, MMP3 ) \
    asm volatile( "psubw    " #MMP1 "," #MMP0 "\n" \
                  "psubw    " #MMP3 "," #MMP2 "\n" \
                  "psllw      $2,     " #MMP1 "\n" \
                  "psllw      $2,     " #MMP3 "\n" \
                  "psubw    " #MMP1 "," #MMP0 "\n" \
                  "psubw    " #MMP3 "," #MMP2 "\n" :: )

/* second pass MM0 = MM0 + 20*(MM1+MM2) */
#define MMX_FILTERTAP2_P2( MMP0, MMP1, MMP2, MMP3, MMP4, MMP5 ) \
    asm volatile( "paddw    " #MMP2 "," #MMP1 "\n" \
                  "paddw    " #MMP5 "," #MMP4 "\n" \
                                                 \
                  "psllw      $2,     " #MMP1 "\n" \
                  "psllw      $2,     " #MMP4 "\n" \
                  "paddw    " #MMP1 "," #MMP0 "\n" \
                  "paddw    " #MMP4 "," #MMP3 "\n" \
                  "psllw      $2,     " #MMP1 "\n" \
                  "psllw      $2,     " #MMP4 "\n" \
                  "paddw    " #MMP1 "," #MMP0 "\n" \
                  "paddw    " #MMP4 "," #MMP3 "\n" :: )

#define MMX_LOAD_1r( m1, dst ) \
    asm volatile( "movq (%0), " #m1 "\n" :: "r"(dst) ); \

#define MMX_SAVE_1r( m1, dst ) \
    asm volatile( "movq " #m1 ", (%0)\n" :: "r"(dst) ); \

#define MMX_LOAD_2r( m1, m2, dst, i_dst ) \
    asm volatile( "movq (%0), " #m1 "\n" :: "r"(&((uint8_t*)dst)[0*(i_dst)]) ); \
    asm volatile( "movq (%0), " #m2 "\n" :: "r"(&((uint8_t*)dst)[1*(i_dst)]) )

#define MMX_SAVE_2r( m1, m2, dst, i_dst ) \
    asm volatile( "movq " #m1 ", (%0)\n" :: "r"(&((uint8_t*)dst)[0*(i_dst)]) ); \
    asm volatile( "movq " #m2 ", (%0)\n" :: "r"(&((uint8_t*)dst)[1*(i_dst)]) )

#define MMX_SAVE_4r( m1, m2, m3, m4, dst, i_dst ) \
    asm volatile( "movq " #m1 ", (%0)\n" :: "r"(&((uint8_t*)dst)[0*(i_dst)]) ); \
    asm volatile( "movq " #m2 ", (%0)\n" :: "r"(&((uint8_t*)dst)[1*(i_dst)]) ); \
    asm volatile( "movq " #m3 ", (%0)\n" :: "r"(&((uint8_t*)dst)[2*(i_dst)]) ); \
    asm volatile( "movq " #m4 ", (%0)\n" :: "r"(&((uint8_t*)dst)[3*(i_dst)]) )

#define MMX_LOAD_4r( m1, m2, m3, m4, dst, i_dst ) \
    asm volatile( "movq (%0), " #m1 "\n" :: "r"(&((uint8_t*)dst)[0*(i_dst)]) ); \
    asm volatile( "movq (%0), " #m2 "\n" :: "r"(&((uint8_t*)dst)[1*(i_dst)]) ); \
    asm volatile( "movq (%0), " #m3 "\n" :: "r"(&((uint8_t*)dst)[2*(i_dst)]) ); \
    asm volatile( "movq (%0), " #m4 "\n" :: "r"(&((uint8_t*)dst)[3*(i_dst)]) )


static inline int x264_tapfilter( uint8_t *pix, int i_pix_next )
{
    return pix[-2*i_pix_next] - 5*pix[-1*i_pix_next] + 20*(pix[0] + pix[1*i_pix_next]) - 5*pix[ 2*i_pix_next] + pix[ 3*i_pix_next];
}
static inline int x264_tapfilter1( uint8_t *pix )
{
    return pix[-2] - 5*pix[-1] + 20*(pix[0] + pix[1]) - 5*pix[ 2] + pix[ 3];
}

typedef void (*pf_mc_t)(uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height );

/* Macro to define NxM functions */
/* mc I+H */
#define MC_IH( name, cpu, width, height, off )  \
static void name##_w##width##_##cpu( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height ) \
{                                                               \
    DECLARE_ALIGNED( uint8_t, tmp[width*height], width );       \
                                                                \
    mc_hh_w##width( src, i_src_stride, tmp, width, i_height );  \
    x264_pixel_avg_w##width##_##cpu( dst, i_dst_stride,         \
                                     src+(off), i_src_stride,   \
                                     tmp, width, i_height );    \
}

/* mc I+V */
#define MC_IV( name, cpu, width, height, off )  \
static void name##_w##width##_##cpu( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height ) \
{                                                               \
    DECLARE_ALIGNED( uint8_t, tmp[width*height], width );       \
                                                                \
    mc_hv_w##width( src, i_src_stride, tmp, width, i_height );  \
    x264_pixel_avg_w##width##_##cpu( dst, i_dst_stride,         \
                                     src+(off), i_src_stride,   \
                                     tmp, width, i_height );    \
}

/* mc H+V */
#define MC_HV( name, cpu, width, height, off1, off2 ) \
static void name##_w##width##_##cpu( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height ) \
{                                                               \
    DECLARE_ALIGNED( uint8_t, tmp1[width*height], width );      \
    DECLARE_ALIGNED( uint8_t, tmp2[width*height], width );      \
                                                                \
    mc_hv_w##width( src+(off1), i_src_stride, tmp1, width, i_height );  \
    mc_hh_w##width( src+(off2), i_src_stride, tmp2, width, i_height );  \
    x264_pixel_avg_w##width##_##cpu( dst, i_dst_stride,         \
                                     tmp1, width, tmp2, width,  \
                                     i_height );                \
}

/* mc C+H */
#define MC_CH( name, cpu, width, height, off ) \
static void name##_w##width##_##cpu( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height ) \
{                                                               \
    DECLARE_ALIGNED( uint8_t, tmp1[width*height], width );      \
    DECLARE_ALIGNED( uint8_t, tmp2[width*height], width );      \
                                                                \
    mc_hc_w##width( src,       i_src_stride, tmp1, width, i_height );  \
    mc_hh_w##width( src+(off), i_src_stride, tmp2, width, i_height );  \
    x264_pixel_avg_w##width##_##cpu( dst, i_dst_stride,         \
                                     tmp1, width, tmp2, width,  \
                                     i_height );                \
}

/* mc C+V */
#define MC_CV( name, cpu, width, height, off ) \
static void name##_w##width##_##cpu( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height ) \
{                                                               \
    DECLARE_ALIGNED( uint8_t, tmp1[width*height], width );      \
    DECLARE_ALIGNED( uint8_t, tmp2[width*height], width );      \
                                                                \
    mc_hc_w##width( src,       i_src_stride, tmp1, width, i_height );  \
    mc_hv_w##width( src+(off), i_src_stride, tmp2, width, i_height );  \
    x264_pixel_avg_w##width##_##cpu( dst, i_dst_stride,         \
                                     tmp1, width, tmp2, width,  \
                                     i_height );                \
}


/*****************************************************************************
 * MC with width == 4 (height <= 8)
 *****************************************************************************/

static inline void mc_hh_w4( uint8_t *src, int i_src, uint8_t *dst, int i_dst, int i_height )
{
    const int h4 = i_height / 4;
    uint8_t  srct[4*8*3];
    uint64_t tmp[4];
    int y;

    src -= 2;

    MMX_ZERO( %%mm7 );
    MMX_INIT( %%mm6, x264_w0x10 );

    for( y = 0; y < h4; y++ )
    {
        int i;

        /* Preload data and transpose them */
        MMX_LOAD_4x4 ( %%mm0, %%mm4, %%mm3, %%mm1, %%mm7, &src[0], i_src );
        MMX_TRANSPOSE( %%mm0, %%mm4, %%mm3, %%mm1, %%mm2 ); /* 0123 */
        MMX_SAVE_4r( %%mm0, %%mm1, %%mm2, %%mm3, &srct[4*8*0], 8 );

        MMX_LOAD_4x4 ( %%mm0, %%mm4, %%mm3, %%mm1, %%mm7, &src[4], i_src );
        MMX_TRANSPOSE( %%mm0, %%mm4, %%mm3, %%mm1, %%mm2 ); /* 0123 */
        MMX_SAVE_4r( %%mm0, %%mm1, %%mm2, %%mm3, &srct[4*8*1], 8 );

        /* we read 2 more bytes that needed */
        MMX_LOAD_4x4 ( %%mm0, %%mm4, %%mm3, %%mm1, %%mm7, &src[8], i_src );
        MMX_TRANSPOSE( %%mm0, %%mm4, %%mm3, %%mm1, %%mm2 ); /* 0123 */
        MMX_SAVE_2r( %%mm0, %%mm1, &srct[4*8*2], 8 );

        /* tap filter */
        for( i = 0; i < 4; i++ )
        {
            MMX_LOAD_4r( %%mm0, %%mm1, %%mm2, %%mm3, &srct[8*(i+0)], 8 );
            MMX_FILTERTAP_P1( %%mm0, %%mm1 );
            MMX_FILTERTAP_P2( %%mm0, %%mm2, %%mm3 );

            MMX_LOAD_2r( %%mm1, %%mm2, &srct[8*(i+4)], 8 );
            MMX_FILTERTAP_P3( %%mm0, %%mm1, %%mm2, %%mm6, %%mm7 );

            MMX_SAVE_1r( %%mm0, &tmp[i] );
        }

        MMX_LOAD_4r( %%mm0, %%mm4, %%mm3, %%mm1, tmp, 8 );
        MMX_TRANSPOSE( %%mm0, %%mm4, %%mm3, %%mm1, %%mm2 ); /* 0123 */
        MMX_SAVE_4P( %%mm0, %%mm7, &dst[0*i_dst] );
        MMX_SAVE_4P( %%mm1, %%mm7, &dst[1*i_dst] );
        MMX_SAVE_4P( %%mm2, %%mm7, &dst[2*i_dst] );
        MMX_SAVE_4P( %%mm3, %%mm7, &dst[3*i_dst] );

        src += 4 * i_src;
        dst += 4 * i_dst;
    }
}
static inline void mc_hv_w4( uint8_t *src, int i_src, uint8_t *dst, int i_dst, int i_height )
{
    int y;

    src -= 2 * i_src;

    MMX_ZERO( %%mm7 );
    MMX_INIT( %%mm6, x264_w0x10 );

    for( y = 0; y < i_height; y++ )
    {
        MMX_LOAD_4x4( %%mm0, %%mm1, %%mm2, %%mm3, %%mm7, src, i_src );
        MMX_FILTERTAP_P1( %%mm0, %%mm1 );
        MMX_FILTERTAP_P2( %%mm0, %%mm2, %%mm3 );

        MMX_LOAD_2x4( %%mm4, %%mm5, %%mm7, &src[4*i_src], i_src );
        MMX_FILTERTAP_P3( %%mm0, %%mm4, %%mm5, %%mm6, %%mm7 );
        MMX_SAVE_4P( %%mm0, %%mm7, dst );

        src += i_src;
        dst += i_dst;
    }
}

static inline void mc_hc_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    int i, x, y;

    for( y = 0; y < i_height; y++ )
    {
        int16_t tap[5+4];

        for( i = 0; i < 5+4; i++ )
        {
            tap[i] = x264_tapfilter( &src[-2+i], i_src_stride );
        }

        for( x = 0; x < 4; x++ )
        {
            dst[x] = x264_mc_clip1( ( tap[0+x] - 5*tap[1+x] + 20 * tap[2+x] + 20 * tap[3+x] -5*tap[4+x] + tap[5+x] + 512 ) >> 10 );
        }

        src += i_src_stride;
        dst += i_dst_stride;
    }
}

MC_IH( mc_xy10, mmxext, 4, 8, 0 )
MC_IH( mc_xy30, mmxext, 4, 8, 1 )

MC_IV( mc_xy01, mmxext, 4, 8, 0 )
MC_IV( mc_xy03, mmxext, 4, 8, i_src_stride )

MC_HV( mc_xy11, mmxext, 4, 8, 0, 0 )
MC_HV( mc_xy31, mmxext, 4, 8, 1, 0 )
MC_HV( mc_xy13, mmxext, 4, 8, 0, i_src_stride )
MC_HV( mc_xy33, mmxext, 4, 8, 1, i_src_stride )

MC_CH( mc_xy21, mmxext, 4, 8, 0 )
MC_CH( mc_xy23, mmxext, 4, 8, i_src_stride )

MC_CV( mc_xy12, mmxext, 4, 8, 0 )
MC_CV( mc_xy32, mmxext, 4, 8, 1 )

#if 0
static void mc_xy10_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp[4*8];
    mc_hh_w4( src, i_src_stride, tmp, 4, i_height );
    pixel_avg_w4( dst, i_dst_stride, src, i_src_stride, tmp, 4, i_height );
}
static void mc_xy30_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp[4*8];
    mc_hh_w4( src, i_src_stride, tmp, 4, i_height );
    pixel_avg_w4( dst, i_dst_stride, src+1, i_src_stride, tmp, 4, i_height );
}

static void mc_xy01_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp[4*8];
    mc_hv_w4( src, i_src_stride, tmp, 4, i_height );
    pixel_avg_w4( dst, i_dst_stride, src, i_src_stride, tmp, 4, i_height );
}
static void mc_xy03_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp[4*8];
    mc_hv_w4( src, i_src_stride, tmp, 4, i_height );
    pixel_avg_w4( dst, i_dst_stride, src+i_src_stride, i_src_stride, tmp, 4, i_height );
}

static void mc_xy11_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[4*8];
    uint8_t tmp2[4*8];

    mc_hv_w4( src, i_src_stride, tmp1, 4, i_height );
    mc_hh_w4( src, i_src_stride, tmp2, 4, i_height );
    pixel_avg_w4( dst, i_dst_stride, tmp1, 4, tmp2, 4, i_height );
}
static void mc_xy31_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[4*8];
    uint8_t tmp2[4*8];

    mc_hv_w4( src+1, i_src_stride, tmp1, 4, i_height );
    mc_hh_w4( src,   i_src_stride, tmp2, 4, i_height );
    pixel_avg_w4( dst, i_dst_stride, tmp1, 4, tmp2, 4, i_height );
}
static void mc_xy13_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[4*8];
    uint8_t tmp2[4*8];

    mc_hv_w4( src,              i_src_stride, tmp1, 4, i_height );
    mc_hh_w4( src+i_src_stride, i_src_stride, tmp2, 4, i_height );
    pixel_avg_w4( dst, i_dst_stride, tmp1, 4, tmp2, 4, i_height );
}
static void mc_xy33_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[4*8];
    uint8_t tmp2[4*8];

    mc_hv_w4( src+1,            i_src_stride, tmp1, 4, i_height );
    mc_hh_w4( src+i_src_stride, i_src_stride, tmp2, 4, i_height );
    pixel_avg_w4( dst, i_dst_stride, tmp1, 4, tmp2, 4, i_height );
}

static void mc_xy21_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[4*8];
    uint8_t tmp2[4*8];

    mc_hc_w4( src, i_src_stride, tmp1, 4, i_height );
    mc_hh_w4( src, i_src_stride, tmp2, 4, i_height );
    pixel_avg_w4( dst, i_dst_stride, tmp1, 4, tmp2, 4, i_height );
}
static void mc_xy23_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[4*8];
    uint8_t tmp2[4*8];

    mc_hc_w4( src,              i_src_stride, tmp1, 4, i_height );
    mc_hh_w4( src+i_src_stride, i_src_stride, tmp2, 4, i_height );
    pixel_avg_w4( dst, i_dst_stride, tmp1, 4, tmp2, 4, i_height );
}

static void mc_xy12_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[4*8];
    uint8_t tmp2[4*8];

    mc_hc_w4( src, i_src_stride, tmp1, 4, i_height );
    mc_hv_w4( src, i_src_stride, tmp2, 4, i_height );
    pixel_avg_w4( dst, i_dst_stride, tmp1, 4, tmp2, 4, i_height );
}
static void mc_xy32_w4( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[4*8];
    uint8_t tmp2[4*8];

    mc_hc_w4( src,   i_src_stride, tmp1, 4, i_height );
    mc_hv_w4( src+1, i_src_stride, tmp2, 4, i_height );
    pixel_avg_w4( dst, i_dst_stride, tmp1, 4, tmp2, 4, i_height );
}
#endif

/*****************************************************************************
 * MC with width == 8 (height <= 16)
 *****************************************************************************/

static inline void mc_hh_w8( uint8_t *src, int i_src, uint8_t *dst, int i_dst, int i_height )
{
    mc_hh_w4( &src[0], i_src, &dst[0], i_dst, i_height );
    mc_hh_w4( &src[4], i_src, &dst[4], i_dst, i_height );
}
static inline void mc_hv_w8( uint8_t *src, int i_src, uint8_t *dst, int i_dst, int i_height )
{
    int y;

    src -= 2 * i_src;

    MMX_ZERO( %%mm7 );
    MMX_INIT( %%mm6, x264_w0x10 );

    for( y = 0; y < i_height; y++ )
    {
        MMX_LOAD_2x8( %%mm0, %%mm5, %%mm1, %%mm2, %%mm7,  &src[0*i_src], i_src );
        MMX_FILTERTAP2_P1( %%mm0, %%mm1, %%mm5, %%mm2 );


        MMX_LOAD_2x8( %%mm1, %%mm3, %%mm2, %%mm4, %%mm7,  &src[2*i_src], i_src );
        MMX_FILTERTAP2_P2( %%mm0, %%mm1, %%mm2, %%mm5, %%mm3, %%mm4 );

        MMX_LOAD_2x8( %%mm1, %%mm3, %%mm2, %%mm4, %%mm7,  &src[4*i_src], i_src );
        MMX_FILTERTAP_P3( %%mm0, %%mm1, %%mm2, %%mm6, %%mm7 );
        MMX_FILTERTAP_P3( %%mm5, %%mm3, %%mm4, %%mm6, %%mm7 );

        MMX_SAVEPACK_8P( %%mm0, %%mm5, %%mm7, dst );

        src += i_src;
        dst += i_dst;
    }
}

static inline void mc_hc_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    int x, y;

    asm volatile( "pxor %%mm7,        %%mm7\n" : : );

    for( y = 0; y < i_height; y++ )
    {
        int16_t tap[5+8];

        /* first 8 */
        asm volatile(
            "leal   (%0, %1),   %%eax\n"

            "movq       (%0),   %%mm0\n"    /* load pix-2 */
            "movq       %%mm0,  %%mm2\n"
            "punpcklbw  %%mm7,  %%mm0\n"
            "punpckhbw  %%mm7,  %%mm2\n"

            "movq       (%%eax),%%mm1\n"    /* load pix-1 */
            "movq       %%mm1,  %%mm3\n"
            "punpcklbw  %%mm7,  %%mm1\n"
            "punpckhbw  %%mm7,  %%mm3\n"
            "psubw      %%mm1,  %%mm0\n"
            "psllw      $2,     %%mm1\n"
            "psubw      %%mm1,  %%mm0\n"
            "psubw      %%mm3,  %%mm2\n"
            "psllw      $2,     %%mm3\n"
            "psubw      %%mm3,  %%mm2\n"

            "movq       (%%eax,%1),%%mm1\n"  /* load pix */
            "movq       %%mm1,  %%mm3\n"
            "punpcklbw  %%mm7,  %%mm1\n"
            "punpckhbw  %%mm7,  %%mm3\n"
            "psllw      $2,     %%mm1\n"
            "paddw      %%mm1,  %%mm0\n"
            "psllw      $2,     %%mm1\n"
            "paddw      %%mm1,  %%mm0\n"
            "psllw      $2,     %%mm3\n"
            "paddw      %%mm3,  %%mm2\n"
            "psllw      $2,     %%mm3\n"
            "paddw      %%mm3,  %%mm2\n"

            "movq       (%%eax,%1,2),%%mm1\n"  /* load pix+1 */
            "movq       %%mm1,  %%mm3\n"
            "punpcklbw  %%mm7,  %%mm1\n"
            "punpckhbw  %%mm7,  %%mm3\n"
            "psllw      $2,     %%mm1\n"
            "paddw      %%mm1,  %%mm0\n"
            "psllw      $2,     %%mm1\n"
            "paddw      %%mm1,  %%mm0\n"
            "psllw      $2,     %%mm3\n"
            "paddw      %%mm3,  %%mm2\n"
            "psllw      $2,     %%mm3\n"
            "paddw      %%mm3,  %%mm2\n"

            "movq       (%0,%1,4),%%mm1\n"  /* load pix+2 */
            "movq       %%mm1,  %%mm3\n"
            "punpcklbw  %%mm7,  %%mm1\n"
            "punpckhbw  %%mm7,  %%mm3\n"
            "psubw      %%mm1,  %%mm0\n"
            "psllw      $2,     %%mm1\n"
            "psubw      %%mm1,  %%mm0\n"
            "psubw      %%mm3,  %%mm2\n"
            "psllw      $2,     %%mm3\n"
            "psubw      %%mm3,  %%mm2\n"

            "movq       (%%eax,%1,4),%%mm1\n"  /* load pix+3 */
            "movq       %%mm1,  %%mm3\n"
            "punpcklbw  %%mm7,  %%mm1\n"
            "punpckhbw  %%mm7,  %%mm3\n"
            "paddw      %%mm1,  %%mm0\n"
            "paddw      %%mm3,  %%mm2\n"

            "movq       %%mm0,   (%2)\n"
            "movq       %%mm2,  8(%2)\n"


            "addl   $8,         %%eax\n"
            "addl   $8,         %0\n"


            "movd       (%0),   %%mm0\n"    /* load pix-2 */
            "punpcklbw  %%mm7,  %%mm0\n"

            "movd       (%%eax),%%mm1\n"    /* load pix-1 */
            "punpcklbw  %%mm7,  %%mm1\n"
            "psubw      %%mm1,  %%mm0\n"
            "psllw      $2,     %%mm1\n"
            "psubw      %%mm1,  %%mm0\n"

            "movd       (%%eax,%1),%%mm1\n"  /* load pix */
            "punpcklbw  %%mm7,  %%mm1\n"
            "psllw      $2,     %%mm1\n"
            "paddw      %%mm1,  %%mm0\n"
            "psllw      $2,     %%mm1\n"
            "paddw      %%mm1,  %%mm0\n"

            "movd       (%%eax,%1,2),%%mm1\n"  /* load pix+1 */
            "punpcklbw  %%mm7,  %%mm1\n"
            "psllw      $2,     %%mm1\n"
            "paddw      %%mm1,  %%mm0\n"
            "psllw      $2,     %%mm1\n"
            "paddw      %%mm1,  %%mm0\n"

            "movd       (%0,%1,4),%%mm1\n"  /* load pix+2 */
            "punpcklbw  %%mm7,  %%mm1\n"
            "psubw      %%mm1,  %%mm0\n"
            "psllw      $2,     %%mm1\n"
            "psubw      %%mm1,  %%mm0\n"

            "movd       (%%eax,%1,4),%%mm1\n"  /* load pix+3 */
            "punpcklbw  %%mm7,  %%mm1\n"
            "paddw      %%mm1,  %%mm0\n"

            "movq       %%mm0,  16(%2)\n"
            : : "r"(src-2*i_src_stride-2), "r"(i_src_stride), "r"(&tap[0]) : "%eax" );

        /* last one */
        tap[8+4] = x264_tapfilter( &src[-2+8+4], i_src_stride );

        for( x = 0; x < 8; x++ )
        {
            dst[x] = x264_mc_clip1( ( tap[0+x] - 5*tap[1+x] + 20 * tap[2+x] + 20 * tap[3+x] -5*tap[4+x] + tap[5+x] + 512 ) >> 10 );
        }

        src += i_src_stride;
        dst += i_dst_stride;
    }
}

MC_IH( mc_xy10, mmxext, 8, 16, 0 )
MC_IH( mc_xy30, mmxext, 8, 16, 1 )

MC_IV( mc_xy01, mmxext, 8, 16, 0 )
MC_IV( mc_xy03, mmxext, 8, 16, i_src_stride )

MC_HV( mc_xy11, mmxext, 8, 16, 0, 0 )
MC_HV( mc_xy31, mmxext, 8, 16, 1, 0 )
MC_HV( mc_xy13, mmxext, 8, 16, 0, i_src_stride )
MC_HV( mc_xy33, mmxext, 8, 16, 1, i_src_stride )

MC_CH( mc_xy21, mmxext, 8, 16, 0 )
MC_CH( mc_xy23, mmxext, 8, 16, i_src_stride )

MC_CV( mc_xy12, mmxext, 8, 16, 0 )
MC_CV( mc_xy32, mmxext, 8, 16, 1 )

#if 0
/* mc I+H */
static void mc_xy10_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp[8*16];
    mc_hh_w8( src, i_src_stride, tmp, 8, i_height );
    pixel_avg_w8( dst, i_dst_stride, src, i_src_stride, tmp, 8, i_height );
}
static void mc_xy30_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp[8*16];
    mc_hh_w8( src, i_src_stride, tmp, 8, i_height );
    pixel_avg_w8( dst, i_dst_stride, src+1, i_src_stride, tmp, 8, i_height );
}
/* mc I+V */
static void mc_xy01_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp[8*16];
    mc_hv_w8( src, i_src_stride, tmp, 8, i_height );
    pixel_avg_w8( dst, i_dst_stride, src, i_src_stride, tmp, 8, i_height );
}
static void mc_xy03_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp[8*16];
    mc_hv_w8( src, i_src_stride, tmp, 8, i_height );
    pixel_avg_w8( dst, i_dst_stride, src+i_src_stride, i_src_stride, tmp, 8, i_height );
}
/* H+V */
static void mc_xy11_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[8*16];
    uint8_t tmp2[8*16];

    mc_hv_w8( src, i_src_stride, tmp1, 8, i_height );
    mc_hh_w8( src, i_src_stride, tmp2, 8, i_height );
    pixel_avg_w8( dst, i_dst_stride, tmp1, 8, tmp2, 8, i_height );
}
static void mc_xy31_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[8*16];
    uint8_t tmp2[8*16];

    mc_hv_w8( src+1, i_src_stride, tmp1, 8, i_height );
    mc_hh_w8( src,   i_src_stride, tmp2, 8, i_height );
    pixel_avg_w8( dst, i_dst_stride, tmp1, 8, tmp2, 8, i_height );
}
static void mc_xy13_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[8*16];
    uint8_t tmp2[8*16];

    mc_hv_w8( src,              i_src_stride, tmp1, 8, i_height );
    mc_hh_w8( src+i_src_stride, i_src_stride, tmp2, 8, i_height );
    pixel_avg_w8( dst, i_dst_stride, tmp1, 8, tmp2, 8, i_height );
}
static void mc_xy33_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[8*16];
    uint8_t tmp2[8*16];

    mc_hv_w8( src+1,            i_src_stride, tmp1, 8, i_height );
    mc_hh_w8( src+i_src_stride, i_src_stride, tmp2, 8, i_height );
    pixel_avg_w8( dst, i_dst_stride, tmp1, 8, tmp2, 8, i_height );
}
static void mc_xy21_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[8*16];
    uint8_t tmp2[8*16];

    mc_hc_w8( src, i_src_stride, tmp1, 8, i_height );
    mc_hh_w8( src, i_src_stride, tmp2, 8, i_height );
    pixel_avg_w8( dst, i_dst_stride, tmp1, 8, tmp2, 8, i_height );
}
static void mc_xy12_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[8*16];
    uint8_t tmp2[8*16];

    mc_hc_w8( src, i_src_stride, tmp1, 8, i_height );
    mc_hv_w8( src, i_src_stride, tmp2, 8, i_height );
    pixel_avg_w8( dst, i_dst_stride, tmp1, 8, tmp2, 8, i_height );
}
static void mc_xy32_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[8*16];
    uint8_t tmp2[8*16];

    mc_hc_w8( src,   i_src_stride, tmp1, 8, i_height );
    mc_hv_w8( src+1, i_src_stride, tmp2, 8, i_height );
    pixel_avg_w8( dst, i_dst_stride, tmp1, 8, tmp2, 8, i_height );
}
static void mc_xy23_w8( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    uint8_t tmp1[8*16];
    uint8_t tmp2[8*16];

    mc_hc_w8( src,              i_src_stride, tmp1, 8, i_height );
    mc_hh_w8( src+i_src_stride, i_src_stride, tmp2, 8, i_height );
    pixel_avg_w8( dst, i_dst_stride, tmp1, 8, tmp2, 8, i_height );
}
#endif

/*****************************************************************************
 * MC with width == 16 (height <= 16)
 *****************************************************************************/

static inline void mc_hh_w16( uint8_t *src, int i_src, uint8_t *dst, int i_dst, int i_height )
{
    mc_hh_w4( &src[ 0], i_src, &dst[ 0], i_dst, i_height );
    mc_hh_w4( &src[ 4], i_src, &dst[ 4], i_dst, i_height );
    mc_hh_w4( &src[ 8], i_src, &dst[ 8], i_dst, i_height );
    mc_hh_w4( &src[12], i_src, &dst[12], i_dst, i_height );
}
static inline void mc_hv_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    mc_hv_w8( src,     i_src_stride, dst,     i_dst_stride, i_height );
    mc_hv_w8( &src[8], i_src_stride, &dst[8], i_dst_stride, i_height );
}

static inline void mc_hc_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    mc_hc_w8( src,     i_src_stride, dst,     i_dst_stride, i_height );
    mc_hc_w8( &src[8], i_src_stride, &dst[8], i_dst_stride, i_height );
}

/* MMX avg/copy */
MC_IH( mc_xy10, mmxext, 16, 16, 0 )
MC_IH( mc_xy30, mmxext, 16, 16, 1 )

MC_IV( mc_xy01, mmxext, 16, 16, 0 )
MC_IV( mc_xy03, mmxext, 16, 16, i_src_stride )

MC_HV( mc_xy11, mmxext, 16, 16, 0, 0 )
MC_HV( mc_xy31, mmxext, 16, 16, 1, 0 )
MC_HV( mc_xy13, mmxext, 16, 16, 0, i_src_stride )
MC_HV( mc_xy33, mmxext, 16, 16, 1, i_src_stride )

MC_CH( mc_xy21, mmxext, 16, 16, 0 )
MC_CH( mc_xy23, mmxext, 16, 16, i_src_stride )

MC_CV( mc_xy12, mmxext, 16, 16, 0 )
MC_CV( mc_xy32, mmxext, 16, 16, 1 )

/* SSE2 avg/copy */
MC_IH( mc_xy10, sse2, 16, 16, 0 )
MC_IH( mc_xy30, sse2, 16, 16, 1 )

MC_IV( mc_xy01, sse2, 16, 16, 0 )
MC_IV( mc_xy03, sse2, 16, 16, i_src_stride )

MC_HV( mc_xy11, sse2, 16, 16, 0, 0 )
MC_HV( mc_xy31, sse2, 16, 16, 1, 0 )
MC_HV( mc_xy13, sse2, 16, 16, 0, i_src_stride )
MC_HV( mc_xy33, sse2, 16, 16, 1, i_src_stride )

MC_CH( mc_xy21, sse2, 16, 16, 0 )
MC_CH( mc_xy23, sse2, 16, 16, i_src_stride )

MC_CV( mc_xy12, sse2, 16, 16, 0 )
MC_CV( mc_xy32, sse2, 16, 16, 1 )


#if 0
/* mc I+H */
static void mc_xy10_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    DECLARE_ALIGNED(uint8_t, tmp[16*16], 16);
    mc_hh_w16( src, i_src_stride, tmp, 16, i_height );
    pixel_avg_w16( dst, i_dst_stride, src, i_src_stride, tmp, 16, i_height );
}
static void mc_xy30_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    DECLARE_ALIGNED(uint8_t, tmp[16*16], 16);
    mc_hh_w16( src, i_src_stride, tmp, 16, i_height );
    pixel_avg_w16( dst, i_dst_stride, src+1, i_src_stride, tmp, 16, i_height );
}
/* mc I+V */
static void mc_xy01_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    DECLARE_ALIGNED(uint8_t, tmp[16*16], 16);
    mc_hv_w16( src, i_src_stride, tmp, 16, i_height );
    pixel_avg_w16( dst, i_dst_stride, src, i_src_stride, tmp, 16, i_height );
}
static void mc_xy03_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    DECLARE_ALIGNED(uint8_t, tmp[16*16], 16);
    mc_hv_w16( src, i_src_stride, tmp, 16, i_height );
    pixel_avg_w16( dst, i_dst_stride, src+i_src_stride, i_src_stride, tmp, 16, i_height );
}
/* H+V */
static void mc_xy11_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    DECLARE_ALIGNED(uint8_t, tmp1[16*16], 16);
    DECLARE_ALIGNED(uint8_t, tmp2[16*16], 16);

    mc_hv_w16( src, i_src_stride, tmp1, 16, i_height );
    mc_hh_w16( src, i_src_stride, tmp2, 16, i_height );
    pixel_avg_w16( dst, i_dst_stride, tmp1, 16, tmp2, 16, i_height );
}
static void mc_xy31_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    DECLARE_ALIGNED(uint8_t, tmp1[16*16], 16);
    DECLARE_ALIGNED(uint8_t, tmp2[16*16], 16);

    mc_hv_w16( src+1, i_src_stride, tmp1, 16, i_height );
    mc_hh_w16( src,   i_src_stride, tmp2, 16, i_height );
    pixel_avg_w16( dst, i_dst_stride, tmp1, 16, tmp2, 16, i_height );
}
static void mc_xy13_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    DECLARE_ALIGNED(uint8_t, tmp1[16*16], 16);
    DECLARE_ALIGNED(uint8_t, tmp2[16*16], 16);

    mc_hv_w16( src,              i_src_stride, tmp1, 16, i_height );
    mc_hh_w16( src+i_src_stride, i_src_stride, tmp2, 16, i_height );
    pixel_avg_w16( dst, i_dst_stride, tmp1, 16, tmp2, 16, i_height );
}
static void mc_xy33_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    DECLARE_ALIGNED(uint8_t, tmp1[16*16], 16);
    DECLARE_ALIGNED(uint8_t, tmp2[16*16], 16);

    mc_hv_w16( src+1,            i_src_stride, tmp1, 16, i_height );
    mc_hh_w16( src+i_src_stride, i_src_stride, tmp2, 16, i_height );
    pixel_avg_w16( dst, i_dst_stride, tmp1, 16, tmp2, 16, i_height );
}
static void mc_xy21_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    DECLARE_ALIGNED(uint8_t, tmp1[16*16], 16);
    DECLARE_ALIGNED(uint8_t, tmp2[16*16], 16);

    mc_hc_w16( src, i_src_stride, tmp1, 16, i_height );
    mc_hh_w16( src, i_src_stride, tmp2, 16, i_height );
    pixel_avg_w16( dst, i_dst_stride, tmp1, 16, tmp2, 16, i_height );
}
static void mc_xy12_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    DECLARE_ALIGNED(uint8_t, tmp1[16*16], 16);
    DECLARE_ALIGNED(uint8_t, tmp2[16*16], 16);

    mc_hc_w16( src, i_src_stride, tmp1, 16, i_height );
    mc_hv_w16( src, i_src_stride, tmp2, 16, i_height );
    pixel_avg_w16( dst, i_dst_stride, tmp1, 16, tmp2, 16, i_height );
}
static void mc_xy32_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    DECLARE_ALIGNED(uint8_t, tmp1[16*16], 16);
    DECLARE_ALIGNED(uint8_t, tmp2[16*16], 16);

    mc_hc_w16( src,   i_src_stride, tmp1, 16, i_height );
    mc_hv_w16( src+1, i_src_stride, tmp2, 16, i_height );
    pixel_avg_w16( dst, i_dst_stride, tmp1, 16, tmp2, 16, i_height );
}
static void mc_xy23_w16( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_height )
{
    DECLARE_ALIGNED(uint8_t, tmp1[16*16], 16);
    DECLARE_ALIGNED(uint8_t, tmp2[16*16], 16);

    mc_hc_w16( src,              i_src_stride, tmp1, 16, i_height );
    mc_hh_w16( src+i_src_stride, i_src_stride, tmp2, 16, i_height );
    pixel_avg_w16( dst, i_dst_stride, tmp1, 16, tmp2, 16, i_height );
}
#endif

#define MOTION_COMPENSATION_LUMA \
    src += (mvy >> 2) * i_src_stride + (mvx >> 2);  \
    if( i_width == 4 )                              \
    {                                               \
        pf_mc[0][mvy&0x03][mvx&0x03]( src, i_src_stride, dst, i_dst_stride, i_height ); \
    }                                               \
    else if( i_width == 8 )                         \
    {                                               \
        pf_mc[1][mvy&0x03][mvx&0x03]( src, i_src_stride, dst, i_dst_stride, i_height ); \
    }                                               \
    else if( i_width == 16 )                        \
    {                                               \
        pf_mc[2][mvy&0x03][mvx&0x03]( src, i_src_stride, dst, i_dst_stride, i_height ); \
    }                                               \
    else                                            \
    {                                               \
        fprintf( stderr, "Error: motion_compensation_luma called with invalid width" ); \
    }

static void motion_compensation_luma_mmxext( uint8_t *src, int i_src_stride,
                                             uint8_t *dst, int i_dst_stride,
                                             int mvx,int mvy,
                                             int i_width, int i_height )
{
    static const pf_mc_t pf_mc[3][4][4] =    /*XXX [dqy][dqx] */
    {
        {
            { x264_mc_copy_w4_mmxext,   mc_xy10_w4_mmxext,    mc_hh_w4,             mc_xy30_w4_mmxext },
            { mc_xy01_w4_mmxext,        mc_xy11_w4_mmxext,    mc_xy21_w4_mmxext,    mc_xy31_w4_mmxext },
            { mc_hv_w4,                 mc_xy12_w4_mmxext,    mc_hc_w4,             mc_xy32_w4_mmxext },
            { mc_xy03_w4_mmxext,        mc_xy13_w4_mmxext,    mc_xy23_w4_mmxext,    mc_xy33_w4_mmxext },
        },
        {
            { x264_mc_copy_w8_mmxext,   mc_xy10_w8_mmxext,    mc_hh_w8,             mc_xy30_w8_mmxext },
            { mc_xy01_w8_mmxext,        mc_xy11_w8_mmxext,    mc_xy21_w8_mmxext,    mc_xy31_w8_mmxext },
            { mc_hv_w8,                 mc_xy12_w8_mmxext,    mc_hc_w8,             mc_xy32_w8_mmxext },
            { mc_xy03_w8_mmxext,        mc_xy13_w8_mmxext,    mc_xy23_w8_mmxext,    mc_xy33_w8_mmxext },
        },
        {
            { x264_mc_copy_w16_mmxext,   mc_xy10_w16_mmxext,    mc_hh_w16,             mc_xy30_w16_mmxext },
            { mc_xy01_w16_mmxext,        mc_xy11_w16_mmxext,    mc_xy21_w16_mmxext,    mc_xy31_w16_mmxext },
            { mc_hv_w16,                 mc_xy12_w16_mmxext,    mc_hc_w16,             mc_xy32_w16_mmxext },
            { mc_xy03_w16_mmxext,        mc_xy13_w16_mmxext,    mc_xy23_w16_mmxext,    mc_xy33_w16_mmxext },
        }
    };

    MOTION_COMPENSATION_LUMA
}

static void motion_compensation_luma_sse2( uint8_t *src, int i_src_stride,
                                           uint8_t *dst, int i_dst_stride,
                                           int mvx,int mvy,
                                           int i_width, int i_height )
{
    static const pf_mc_t pf_mc[3][4][4] =    /*XXX [dqy][dqx] */
    {
        {
            { x264_mc_copy_w4_mmxext,   mc_xy10_w4_mmxext,    mc_hh_w4,             mc_xy30_w4_mmxext },
            { mc_xy01_w4_mmxext,        mc_xy11_w4_mmxext,    mc_xy21_w4_mmxext,    mc_xy31_w4_mmxext },
            { mc_hv_w4,                 mc_xy12_w4_mmxext,    mc_hc_w4,             mc_xy32_w4_mmxext },
            { mc_xy03_w4_mmxext,        mc_xy13_w4_mmxext,    mc_xy23_w4_mmxext,    mc_xy33_w4_mmxext },
        },
        {
            { x264_mc_copy_w8_mmxext,   mc_xy10_w8_mmxext,    mc_hh_w8,             mc_xy30_w8_mmxext },
            { mc_xy01_w8_mmxext,        mc_xy11_w8_mmxext,    mc_xy21_w8_mmxext,    mc_xy31_w8_mmxext },
            { mc_hv_w8,                 mc_xy12_w8_mmxext,    mc_hc_w8,             mc_xy32_w8_mmxext },
            { mc_xy03_w8_mmxext,        mc_xy13_w8_mmxext,    mc_xy23_w8_mmxext,    mc_xy33_w8_mmxext },
        },
        {
            { x264_mc_copy_w16_sse2,   mc_xy10_w16_sse2,    mc_hh_w16,             mc_xy30_w16_sse2 },
            { mc_xy01_w16_sse2,        mc_xy11_w16_sse2,    mc_xy21_w16_sse2,    mc_xy31_w16_sse2 },
            { mc_hv_w16,                 mc_xy12_w16_sse2,    mc_hc_w16,             mc_xy32_w16_sse2 },
            { mc_xy03_w16_sse2,        mc_xy13_w16_sse2,    mc_xy23_w16_sse2,    mc_xy33_w16_sse2 },
        }
    };
    MOTION_COMPENSATION_LUMA
}

#endif

void mc_luma_mmx( uint8_t *src[4], int i_src_stride,
              uint8_t *dst,    int i_dst_stride,
              int mvx,int mvy,
              int i_width, int i_height )
{
    uint8_t *src1, *src2;

    int correction = (mvx&1) && (mvy&1) && ((mvx&2) ^ (mvy&2));
    int hpel1x = mvx>>1;
    int hpel1y = (mvy+1-correction)>>1;
    int filter1 = (hpel1x & 1) + ( (hpel1y & 1) << 1 );

    src1 = src[filter1] + (hpel1y >> 1) * i_src_stride + (hpel1x >> 1);

    if ( (mvx|mvy) & 1 ) /* qpel interpolation needed */
    {
        int hpel2x = (mvx+1)>>1;
        int hpel2y = (mvy+correction)>>1;
        int filter2 = (hpel2x & 1) + ( (hpel2y & 1) <<1 );

        src2 = src[filter2] + (hpel2y >> 1) * i_src_stride + (hpel2x >> 1);

        switch(i_width) {
        case 4:
            x264_pixel_avg_w4_mmxext( dst, i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
            break;
        case 8:
            x264_pixel_avg_w8_mmxext( dst, i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
            break;
        case 16:
        default:
            x264_pixel_avg_w16_mmxext(dst, i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
        }
    }
    else
    {
        switch(i_width) {
        case 4:
            x264_mc_copy_w4_mmxext( src1, i_src_stride, dst, i_dst_stride, i_height );
            break;
        case 8:
            x264_mc_copy_w8_mmxext( src1, i_src_stride, dst, i_dst_stride, i_height );
            break;
        case 16:
            x264_mc_copy_w16_mmxext( src1, i_src_stride, dst, i_dst_stride, i_height );
            break;
        }
    }
}

uint8_t *get_ref_mmx( uint8_t *src[4], int i_src_stride,
                      uint8_t *dst,   int *i_dst_stride,
                      int mvx,int mvy,
                      int i_width, int i_height )
{
    uint8_t *src1, *src2;

    int correction = (mvx&1) && (mvy&1) && ((mvx&2) ^ (mvy&2));
    int hpel1x = mvx>>1;
    int hpel1y = (mvy+1-correction)>>1;
    int filter1 = (hpel1x & 1) + ( (hpel1y & 1) << 1 );

    src1 = src[filter1] + (hpel1y >> 1) * i_src_stride + (hpel1x >> 1);

    if ( (mvx|mvy) & 1 ) /* qpel interpolation needed */
    {
        int hpel2x = (mvx+1)>>1;
        int hpel2y = (mvy+correction)>>1;
        int filter2 = (hpel2x & 1) + ( (hpel2y & 1) <<1 );

        src2 = src[filter2] + (hpel2y >> 1) * i_src_stride + (hpel2x >> 1);
    
        switch(i_width) {
        case 4:
            x264_pixel_avg_w4_mmxext( dst, *i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
            break;
        case 8:
            x264_pixel_avg_w8_mmxext( dst, *i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
            break;
        case 16:
        default:
            x264_pixel_avg_w16_mmxext(dst, *i_dst_stride, src1, i_src_stride,
                          src2, i_src_stride, i_height );
        }
        return dst;

    }
    else
    {
        *i_dst_stride = i_src_stride;
        return src1;
    }
}


void x264_mc_mmxext_init( x264_mc_functions_t *pf )
{
    pf->mc_luma   = mc_luma_mmx;
    pf->get_ref   = get_ref_mmx;
}
void x264_mc_sse2_init( x264_mc_functions_t *pf )
{
    /* todo: use sse2 */
    pf->mc_luma   = mc_luma_mmx;
    pf->get_ref   = get_ref_mmx;
}

#if 0
void get_funcs_mmx(pf_mc_t *int_h, pf_mc_t *int_v, pf_mc_t *int_hv)
{
    *int_h = mc_hh_w16;
    *int_v = mc_hv_w16;
    *int_hv = mc_hc_w16;
}

void get_funcs_sse2(pf_mc_t *int_h, pf_mc_t *int_v, pf_mc_t *int_hv)
{
    *int_h = mc_hh_w16;
    *int_v = mc_hv_w16;
    *int_hv = mc_hc_w16;
}
#endif
