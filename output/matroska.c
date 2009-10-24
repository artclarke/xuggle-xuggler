/*****************************************************************************
 * matroska.c: x264 matroska output module
 *****************************************************************************
 * Copyright (C) 2005 Mike Matsnev
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
#include "matroska_ebml.h"

typedef struct
{
    mk_writer *w;

    uint8_t *sps, *pps;
    int sps_len, pps_len;

    int width, height, d_width, d_height;

    int64_t frame_duration;
    int fps_num;

    int b_header_written;
    char b_writing_frame;
} mkv_hnd_t;

static int write_header( mkv_hnd_t *p_mkv )
{
    int ret;
    uint8_t *avcC;
    int avcC_len;

    if( !p_mkv->sps || !p_mkv->pps ||
        !p_mkv->width || !p_mkv->height ||
        !p_mkv->d_width || !p_mkv->d_height )
        return -1;

    avcC_len = 5 + 1 + 2 + p_mkv->sps_len + 1 + 2 + p_mkv->pps_len;
    avcC = malloc( avcC_len );
    if( !avcC )
        return -1;

    avcC[0] = 1;
    avcC[1] = p_mkv->sps[1];
    avcC[2] = p_mkv->sps[2];
    avcC[3] = p_mkv->sps[3];
    avcC[4] = 0xff; // nalu size length is four bytes
    avcC[5] = 0xe1; // one sps

    avcC[6] = p_mkv->sps_len >> 8;
    avcC[7] = p_mkv->sps_len;

    memcpy( avcC+8, p_mkv->sps, p_mkv->sps_len );

    avcC[8+p_mkv->sps_len] = 1; // one pps
    avcC[9+p_mkv->sps_len] = p_mkv->pps_len >> 8;
    avcC[10+p_mkv->sps_len] = p_mkv->pps_len;

    memcpy( avcC+11+p_mkv->sps_len, p_mkv->pps, p_mkv->pps_len );

    ret = mk_writeHeader( p_mkv->w, "x264", "V_MPEG4/ISO/AVC",
                          avcC, avcC_len, p_mkv->frame_duration, 50000,
                          p_mkv->width, p_mkv->height,
                          p_mkv->d_width, p_mkv->d_height );

    free( avcC );

    p_mkv->b_header_written = 1;

    return ret;
}

static int open_file( char *psz_filename, hnd_t *p_handle )
{
    mkv_hnd_t *p_mkv;

    *p_handle = NULL;

    p_mkv  = malloc( sizeof(*p_mkv) );
    if( !p_mkv )
        return -1;

    memset( p_mkv, 0, sizeof(*p_mkv) );

    p_mkv->w = mk_create_writer( psz_filename );
    if( !p_mkv->w )
    {
        free( p_mkv );
        return -1;
    }

    *p_handle = p_mkv;

    return 0;
}

static int set_param( hnd_t handle, x264_param_t *p_param )
{
    mkv_hnd_t   *p_mkv = handle;
    int64_t dw, dh;

    if( p_param->i_fps_num > 0 )
    {
        p_mkv->frame_duration = (int64_t)p_param->i_fps_den *
                                (int64_t)1000000000 / p_param->i_fps_num;
        p_mkv->fps_num = p_param->i_fps_num;
    }
    else
    {
        p_mkv->frame_duration = 0;
        p_mkv->fps_num = 1;
    }

    p_mkv->width = p_param->i_width;
    p_mkv->height = p_param->i_height;

    if( p_param->vui.i_sar_width && p_param->vui.i_sar_height )
    {
        dw = (int64_t)p_param->i_width  * p_param->vui.i_sar_width;
        dh = (int64_t)p_param->i_height * p_param->vui.i_sar_height;
    }
    else
    {
        dw = p_param->i_width;
        dh = p_param->i_height;
    }

    if( dw > 0 && dh > 0 )
    {
        int64_t x = gcd( dw, dh );
        dw /= x;
        dh /= x;
    }

    p_mkv->d_width = (int)dw;
    p_mkv->d_height = (int)dh;

    return 0;
}

static int write_nalu( hnd_t handle, uint8_t *p_nalu, int i_size )
{
    mkv_hnd_t *p_mkv = handle;
    uint8_t type = p_nalu[4] & 0x1f;
    uint8_t dsize[4];
    int psize;

    switch( type )
    {
        // sps
        case 0x07:
            if( !p_mkv->sps )
            {
                p_mkv->sps = malloc( i_size - 4 );
                if( !p_mkv->sps )
                    return -1;
                p_mkv->sps_len = i_size - 4;
                memcpy( p_mkv->sps, p_nalu + 4, i_size - 4 );
            }
            break;

        // pps
        case 0x08:
            if( !p_mkv->pps )
            {
                p_mkv->pps = malloc( i_size - 4 );
                if( !p_mkv->pps )
                    return -1;
                p_mkv->pps_len = i_size - 4;
                memcpy( p_mkv->pps, p_nalu + 4, i_size - 4 );
            }
            break;

        // slice, sei
        case 0x1:
        case 0x5:
        case 0x6:
            if( !p_mkv->b_writing_frame )
            {
                if( mk_start_frame( p_mkv->w ) < 0 )
                    return -1;
                p_mkv->b_writing_frame = 1;
            }
            psize = i_size - 4;
            dsize[0] = psize >> 24;
            dsize[1] = psize >> 16;
            dsize[2] = psize >> 8;
            dsize[3] = psize;
            if( mk_add_frame_data( p_mkv->w, dsize, 4 ) < 0 ||
                mk_add_frame_data( p_mkv->w, p_nalu + 4, i_size - 4 ) < 0 )
                return -1;
            break;

        default:
            break;
    }

    if( !p_mkv->b_header_written && p_mkv->pps && p_mkv->sps &&
        write_header( p_mkv ) < 0 )
        return -1;

    return i_size;
}

static int set_eop( hnd_t handle, x264_picture_t *p_picture )
{
    mkv_hnd_t *p_mkv = handle;
    int64_t i_stamp = (int64_t)(p_picture->i_pts * 1e9 / p_mkv->fps_num);

    p_mkv->b_writing_frame = 0;

    return mk_set_frame_flags( p_mkv->w, i_stamp, p_picture->i_type == X264_TYPE_IDR );
}

static int close_file( hnd_t handle )
{
    mkv_hnd_t *p_mkv = handle;
    int ret;

    if( p_mkv->sps )
        free( p_mkv->sps );
    if( p_mkv->pps )
        free( p_mkv->pps );

    ret = mk_close( p_mkv->w );

    free( p_mkv );

    return ret;
}

cli_output_t mkv_output = { open_file, set_param, write_nalu, set_eop, close_file };
