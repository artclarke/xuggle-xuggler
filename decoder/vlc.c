/*****************************************************************************
 * vlc.c: VLC lookup table generation.
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: vlc.c,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "common/common.h"
#include "common/vlc.h"
#include "vlc.h"


static int  vlc_table_realloc( x264_vlc_table_t *table, int i_size )
{
    int i_index;

    i_index = table->i_lookup;

    table->i_lookup += i_size;
    table->lookup = x264_realloc( table->lookup, sizeof( vlc_lookup_t ) * table->i_lookup );

    return( i_index );
}

static int vlc_table_create_part( x264_vlc_table_t *table, const vlc_t *vlc, int i_lookup_bits, int i_nb_vlc, int i_prefix_code, int i_prefix_length )
{
    int i;
    int i_nb_lookup;
    vlc_lookup_t *lookup;
    int i_table_index;

    i_nb_lookup = 1 << i_lookup_bits;

    i_table_index = vlc_table_realloc( table, i_nb_lookup );
    lookup = &table->lookup[i_table_index];

    for( i = 0; i < i_nb_lookup; i++ )
    {
        lookup[i].i_value  = -1;
        lookup[i].i_size = 0;
    }

    for( i = 0; i < i_nb_vlc; i++ )
    {
        int i_bits;
        if( vlc[i].i_size <= 0 )
        {
            continue;
        }

        i_bits = vlc[i].i_size - i_prefix_length;
        if( i_bits > 0 && ( vlc[i].i_bits >> i_bits ) == i_prefix_code )
        {
            if( i_bits <= i_lookup_bits )
            {
                int i_lookup_index;
                int nb;

                i_lookup_index = ( vlc[i].i_bits << ( i_lookup_bits - i_bits ) )%i_nb_lookup;
                nb = 1 << ( i_lookup_bits - i_bits );
                for( nb = 0; nb < (1 << ( i_lookup_bits - i_bits)); nb++ )
                {
                    lookup[i_lookup_index].i_value = i; /* vlc[i].i_value; */
                    lookup[i_lookup_index].i_size = i_bits;
                    i_lookup_index++;
                }
            }
            else
            {
                int i_bits_max;
                int i_lookup_index;
                /* need another table */
                i_lookup_index = ( vlc[i].i_bits >> (i_bits - i_lookup_bits ) )%i_nb_lookup;

                i_bits_max =  -lookup[i_lookup_index].i_size;
                if( i_bits_max < i_bits - i_lookup_bits )
                {
                    i_bits_max = i_bits - i_lookup_bits;
                }
                lookup[i_lookup_index].i_size = -i_bits_max;
            }
        }
    }

    /* create other level table */
    for( i = 0; i < i_nb_lookup; i++ )
    {
        if( lookup[i].i_size < 0 )
        {
            int i_bits;
            int i_index;
            i_bits = -lookup[i].i_size;
            if( i_bits > i_lookup_bits )
            {
                lookup[i].i_size = -i_lookup_bits;
                i_bits = i_lookup_bits;
            }

            i_index = vlc_table_create_part( table, vlc, i_bits, i_nb_vlc,
                                             (i_prefix_code << i_lookup_bits)|i,
                                              i_lookup_bits+i_prefix_length );
            lookup = &table->lookup[i_table_index]; // reallocated
            lookup[i].i_value = i_index;
        }
    }

    return( i_table_index );
}


x264_vlc_table_t *x264_vlc_table_lookup_new( const vlc_t *vlc, int i_vlc, int i_lookup_bits )
{
    x264_vlc_table_t *table = x264_malloc( sizeof( x264_vlc_table_t ) );

    table->i_lookup_bits = i_lookup_bits;
    table->i_lookup = 0;
    table->lookup   = NULL;

    vlc_table_create_part( table, vlc, i_lookup_bits, i_vlc, 0, 0 );

    return table;
}

void x264_vlc_table_lookup_delete( x264_vlc_table_t *table )
{
    x264_free( table->lookup );
    x264_free( table );
}

#if 0
void x264_vlc_table_lookup_print( x264_vlc_table_t *table )
{
    int idx;

    fprintf( stderr, "       " );
    for( idx = 0; idx < table->i_lookup; idx++ )
    {
        if( table->lookup[idx].i_value == -1 )
        {
            fprintf( stderr, " MKVLCLU(    -1,  0 )," );
        }
        else
        {
            fprintf( stderr, " MKVLCLU( 0x%.3x, % 2d ),", table->lookup[idx].i_value, table->lookup[idx].i_size );
        }
        if( (idx+1)%4 == 0 && idx < table->i_lookup - 1)
        {
            fprintf( stderr, "\n       " );
        }
    }
    fprintf( stderr, "\n" );
}

int main(void)
{
    int i;
    x264_vlc_table_t *table;


    printf( "typedef struct\n    int i_value;\n    int i_size;\n} vlc_lookup_t;\n\n#define MKVLCLU(a,b) { .i_value=a, .i_size=b}" );

    /* create vlc  entry table and then vlc_lookup_t table */

    /* x264_coeff_token */
    fprintf( stderr, "static const vlc_lookup_t x264_coeff_token_lookup[5][]=\n{\n" );
    for( i = 0; i < 5; i++ )
    {
        fprintf( stderr, "    {\n" );
        table = x264_vlc_table_lookup_new( x264_coeff_token[i], 17*4, 6 );
        x264_vlc_table_lookup_print( table );
        x264_vlc_table_lookup_delete( table );
        fprintf( stderr, "    },\n" );
    }
    fprintf( stderr, "};\n" );

#if 0

    vlce = convert_vlc_to_vlce( x264_level_prefix, 16 );
    do_vlc_table_create( vlce, 16, "x264_level_prefix_lookup", 8 );
    free( vlce );

    for( i_table = 0; i_table < 15; i_table++ )
    {
        char name[512];
        vlce = convert_vlc_to_vlce( x264_total_zeros[i_table], 16 );
        sprintf( name, "x264_total_zeros_%d", i_table );
        do_vlc_table_create( vlce, 16, name, 6 );

        free( vlce );
    }

    for( i_table = 0; i_table < 3; i_table++ )
    {
        char name[512];

        vlce = convert_vlc_to_vlce( x264_total_zeros_dc[i_table], 4 );
        sprintf( name, "x264_total_zeros_dc_%d", i_table );
        do_vlc_table_create( vlce, 4, name, 3 );

        free( vlce );
    }

    for( i_table = 0; i_table < 7; i_table++ )
    {
        char name[512];
        vlce = convert_vlc_to_vlce( x264_run_before[i_table], 15 );
        sprintf( name, "x264_run_before_%d", i_table );
        do_vlc_table_create( vlce, 15, name, 6 );

        free( vlce );
    }
#endif
    return 0;
}

#endif
