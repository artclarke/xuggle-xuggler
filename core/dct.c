/*****************************************************************************
 * dct.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: dct.c,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#include "x264.h"

#include "dct.h"
#ifdef HAVE_MMXEXT
#   include "i386/dct.h"
#endif


static inline int clip_uint8( int a )
{
    if (a&(~255))
        return (-a)>>31;
    else
        return a;
}

/*
 * XXX For all dct dc : input could be equal to output so ...
 */

static void dct2x2dc( int16_t d[2][2] )
{
    int tmp[2][2];

    tmp[0][0] = d[0][0] + d[0][1];
    tmp[1][0] = d[0][0] - d[0][1];
    tmp[0][1] = d[1][0] + d[1][1];
    tmp[1][1] = d[1][0] - d[1][1];

    d[0][0] = tmp[0][0] + tmp[0][1];
    d[0][1] = tmp[1][0] + tmp[1][1];
    d[1][0] = tmp[0][0] - tmp[0][1];
    d[1][1] = tmp[1][0] - tmp[1][1];
}

static void dct4x4dc( int16_t d[4][4] )
{
    int16_t tmp[4][4];
    int s01, s23;
    int d01, d23;
    int i;

    for( i = 0; i < 4; i++ )
    {
        s01 = d[i][0] + d[i][1];
        d01 = d[i][0] - d[i][1];
        s23 = d[i][2] + d[i][3];
        d23 = d[i][2] - d[i][3];

        tmp[0][i] = s01 + s23;
        tmp[1][i] = s01 - s23;
        tmp[2][i] = d01 - d23;
        tmp[3][i] = d01 + d23;
    }

    for( i = 0; i < 4; i++ )
    {
        s01 = tmp[i][0] + tmp[i][1];
        d01 = tmp[i][0] - tmp[i][1];
        s23 = tmp[i][2] + tmp[i][3];
        d23 = tmp[i][2] - tmp[i][3];

        d[0][i] = ( s01 + s23 + 1 ) >> 1;
        d[1][i] = ( s01 - s23 + 1 ) >> 1;
        d[2][i] = ( d01 - d23 + 1 ) >> 1;
        d[3][i] = ( d01 + d23 + 1 ) >> 1;
    }
}

static void idct4x4dc( int16_t d[4][4] )
{
    int16_t tmp[4][4];
    int s01, s23;
    int d01, d23;
    int i;

    for( i = 0; i < 4; i++ )
    {
        s01 = d[0][i] + d[1][i];
        d01 = d[0][i] - d[1][i];
        s23 = d[2][i] + d[3][i];
        d23 = d[2][i] - d[3][i];

        tmp[0][i] = s01 + s23;
        tmp[1][i] = s01 - s23;
        tmp[2][i] = d01 - d23;
        tmp[3][i] = d01 + d23;
    }

    for( i = 0; i < 4; i++ )
    {
        s01 = tmp[i][0] + tmp[i][1];
        d01 = tmp[i][0] - tmp[i][1];
        s23 = tmp[i][2] + tmp[i][3];
        d23 = tmp[i][2] - tmp[i][3];

        d[i][0] = s01 + s23;
        d[i][1] = s01 - s23;
        d[i][2] = d01 - d23;
        d[i][3] = d01 + d23;
    }
}

static void sub4x4_dct( int16_t dct[4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    int16_t d[4][4];
    int16_t tmp[4][4];
    int y, x;
    int i;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            d[y][x] = pix1[x] - pix2[x];
        }
        pix1 += i_pix1;
        pix2 += i_pix2;
    }

    for( i = 0; i < 4; i++ )
    {
        const int s03 = d[i][0] + d[i][3];
        const int s12 = d[i][1] + d[i][2];
        const int d03 = d[i][0] - d[i][3];
        const int d12 = d[i][1] - d[i][2];

        tmp[0][i] =   s03 +   s12;
        tmp[1][i] = 2*d03 +   d12;
        tmp[2][i] =   s03 -   s12;
        tmp[3][i] =   d03 - 2*d12;
    }

    for( i = 0; i < 4; i++ )
    {
        const int s03 = tmp[i][0] + tmp[i][3];
        const int s12 = tmp[i][1] + tmp[i][2];
        const int d03 = tmp[i][0] - tmp[i][3];
        const int d12 = tmp[i][1] - tmp[i][2];

        dct[0][i] =   s03 +   s12;
        dct[1][i] = 2*d03 +   d12;
        dct[2][i] =   s03 -   s12;
        dct[3][i] =   d03 - 2*d12;
    }
}

static void sub8x8_dct( int16_t dct[4][4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    sub4x4_dct( dct[0], &pix1[0], i_pix1, &pix2[0], i_pix2 );
    sub4x4_dct( dct[1], &pix1[4], i_pix1, &pix2[4], i_pix2 );
    sub4x4_dct( dct[2], &pix1[4*i_pix1+0], i_pix1, &pix2[4*i_pix2+0], i_pix2 );
    sub4x4_dct( dct[3], &pix1[4*i_pix1+4], i_pix1, &pix2[4*i_pix2+4], i_pix2 );
}

static void sub16x16_dct( int16_t dct[16][4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    sub8x8_dct( &dct[ 0], pix1, i_pix1, pix2, i_pix2 );
    sub8x8_dct( &dct[ 4], &pix1[8], i_pix1, &pix2[8], i_pix2 );
    sub8x8_dct( &dct[ 8], &pix1[8*i_pix1], i_pix1, &pix2[8*i_pix2], i_pix2 );
    sub8x8_dct( &dct[12], &pix1[8*i_pix1+8], i_pix1, &pix2[8*i_pix2+8], i_pix2 );
}


static void add4x4_idct( uint8_t *p_dst, int i_dst, int16_t dct[4][4] )
{
    int16_t d[4][4];
    int16_t tmp[4][4];
    int x, y;
    int i;

    for( i = 0; i < 4; i++ )
    {
        const int s02 =  dct[i][0]     +  dct[i][2];
        const int d02 =  dct[i][0]     -  dct[i][2];
        const int s13 =  dct[i][1]     + (dct[i][3]>>1);
        const int d13 = (dct[i][1]>>1) -  dct[i][3];

        tmp[i][0] = s02 + s13;
        tmp[i][1] = d02 + d13;
        tmp[i][2] = d02 - d13;
        tmp[i][3] = s02 - s13;
    }

    for( i = 0; i < 4; i++ )
    {
        const int s02 =  tmp[0][i]     +  tmp[2][i];
        const int d02 =  tmp[0][i]     -  tmp[2][i];
        const int s13 =  tmp[1][i]     + (tmp[3][i]>>1);
        const int d13 = (tmp[1][i]>>1) -   tmp[3][i];

        d[0][i] = ( s02 + s13 + 32 ) >> 6;
        d[1][i] = ( d02 + d13 + 32 ) >> 6;
        d[2][i] = ( d02 - d13 + 32 ) >> 6;
        d[3][i] = ( s02 - s13 + 32 ) >> 6;
    }


    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            p_dst[x] = clip_uint8( p_dst[x] + d[y][x] );
        }
        p_dst += i_dst;
    }
}

static void add8x8_idct( uint8_t *p_dst, int i_dst, int16_t dct[4][4][4] )
{
    add4x4_idct( p_dst, i_dst,             dct[0] );
    add4x4_idct( &p_dst[4], i_dst,         dct[1] );
    add4x4_idct( &p_dst[4*i_dst+0], i_dst, dct[2] );
    add4x4_idct( &p_dst[4*i_dst+4], i_dst, dct[3] );
}

static void add16x16_idct( uint8_t *p_dst, int i_dst, int16_t dct[16][4][4] )
{
    add8x8_idct( &p_dst[0], i_dst, &dct[0] );
    add8x8_idct( &p_dst[8], i_dst, &dct[4] );
    add8x8_idct( &p_dst[8*i_dst], i_dst, &dct[8] );
    add8x8_idct( &p_dst[8*i_dst+8], i_dst, &dct[12] );
}



/****************************************************************************
 * x264_dct_init:
 ****************************************************************************/
void x264_dct_init( int cpu, x264_dct_function_t *dctf )
{
    dctf->sub4x4_dct    = sub4x4_dct;
    dctf->add4x4_idct   = add4x4_idct;

    dctf->sub8x8_dct    = sub8x8_dct;
    dctf->add8x8_idct   = add8x8_idct;

    dctf->sub16x16_dct    = sub16x16_dct;
    dctf->add16x16_idct   = add16x16_idct;

    dctf->dct4x4dc  = dct4x4dc;
    dctf->idct4x4dc = idct4x4dc;

    dctf->dct2x2dc  = dct2x2dc;
    dctf->idct2x2dc = dct2x2dc;

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT )
    {
        dctf->sub4x4_dct    = x264_sub4x4_dct_mmxext;
        dctf->sub8x8_dct    = x264_sub8x8_dct_mmxext;
        dctf->sub16x16_dct  = x264_sub16x16_dct_mmxext;

        dctf->add4x4_idct   = x264_add4x4_idct_mmxext;
        dctf->add8x8_idct   = x264_add8x8_idct_mmxext;
        dctf->add16x16_idct = x264_add16x16_idct_mmxext;

        dctf->dct4x4dc  = x264_dct4x4dc_mmxext;
        dctf->idct4x4dc = x264_idct4x4dc_mmxext;
    }
#endif
}

