/*****************************************************************************
 * y4m.c: y4m input
 *****************************************************************************
 * Copyright (C) 2003-2012 x264 project
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
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

#include "input.h"
#define FAIL_IF_ERROR( cond, ... ) FAIL_IF_ERR( cond, "y4m", __VA_ARGS__ )

typedef struct
{
    FILE *fh;
    int next_frame;
    int seq_header_len;
    int frame_header_len;
    uint64_t frame_size;
    uint64_t plane_size[3];
} y4m_hnd_t;

#define Y4M_MAGIC "YUV4MPEG2"
#define MAX_YUV4_HEADER 80
#define Y4M_FRAME_MAGIC "FRAME"
#define MAX_FRAME_HEADER 80

static int csp_string_to_int( char *csp_name )
{
    int csp = X264_CSP_MAX;
    if( !strncmp( "420", csp_name, 3 ) )
        csp = X264_CSP_I420;
    else if( !strncmp( "422", csp_name, 3 ) )
        csp = X264_CSP_I422;
    else if( !strncmp( "444", csp_name, 3 ) && strncmp( "444alpha", csp_name, 8 ) ) // only accept alphaless 4:4:4
        csp = X264_CSP_I444;
    return csp;
}

static int open_file( char *psz_filename, hnd_t *p_handle, video_info_t *info, cli_input_opt_t *opt )
{
    y4m_hnd_t *h = malloc( sizeof(y4m_hnd_t) );
    int i;
    uint32_t n, d;
    char header[MAX_YUV4_HEADER+10];
    char *tokend, *header_end;
    int colorspace = X264_CSP_NONE;
    int alt_colorspace = X264_CSP_NONE;
    if( !h )
        return -1;

    h->next_frame = 0;
    info->vfr = 0;

    if( !strcmp( psz_filename, "-" ) )
        h->fh = stdin;
    else
        h->fh = fopen(psz_filename, "rb");
    if( h->fh == NULL )
        return -1;

    h->frame_header_len = strlen( Y4M_FRAME_MAGIC )+1;

    /* Read header */
    for( i = 0; i < MAX_YUV4_HEADER; i++ )
    {
        header[i] = fgetc( h->fh );
        if( header[i] == '\n' )
        {
            /* Add a space after last option. Makes parsing "444" vs
               "444alpha" easier. */
            header[i+1] = 0x20;
            header[i+2] = 0;
            break;
        }
    }
    if( i == MAX_YUV4_HEADER || strncmp( header, Y4M_MAGIC, strlen( Y4M_MAGIC ) ) )
        return -1;

    /* Scan properties */
    header_end = &header[i+1]; /* Include space */
    h->seq_header_len = i+1;
    for( char *tokstart = &header[strlen( Y4M_MAGIC )+1]; tokstart < header_end; tokstart++ )
    {
        if( *tokstart == 0x20 )
            continue;
        switch( *tokstart++ )
        {
            case 'W': /* Width. Required. */
                info->width = strtol( tokstart, &tokend, 10 );
                tokstart=tokend;
                break;
            case 'H': /* Height. Required. */
                info->height = strtol( tokstart, &tokend, 10 );
                tokstart=tokend;
                break;
            case 'C': /* Color space */
                colorspace = csp_string_to_int( tokstart );
                tokstart = strchr( tokstart, 0x20 );
                break;
            case 'I': /* Interlace type */
                switch( *tokstart++ )
                {
                    case 't':
                        info->interlaced = 1;
                        info->tff = 1;
                        break;
                    case 'b':
                        info->interlaced = 1;
                        info->tff = 0;
                        break;
                    case 'm':
                        info->interlaced = 1;
                        break;
                    //case '?':
                    //case 'p':
                    default:
                        break;
                }
                break;
            case 'F': /* Frame rate - 0:0 if unknown */
                if( sscanf( tokstart, "%u:%u", &n, &d ) == 2 && n && d )
                {
                    x264_reduce_fraction( &n, &d );
                    info->fps_num = n;
                    info->fps_den = d;
                }
                tokstart = strchr( tokstart, 0x20 );
                break;
            case 'A': /* Pixel aspect - 0:0 if unknown */
                /* Don't override the aspect ratio if sar has been explicitly set on the commandline. */
                if( sscanf( tokstart, "%u:%u", &n, &d ) == 2 && n && d )
                {
                    x264_reduce_fraction( &n, &d );
                    info->sar_width  = n;
                    info->sar_height = d;
                }
                tokstart = strchr( tokstart, 0x20 );
                break;
            case 'X': /* Vendor extensions */
                if( !strncmp( "YSCSS=", tokstart, 6 ) )
                {
                    /* Older nonstandard pixel format representation */
                    tokstart += 6;
                    alt_colorspace = csp_string_to_int( tokstart );
                }
                tokstart = strchr( tokstart, 0x20 );
                break;
        }
    }

    if( colorspace == X264_CSP_NONE )
        colorspace = alt_colorspace;

    // default to 4:2:0 if nothing is specified
    if( colorspace == X264_CSP_NONE )
        colorspace = X264_CSP_I420;

    FAIL_IF_ERROR( colorspace <= X264_CSP_NONE || colorspace >= X264_CSP_MAX, "colorspace unhandled\n" )

    info->thread_safe = 1;
    info->num_frames  = 0;
    info->csp         = colorspace;
    h->frame_size     = h->frame_header_len;
    for( i = 0; i < x264_cli_csps[info->csp].planes; i++ )
    {
        h->plane_size[i] = x264_cli_pic_plane_size( info->csp, info->width, info->height, i );
        h->frame_size += h->plane_size[i];
    }

    /* Most common case: frame_header = "FRAME" */
    if( x264_is_regular_file( h->fh ) )
    {
        uint64_t init_pos = ftell( h->fh );
        fseek( h->fh, 0, SEEK_END );
        uint64_t i_size = ftell( h->fh );
        fseek( h->fh, init_pos, SEEK_SET );
        info->num_frames = (i_size - h->seq_header_len) / h->frame_size;
    }

    *p_handle = h;
    return 0;
}

static int read_frame_internal( cli_pic_t *pic, y4m_hnd_t *h )
{
    size_t slen = strlen( Y4M_FRAME_MAGIC );
    int i = 0;
    char header[16];

    /* Read frame header - without terminating '\n' */
    if( fread( header, 1, slen, h->fh ) != slen )
        return -1;

    header[slen] = 0;
    FAIL_IF_ERROR( strncmp( header, Y4M_FRAME_MAGIC, slen ), "bad header magic (%"PRIx32" <=> %s)\n",
                   M32(header), header )

    /* Skip most of it */
    while( i < MAX_FRAME_HEADER && fgetc( h->fh ) != '\n' )
        i++;
    FAIL_IF_ERROR( i == MAX_FRAME_HEADER, "bad frame header!\n" )
    h->frame_size = h->frame_size - h->frame_header_len + i+slen+1;
    h->frame_header_len = i+slen+1;

    int error = 0;
    for( i = 0; i < pic->img.planes && !error; i++ )
        error |= fread( pic->img.plane[i], h->plane_size[i], 1, h->fh ) <= 0;
    return error;
}

static int read_frame( cli_pic_t *pic, hnd_t handle, int i_frame )
{
    y4m_hnd_t *h = handle;

    if( i_frame > h->next_frame )
    {
        if( x264_is_regular_file( h->fh ) )
            fseek( h->fh, h->frame_size * i_frame + h->seq_header_len, SEEK_SET );
        else
            while( i_frame > h->next_frame )
            {
                if( read_frame_internal( pic, h ) )
                    return -1;
                h->next_frame++;
            }
    }

    if( read_frame_internal( pic, h ) )
        return -1;

    h->next_frame = i_frame+1;
    return 0;
}

static int close_file( hnd_t handle )
{
    y4m_hnd_t *h = handle;
    if( !h || !h->fh )
        return 0;
    fclose( h->fh );
    free( h );
    return 0;
}

const cli_input_t y4m_input = { open_file, x264_cli_pic_alloc, read_frame, NULL, x264_cli_pic_clean, close_file };
