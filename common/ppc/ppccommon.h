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
#define LOAD_ZERO vec_u8_t zerov = vec_splat_u8( 0 )

#define zero_u8v  (vec_u8_t)  zerov
#define zero_s8v  (vec_s8_t)  zerov
#define zero_u16v (vec_u16_t) zerov
#define zero_s16v (vec_s16_t) zerov
#define zero_u32v (vec_u32_t) zerov
#define zero_s32v (vec_s32_t) zerov

/***********************************************************************
 * CONVERT_*
 **********************************************************************/
#define CONVERT_U8_TO_U16( s, d ) \
    d = (vec_u16_t) vec_mergeh( zero_u8v, (vec_u8_t) s )
#define CONVERT_U8_TO_S16( s, d ) \
    d = (vec_s16_t) vec_mergeh( zero_u8v, (vec_u8_t) s )
#define CONVERT_U16_TO_U8( s, d ) \
    d = (vec_u8_t) vec_pack( (vec_u16_t) s, zero_u16v )
#define CONVERT_S16_TO_U8( s, d ) \
    d = (vec_u8_t) vec_pack( (vec_s16_t) s, zero_s16v )

/***********************************************************************
 * LOAD_16
 ***********************************************************************
 * p: uint8_t *
 * v: vec_u8_t
 * Loads 16 bytes from p into v
 **********************************************************************/
#define LOAD_16( p, v )                                \
    if( (long) p & 0xF )                               \
    {                                                  \
        v = vec_perm( vec_ld( 0, p ), vec_ld( 16, p ), \
                      vec_lvsl( 0, p ) );              \
    }                                                  \
    else                                               \
    {                                                  \
        v = vec_ld( 0, p );                            \
    }

/***********************************************************************
 * LOAD_8
 ***********************************************************************
 * p: uint8_t *
 * v: vec_u8_t
 * Loads 8 bytes from p into the first half of v
 **********************************************************************/
#define LOAD_8( p, v )                                 \
    if( !( (long) p & 0xF ) )                          \
    {                                                  \
        v = vec_ld( 0, p );                            \
    }                                                  \
    else if( ( (long) p & 0xF ) < 9 )                  \
    {                                                  \
        v = vec_perm( vec_ld( 0, p ), zero_u8v,        \
                      vec_lvsl( 0, p ) );              \
    }                                                  \
    else                                               \
    {                                                  \
        v = vec_perm( vec_ld( 0, p ), vec_ld( 16, p ), \
                      vec_lvsl( 0, p ) );              \
    }

/***********************************************************************
 * LOAD_4
 ***********************************************************************
 * p: uint8_t *
 * v: vec_u8_t
 * Loads 4 bytes from p into the first quarter of v
 **********************************************************************/
#define LOAD_4( p, v )                                 \
    if( !( (long) p & 0xF ) )                          \
    {                                                  \
        v = vec_ld( 0, p );                            \
    }                                                  \
    else if( ( (long) p & 0xF ) < 13 )                 \
    {                                                  \
        v = vec_perm( vec_ld( 0, p ), zero_u8v,        \
                      vec_lvsl( 0, p ) );              \
    }                                                  \
    else                                               \
    {                                                  \
        v = vec_perm( vec_ld( 0, p ), vec_ld( 16, p ), \
                      vec_lvsl( 0, p ) );              \
    }

/***********************************************************************
 * STORE_16
 ***********************************************************************
 * v: vec_u8_t
 * p: uint8_t *
 * Stores the 16 bytes from v at address p
 **********************************************************************/
#define STORE_16( v, p )                  \
    if( (long) p & 0xF )                  \
    {                                     \
        vec_u8_t hv, lv, tmp1, tmp2;      \
        hv   = vec_ld( 0, p );            \
        lv   = vec_ld( 16, p );           \
        tmp2 = vec_lvsl( 0, p );          \
        tmp1 = vec_perm( lv, hv, tmp2 );  \
        tmp2 = vec_lvsr( 0, p );          \
        hv   = vec_perm( tmp1, v, tmp2 ); \
        lv   = vec_perm( v, tmp1, tmp2 ); \
        vec_st( lv, 16, p );              \
        vec_st( hv, 0, p );               \
    }                                     \
    else                                  \
    {                                     \
        vec_st( v, 0, p );                \
    }

/* FIXME We can do better than that */
#define STORE_8( v, p ) \
    { \
        DECLARE_ALIGNED( uint8_t, _p[16], 16 ); \
        vec_st( v, 0, _p ); \
        memcpy( p, _p, 8 ); \
    }

/* Transpose 8x8 (vec_s16_t [8]) */
#define TRANSPOSE8x8( a, b )           \
    b[0] = vec_mergeh( a[0], a[4] ); \
    b[1] = vec_mergel( a[0], a[4] ); \
    b[2] = vec_mergeh( a[1], a[5] ); \
    b[3] = vec_mergel( a[1], a[5] ); \
    b[4] = vec_mergeh( a[2], a[6] ); \
    b[5] = vec_mergel( a[2], a[6] ); \
    b[6] = vec_mergeh( a[3], a[7] ); \
    b[7] = vec_mergel( a[3], a[7] ); \
    a[0] = vec_mergeh( b[0], b[4] ); \
    a[1] = vec_mergel( b[0], b[4] ); \
    a[2] = vec_mergeh( b[1], b[5] ); \
    a[3] = vec_mergel( b[1], b[5] ); \
    a[4] = vec_mergeh( b[2], b[6] ); \
    a[5] = vec_mergel( b[2], b[6] ); \
    a[6] = vec_mergeh( b[3], b[7] ); \
    a[7] = vec_mergel( b[3], b[7] ); \
    b[0] = vec_mergeh( a[0], a[4] ); \
    b[1] = vec_mergel( a[0], a[4] ); \
    b[2] = vec_mergeh( a[1], a[5] ); \
    b[3] = vec_mergel( a[1], a[5] ); \
    b[4] = vec_mergeh( a[2], a[6] ); \
    b[5] = vec_mergel( a[2], a[6] ); \
    b[6] = vec_mergeh( a[3], a[7] ); \
    b[7] = vec_mergel( a[3], a[7] );

/* Transpose 4x4 (vec_s16_t [4]) */
#define TRANSPOSE4x4( a, b ) \
    (b)[0] = vec_mergeh( (a)[0], zero_s16v ); \
    (b)[1] = vec_mergeh( (a)[1], zero_s16v ); \
    (b)[2] = vec_mergeh( (a)[2], zero_s16v ); \
    (b)[3] = vec_mergeh( (a)[3], zero_s16v ); \
    (a)[0] = vec_mergeh( (b)[0], (b)[2] );   \
    (a)[1] = vec_mergel( (b)[0], (b)[2] );   \
    (a)[2] = vec_mergeh( (b)[1], (b)[3] );   \
    (a)[3] = vec_mergel( (b)[1], (b)[3] );   \
    (b)[0] = vec_mergeh( (a)[0], (a)[2] );   \
    (b)[1] = vec_mergel( (a)[0], (a)[2] );   \
    (b)[2] = vec_mergeh( (a)[1], (a)[3] );   \
    (b)[3] = vec_mergel( (a)[1], (a)[3] );

/* Hadamar (vec_s16_t [4]) */
#define HADAMAR( a, b ) \
    s01v   = vec_add( (a)[0], (a)[1] ); \
    s23v   = vec_add( (a)[2], (a)[3] ); \
    d01v   = vec_sub( (a)[0], (a)[1] ); \
    d23v   = vec_sub( (a)[2], (a)[3] ); \
    (b)[0] = vec_add( s01v, s23v );     \
    (b)[1] = vec_sub( s01v, s23v );     \
    (b)[2] = vec_sub( d01v, d23v );     \
    (b)[3] = vec_add( d01v, d23v );

