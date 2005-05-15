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
        uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    PREP_DIFF;
    PREP_STORE8;
    vec_s16_t dct0v, dct1v, dct2v, dct3v;
    vec_s16_t tmp0v, tmp1v, tmp2v, tmp3v;

    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, dct0v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, dct1v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, dct2v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 4, dct3v );
    VEC_DCT( dct0v, dct1v, dct2v, dct3v, tmp0v, tmp1v, tmp2v, tmp3v );
    VEC_TRANSPOSE_4( tmp0v, tmp1v, tmp2v, tmp3v,
                     dct0v, dct1v, dct2v, dct3v );
    VEC_DCT( dct0v, dct1v, dct2v, dct3v, tmp0v, tmp1v, tmp2v, tmp3v );
    VEC_TRANSPOSE_4( tmp0v, tmp1v, tmp2v, tmp3v,
                     dct0v, dct1v, dct2v, dct3v );
    VEC_STORE8( dct0v, dct[0] );
    VEC_STORE8( dct1v, dct[1] );
    VEC_STORE8( dct2v, dct[2] );
    VEC_STORE8( dct3v, dct[3] );
}

void x264_sub8x8_dct_altivec( int16_t dct[4][4][4],
        uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    PREP_DIFF;
    PREP_STORE8_HL;
    vec_s16_t dct0v, dct1v, dct2v, dct3v, dct4v, dct5v, dct6v, dct7v;
    vec_s16_t tmp0v, tmp1v, tmp2v, tmp3v, tmp4v, tmp5v, tmp6v, tmp7v;

    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, dct0v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, dct1v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, dct2v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, dct3v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, dct4v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, dct5v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, dct6v );
    VEC_DIFF_H( pix1, i_pix1, pix2, i_pix2, 8, dct7v );
    VEC_DCT( dct0v, dct1v, dct2v, dct3v, tmp0v, tmp1v, tmp2v, tmp3v );
    VEC_DCT( dct4v, dct5v, dct6v, dct7v, tmp4v, tmp5v, tmp6v, tmp7v );
    VEC_TRANSPOSE_8( tmp0v, tmp1v, tmp2v, tmp3v,
                     tmp4v, tmp5v, tmp6v, tmp7v,
                     dct0v, dct1v, dct2v, dct3v,
                     dct4v, dct5v, dct6v, dct7v );
    VEC_DCT( dct0v, dct1v, dct2v, dct3v, tmp0v, tmp1v, tmp2v, tmp3v );
    VEC_DCT( dct4v, dct5v, dct6v, dct7v, tmp4v, tmp5v, tmp6v, tmp7v );
    VEC_TRANSPOSE_8( tmp0v, tmp1v, tmp2v, tmp3v,
                     tmp4v, tmp5v, tmp6v, tmp7v,
                     dct0v, dct1v, dct2v, dct3v,
                     dct4v, dct5v, dct6v, dct7v );
    VEC_STORE8_H( dct0v, dct[0][0] );
    VEC_STORE8_L( dct0v, dct[1][0] );
    VEC_STORE8_H( dct1v, dct[0][1] );
    VEC_STORE8_L( dct1v, dct[1][1] );
    VEC_STORE8_H( dct2v, dct[0][2] );
    VEC_STORE8_L( dct2v, dct[1][2] );
    VEC_STORE8_H( dct3v, dct[0][3] );
    VEC_STORE8_L( dct3v, dct[1][3] );
    VEC_STORE8_H( dct4v, dct[2][0] );
    VEC_STORE8_L( dct4v, dct[3][0] );
    VEC_STORE8_H( dct5v, dct[2][1] );
    VEC_STORE8_L( dct5v, dct[3][1] );
    VEC_STORE8_H( dct6v, dct[2][2] );
    VEC_STORE8_L( dct6v, dct[3][2] );
    VEC_STORE8_H( dct7v, dct[2][3] );
    VEC_STORE8_L( dct7v, dct[3][3] );
}
    
void x264_sub16x16_dct_altivec( int16_t dct[16][4][4],
        uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 ) 
{
    PREP_DIFF;
    PREP_STORE8_HL;
    vec_s16_t dcth0v, dcth1v, dcth2v, dcth3v,
              dcth4v, dcth5v, dcth6v, dcth7v,
              dctl0v, dctl1v, dctl2v, dctl3v,
              dctl4v, dctl5v, dctl6v, dctl7v;
    vec_s16_t temp0v, temp1v, temp2v, temp3v,
              temp4v, temp5v, temp6v, temp7v;

    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth0v, dctl0v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth1v, dctl1v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth2v, dctl2v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth3v, dctl3v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth4v, dctl4v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth5v, dctl5v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth6v, dctl6v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth7v, dctl7v );

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
    VEC_DCT( dcth4v, dcth5v, dcth6v, dcth7v,
             temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     dcth0v, dcth1v, dcth2v, dcth3v,
                     dcth4v, dcth5v, dcth6v, dcth7v );
    VEC_STORE8_H( dcth0v, dct[0][0] );
    VEC_STORE8_L( dcth0v, dct[1][0] );
    VEC_STORE8_H( dcth1v, dct[0][1] );
    VEC_STORE8_L( dcth1v, dct[1][1] );
    VEC_STORE8_H( dcth2v, dct[0][2] );
    VEC_STORE8_L( dcth2v, dct[1][2] );
    VEC_STORE8_H( dcth3v, dct[0][3] );
    VEC_STORE8_L( dcth3v, dct[1][3] );
    VEC_STORE8_H( dcth4v, dct[2][0] );
    VEC_STORE8_L( dcth4v, dct[3][0] );
    VEC_STORE8_H( dcth5v, dct[2][1] );
    VEC_STORE8_L( dcth5v, dct[3][1] );
    VEC_STORE8_H( dcth6v, dct[2][2] );
    VEC_STORE8_L( dcth6v, dct[3][2] );
    VEC_STORE8_H( dcth7v, dct[2][3] );
    VEC_STORE8_L( dcth7v, dct[3][3] );

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
    VEC_DCT( dctl4v, dctl5v, dctl6v, dctl7v,
             temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     dctl0v, dctl1v, dctl2v, dctl3v,
                     dctl4v, dctl5v, dctl6v, dctl7v );
    VEC_STORE8_H( dctl0v, dct[4][0] );
    VEC_STORE8_L( dctl0v, dct[5][0] );
    VEC_STORE8_H( dctl1v, dct[4][1] );
    VEC_STORE8_L( dctl1v, dct[5][1] );
    VEC_STORE8_H( dctl2v, dct[4][2] );
    VEC_STORE8_L( dctl2v, dct[5][2] );
    VEC_STORE8_H( dctl3v, dct[4][3] );
    VEC_STORE8_L( dctl3v, dct[5][3] );
    VEC_STORE8_H( dctl4v, dct[6][0] );
    VEC_STORE8_L( dctl4v, dct[7][0] );
    VEC_STORE8_H( dctl5v, dct[6][1] );
    VEC_STORE8_L( dctl5v, dct[7][1] );
    VEC_STORE8_H( dctl6v, dct[6][2] );
    VEC_STORE8_L( dctl6v, dct[7][2] );
    VEC_STORE8_H( dctl7v, dct[6][3] );
    VEC_STORE8_L( dctl7v, dct[7][3] );

    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth0v, dctl0v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth1v, dctl1v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth2v, dctl2v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth3v, dctl3v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth4v, dctl4v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth5v, dctl5v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth6v, dctl6v );
    VEC_DIFF_HL( pix1, i_pix1, pix2, i_pix2, dcth7v, dctl7v );

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
    VEC_DCT( dcth4v, dcth5v, dcth6v, dcth7v,
             temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     dcth0v, dcth1v, dcth2v, dcth3v,
                     dcth4v, dcth5v, dcth6v, dcth7v );
    VEC_STORE8_H( dcth0v, dct[8][0] );
    VEC_STORE8_L( dcth0v, dct[9][0] );
    VEC_STORE8_H( dcth1v, dct[8][1] );
    VEC_STORE8_L( dcth1v, dct[9][1] );
    VEC_STORE8_H( dcth2v, dct[8][2] );
    VEC_STORE8_L( dcth2v, dct[9][2] );
    VEC_STORE8_H( dcth3v, dct[8][3] );
    VEC_STORE8_L( dcth3v, dct[9][3] );
    VEC_STORE8_H( dcth4v, dct[10][0] );
    VEC_STORE8_L( dcth4v, dct[11][0] );
    VEC_STORE8_H( dcth5v, dct[10][1] );
    VEC_STORE8_L( dcth5v, dct[11][1] );
    VEC_STORE8_H( dcth6v, dct[10][2] );
    VEC_STORE8_L( dcth6v, dct[11][2] );
    VEC_STORE8_H( dcth7v, dct[10][3] );
    VEC_STORE8_L( dcth7v, dct[11][3] );

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
    VEC_DCT( dctl4v, dctl5v, dctl6v, dctl7v,
             temp4v, temp5v, temp6v, temp7v );
    VEC_TRANSPOSE_8( temp0v, temp1v, temp2v, temp3v,
                     temp4v, temp5v, temp6v, temp7v,
                     dctl0v, dctl1v, dctl2v, dctl3v,
                     dctl4v, dctl5v, dctl6v, dctl7v );
    VEC_STORE8_H( dctl0v, dct[12][0] );
    VEC_STORE8_L( dctl0v, dct[13][0] );
    VEC_STORE8_H( dctl1v, dct[12][1] );
    VEC_STORE8_L( dctl1v, dct[13][1] );
    VEC_STORE8_H( dctl2v, dct[12][2] );
    VEC_STORE8_L( dctl2v, dct[13][2] );
    VEC_STORE8_H( dctl3v, dct[12][3] );
    VEC_STORE8_L( dctl3v, dct[13][3] );
    VEC_STORE8_H( dctl4v, dct[14][0] );
    VEC_STORE8_L( dctl4v, dct[15][0] );
    VEC_STORE8_H( dctl5v, dct[14][1] );
    VEC_STORE8_L( dctl5v, dct[15][1] );
    VEC_STORE8_H( dctl6v, dct[14][2] );
    VEC_STORE8_L( dctl6v, dct[15][2] );
    VEC_STORE8_H( dctl7v, dct[14][3] );
    VEC_STORE8_L( dctl7v, dct[15][3] );
}
