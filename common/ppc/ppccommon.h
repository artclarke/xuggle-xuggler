/*****************************************************************************
 * ppccommon.h: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: ppccommon.h,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

/***********************************************************************
 * For constant vectors, use parentheses on OS X and braces on Linux
 **********************************************************************/
#ifdef SYS_MACOSX
#define CV(a...) (a)
#else
#define CV(a...) {a}
#endif

/***********************************************************************
 * Vector types
 **********************************************************************/
#define vec_u8_t  vector unsigned char
#define vec_s8_t  vector signed char
#define vec_u16_t vector unsigned short
#define vec_s16_t vector signed short
#define vec_u32_t vector unsigned int
#define vec_s32_t vector signed int

/***********************************************************************
 * Null vector
 **********************************************************************/
#define LOAD_ZERO const vec_u8_t zerov = vec_splat_u8( 0 )

#define zero_u8v  (vec_u8_t)  zerov
#define zero_s8v  (vec_s8_t)  zerov
#define zero_u16v (vec_u16_t) zerov
#define zero_s16v (vec_s16_t) zerov
#define zero_u32v (vec_u32_t) zerov
#define zero_s32v (vec_s32_t) zerov

/***********************************************************************
 * 8 <-> 16 bits conversions
 **********************************************************************/
#define vec_u8_to_u16_h(v) (vec_u16_t) vec_mergeh( zero_u8v, (vec_u8_t) v )
#define vec_u8_to_u16_l(v) (vec_u16_t) vec_mergel( zero_u8v, (vec_u8_t) v )
#define vec_u8_to_s16_h(v) (vec_s16_t) vec_mergeh( zero_u8v, (vec_u8_t) v )
#define vec_u8_to_s16_l(v) (vec_s16_t) vec_mergel( zero_u8v, (vec_u8_t) v )

#define vec_u8_to_u16(v) vec_u8_to_u16_h(v)
#define vec_u8_to_s16(v) vec_u8_to_s16_h(v)

#define vec_u16_to_u8(v) vec_pack( v, zero_u16v )
#define vec_s16_to_u8(v) vec_pack( v, zero_u16v )

/***********************************************************************
 * PREP_LOAD: declares two vectors required to perform unaligned loads
 * VEC_LOAD:  loads n bytes from u8 * p into vector v of type t
 **********************************************************************/
#define PREP_LOAD \
    vec_u8_t _hv, _lv

#define VEC_LOAD( p, v, n, t )                  \
    _hv = vec_ld( 0, p );                       \
    v   = (t) vec_lvsl( 0, p );                 \
    _lv = vec_ld( n - 1, p );                   \
    v   = (t) vec_perm( _hv, _lv, (vec_u8_t) v )

/***********************************************************************
 * PREP_STORE##n: declares required vectors to store n bytes to a
 *                potentially unaligned address
 * VEC_STORE##n:  stores n bytes from vector v to address p
 **********************************************************************/
#define PREP_STORE16 \
    vec_u8_t _tmp1v, _tmp2v \

#define VEC_STORE16( v, p ) \
    _hv    = vec_ld( 0, p ); \
    _tmp2v = vec_lvsl( 0, p ); \
    _lv    = vec_ld( 15, p ); \
    _tmp1v = vec_perm( _lv, _hv, _tmp2v ); \
    _tmp2v = vec_lvsr( 0, p ); \
    _lv    = vec_perm( (vec_u8_t) v, _tmp1v, _tmp2v ); \
    vec_st( _lv, 15, (uint8_t *) p ); \
    _hv    = vec_perm( _tmp1v, (vec_u8_t) v, _tmp2v ); \
    vec_st( _hv, 0, (uint8_t *) p )

#define PREP_STORE8 \
    PREP_STORE16; \
    vec_u8_t _tmp3v, _tmp4v; \
    const vec_u8_t sel_h = \
        (vec_u8_t) CV(-1,-1,-1,-1,-1,-1,-1,-1,0,0,0,0,0,0,0,0)

#define PREP_STORE8_HL \
    PREP_STORE8; \
    const vec_u8_t sel_l = \
        (vec_u8_t) CV(0,0,0,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1)

#define VEC_STORE8 VEC_STORE8_H

#define VEC_STORE8_H( v, p ) \
    _tmp3v = vec_lvsr( 0, (uint8_t *) p ); \
    _tmp4v = vec_perm( (vec_u8_t) v, (vec_u8_t) v, _tmp3v ); \
    _lv    = vec_ld( 7, (uint8_t *) p ); \
    _tmp1v = vec_perm( sel_h, zero_u8v, _tmp3v ); \
    _lv    = vec_sel( _lv, _tmp4v, _tmp1v ); \
    vec_st( _lv, 7, (uint8_t *) p ); \
    _hv    = vec_ld( 0, (uint8_t *) p ); \
    _tmp2v = vec_perm( zero_u8v, sel_h, _tmp3v ); \
    _hv    = vec_sel( _hv, _tmp4v, _tmp2v ); \
    vec_st( _hv, 0, (uint8_t *) p )

#define VEC_STORE8_L( v, p ) \
    _tmp3v = vec_lvsr( 8, (uint8_t *) p ); \
    _tmp4v = vec_perm( (vec_u8_t) v, (vec_u8_t) v, _tmp3v ); \
    _lv    = vec_ld( 7, (uint8_t *) p ); \
    _tmp1v = vec_perm( sel_l, zero_u8v, _tmp3v ); \
    _lv    = vec_sel( _lv, _tmp4v, _tmp1v ); \
    vec_st( _lv, 7, (uint8_t *) p ); \
    _hv    = vec_ld( 0, (uint8_t *) p ); \
    _tmp2v = vec_perm( zero_u8v, sel_l, _tmp3v ); \
    _hv    = vec_sel( _hv, _tmp4v, _tmp2v ); \
    vec_st( _hv, 0, (uint8_t *) p )

#define PREP_STORE4 \
    PREP_STORE16; \
    vec_u8_t _tmp3v; \
    const vec_u8_t sel = \
        (vec_u8_t) CV(-1,-1,-1,-1,0,0,0,0,0,0,0,0,0,0,0,0)

#define VEC_STORE4( v, p ) \
    _tmp3v = vec_lvsr( 0, p ); \
    v      = vec_perm( v, v, _tmp3v ); \
    _lv    = vec_ld( 3, p ); \
    _tmp1v = vec_perm( sel, zero_u8v, _tmp3v ); \
    _lv    = vec_sel( _lv, v, _tmp1v ); \
    vec_st( _lv, 3, p ); \
    _hv    = vec_ld( 0, p ); \
    _tmp2v = vec_perm( zero_u8v, sel, _tmp3v ); \
    _hv    = vec_sel( _hv, v, _tmp2v ); \
    vec_st( _hv, 0, p )

/***********************************************************************
 * VEC_TRANSPOSE_8
 ***********************************************************************
 * Transposes a 8x8 matrix of s16 vectors
 **********************************************************************/
#define VEC_TRANSPOSE_8(a0,a1,a2,a3,a4,a5,a6,a7,b0,b1,b2,b3,b4,b5,b6,b7) \
    b0 = vec_mergeh( a0, a4 ); \
    b1 = vec_mergel( a0, a4 ); \
    b2 = vec_mergeh( a1, a5 ); \
    b3 = vec_mergel( a1, a5 ); \
    b4 = vec_mergeh( a2, a6 ); \
    b5 = vec_mergel( a2, a6 ); \
    b6 = vec_mergeh( a3, a7 ); \
    b7 = vec_mergel( a3, a7 ); \
    a0 = vec_mergeh( b0, b4 ); \
    a1 = vec_mergel( b0, b4 ); \
    a2 = vec_mergeh( b1, b5 ); \
    a3 = vec_mergel( b1, b5 ); \
    a4 = vec_mergeh( b2, b6 ); \
    a5 = vec_mergel( b2, b6 ); \
    a6 = vec_mergeh( b3, b7 ); \
    a7 = vec_mergel( b3, b7 ); \
    b0 = vec_mergeh( a0, a4 ); \
    b1 = vec_mergel( a0, a4 ); \
    b2 = vec_mergeh( a1, a5 ); \
    b3 = vec_mergel( a1, a5 ); \
    b4 = vec_mergeh( a2, a6 ); \
    b5 = vec_mergel( a2, a6 ); \
    b6 = vec_mergeh( a3, a7 ); \
    b7 = vec_mergel( a3, a7 )

/***********************************************************************
 * VEC_TRANSPOSE_4
 ***********************************************************************
 * Transposes a 4x4 matrix of s16 vectors.
 * Actually source and destination are 8x4. The low elements of the
 * source are discarded and the low elements of the destination mustn't
 * be used.
 **********************************************************************/
#define VEC_TRANSPOSE_4(a0,a1,a2,a3,b0,b1,b2,b3) \
    b0 = vec_mergeh( a0, a0 ); \
    b1 = vec_mergeh( a1, a0 ); \
    b2 = vec_mergeh( a2, a0 ); \
    b3 = vec_mergeh( a3, a0 ); \
    a0 = vec_mergeh( b0, b2 ); \
    a1 = vec_mergel( b0, b2 ); \
    a2 = vec_mergeh( b1, b3 ); \
    a3 = vec_mergel( b1, b3 ); \
    b0 = vec_mergeh( a0, a2 ); \
    b1 = vec_mergel( a0, a2 ); \
    b2 = vec_mergeh( a1, a3 ); \
    b3 = vec_mergel( a1, a3 )

/***********************************************************************
 * VEC_DIFF_H
 ***********************************************************************
 * p1, p2:    u8 *
 * i1, i2, n: int
 * d:         s16v
 *
 * Loads n bytes from p1 and p2, do the diff of the high elements into
 * d, increments p1 and p2 by i1 and i2
 **********************************************************************/
#define PREP_DIFF           \
    LOAD_ZERO;              \
    PREP_LOAD;              \
    vec_s16_t pix1v, pix2v;

#define VEC_DIFF_H(p1,i1,p2,i2,n,d)      \
    VEC_LOAD( p1, pix1v, n, vec_s16_t ); \
    pix1v = vec_u8_to_s16( pix1v );      \
    VEC_LOAD( p2, pix2v, n, vec_s16_t ); \
    pix2v = vec_u8_to_s16( pix2v );      \
    d     = vec_sub( pix1v, pix2v );     \
    p1   += i1;                          \
    p2   += i2

/***********************************************************************
 * VEC_DIFF_HL
 ***********************************************************************
 * p1, p2: u8 *
 * i1, i2: int
 * dh, dl: s16v
 *
 * Loads 16 bytes from p1 and p2, do the diff of the high elements into
 * dh, the diff of the low elements into dl, increments p1 and p2 by i1
 * and i2
 **********************************************************************/
#define VEC_DIFF_HL(p1,i1,p2,i2,dh,dl)    \
    VEC_LOAD( p1, pix1v, 16, vec_s16_t ); \
    temp0v = vec_u8_to_s16_h( pix1v );    \
    temp1v = vec_u8_to_s16_l( pix1v );    \
    VEC_LOAD( p2, pix2v, 16, vec_s16_t ); \
    temp2v = vec_u8_to_s16_h( pix2v );    \
    temp3v = vec_u8_to_s16_l( pix2v );    \
    dh     = vec_sub( temp0v, temp2v );   \
    dl     = vec_sub( temp1v, temp3v );   \
    p1    += i1;                          \
    p2    += i2
