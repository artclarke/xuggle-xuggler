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

/* get_csp:
 *  return a valid x264 CSP or X264_CSP_NULL if unsuported */
static int get_csp( BITMAPINFOHEADER *hdr )
{
    int i_vlip = hdr->biHeight < 0 ? 0 : X264_CSP_VFLIP;

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
            if( hdr->biBitCount == 24 )
                return X264_CSP_BGR | i_vlip;
            if( hdr->biBitCount == 32 )
                return X264_CSP_BGRA | i_vlip;
            else
                return X264_CSP_NONE;

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
    return ICERR_OK;
}

/* */
LRESULT compress_begin(CODEC * codec, BITMAPINFO * lpbiInput, BITMAPINFO * lpbiOutput )
{
    CONFIG *config = &codec->config;
    x264_param_t param;
    static char statsfile[] = ".\\x264.stats";

    /* Destroy previous handle */
    if( codec->h != NULL )
    {
        x264_encoder_close( codec->h );
        codec->h = NULL;
    }

    /* Get default param */
    x264_param_default( &param );

    param.i_log_level = X264_LOG_NONE;
    param.analyse.b_psnr = 0;
    param.analyse.inter = 0;
    param.analyse.intra = X264_ANALYSE_I4x4;

    /* Set params: TODO to complete */
    param.i_width = lpbiInput->bmiHeader.biWidth;
    param.i_height= lpbiInput->bmiHeader.biHeight;

    param.i_fps_num = codec->fbase;
    param.i_fps_den = codec->fincr;

    param.i_frame_reference = config->i_refmax;
    param.i_keyint_min = config->i_keyint_min;
    param.i_keyint_max   = config->i_keyint_max;
    param.b_deblocking_filter = config->b_filter;
    param.b_cabac = config->b_cabac;

    param.i_bframe = config->i_bframe;
    param.analyse.i_subpel_refine = config->i_subpel_refine + 1; /* 0..4 -> 1..5 */

    /* bframe prediction - gui goes alphabetically, so 0=NONE, 1=SPATIAL, 2=TEMPORAL */
    switch(config->i_direct_mv_pred) {
        case 0: param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_NONE; break;
        case 1: param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_SPATIAL; break;
        case 2: param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_TEMPORAL; break;
    }
    param.i_deblocking_filter_alphac0 = config->i_inloop_a;
    param.i_deblocking_filter_beta = config->i_inloop_b;

    if( config->b_bsub16x16 )
        param.analyse.inter |= X264_ANALYSE_BSUB16x16;

    if( config->b_psub16x16 )
        param.analyse.inter |= X264_ANALYSE_PSUB16x16;
    if( config->b_psub8x8 )
        param.analyse.inter |= X264_ANALYSE_PSUB8x8;
    if( config->b_i4x4 )
        param.analyse.inter |= X264_ANALYSE_I4x4;

    switch( config->i_encoding_type )
    {
        case 0: /* 1 PASS CBR */
            param.rc.b_cbr = 1;
            param.rc.i_bitrate = config->bitrate;
            break;
        case 1: /* 1 PASS CQ */
            param.rc.i_qp_constant = config->i_qp;
            break;
        default:
        case 2: /* 2 PASS */
            param.rc.psz_stat_out = param.rc.psz_stat_in = statsfile;
            if (config->i_pass == 1)
                param.rc.b_stat_write = 1;
            else {    
                param.rc.i_bitrate = config->i_2passbitrate;
                param.rc.b_stat_read = 1;
                param.rc.b_cbr = 1;
            }
            break;
    }

    /* Open the encoder */
    codec->h = x264_encoder_open( &param );
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
    x264_encoder_encode( codec->h, &nal, &i_nal, &pic );

    /* create bitstream, unless we're dropping it in 1st pass */
    i_out = 0;

    if (codec->config.i_encoding_type != 2 || codec->config.i_pass > 1) {
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

