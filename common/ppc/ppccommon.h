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
#define LOAD_ZERO vec_u8_t zerov = vec_splat_u8( 0 )

#define zero_u8v  (vec_u8_t)  zerov
#define zero_s8v  (vec_s8_t)  zerov
#define zero_u16v (vec_u16_t) zerov
#define zero_s16v (vec_s16_t) zerov
#define zero_u32v (vec_u32_t) zerov
#define zero_s32v (vec_s32_t) zerov

/***********************************************************************
 * Conversions
 **********************************************************************/
static inline vec_u16_t vec_u8_to_u16( vec_u8_t v )
{
    LOAD_ZERO;
    return (vec_u16_t) vec_mergeh( zero_u8v, v );
}
static inline vec_u8_t vec_u16_to_u8( vec_u16_t v )
{
    LOAD_ZERO;
    return (vec_u8_t) vec_pack( v, zero_u16v );
}
#define vec_u8_to_s16(v) (vec_s16_t) vec_u8_to_u16(v)
#define vec_s16_to_u8(v) vec_u16_to_u8( (vec_u16_t) v )

/***********************************************************************
 * vec_load16
 **********************************************************************/
static inline vec_u8_t vec_load16( uint8_t * p )
{
    if( (long) p & 0xF )
    {
        vec_u8_t hv, lv, perm;
        hv   = vec_ld( 0, p );
        lv   = vec_ld( 16, p );
        perm = vec_lvsl( 0, p );
        return vec_perm( hv, lv, perm );
    }
    return vec_ld( 0, p );
}

/***********************************************************************
 * vec_load8
 **********************************************************************/
static inline vec_u8_t vec_load8( uint8_t * p )
{
    long align = (long) p & 0xF;

    if( align )
    {
        vec_u8_t hv, perm;
        hv   = vec_ld( 0, p );
        perm = vec_lvsl( 0, p );
        if( align > 8 )
        {
            vec_u8_t lv;
            lv = vec_ld( 16, p );
            return vec_perm( hv, lv, perm );
        }
        return vec_perm( hv, hv, perm );
    }
    return vec_ld( 0, p );
}

/***********************************************************************
 * vec_load4
 **********************************************************************/
static inline vec_u8_t vec_load4( uint8_t * p )
{
    long align = (long) p & 0xF;

    if( align )
    {
        vec_u8_t hv, perm;
        hv   = vec_ld( 0, p );
        perm = vec_lvsl( 0, p );
        if( align > 12 )
        {
            vec_u8_t lv;
            lv = vec_ld( 16, p );
            return vec_perm( hv, lv, perm );
        }
        return vec_perm( hv, hv, perm );
    }
    return vec_ld( 0, p );
}

/***********************************************************************
 * vec_store16
 **********************************************************************/
static inline void vec_store16( vec_u8_t v, uint8_t * p )
{
    if( (long) p & 0xF )
    {
        vec_u8_t hv, lv, tmp1, tmp2;
        hv   = vec_ld( 0, p );
        lv   = vec_ld( 16, p );
        tmp2 = vec_lvsl( 0, p );
        tmp1 = vec_perm( lv, hv, tmp2 );
        tmp2 = vec_lvsr( 0, p );
        hv   = vec_perm( tmp1, v, tmp2 );
        lv   = vec_perm( v, tmp1, tmp2 );
        vec_st( lv, 16, p );
        vec_st( hv, 0, p );
        return;
    }
    vec_st( v, 0, p );
}

/***********************************************************************
 * vec_store8
 **********************************************************************/
static inline void vec_store8( vec_u8_t v, uint8_t * p )
{
    LOAD_ZERO;
    long     align;
    vec_u8_t hv, sel;

    align = (long) p & 0xF;
    hv    = vec_ld( 0, p );
    sel   = (vec_u8_t) CV(-1,-1,-1,-1,-1,-1,-1,-1,0,0,0,0,0,0,0,0);

    if( align )
    {
        vec_u8_t perm;
        perm = vec_lvsr( 0, p );
        v    = vec_perm( v, v, perm );
        if( align > 8 )
        {
            vec_u8_t lv, hsel, lsel;
            lv   = vec_ld( 16, p );
            hsel = vec_perm( zero_u8v, sel, perm );
            lsel = vec_perm( sel, zero_u8v, perm );
            hv   = vec_sel( hv, v, hsel );
            lv   = vec_sel( lv, v, lsel );
            vec_st( lv, 16, p );
        }
        else
        {
            sel = vec_perm( sel, sel, perm );
            hv  = vec_sel( hv, v, sel );
        }
    }
    else
    {
        hv = vec_sel( hv, v, sel );
    }
    vec_st( hv, 0, p );
}

/***********************************************************************
 * vec_transpose8x8
 **********************************************************************/
static inline void vec_transpose8x8( vec_s16_t * a, vec_s16_t * b )
{
    b[0] = vec_mergeh( a[0], a[4] );
    b[1] = vec_mergel( a[0], a[4] );
    b[2] = vec_mergeh( a[1], a[5] );
    b[3] = vec_mergel( a[1], a[5] );
    b[4] = vec_mergeh( a[2], a[6] );
    b[5] = vec_mergel( a[2], a[6] );
    b[6] = vec_mergeh( a[3], a[7] );
    b[7] = vec_mergel( a[3], a[7] );
    a[0] = vec_mergeh( b[0], b[4] );
    a[1] = vec_mergel( b[0], b[4] );
    a[2] = vec_mergeh( b[1], b[5] );
    a[3] = vec_mergel( b[1], b[5] );
    a[4] = vec_mergeh( b[2], b[6] );
    a[5] = vec_mergel( b[2], b[6] );
    a[6] = vec_mergeh( b[3], b[7] );
    a[7] = vec_mergel( b[3], b[7] );
    b[0] = vec_mergeh( a[0], a[4] );
    b[1] = vec_mergel( a[0], a[4] );
    b[2] = vec_mergeh( a[1], a[5] );
    b[3] = vec_mergel( a[1], a[5] );
    b[4] = vec_mergeh( a[2], a[6] );
    b[5] = vec_mergel( a[2], a[6] );
    b[6] = vec_mergeh( a[3], a[7] );
    b[7] = vec_mergel( a[3], a[7] );
}

/***********************************************************************
 * vec_transpose4x4
 **********************************************************************/
static inline void vec_transpose4x4( vec_s16_t * a, vec_s16_t * b )
{
#define WHATEVER a[0]
    b[0] = vec_mergeh( a[0], WHATEVER );
    b[1] = vec_mergeh( a[1], WHATEVER );
    b[2] = vec_mergeh( a[2], WHATEVER );
    b[3] = vec_mergeh( a[3], WHATEVER );
    a[0] = vec_mergeh( b[0], b[2] );
    a[1] = vec_mergel( b[0], b[2] );
    a[2] = vec_mergeh( b[1], b[3] );
    a[3] = vec_mergel( b[1], b[3] );
    b[0] = vec_mergeh( a[0], a[2] );
    b[1] = vec_mergel( a[0], a[2] );
    b[2] = vec_mergeh( a[1], a[3] );
    b[3] = vec_mergel( a[1], a[3] );
#undef WHATEVER
}

/***********************************************************************
 * vec_hadamar
 ***********************************************************************
 * b[0] = a[0] + a[1] + a[2] + a[3]
 * b[1] = a[0] + a[1] - a[2] - a[3]
 * b[2] = a[0] - a[1] - a[2] + a[3]
 * b[3] = a[0] - a[1] + a[2] - a[3]
 **********************************************************************/
static inline void vec_hadamar( vec_s16_t * a, vec_s16_t * b )
{
    b[2] = vec_add( a[0], a[1] );
    b[3] = vec_add( a[2], a[3] );
    a[0] = vec_sub( a[0], a[1] );
    a[2] = vec_sub( a[2], a[3] );
    b[0] = vec_add( b[2], b[3] );
    b[1] = vec_sub( b[2], b[3] );
    b[2] = vec_sub( a[0], a[2] );
    b[3] = vec_add( a[0], a[2] );
}
