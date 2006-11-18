/*****************************************************************************
* quant.c: h264 encoder
*****************************************************************************
* Authors: Guillaume Poirier <poirierg@gmail.com>
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

#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif

#include "common/common.h"
#include "ppccommon.h"
#include "quant.h"            

// quant of a whole 4x4 block, unrolled 2x and "pre-scheduled"
#define QUANT_16_U( dct0, dct1, quant_mf0, quant_mf1, quant_mf2, quant_mf3 ) \
temp1v = vec_ld((dct0), *dct);                                               \
temp2v = vec_ld((dct1), *dct);                                               \
mfvA = (vec_u16_t) vec_packs((vec_u32_t)vec_ld((quant_mf0), *quant_mf), (vec_u32_t)vec_ld((quant_mf1), *quant_mf));    \
mfvB = (vec_u16_t) vec_packs((vec_u32_t)vec_ld((quant_mf2), *quant_mf), (vec_u32_t)vec_ld((quant_mf3), *quant_mf));    \
mskA = vec_cmplt(temp1v, zerov);                                             \
mskB = vec_cmplt(temp2v, zerov);                                             \
coefvA = (vec_u16_t)vec_max(vec_sub(zerov, temp1v), temp1v);                 \
coefvB = (vec_u16_t)vec_max(vec_sub(zerov, temp2v), temp2v);                 \
multEvenvA = vec_mule(coefvA, mfvA);                                         \
multOddvA = vec_mulo(coefvA, mfvA);                                          \
multEvenvB = vec_mule(coefvB, mfvB);                                         \
multOddvB = vec_mulo(coefvB, mfvB);                                          \
multEvenvA = vec_adds(multEvenvA, fV);                                        \
multOddvA = vec_adds(multOddvA, fV);                                          \
multEvenvB = vec_adds(multEvenvB, fV);                                        \
multOddvB = vec_adds(multOddvB, fV);                                          \
multEvenvA = vec_sr(multEvenvA, i_qbitsv);                                   \
multOddvA = vec_sr(multOddvA, i_qbitsv);                                     \
multEvenvB = vec_sr(multEvenvB, i_qbitsv);                                   \
multOddvB = vec_sr(multOddvB, i_qbitsv);                                     \
temp1v = (vec_s16_t) vec_packs(vec_mergeh(multEvenvA, multOddvA), vec_mergel(multEvenvA, multOddvA)); \
temp2v = (vec_s16_t) vec_packs(vec_mergeh(multEvenvB, multOddvB), vec_mergel(multEvenvB, multOddvB)); \
temp1v = vec_xor(temp1v, mskA);                                              \
temp2v = vec_xor(temp2v, mskB);                                              \
temp1v = vec_adds(temp1v, vec_and(mskA, one));                                \
vec_st(temp1v, (dct0), dct);                                                 \
temp2v = vec_adds(temp2v, vec_and(mskB, one));                                \
vec_st(temp2v, (dct1), dct);
                
void x264_quant_4x4_altivec( int16_t dct[4][4], int quant_mf[4][4], int const i_qbits, int const f ) {
    vector bool short mskA;
    vec_s32_t i_qbitsv;
    vec_u16_t coefvA;
    vec_u32_t multEvenvA, multOddvA;
    vec_u32_t mfvA;
    vec_s16_t zerov, one;
    vec_s32_t fV;

    vector bool short mskB;
    vec_u16_t coefvB;
    vec_u32_t multEvenvB, multOddvB;
    vec_u32_t mfvB;

    vec_s16_t temp1v, temp2v;

    vect_sint_u qbits_u;
    qbits_u.s[0]=i_qbits;
    i_qbitsv = vec_splat(qbits_u.v, 0);

    vect_sint_u f_u;
    f_u.s[0]=f;

    fV = vec_splat(f_u.v, 0);

    zerov = vec_splat_s16(0);
    one = vec_splat_s16(1);

    QUANT_16_U( 0, 16, 0, 16, 32, 48 );
}

// DC quant of a whole 4x4 block, unrolled 2x and "pre-scheduled"
#define QUANT_16_U_DC( dct0, dct1 )                             \
temp1v = vec_ld((dct0), *dct);                                  \
temp2v = vec_ld((dct1), *dct);                                  \
mskA = vec_cmplt(temp1v, zerov);                                \
mskB = vec_cmplt(temp2v, zerov);                                \
coefvA = (vec_u16_t) vec_max(vec_sub(zerov, temp1v), temp1v);   \
coefvB = (vec_u16_t) vec_max(vec_sub(zerov, temp2v), temp2v);   \
multEvenvA = vec_mule(coefvA, mfv);                             \
multOddvA = vec_mulo(coefvA, mfv);                              \
multEvenvB = vec_mule(coefvB, mfv);                             \
multOddvB = vec_mulo(coefvB, mfv);                              \
multEvenvA = vec_add(multEvenvA, fV);                           \
multOddvA = vec_add(multOddvA, fV);                             \
multEvenvB = vec_add(multEvenvB, fV);                           \
multOddvB = vec_add(multOddvB, fV);                             \
multEvenvA = vec_sr(multEvenvA, i_qbitsv);                      \
multOddvA = vec_sr(multOddvA, i_qbitsv);                        \
multEvenvB = vec_sr(multEvenvB, i_qbitsv);                      \
multOddvB = vec_sr(multOddvB, i_qbitsv);                        \
temp1v = (vec_s16_t) vec_packs(vec_mergeh(multEvenvA, multOddvA), vec_mergel(multEvenvA, multOddvA)); \
temp2v = (vec_s16_t) vec_packs(vec_mergeh(multEvenvB, multOddvB), vec_mergel(multEvenvB, multOddvB)); \
temp1v = vec_xor(temp1v, mskA);                                 \
temp2v = vec_xor(temp2v, mskB);                                 \
temp1v = vec_add(temp1v, vec_and(mskA, one));                   \
vec_st(temp1v, (dct0), dct);                                    \
temp2v = vec_add(temp2v, vec_and(mskB, one));                   \
vec_st(temp2v, (dct1), dct);


void x264_quant_4x4_dc_altivec( int16_t dct[4][4], int i_quant_mf, int const i_qbits, int const f ) {
    vector bool short mskA;
    vec_s32_t i_qbitsv;
    vec_u16_t coefvA;
    vec_u32_t multEvenvA, multOddvA;
    vec_s16_t zerov, one;
    vec_s32_t fV;

    vector bool short mskB;
    vec_u16_t coefvB;
    vec_u32_t multEvenvB, multOddvB;

    vec_s16_t temp1v, temp2v;

    vec_u32_t mfv;
    vect_int_u mf_u;
    mf_u.s[0]=i_quant_mf;
    mfv = vec_splat( mf_u.v, 0 );
    mfv = vec_packs( mfv, mfv);

    vect_sint_u qbits_u;
    qbits_u.s[0]=i_qbits;
    i_qbitsv = vec_splat(qbits_u.v, 0);

    vect_sint_u f_u;
    f_u.s[0]=f;
    fV = vec_splat(f_u.v, 0);

    zerov = vec_splat_s16(0);
    one = vec_splat_s16(1);

    QUANT_16_U_DC( 0, 16 );
}


void x264_quant_8x8_altivec( int16_t dct[8][8], int quant_mf[8][8], int const i_qbits, int const f ) {
    vector bool short mskA;
    vec_s32_t i_qbitsv;
    vec_u16_t coefvA;
    vec_s32_t multEvenvA, multOddvA, mfvA;
    vec_s16_t zerov, one;
    vec_s32_t fV;
    
    vector bool short mskB;
    vec_u16_t coefvB;
    vec_u32_t multEvenvB, multOddvB, mfvB;
    
    vec_s16_t temp1v, temp2v;
    
    vect_int_u qbits_u;
    qbits_u.s[0]=i_qbits;
    i_qbitsv = vec_splat(qbits_u.v, 0);

    vect_sint_u f_u;
    f_u.s[0]=f;
    fV = vec_splat(f_u.v, 0);

    zerov = vec_splat_s16(0);
    one = vec_splat_s16(1);
    
    int i;

    for ( i=0; i<4; i++ ) {
      QUANT_16_U( i*2*16, i*2*16+16, i*4*16, i*4*16+16, i*4*16+32, i*4*16+48 );
    }
}

