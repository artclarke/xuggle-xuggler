/*****************************************************************************
 * codec.c: vfw x264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: codec.c,v 1.1 2004/06/03 19:27:09 fenrir Exp $
 *
 * Authors: Justin Clay
 *          Laurent Aimar <fenrir@via.ecp.fr>
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

#include "x264vfw.h"

#include <stdio.h> /* debug only */
#include <io.h>

#define X264_MAX(a,b) ( (a)>(b) ? (a) : (b) )
#define X264_MIN(a,b) ( (a)<(b) ? (a) : (b) )

/* get_csp:
 *  return a valid x264 CSP or X264_CSP_NULL if unsuported */
static int get_csp( BITMAPINFOHEADER *hdr )
{
    switch( hdr->biCompression )
    {
        case FOURCC_I420:
        case FOURCC_IYUV:
            return X264_CSP_I420;

        case FOURCC_YV12:
            return X264_CSP_YV12;

        case FOURCC_YUYV:
        case FOURCC_YUY2:
            return X264_CSP_YUYV;

        case BI_RGB:
        {
            int i_vflip = hdr->biHeight < 0 ? 0 : X264_CSP_VFLIP;

            if( hdr->biBitCount == 24 )
                return X264_CSP_BGR | i_vflip;
            if( hdr->biBitCount == 32 )
                return X264_CSP_BGRA | i_vflip;
            else
                return X264_CSP_NONE;
        }

        default:
            return X264_CSP_NONE;
    }
}

/* Test that we can do the compression */
LRESULT compress_query( CODEC *codec, BITMAPINFO *lpbiInput, BITMAPINFO *lpbiOutput )
{
    BITMAPINFOHEADER *inhdr = &lpbiInput->bmiHeader;
    BITMAPINFOHEADER *outhdr = &lpbiOutput->bmiHeader;
    CONFIG           *config = &codec->config;

    if( get_csp( inhdr ) == X264_CSP_NONE )
        return ICERR_BADFORMAT;

    if( lpbiOutput == NULL )
        return ICERR_OK;

    if( inhdr->biWidth != outhdr->biWidth ||
        inhdr->biHeight != outhdr->biHeight )
        return ICERR_BADFORMAT;

    /* We need x16 width/height */
    if( inhdr->biWidth % 16 != 0 || inhdr->biHeight % 16 != 0 )
        return ICERR_BADFORMAT;


    if( inhdr->biCompression != mmioFOURCC( config->fcc[0], config->fcc[1],
                                            config->fcc[2], config->fcc[3] ) )
        return ICERR_BADFORMAT;

    return ICERR_OK;
}

/* */
LRESULT compress_get_format( CODEC *codec, BITMAPINFO *lpbiInput, BITMAPINFO *lpbiOutput )
{
    BITMAPINFOHEADER *inhdr = &lpbiInput->bmiHeader;
    BITMAPINFOHEADER *outhdr = &lpbiOutput->bmiHeader;
    CONFIG           *config = &codec->config;

    if( get_csp( inhdr ) == X264_CSP_NONE )
        return ICERR_BADFORMAT;

    if( lpbiOutput == NULL )
        return sizeof(BITMAPINFOHEADER);

    memcpy( outhdr, inhdr, sizeof( BITMAPINFOHEADER ) );
    outhdr->biSize = sizeof( BITMAPINFOHEADER );
    outhdr->biSizeImage = compress_get_size( codec, lpbiInput, lpbiOutput );
    outhdr->biXPelsPerMeter = 0;
    outhdr->biYPelsPerMeter = 0;
    outhdr->biClrUsed = 0;
    outhdr->biClrImportant = 0;
    outhdr->biCompression = mmioFOURCC( config->fcc[0], config->fcc[1],
                                        config->fcc[2], config->fcc[3] );

    return ICERR_OK;
}

/* */
LRESULT compress_get_size( CODEC *codec, BITMAPINFO *lpbiInput, BITMAPINFO *lpbiOutput )
{
    return 2 * lpbiOutput->bmiHeader.biWidth * lpbiOutput->bmiHeader.biHeight * 3;
}

/* */
LRESULT compress_frames_info(CODEC * codec, ICCOMPRESSFRAMES * icf )
{
    codec->fincr = icf->dwScale;
    codec->fbase = icf->dwRate;
    codec->config.i_frame_total = icf->lFrameCount;
    return ICERR_OK;
}

static void x264_log_vfw( void *p_private, int i_level, const char *psz_fmt, va_list arg )
{
    char error_msg[1024];
    int idx;
    HWND *hCons = p_private;

    vsprintf( error_msg, psz_fmt, arg );

    /* strip final linefeeds (required) */
    idx=strlen( error_msg ) - 1;
    while( idx >= 0 && error_msg[idx] == '\n' )
        error_msg[idx--] = 0;

    if(!( *hCons ) ) {
        *hCons = CreateDialog( g_hInst, MAKEINTRESOURCE( IDD_ERRCONSOLE ), NULL,
                 callback_err_console );
        //ShowWindow( *hCons, SW_SHOW );
    }
    idx = SendDlgItemMessage( *hCons, IDC_CONSOLE, LB_ADDSTRING, 0, ( LPARAM )error_msg );

    /* make sure that the last item added is visible (autoscroll) */
    if( idx >= 0 )
        SendDlgItemMessage( *hCons, IDC_CONSOLE, LB_SETTOPINDEX, ( WPARAM )idx, 0 );

}

static void statsfilename_renumber( char *dest, char *src, int i_pass )
{
    char *last_dot = strrchr( src, '.' );
    char *last_slash = X264_MAX( strrchr( src, '/' ), strrchr( src, '\\' ) );
    char pass_str[5];

    sprintf( pass_str, "-%i", i_pass );
    strcpy( dest, src );
    if( last_slash < last_dot ) {
        dest[ last_dot - src ] = 0;
        strcat( dest, pass_str );
        strcat( dest, last_dot );
    }
    else
    {
        strcat( dest, pass_str );
    }
}

/* */
LRESULT compress_begin(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput )
{
    CONFIG *config = &codec->config;
    x264_param_t param;
    int pass_number;

    /* Destroy previous handle */
    if( codec->h != NULL )
    {
        x264_encoder_close( codec->h );
        codec->h = NULL;
    }

    /* Get default param */
    x264_param_default( &param );

    param.rc.psz_stat_out = malloc (MAX_PATH);
    param.rc.psz_stat_in = malloc (MAX_PATH);
    param.i_threads = config->i_threads;
    param.analyse.i_noise_reduction = config->i_noise_reduction;

    param.i_log_level = config->i_log_level - 1;
    param.pf_log = x264_log_vfw;
    param.p_log_private = malloc( sizeof( HWND ) );
    *( ( HWND * )param.p_log_private ) = NULL; /* error console window handle */
    codec->hCons = ( HWND * )param.p_log_private;

    param.analyse.b_psnr = 0;

    /* Set params: TODO to complete */
    param.i_width = lpbiInput->bmiHeader.biWidth;
    param.i_height= lpbiInput->bmiHeader.biHeight;

    param.i_fps_num = codec->fbase;
    param.i_fps_den = codec->fincr;
    param.i_frame_total = config->i_frame_total;

    param.i_frame_reference = config->i_refmax;
    param.i_keyint_min = config->i_keyint_min;
    param.i_keyint_max = config->i_keyint_max;
    param.i_scenecut_threshold = config->i_scenecut_threshold;
    param.rc.i_qp_min = config->i_qp_min;
    param.rc.i_qp_max = config->i_qp_max;
    param.rc.i_qp_step = config->i_qp_step;
    param.b_deblocking_filter = config->b_filter;
    param.b_cabac = config->b_cabac;
    if( config->b_cabac && config->i_trellis )
        param.analyse.i_trellis = 1;

    param.analyse.b_chroma_me = config->b_chroma_me;
    param.rc.f_ip_factor = 1 + (float)config->i_key_boost / 100;
    param.rc.f_pb_factor = 1 + (float)config->i_b_red / 100;
    param.rc.f_qcompress = (float)config->i_curve_comp / 100;
    param.vui.i_sar_width = config->i_sar_width;
    param.vui.i_sar_height = config->i_sar_height;

    param.i_bframe = config->i_bframe;
    if( config->i_bframe > 1 && config->b_b_wpred)
        param.analyse.b_weighted_bipred = 1;
    if( config->i_bframe > 1 && config->b_b_refs)
        param.b_bframe_pyramid = 1;
    param.b_bframe_adaptive = config->b_bframe_adaptive;
    param.analyse.b_bidir_me = config->b_bidir_me;
    param.i_bframe_bias = config->i_bframe_bias;
    param.analyse.i_subpel_refine = config->i_subpel_refine + 1; /* 0..5 -> 1..6 */
    if (param.analyse.i_subpel_refine == 7)
    {
        param.analyse.i_subpel_refine = 6;
        param.analyse.b_bframe_rdo = 1;
    }

    param.analyse.i_me_method = config->i_me_method;
    param.analyse.i_me_range = config->i_me_range;

    /* bframe prediction - gui goes alphabetically, so 1=SPATIAL, 2=TEMPORAL */
    switch(config->i_direct_mv_pred) {
        case 0: param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_SPATIAL; break;
        case 1: param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_TEMPORAL; break;
    }
    param.i_deblocking_filter_alphac0 = config->i_inloop_a;
    param.i_deblocking_filter_beta = config->i_inloop_b;
    param.analyse.inter = 0;
    if( config->b_bsub16x16 )
        param.analyse.inter |= X264_ANALYSE_BSUB16x16;
    if( config->b_psub16x16 )
    {
        param.analyse.inter |= X264_ANALYSE_PSUB16x16;
        if( config->b_psub8x8 )
            param.analyse.inter |= X264_ANALYSE_PSUB8x8;
    }
    if( config->b_i4x4 )
        param.analyse.inter |= X264_ANALYSE_I4x4;
    if( config->b_i8x8 )
        param.analyse.inter |= X264_ANALYSE_I8x8;
    param.analyse.b_transform_8x8 = config->b_dct8x8;
    if( config->b_mixedref )
        param.analyse.b_mixed_references = 1;


    switch( config->i_encoding_type )
    {
        case 0: /* 1 PASS ABR */
            param.rc.b_cbr = 1;
            param.rc.i_bitrate = config->bitrate;
            break;
        case 1: /* 1 PASS CQ */
            param.rc.i_qp_constant = config->i_qp;
            break;
        default:
        case 2: /* 2 PASS */
        {
            for( pass_number = 1; pass_number < 99; pass_number++ )
            {
                FILE *f;
                statsfilename_renumber( param.rc.psz_stat_out, config->stats, pass_number );
                if( ( f = fopen( param.rc.psz_stat_out, "r" ) ) != NULL )
                {
                    fclose( f );
                    if( config->i_pass == 1 )
                        unlink( param.rc.psz_stat_out );
                }
                else break;
            }

            if( config->i_pass > pass_number )
            {
                /* missing 1st pass statsfile */
                free( param.rc.psz_stat_out );
                free( param.rc.psz_stat_in );
                return ICERR_ERROR;
            }

            param.rc.i_bitrate = config->i_2passbitrate;
            param.rc.b_cbr = 1;

            if( config->i_pass == 1 )
            {
                statsfilename_renumber( param.rc.psz_stat_out, config->stats, 1 );
                param.rc.b_stat_write = 1;
                param.rc.f_rate_tolerance = 4;
                if( config->b_fast1pass )
                {
                    /* adjust or turn off some flags to gain speed, if needed */
                    param.analyse.i_subpel_refine = X264_MAX( X264_MIN( 3, param.analyse.i_subpel_refine - 1 ), 1 );
                    param.i_frame_reference = ( param.i_frame_reference + 1 ) >> 1;
                    param.analyse.inter &= ( ~X264_ANALYSE_PSUB8x8 );
                    param.analyse.inter &= ( ~X264_ANALYSE_BSUB16x16 );
                }
            }
            else
            {
                statsfilename_renumber( param.rc.psz_stat_in, config->stats, pass_number - 1 );
                param.rc.b_stat_read = 1;
                if( config->b_updatestats )
                    param.rc.b_stat_write = 1;
                param.rc.f_rate_tolerance = 1;
            }

            break;
        }
    }

    /* Open the encoder */
    codec->h = x264_encoder_open( &param );

    free( param.rc.psz_stat_out );
    free( param.rc.psz_stat_in );

    if( codec->h == NULL )
        return ICERR_ERROR;

    return ICERR_OK;
}

/* */
LRESULT compress_end(CODEC * codec)
{
    if( codec->h != NULL )
    {
        x264_encoder_close( codec->h );
        codec->h = NULL;
    }

    free( codec->hCons );
    codec->hCons = NULL;
    return ICERR_OK;
}

/* */
LRESULT compress( CODEC *codec, ICCOMPRESS *icc )
{
    BITMAPINFOHEADER *inhdr = icc->lpbiInput;
    BITMAPINFOHEADER *outhdr = icc->lpbiOutput;

    x264_picture_t pic;

    int        i_nal;
    x264_nal_t *nal;
    int        i_out;

    int i;

    /* Init the picture */
    memset( &pic, 0, sizeof( x264_picture_t ) );
    pic.img.i_csp = get_csp( inhdr );

    /* For now biWidth can be divided by 16 so no problem */
    switch( pic.img.i_csp & X264_CSP_MASK )
    {
        case X264_CSP_I420:
        case X264_CSP_YV12:
            pic.img.i_plane = 3;
            pic.img.i_stride[0] = inhdr->biWidth;
            pic.img.i_stride[1] =
            pic.img.i_stride[2] = inhdr->biWidth / 2;

            pic.img.plane[0]    = (uint8_t*)icc->lpInput;
            pic.img.plane[1]    = pic.img.plane[0] + inhdr->biWidth * inhdr->biHeight;
            pic.img.plane[2]    = pic.img.plane[1] + inhdr->biWidth * inhdr->biHeight / 4;
            break;

        case X264_CSP_YUYV:
            pic.img.i_plane = 1;
            pic.img.i_stride[0] = 2 * inhdr->biWidth;
            pic.img.plane[0]    = (uint8_t*)icc->lpInput;
            break;

        case X264_CSP_BGR:
            pic.img.i_plane = 1;
            pic.img.i_stride[0] = 3 * inhdr->biWidth;
            pic.img.plane[0]    = (uint8_t*)icc->lpInput;
            break;

        case X264_CSP_BGRA:
            pic.img.i_plane = 1;
            pic.img.i_stride[0] = 4 * inhdr->biWidth;
            pic.img.plane[0]    = (uint8_t*)icc->lpInput;
            break;

        default:
            return ICERR_BADFORMAT;
    }

    /* encode it */
    x264_encoder_encode( codec->h, &nal, &i_nal, &pic, &pic );

    /* create bitstream, unless we're dropping it in 1st pass */
    i_out = 0;

    if( codec->config.i_encoding_type != 2 || codec->config.i_pass > 1 ) {
        for( i = 0; i < i_nal; i++ ) {
            int i_size = outhdr->biSizeImage - i_out;
            x264_nal_encode( (uint8_t*)icc->lpOutput + i_out, &i_size, 1, &nal[i] );

            i_out += i_size;
        }
    }

    outhdr->biSizeImage = i_out;

    /* Set key frame only for IDR, as they are real synch point, I frame
       aren't always synch point (ex: with multi refs, ref marking) */
    if( pic.i_type == X264_TYPE_IDR )
        *icc->lpdwFlags = AVIIF_KEYFRAME;
    else
        *icc->lpdwFlags = 0;

    return ICERR_OK;
}

