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

void x264_quant_8x8_core16_mmx( int16_t dct[8][8], int quant_mf[8][8], int i_qbits, int f );
void x264_quant_4x4_core16_mmx( int16_t dct[4][4], int quant_mf[4][4], int i_qbits, int f );
void x264_quant_8x8_core32_mmx( int16_t dct[8][8], int quant_mf[8][8], int i_qbits, int f );
void x264_quant_4x4_core32_mmx( int16_t dct[4][4], int quant_mf[4][4], int i_qbits, int f );
void x264_quant_4x4_dc_core32_mmx( int16_t dct[4][4], int i_quant_mf, int i_qbits, int f );
void x264_quant_2x2_dc_core32_mmx( int16_t dct[2][2], int i_quant_mf, int i_qbits, int f );


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
    const char *name[4] = { "C", "C", "C", "C" };

    pf->quant_8x8_core = quant_8x8_core;
    pf->quant_4x4_core = quant_4x4_core;
    pf->quant_4x4_dc_core = quant_4x4_dc_core;
    pf->quant_2x2_dc_core = quant_2x2_dc_core;

#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMX )
    {
        int i;

        pf->quant_8x8_core = x264_quant_8x8_core16_mmx;
        pf->quant_4x4_core = x264_quant_4x4_core16_mmx;
        pf->quant_4x4_dc_core = x264_quant_4x4_dc_core32_mmx;
        pf->quant_2x2_dc_core = x264_quant_2x2_dc_core32_mmx;

        name[0] = name[1] = "16MMX";
        name[2] = name[3] = "32MMX";

        for( i = 0; i < 2*6*8*8; i++ )
            if( (***h->quant8_mf)[i] >= 0x8000 )
            {
                pf->quant_8x8_core = x264_quant_8x8_core32_mmx;
                name[0] = "32MMX";
            }

        for( i = 0; i < 4*6*4*4; i++ )
            if( (***h->quant4_mf)[i] >= 0x8000 )
            {
                pf->quant_4x4_core = x264_quant_4x4_core32_mmx;
                name[1] = "32MMX";
            }
    }
#endif

    x264_log( h, X264_LOG_DEBUG, "using quant functions 8x8=%s 4x4=%s dc4x4=%s dc2x2=%s\n",
              name[0], name[1], name[2], name[3] );
}
