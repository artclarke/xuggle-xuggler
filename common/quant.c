/*****************************************************************************
 * quant.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2005 x264 project
 *
 * Authors: Christian Heine <sennindemokrit@gmx.net>
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

#include "common.h"

#ifdef HAVE_MMXEXT
#include "i386/quant.h"
#endif

#define QUANT_ONE( coef, mf ) \
{ \
    if( (coef) > 0 ) \
        (coef) = ( f + (coef) * (mf) ) >> i_qbits; \
    else \
        (coef) = - ( ( f - (coef) * (mf) ) >> i_qbits ); \
}

static void quant_8x8_core( int16_t dct[8][8], int quant_mf[8][8], int i_qbits, int f )
{
    int i;
    for( i = 0; i < 64; i++ )
        QUANT_ONE( dct[0][i], quant_mf[0][i] );
}

static void quant_4x4_core( int16_t dct[4][4], int quant_mf[4][4], int i_qbits, int f )
{
    int i;
    for( i = 0; i < 16; i++ )
        QUANT_ONE( dct[0][i], quant_mf[0][i] );
}

static void quant_4x4_dc_core( int16_t dct[4][4], int i_quant_mf, int i_qbits, int f )
{
    int i;
    for( i = 0; i < 16; i++ )
        QUANT_ONE( dct[0][i], i_quant_mf );
}

static void quant_2x2_dc_core( int16_t dct[2][2], int i_quant_mf, int i_qbits, int f )
{
    QUANT_ONE( dct[0][0], i_quant_mf );
    QUANT_ONE( dct[0][1], i_quant_mf );
    QUANT_ONE( dct[0][2], i_quant_mf );
    QUANT_ONE( dct[0][3], i_quant_mf );
}


void x264_quant_init( x264_t *h, int cpu, x264_quant_function_t *pf )
{
    int i, maxQ8=0, maxQ4=0, maxQdc=0;

    pf->quant_8x8_core = quant_8x8_core;
    pf->quant_4x4_core = quant_4x4_core;
    pf->quant_4x4_dc_core = quant_4x4_dc_core;
    pf->quant_2x2_dc_core = quant_2x2_dc_core;

#ifdef HAVE_MMXEXT

    /* determine the biggest coeffient in all quant8_mf tables */
    for( i = 0; i < 2*6*8*8; i++ )
    {
        int q = h->quant8_mf[0][0][0][i];
        if( maxQ8 < q )
            maxQ8 = q;
    }

    /* determine the biggest coeffient in all quant4_mf tables ( maxQ4 )
       and the biggest DC coefficient if all quant4_mf tables ( maxQdc ) */
    for( i = 0; i < 4*6*4*4; i++ )
    {
        int q = h->quant4_mf[0][0][0][i];
        if( maxQ4 < q )
            maxQ4 = q;
        if( maxQdc < q && i%16 == 0 )
            maxQdc = q;
    }

    /* select quant_8x8 based on CPU and maxQ8 */
    if( maxQ8 < (1<<15) && cpu&X264_CPU_MMX )
        pf->quant_8x8_core = x264_quant_8x8_core15_mmx;
    else
    if( maxQ8 < (1<<16) && cpu&X264_CPU_MMXEXT )
        pf->quant_8x8_core = x264_quant_8x8_core16_mmxext;
    else
    if( cpu&X264_CPU_MMXEXT )
        pf->quant_8x8_core = x264_quant_8x8_core32_mmxext;

    /* select quant_4x4 based on CPU and maxQ4 */
    if( maxQ4 < (1<<15) && cpu&X264_CPU_MMX )
        pf->quant_4x4_core = x264_quant_4x4_core15_mmx;
    else
    if( maxQ4 < (1<<16) && cpu&X264_CPU_MMXEXT )
        pf->quant_4x4_core = x264_quant_4x4_core16_mmxext;
    else
    if( cpu&X264_CPU_MMXEXT )
        pf->quant_4x4_core = x264_quant_4x4_core32_mmxext;

    /* select quant_XxX_dc based on CPU and maxQdc */
    if( maxQdc < (1<<16) && cpu&X264_CPU_MMXEXT )
    {
        pf->quant_4x4_dc_core = x264_quant_4x4_dc_core16_mmxext;
        pf->quant_2x2_dc_core = x264_quant_2x2_dc_core16_mmxext;
    }
    else
    if( maxQdc < (1<<15) && cpu&X264_CPU_MMX )
    {
        pf->quant_4x4_dc_core = x264_quant_4x4_dc_core15_mmx;
        pf->quant_2x2_dc_core = x264_quant_2x2_dc_core15_mmx;
    }
    else
    if( cpu&X264_CPU_MMXEXT )
    {
        pf->quant_4x4_dc_core = x264_quant_4x4_dc_core32_mmxext;
        pf->quant_2x2_dc_core = x264_quant_2x2_dc_core32_mmxext;
    }

#endif  /* HAVE_MMXEXT */
}
