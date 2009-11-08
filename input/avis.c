/*****************************************************************************
 * avis.c: x264 avi/avs input module
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
#include <windows.h>
#include <vfw.h>

typedef struct
{
    PAVISTREAM p_avi;
    int width, height;
} avis_hnd_t;

static int open_file( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param )
{
    FILE *fh = fopen( psz_filename, "r" );
    if( !fh )
        return -1;
    else if( !x264_is_regular_file( fh ) )
    {
        fprintf( stderr, "avis [error]: AVIS input is incompatible with non-regular file `%s'\n", psz_filename );
        return -1;
    }
    fclose( fh );

    avis_hnd_t *h = malloc( sizeof(avis_hnd_t) );
    if( !h )
        return -1;
    AVISTREAMINFO info;
    int i;

    *p_handle = h;

    AVIFileInit();
    if( AVIStreamOpenFromFile( &h->p_avi, psz_filename, streamtypeVIDEO, 0, OF_READ, NULL ) )
    {
        AVIFileExit();
        return -1;
    }

    if( AVIStreamInfo( h->p_avi, &info, sizeof(AVISTREAMINFO) ) )
    {
        AVIStreamRelease( h->p_avi );
        AVIFileExit();
        return -1;
    }

    // check input format
    if( info.fccHandler != MAKEFOURCC('Y', 'V', '1', '2') )
    {
        fprintf( stderr, "avis [error]: unsupported input format (%c%c%c%c)\n",
            (char)(info.fccHandler & 0xff), (char)((info.fccHandler >> 8) & 0xff),
            (char)((info.fccHandler >> 16) & 0xff), (char)((info.fccHandler >> 24)) );

        AVIStreamRelease( h->p_avi );
        AVIFileExit();

        return -1;
    }

    h->width =
    p_param->i_width = info.rcFrame.right - info.rcFrame.left;
    h->height =
    p_param->i_height = info.rcFrame.bottom - info.rcFrame.top;
    i = gcd( info.dwRate, info.dwScale );
    p_param->i_fps_den = info.dwScale / i;
    p_param->i_fps_num = info.dwRate / i;

    fprintf( stderr, "avis [info]: %dx%d @ %.2f fps (%d frames)\n",
             p_param->i_width, p_param->i_height,
             (double)p_param->i_fps_num / (double)p_param->i_fps_den,
             (int)info.dwLength );

    return 0;
}

static int get_frame_total( hnd_t handle )
{
    avis_hnd_t *h = handle;
    AVISTREAMINFO info;

    if( AVIStreamInfo( h->p_avi, &info, sizeof(AVISTREAMINFO) ) )
        return -1;

    return info.dwLength;
}

static int read_frame( x264_picture_t *p_pic, hnd_t handle, int i_frame )
{
    avis_hnd_t *h = handle;

    p_pic->img.i_csp = X264_CSP_YV12;

    if( AVIStreamRead( h->p_avi, i_frame, 1, p_pic->img.plane[0], h->width * h->height * 3 / 2, NULL, NULL ) )
        return -1;

    return 0;
}

static int close_file( hnd_t handle )
{
    avis_hnd_t *h = handle;
    AVIStreamRelease( h->p_avi );
    AVIFileExit();
    free( h );
    return 0;
}

cli_input_t avis_input = { open_file, get_frame_total, read_frame, close_file };
