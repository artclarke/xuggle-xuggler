/*****************************************************************************
 * dct.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id$
 *
 * Authors: Eric Petit <titer@m0k.org>
 *          Guillaume Poirier <gpoirier@mplayerhq.hu>
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

#ifdef SYS_LINUX
#include <altivec.h>
#endif

#include "common/common.h"
#include "ppccommon.h"

#define VEC_DCT(a0,a1,a2,a3,b0,b1,b2,b3) \
    b1 = vec_add( a0, a3 );              \
    b3 = vec_add( a1, a2 );              \
    b0 = vec_add( b1, b3 );              \
    b2 = vec_sub( b1, b3 );              \
    a0 = vec_sub( a0, a3 );              \
    a1 = vec_sub( a1, a2 );              \
    b1 = vec_add( a0, a0 );              \
    b1 = vec_add( b1, a1 );              \
    b3 = vec_sub( a0, a1 );              \
    b3 = vec_sub( b3, a1 )

void x264_sub4x4_dct_altivec( int16_t dct[4][4],
        uint8_t *pix1, uint8_t *pix2 )
{
    PREP_DIFF_8BYTEALIGNED;
    vec_s16_t dct0v, dct1v, dct2v, dct3v;
    vec_s16_t tmp0v, tmp1v, tmp2v, tmp3v;

    vec_u8_t permHighv;

    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 4, dct0v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 4, dct1v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 4, dct2v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 4, dct3v );
    VEC_DCT( dct0v, dct1v, dct2v, dct3v, tmp0v, tmp1v, tmp2v, tmp3v );
    VEC_TRANSPOSE_4( tmp0v, tmp1v, tmp2v, tmp3v,
                     dct0v, dct1v, dct2v, dct3v );
    permHighv = (vec_u8_t) CV(0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17);
    VEC_DCT( dct0v, dct1v, dct2v, dct3v, tmp0v, tmp1v, tmp2v, tmp3v );

    vec_st(vec_perm(tmp0v, tmp1v, permHighv), 0, dct);
    vec_st(vec_perm(tmp2v, tmp3v, permHighv), 16, dct);
}

void x264_sub8x8_dct_altivec( int16_t dct[4][4][4],
        uint8_t *pix1, uint8_t *pix2 )
{
    PREP_DIFF_8BYTEALIGNED;
    vec_s16_t dct0v, dct1v, dct2v, dct3v, dct4v, dct5v, dct6v, dct7v;
    vec_s16_t tmp0v, tmp1v, tmp2v, tmp3v, tmp4v, tmp5v, tmp6v, tmp7v;

    vec_u8_t permHighv, permLowv;

    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct0v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct1v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct2v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct3v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct4v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct5v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct6v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct7v );
    VEC_DCT( dct0v, dct1v, dct2v, dct3v, tmp0v, tmp1v, tmp2v, tmp3v );
    VEC_DCT( dct4v, dct5v, dct6v, dct7v, tmp4v, tmp5v, tmp6v, tmp7v );
    VEC_TRANSPOSE_8( tmp0v, tmp1v, tmp2v, tmp3v,
                     tmp4v, tmp5v, tmp6v, tmp7v,
                     dct0v, dct1v, dct2v, dct3v,
                     dct4v, dct5v, dct6v, dct7v );

    permHighv = (vec_u8_t) CV(0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17);
    permLowv  = (vec_u8_t) CV(0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F);

    VEC_DCT( dct0v, dct1v, dct2v, dct3v, tmp0v, tmp1v, tmp2v, tmp3v );
    VEC_DCT( dct4v, dct5v, dct6v, dct7v, tmp4v, tmp5v, tmp6v, tmp7v );

    vec_st(vec_perm(tmp0v, tmp1v, permHighv), 0, dct);
    vec_st(vec_perm(tmp2v, tmp3v, permHighv), 16, dct);
    vec_st(vec_perm(tmp4v, tmp5v, permHighv), 32, dct);
    vec_st(vec_perm(tmp6v, tmp7v, permHighv), 48, dct);
    vec_st(vec_perm(tmp0v, tmp1v, permLowv),  64, dct);
    vec_st(vec_perm(tmp2v, tmp3v, permLowv), 80, dct);
    vec_st(vec_perm(tmp4v, tmp5v, permLowv), 96, dct);
    vec_st(vec_perm(tmp6v, tmp7v, permLowv), 112, dct);
}

void x264_sub16x16_dct_altivec( int16_t dct[16][4][4],
        uint8_t *pix1, uint8_t *pix2 ) 
{
    x264_sub8x8_dct_altivec( &dct[ 0], &pix1[0], &pix2[0] );
    x264_sub8x8_dct_altivec( &dct[ 4], &pix1[8], &pix2[8] );
    x264_sub8x8_dct_altivec( &dct[ 8], &pix1[8*FENC_STRIDE+0], &pix2[8*FDEC_STRIDE+0] );
    x264_sub8x8_dct_altivec( &dct[12], &pix1[8*FENC_STRIDE+8], &pix2[8*FDEC_STRIDE+8] );
}

/***************************************************************************
 * 8x8 transform:
 ***************************************************************************/

/* DCT8_1D unrolled by 8 in Altivec */
#define DCT8_1D_ALTIVEC( dct0v, dct1v, dct2v, dct3v, dct4v, dct5v, dct6v, dct7v ) \
{ \
    /* int s07 = SRC(0) + SRC(7);         */ \
    vec_s16_t s07v = vec_add( dct0v, dct7v); \
    /* int s16 = SRC(1) + SRC(6);         */ \
    vec_s16_t s16v = vec_add( dct1v, dct6v); \
    /* int s25 = SRC(2) + SRC(5);         */ \
    vec_s16_t s25v = vec_add( dct2v, dct5v); \
    /* int s34 = SRC(3) + SRC(4);         */ \
    vec_s16_t s34v = vec_add( dct3v, dct4v); \
\
    /* int a0 = s07 + s34;                */ \
    vec_s16_t a0v = vec_add(s07v, s34v);     \
    /* int a1 = s16 + s25;                */ \
    vec_s16_t a1v = vec_add(s16v, s25v);     \
    /* int a2 = s07 - s34;                */ \
    vec_s16_t a2v = vec_sub(s07v, s34v);     \
    /* int a3 = s16 - s25;                */ \
    vec_s16_t a3v = vec_sub(s16v, s25v);     \
\
    /* int d07 = SRC(0) - SRC(7);         */ \
    vec_s16_t d07v = vec_sub( dct0v, dct7v); \
    /* int d16 = SRC(1) - SRC(6);         */ \
    vec_s16_t d16v = vec_sub( dct1v, dct6v); \
    /* int d25 = SRC(2) - SRC(5);         */ \
    vec_s16_t d25v = vec_sub( dct2v, dct5v); \
    /* int d34 = SRC(3) - SRC(4);         */ \
    vec_s16_t d34v = vec_sub( dct3v, dct4v); \
\
    /* int a4 = d16 + d25 + (d07 + (d07>>1)); */ \
    vec_s16_t a4v = vec_add( vec_add(d16v, d25v), vec_add(d07v, vec_sra(d07v, onev)) );\
    /* int a5 = d07 - d34 - (d25 + (d25>>1)); */ \
    vec_s16_t a5v = vec_sub( vec_sub(d07v, d34v), vec_add(d25v, vec_sra(d25v, onev)) );\
    /* int a6 = d07 + d34 - (d16 + (d16>>1)); */ \
    vec_s16_t a6v = vec_sub( vec_add(d07v, d34v), vec_add(d16v, vec_sra(d16v, onev)) );\
    /* int a7 = d16 - d25 + (d34 + (d34>>1)); */ \
    vec_s16_t a7v = vec_add( vec_sub(d16v, d25v), vec_add(d34v, vec_sra(d34v, onev)) );\
\
    /* DST(0) =  a0 + a1;                    */ \
    dct0v = vec_add( a0v, a1v );                \
    /* DST(1) =  a4 + (a7>>2);               */ \
    dct1v = vec_add( a4v, vec_sra(a7v, twov) ); \
    /* DST(2) =  a2 + (a3>>1);               */ \
    dct2v = vec_add( a2v, vec_sra(a3v, onev) ); \
    /* DST(3) =  a5 + (a6>>2);               */ \
    dct3v = vec_add( a5v, vec_sra(a6v, twov) ); \
    /* DST(4) =  a0 - a1;                    */ \
    dct4v = vec_sub( a0v, a1v );                \
    /* DST(5) =  a6 - (a5>>2);               */ \
    dct5v = vec_sub( a6v, vec_sra(a5v, twov) ); \
    /* DST(6) = (a2>>1) - a3 ;               */ \
    dct6v = vec_sub( vec_sra(a2v, onev), a3v ); \
    /* DST(7) = (a4>>2) - a7 ;               */ \
    dct7v = vec_sub( vec_sra(a4v, twov), a7v ); \
}


void x264_sub8x8_dct8_altivec( int16_t dct[8][8], uint8_t *pix1, uint8_t *pix2 )
{
    vec_u16_t onev = vec_splat_u16(1);
    vec_u16_t twov = vec_add( onev, onev );

    PREP_DIFF_8BYTEALIGNED;

    vec_s16_t dct0v, dct1v, dct2v, dct3v,
              dct4v, dct5v, dct6v, dct7v;

    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct0v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct1v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct2v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct3v );

    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct4v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct5v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct6v );
    VEC_DIFF_H_8BYTE_ALIGNED( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct7v );

    DCT8_1D_ALTIVEC( dct0v, dct1v, dct2v, dct3v,
                     dct4v, dct5v, dct6v, dct7v );

    vec_s16_t dct_tr0v, dct_tr1v, dct_tr2v, dct_tr3v,
        dct_tr4v, dct_tr5v, dct_tr6v, dct_tr7v;

    VEC_TRANSPOSE_8(dct0v, dct1v, dct2v, dct3v,
                    dct4v, dct5v, dct6v, dct7v,
                    dct_tr0v, dct_tr1v, dct_tr2v, dct_tr3v,
                    dct_tr4v, dct_tr5v, dct_tr6v, dct_tr7v );

    DCT8_1D_ALTIVEC( dct_tr0v, dct_tr1v, dct_tr2v, dct_tr3v,
                     dct_tr4v, dct_tr5v, dct_tr6v, dct_tr7v );

    vec_st( dct_tr0v,  0,  (signed short *)dct );
    vec_st( dct_tr1v, 16,  (signed short *)dct );
    vec_st( dct_tr2v, 32,  (signed short *)dct );
    vec_st( dct_tr3v, 48,  (signed short *)dct );
    
    vec_st( dct_tr4v, 64,  (signed short *)dct );
    vec_st( dct_tr5v, 80,  (signed short *)dct );
    vec_st( dct_tr6v, 96,  (signed short *)dct );
    vec_st( dct_tr7v, 112, (signed short *)dct );
}

void x264_sub16x16_dct8_altivec( int16_t dct[4][8][8], uint8_t *pix1, uint8_t *pix2 )
{
    x264_sub8x8_dct8_altivec( dct[0], &pix1[0],               &pix2[0] );
    x264_sub8x8_dct8_altivec( dct[1], &pix1[8],               &pix2[8] );
    x264_sub8x8_dct8_altivec( dct[2], &pix1[8*FENC_STRIDE+0], &pix2[8*FDEC_STRIDE+0] );
    x264_sub8x8_dct8_altivec( dct[3], &pix1[8*FENC_STRIDE+8], &pix2[8*FDEC_STRIDE+8] );
}

