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

/* Handy */
#define vector_u8_t  vector unsigned char
#define vector_s16_t vector signed short
#define vector_u32_t vector unsigned int
#define vector_s32_t vector signed int

#define LOAD_ZERO    vector_s32_t zero = vec_splat_s32( 0 )
#define zero_u8      (vector_u8_t)  zero
#define zero_s16     (vector_s16_t) zero
#define zero_s32     (vector_s32_t) zero

#define CONVERT_U8_TO_S16( a ) \
    a = (vector_s16_t) vec_mergeh( zero_u8, (vector_u8_t) a )

/* Macros to load aligned or unaligned data without risking buffer
   overflows. */
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

#define LOAD_8( p, v )                                             \
    if( !( (long) p & 0xF ) )                                      \
    {                                                              \
        v = vec_ld( 0, p );                                        \
    }                                                              \
    else if( ( (long) p & 0xF ) < 9 )                              \
    {                                                              \
        v = vec_perm( vec_ld( 0, p ), (vector unsigned char) zero, \
                      vec_lvsl( 0, p ) );                          \
    }                                                              \
    else                                                           \
    {                                                              \
        v = vec_perm( vec_ld( 0, p ), vec_ld( 16, p ),             \
                      vec_lvsl( 0, p ) );                          \
    }

#define LOAD_4( p, v )                                             \
    if( !( (long) p & 0xF ) )                                      \
    {                                                              \
        v = vec_ld( 0, p );                                        \
    }                                                              \
    else if( ( (long) p & 0xF ) < 13 )                             \
    {                                                              \
        v = vec_perm( vec_ld( 0, p ), (vector unsigned char) zero, \
                      vec_lvsl( 0, p ) );                          \
    }                                                              \
    else                                                           \
    {                                                              \
        v = vec_perm( vec_ld( 0, p ), vec_ld( 16, p ),             \
                      vec_lvsl( 0, p ) );                          \
    }

/* Store aligned or unaligned data */
#define STORE_16( v, p )                              \
    if( (long) p & 0xF )                              \
    {                                                 \
        vector unsigned char tmp1, tmp2;              \
        vector unsigned char align, mask;             \
        tmp1 = vec_ld( 0, p );                        \
        tmp2 = vec_ld( 16, p );                       \
        align = vec_lvsr( 0, p );                     \
        mask = vec_perm( (vector unsigned char) {0},  \
                         (vector unsigned char) {1},  \
                         align);                      \
        v = vec_perm( v, v, align);                   \
        tmp1 = vec_sel( tmp1, v, mask );              \
        tmp2 = vec_sel( v, tmp2, mask );              \
        vec_st( tmp1, 0, p );                         \
        vec_st( tmp2, 16, p );                        \
    }                                                 \
    else                                              \
    {                                                 \
        vec_st( v, 0, p );                            \
    }

/* Transpose 8x8 (vector_s16_t [8]) */
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

/* Transpose 4x4 (vector_s16_t [4]) */
#define TRANSPOSE4x4( a, b ) \
    (b)[0] = vec_mergeh( (a)[0], zero_s16 ); \
    (b)[1] = vec_mergeh( (a)[1], zero_s16 ); \
    (b)[2] = vec_mergeh( (a)[2], zero_s16 ); \
    (b)[3] = vec_mergeh( (a)[3], zero_s16 ); \
    (a)[0] = vec_mergeh( (b)[0], (b)[2] );   \
    (a)[1] = vec_mergel( (b)[0], (b)[2] );   \
    (a)[2] = vec_mergeh( (b)[1], (b)[3] );   \
    (a)[3] = vec_mergel( (b)[1], (b)[3] );   \
    (b)[0] = vec_mergeh( (a)[0], (a)[2] );   \
    (b)[1] = vec_mergel( (a)[0], (a)[2] );   \
    (b)[2] = vec_mergeh( (a)[1], (a)[3] );   \
    (b)[3] = vec_mergel( (a)[1], (a)[3] );

/* Hadamar (vector_s16_t [4]) */
#define HADAMAR( a, b ) \
    s01v   = vec_add( (a)[0], (a)[1] ); \
    s23v   = vec_add( (a)[2], (a)[3] ); \
    d01v   = vec_sub( (a)[0], (a)[1] ); \
    d23v   = vec_sub( (a)[2], (a)[3] ); \
    (b)[0] = vec_add( s01v, s23v );     \
    (b)[1] = vec_sub( s01v, s23v );     \
    (b)[2] = vec_sub( d01v, d23v );     \
    (b)[3] = vec_add( d01v, d23v );

