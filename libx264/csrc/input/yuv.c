/*****************************************************************************
 * yuv.c: x264 yuv input module
 *****************************************************************************
 * Copyright (C) 2003-2009 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *****************************************************************************/

#include "muxers.h"

typedef struct
{
    FILE *fh;
    int width, height;
    int next_frame;
} yuv_hnd_t;

static int open_file( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param )
{
    yuv_hnd_t *h = malloc( sizeof(yuv_hnd_t) );
    if( !h )
        return -1;
    h->width = p_param->i_width;
    h->height = p_param->i_height;
    h->next_frame = 0;

    if( !strcmp( psz_filename, "-" ) )
        h->fh = stdin;
    else
        h->fh = fopen( psz_filename, "rb" );
    if( h->fh == NULL )
        return -1;

    *p_handle = h;
    return 0;
}

static int get_frame_total( hnd_t handle )
{
    yuv_hnd_t *h = handle;
    int i_frame_total = 0;

    if( x264_is_regular_file( h->fh ) )
    {
        fseek( h->fh, 0, SEEK_END );
        uint64_t i_size = ftell( h->fh );
        fseek( h->fh, 0, SEEK_SET );
        i_frame_total = (int)(i_size / ( h->width * h->height * 3 / 2 ));
    }

    return i_frame_total;
}

static int read_frame_internal( x264_picture_t *p_pic, yuv_hnd_t *h )
{
    return fread( p_pic->img.plane[0], h->width * h->height, 1, h->fh ) <= 0
        || fread( p_pic->img.plane[1], h->width * h->height / 4, 1, h->fh ) <= 0
        || fread( p_pic->img.plane[2], h->width * h->height / 4, 1, h->fh ) <= 0;
}

static int read_frame( x264_picture_t *p_pic, hnd_t handle, int i_frame )
{
    yuv_hnd_t *h = handle;

    if( i_frame > h->next_frame )
    {
        if( x264_is_regular_file( h->fh ) )
            fseek( h->fh, (uint64_t)i_frame * h->width * h->height * 3 / 2, SEEK_SET );
        else
            while( i_frame > h->next_frame )
            {
                if( read_frame_internal( p_pic, h ) )
                    return -1;
                h->next_frame++;
            }
    }

    if( read_frame_internal( p_pic, h ) )
        return -1;

    h->next_frame = i_frame+1;
    return 0;
}

static int close_file( hnd_t handle )
{
    yuv_hnd_t *h = handle;
    if( !h || !h->fh )
        return 0;
    fclose( h->fh );
    free( h );
    return 0;
}

cli_input_t yuv_input = { open_file, get_frame_total, read_frame, close_file };
