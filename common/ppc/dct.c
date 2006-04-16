/*****************************************************************************
 * dct.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id$
 *
 * Authors: Eric Petit <titer@m0k.org>
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
    PREP_DIFF;
    PREP_STORE8;
    vec_s16_t dct0v, dct1v, dct2v, dct3v;
    vec_s16_t tmp0v, tmp1v, tmp2v, tmp3v;

    VEC_DIFF_H( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 4, dct0v );
    VEC_DIFF_H( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 4, dct1v );
    VEC_DIFF_H( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 4, dct2v );
    VEC_DIFF_H( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 4, dct3v );
    VEC_DCT( dct0v, dct1v, dct2v, dct3v, tmp0v, tmp1v, tmp2v, tmp3v );
    VEC_TRANSPOSE_4( tmp0v, tmp1v, tmp2v, tmp3v,
                     dct0v, dct1v, dct2v, dct3v );
    VEC_DCT( dct0v, dct1v, dct2v, dct3v, tmp0v, tmp1v, tmp2v, tmp3v );
    VEC_STORE8( tmp0v, dct[0] );
    VEC_STORE8( tmp1v, dct[1] );
    VEC_STORE8( tmp2v, dct[2] );
    VEC_STORE8( tmp3v, dct[3] );
}

void x264_sub8x8_dct_altivec( int16_t dct[4][4][4],
        uint8_t *pix1, uint8_t *pix2 )
{
    PREP_DIFF;
    PREP_STORE8_HL;
    vec_s16_t dct0v, dct1v, dct2v, dct3v, dct4v, dct5v, dct6v, dct7v;
    vec_s16_t tmp0v, tmp1v, tmp2v, tmp3v, tmp4v, tmp5v, tmp6v, tmp7v;

    VEC_DIFF_H( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct0v );
    VEC_DIFF_H( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct1v );
    VEC_DIFF_H( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct2v );
    VEC_DIFF_H( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct3v );
    VEC_DIFF_H( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct4v );
    VEC_DIFF_H( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct5v );
    VEC_DIFF_H( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct6v );
    VEC_DIFF_H( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, 8, dct7v );
    VEC_DCT( dct0v, dct1v, dct2v, dct3v, tmp0v, tmp1v, tmp2v, tmp3v );
    VEC_DCT( dct4v, dct5v, dct6v, dct7v, tmp4v, tmp5v, tmp6v, tmp7v );
    VEC_TRANSPOSE_8( tmp0v, tmp1v, tmp2v, tmp3v,
                     tmp4v, tmp5v, tmp6v, tmp7v,
                     dct0v, dct1v, dct2v, dct3v,
                     dct4v, dct5v, dct6v, dct7v );
    VEC_DCT( dct0v, dct1v, dct2v, dct3v, tmp0v, tmp1v, tmp2v, tmp3v );
    VEC_STORE8_H( tmp0v, dct[0][0] );
    VEC_STORE8_H( tmp1v, dct[0][1] );
    VEC_STORE8_H( tmp2v, dct[0][2] );
    VEC_STORE8_H( tmp3v, dct[0][3] );
    VEC_STORE8_L( tmp0v, dct[2][0] );
    VEC_STORE8_L( tmp1v, dct[2][1] );
    VEC_STORE8_L( tmp2v, dct[2][2] );
    VEC_STORE8_L( tmp3v, dct[2][3] );
    VEC_DCT( dct4v, dct5v, dct6v, dct7v, tmp4v, tmp5v, tmp6v, tmp7v );
    VEC_STORE8_H( tmp4v, dct[1][0] );
    VEC_STORE8_H( tmp5v, dct[1][1] );
    VEC_STORE8_H( tmp6v, dct[1][2] );
    VEC_STORE8_H( tmp7v, dct[1][3] );
    VEC_STORE8_L( tmp4v, dct[3][0] );
    VEC_STORE8_L( tmp5v, dct[3][1] );
    VEC_STORE8_L( tmp6v, dct[3][2] );
    VEC_STORE8_L( tmp7v, dct[3][3] );
}
    
void x264_sub16x16_dct_altivec( int16_t dct[16][4][4],
        uint8_t *pix1, uint8_t *pix2 ) 
{
    PREP_DIFF;
    PREP_STORE8_HL;
    vec_s16_t dcth0v, dcth1v, dcth2v, dcth3v,
              dcth4v, dcth5v, dcth6v, dcth7v,
              dctl0v, dctl1v, dctl2v, dctl3v,
              dctl4v, dctl5v, dctl6v, dctl7v;
    vec_s16_t temp0v, temp1v, temp2v, temp3v,
              temp4v, temp5v, temp6v, temp7v;

    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth0v, dctl0v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth1v, dctl1v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth2v, dctl2v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth3v, dctl3v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth4v, dctl4v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth5v, dctl5v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth6v, dctl6v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth7v, dctl7v );

    VEC_DCT( dcth0v, dcth1v, dcth2v, dcth3v,
             temp0v, temp1v, temp2v, temp3v );
    VEC_DCT( dcth4v, dcth5v, dcth6v, dcth7v,
             temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     dcth0v, dcth1v, dcth2v, dcth3v,
                     dcth4v, dcth5v, dcth6v, dcth7v );
    VEC_DCT( dcth0v, dcth1v, dcth2v, dcth3v,
             temp0v, temp1v, temp2v, temp3v );
    VEC_STORE8_H( temp0v, dct[0][0] );
    VEC_STORE8_H( temp1v, dct[0][1] );
    VEC_STORE8_H( temp2v, dct[0][2] );
    VEC_STORE8_H( temp3v, dct[0][3] );
    VEC_STORE8_L( temp0v, dct[2][0] );
    VEC_STORE8_L( temp1v, dct[2][1] );
    VEC_STORE8_L( temp2v, dct[2][2] );
    VEC_STORE8_L( temp3v, dct[2][3] );
    VEC_DCT( dcth4v, dcth5v, dcth6v, dcth7v,
             temp4v, temp5v, temp6v, temp7v );
    VEC_STORE8_H( temp4v, dct[1][0] );
    VEC_STORE8_H( temp5v, dct[1][1] );
    VEC_STORE8_H( temp6v, dct[1][2] );
    VEC_STORE8_H( temp7v, dct[1][3] );
    VEC_STORE8_L( temp4v, dct[3][0] );
    VEC_STORE8_L( temp5v, dct[3][1] );
    VEC_STORE8_L( temp6v, dct[3][2] );
    VEC_STORE8_L( temp7v, dct[3][3] );

    VEC_DCT( dctl0v, dctl1v, dctl2v, dctl3v,
             temp0v, temp1v, temp2v, temp3v );
    VEC_DCT( dctl4v, dctl5v, dctl6v, dctl7v,
             temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     dctl0v, dctl1v, dctl2v, dctl3v,
                     dctl4v, dctl5v, dctl6v, dctl7v );
    VEC_DCT( dctl0v, dctl1v, dctl2v, dctl3v,
             temp0v, temp1v, temp2v, temp3v );
    VEC_STORE8_H( temp0v, dct[4][0] );
    VEC_STORE8_H( temp1v, dct[4][1] );
    VEC_STORE8_H( temp2v, dct[4][2] );
    VEC_STORE8_H( temp3v, dct[4][3] );
    VEC_STORE8_L( temp0v, dct[6][0] );
    VEC_STORE8_L( temp1v, dct[6][1] );
    VEC_STORE8_L( temp2v, dct[6][2] );
    VEC_STORE8_L( temp3v, dct[6][3] );
    VEC_DCT( dctl4v, dctl5v, dctl6v, dctl7v,
             temp4v, temp5v, temp6v, temp7v );
    VEC_STORE8_H( temp4v, dct[5][0] );
    VEC_STORE8_H( temp5v, dct[5][1] );
    VEC_STORE8_H( temp6v, dct[5][2] );
    VEC_STORE8_H( temp7v, dct[5][3] );
    VEC_STORE8_L( temp4v, dct[7][0] );
    VEC_STORE8_L( temp5v, dct[7][1] );
    VEC_STORE8_L( temp6v, dct[7][2] );
    VEC_STORE8_L( temp7v, dct[7][3] );

    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth0v, dctl0v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth1v, dctl1v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth2v, dctl2v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth3v, dctl3v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth4v, dctl4v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth5v, dctl5v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth6v, dctl6v );
    VEC_DIFF_HL( pix1, FENC_STRIDE, pix2, FDEC_STRIDE, dcth7v, dctl7v );

    VEC_DCT( dcth0v, dcth1v, dcth2v, dcth3v,
             temp0v, temp1v, temp2v, temp3v );
    VEC_DCT( dcth4v, dcth5v, dcth6v, dcth7v,
             temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     dcth0v, dcth1v, dcth2v, dcth3v,
                     dcth4v, dcth5v, dcth6v, dcth7v );
    VEC_DCT( dcth0v, dcth1v, dcth2v, dcth3v,
             temp0v, temp1v, temp2v, temp3v );
    VEC_STORE8_H( temp0v, dct[8][0] );
    VEC_STORE8_H( temp1v, dct[8][1] );
    VEC_STORE8_H( temp2v, dct[8][2] );
    VEC_STORE8_H( temp3v, dct[8][3] );
    VEC_STORE8_L( temp0v, dct[10][0] );
    VEC_STORE8_L( temp1v, dct[10][1] );
    VEC_STORE8_L( temp2v, dct[10][2] );
    VEC_STORE8_L( temp3v, dct[10][3] );
    VEC_DCT( dcth4v, dcth5v, dcth6v, dcth7v,
             temp4v, temp5v, temp6v, temp7v );
    VEC_STORE8_H( temp4v, dct[9][0] );
    VEC_STORE8_H( temp5v, dct[9][1] );
    VEC_STORE8_H( temp6v, dct[9][2] );
    VEC_STORE8_H( temp7v, dct[9][3] );
    VEC_STORE8_L( temp4v, dct[11][0] );
    VEC_STORE8_L( temp5v, dct[11][1] );
    VEC_STORE8_L( temp6v, dct[11][2] );
    VEC_STORE8_L( temp7v, dct[11][3] );

    VEC_DCT( dctl0v, dctl1v, dctl2v, dctl3v,
             temp0v, temp1v, temp2v, temp3v );
    VEC_DCT( dctl4v, dctl5v, dctl6v, dctl7v,
             temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     dctl0v, dctl1v, dctl2v, dctl3v,
                     dctl4v, dctl5v, dctl6v, dctl7v );
    VEC_DCT( dctl0v, dctl1v, dctl2v, dctl3v,
             temp0v, temp1v, temp2v, temp3v );
    VEC_STORE8_H( temp0v, dct[12][0] );
    VEC_STORE8_H( temp1v, dct[12][1] );
    VEC_STORE8_H( temp2v, dct[12][2] );
    VEC_STORE8_H( temp3v, dct[12][3] );
    VEC_STORE8_L( temp0v, dct[14][0] );
    VEC_STORE8_L( temp1v, dct[14][1] );
    VEC_STORE8_L( temp2v, dct[14][2] );
    VEC_STORE8_L( temp3v, dct[14][3] );
    VEC_DCT( dctl4v, dctl5v, dctl6v, dctl7v,
             temp4v, temp5v, temp6v, temp7v );
    VEC_STORE8_H( temp4v, dct[13][0] );
    VEC_STORE8_H( temp5v, dct[13][1] );
    VEC_STORE8_H( temp6v, dct[13][2] );
    VEC_STORE8_H( temp7v, dct[13][3] );
    VEC_STORE8_L( temp4v, dct[15][0] );
    VEC_STORE8_L( temp5v, dct[15][1] );
    VEC_STORE8_L( temp6v, dct[15][2] );
    VEC_STORE8_L( temp7v, dct[15][3] );
}
