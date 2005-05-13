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
 * VEC_LOAD:  loads n bytes from address p into vector v
 **********************************************************************/
#define PREP_LOAD \
    vec_u8_t _hv, _lv

#define VEC_LOAD( p, v, n )       \
    _hv = vec_ld( 0, p );         \
    v   = vec_lvsl( 0, p );       \
    _lv = vec_ld( n - 1, p );     \
    v   = vec_perm( _hv, _lv, v )

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
    _lv    = vec_perm( v, _tmp1v, _tmp2v ); \
    vec_st( _lv, 15, p ); \
    _hv    = vec_perm( _tmp1v, v, _tmp2v ); \
    vec_st( _hv, 0, p )

#define PREP_STORE8 \
    PREP_STORE16; \
    vec_u8_t _tmp3v; \
    const vec_u8_t sel = \
        (vec_u8_t) CV(-1,-1,-1,-1,-1,-1,-1,-1,0,0,0,0,0,0,0,0)

#define VEC_STORE8( v, p ) \
    _tmp3v = vec_lvsr( 0, p ); \
    v      = vec_perm( v, v, _tmp3v ); \
    _lv    = vec_ld( 7, p ); \
    _tmp1v = vec_perm( sel, zero_u8v, _tmp3v ); \
    _lv    = vec_sel( _lv, v, _tmp1v ); \
    vec_st( _lv, 7, p ); \
    _hv    = vec_ld( 0, p ); \
    _tmp2v = vec_perm( zero_u8v, sel, _tmp3v ); \
    _hv    = vec_sel( _hv, v, _tmp2v ); \
    vec_st( _hv, 0, p )

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

