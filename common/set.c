/*****************************************************************************
 * set.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2005 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
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
#include <stdio.h>
#include <string.h>

static const int dequant4_scale[6][3] =
{
    { 10, 13, 16 },
    { 11, 14, 18 },
    { 13, 16, 20 },
    { 14, 18, 23 },
    { 16, 20, 25 },
    { 18, 23, 29 }
};
static const int quant4_scale[6][3] =
{
    { 13107, 8066, 5243 },
    { 11916, 7490, 4660 },
    { 10082, 6554, 4194 },
    {  9362, 5825, 3647 },
    {  8192, 5243, 3355 },
    {  7282, 4559, 2893 },
};

static const int quant8_scan[16] =
{
    0,3,4,3, 3,1,5,1, 4,5,2,5, 3,1,5,1
};
static const int dequant8_scale[6][6] =
{
    { 20, 18, 32, 19, 25, 24 },
    { 22, 19, 35, 21, 28, 26 },
    { 26, 23, 42, 24, 33, 31 },
    { 28, 25, 45, 26, 35, 33 },
    { 32, 28, 51, 30, 40, 38 },
    { 36, 32, 58, 34, 46, 43 },
};
static const int quant8_scale[6][6] =
{
    { 13107, 11428, 20972, 12222, 16777, 15481 },
    { 11916, 10826, 19174, 11058, 14980, 14290 },
    { 10082,  8943, 15978,  9675, 12710, 11985 },
    {  9362,  8228, 14913,  8931, 11984, 11259 },
    {  8192,  7346, 13159,  7740, 10486,  9777 },
    {  7282,  6428, 11570,  6830,  9118,  8640 }
};

void x264_cqm_init( x264_t *h )
{
    int def_quant4[6][16];
    int def_quant8[6][64];
    int def_dequant4[6][16];
    int def_dequant8[6][64];
    int q, i, i_list;

    for( q = 0; q < 6; q++ )
    {
        for( i = 0; i < 16; i++ )
        {
            int j = (i&1) + ((i>>2)&1);
            def_dequant4[q][i] = dequant4_scale[q][j];
            def_quant4[q][i]   =   quant4_scale[q][j];
        }
        for( i = 0; i < 64; i++ )
        {
            int j = quant8_scan[((i>>1)&12) | (i&3)];
            def_dequant8[q][i] = dequant8_scale[q][j];
            def_quant8[q][i]   =   quant8_scale[q][j];
        }
    }

    for( q = 0; q < 6; q++ )
    {
        for( i_list = 0; i_list < 4; i_list++ )
            for( i = 0; i < 16; i++ )
            {
                h->dequant4_mf[i_list][q][0][i] = def_dequant4[q][i] * h->pps->scaling_list[i_list][i];
                h->  quant4_mf[i_list][q][0][i] = def_quant4[q][i] * 16 / h->pps->scaling_list[i_list][i];
            }
        for( i_list = 0; i_list < 2; i_list++ )
            for( i = 0; i < 64; i++ )
            {
                h->dequant8_mf[i_list][q][0][i] = def_dequant8[q][i] * h->pps->scaling_list[4+i_list][i];
                h->  quant8_mf[i_list][q][0][i] = def_quant8[q][i] * 16 / h->pps->scaling_list[4+i_list][i];
            }
    }
    for( q = 0; q < 52; q++ )
    {
        for( i_list = 0; i_list < 4; i_list++ )
            for( i = 0; i < 16; i++ )
                h->unquant4_mf[i_list][q][i] = (1 << (q/6 + 15 + 8)) / h->quant4_mf[i_list][q%6][0][i];
        for( i_list = 0; i_list < 2; i_list++ )
            for( i = 0; i < 64; i++ )
                h->unquant8_mf[i_list][q][i] = (1 << (q/6 + 16 + 8)) / h->quant8_mf[i_list][q%6][0][i];
    }
}

int x264_cqm_parse_jmlist( x264_t *h, const char *buf, const char *name,
                           uint8_t *cqm, const uint8_t *jvt, int length )
{
    char *p;
    char *nextvar;
    int i;

    p = strstr( buf, name );
    if( !p )
    {
        memset( cqm, 16, length );
        return 0;
    }

    p += strlen( name );
    if( *p == 'U' || *p == 'V' )
        p++;

    nextvar = strstr( p, "INT" );

    for( i = 0; i < length && (p = strpbrk( p, " \t\n," )) && (p = strpbrk( p, "0123456789" )); i++ )
    {
        int coef = -1;
        sscanf( p, "%d", &coef );
        if( i == 0 && coef == 0 )
        {
            memcpy( cqm, jvt, length );
            return 0;
        }
        if( coef < 1 || coef > 255 )
        {
            x264_log( h, X264_LOG_ERROR, "bad coefficient in list '%s'\n", name );
            return -1;
        }
        cqm[i] = coef;
    }

    if( (nextvar && p > nextvar) || i != length )
    {
        x264_log( h, X264_LOG_ERROR, "not enough coefficients in list '%s'\n", name );
        return -1;
    }

    return 0;
}

int x264_cqm_parse_file( x264_t *h, const char *filename )
{
    char *buf, *p;
    int b_error = 0;

    h->param.i_cqm_preset = X264_CQM_CUSTOM;
    
    buf = x264_slurp_file( filename );
    if( !buf )
    {
        x264_log( h, X264_LOG_ERROR, "can't open file '%s'\n", filename );
        return -1;
    }

    while( (p = strchr( buf, '#' )) != NULL )
        memset( p, ' ', strcspn( p, "\n" ) );

    b_error |= x264_cqm_parse_jmlist( h, buf, "INTRA4X4_LUMA",   h->param.cqm_4iy, x264_cqm_jvt4i, 16 );
    b_error |= x264_cqm_parse_jmlist( h, buf, "INTRA4X4_CHROMA", h->param.cqm_4ic, x264_cqm_jvt4i, 16 );
    b_error |= x264_cqm_parse_jmlist( h, buf, "INTER4X4_LUMA",   h->param.cqm_4py, x264_cqm_jvt4p, 16 );
    b_error |= x264_cqm_parse_jmlist( h, buf, "INTER4X4_CHROMA", h->param.cqm_4pc, x264_cqm_jvt4p, 16 );
    b_error |= x264_cqm_parse_jmlist( h, buf, "INTRA8X8_LUMA",   h->param.cqm_8iy, x264_cqm_jvt8i, 64 );
    b_error |= x264_cqm_parse_jmlist( h, buf, "INTER8X8_LUMA",   h->param.cqm_8py, x264_cqm_jvt8p, 64 );

    x264_free( buf );
    return b_error;
}

