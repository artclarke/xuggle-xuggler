/*****************************************************************************
 * lavf.c: x264 libavformat input module
 *****************************************************************************
 * Copyright (C) 2009 x264 project
 *
 * Authors: Mike Gurlitz <mike.gurlitz@gmail.com>
 *          Steven Walters <kemuri9@gmail.com>
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
#undef DECLARE_ALIGNED
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

typedef struct
{
    AVFormatContext *lavf;
    int stream_id;
    int next_frame;
    int vfr_input;
    int vertical_flip;
    struct SwsContext *scaler;
    int pts_offset_flag;
    int64_t pts_offset;
    x264_picture_t *first_pic;

    int init_width;
    int init_height;

    int cur_width;
    int cur_height;
    enum PixelFormat cur_pix_fmt;
} lavf_hnd_t;

typedef struct
{
    AVFrame frame;
    AVPacket packet;
} lavf_pic_t;

static int check_swscale( lavf_hnd_t *h, AVCodecContext *c, int i_frame )
{
    if( h->scaler && (h->cur_width == c->width) && (h->cur_height == c->height) && (h->cur_pix_fmt == c->pix_fmt) )
        return 0;
    if( h->scaler )
    {
        sws_freeContext( h->scaler );
        fprintf( stderr, "lavf [warning]: stream properties changed to %dx%d, %s at frame %d  \n",
                 c->width, c->height, avcodec_get_pix_fmt_name( c->pix_fmt ), i_frame );
        h->cur_width   = c->width;
        h->cur_height  = c->height;
        h->cur_pix_fmt = c->pix_fmt;
    }
    h->scaler = sws_getContext( h->cur_width, h->cur_height, h->cur_pix_fmt, h->init_width, h->init_height,
                                PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL );
    if( !h->scaler )
    {
        fprintf( stderr, "lavf [error]: could not open swscale context\n" );
        return -1;
    }
    return 0;
}

static int read_frame_internal( x264_picture_t *p_pic, lavf_hnd_t *h, int i_frame, video_info_t *info )
{
    if( h->first_pic && !info )
    {
        /* see if the frame we are requesting is the frame we have already read and stored.
         * if so, retrieve the pts and image data before freeing it. */
        if( !i_frame )
        {
            XCHG( x264_image_t, p_pic->img, h->first_pic->img );
            p_pic->i_pts = h->first_pic->i_pts;
        }
        lavf_input.picture_clean( h->first_pic );
        free( h->first_pic );
        h->first_pic = NULL;
        if( !i_frame )
            return 0;
    }

    AVCodecContext *c = h->lavf->streams[h->stream_id]->codec;
    lavf_pic_t *pic_h = p_pic->opaque;
    AVPacket *pkt = &pic_h->packet;
    AVFrame *frame = &pic_h->frame;

    while( i_frame >= h->next_frame )
    {
        int finished = 0;
        while( !finished && av_read_frame( h->lavf, pkt ) >= 0 )
            if( pkt->stream_index == h->stream_id )
            {
                c->reordered_opaque = pkt->pts;
                if( avcodec_decode_video2( c, frame, &finished, pkt ) < 0 )
                    fprintf( stderr, "lavf [warning]: video decoding failed on frame %d\n", h->next_frame );
            }
        if( !finished )
        {
            if( avcodec_decode_video2( c, frame, &finished, pkt ) < 0 )
                fprintf( stderr, "lavf [warning]: video decoding failed on frame %d\n", h->next_frame );
            if( !finished )
                return -1;
        }
        h->next_frame++;
    }

    if( check_swscale( h, c, i_frame ) )
        return -1;
    /* FIXME: avoid sws_scale where possible (no colorspace conversion). */
    sws_scale( h->scaler, frame->data, frame->linesize, 0, c->height, p_pic->img.plane, p_pic->img.i_stride );

    if( info )
        info->interlaced = frame->interlaced_frame;

    if( h->vfr_input )
    {
        p_pic->i_pts = 0;
        if( frame->reordered_opaque != AV_NOPTS_VALUE )
            p_pic->i_pts = frame->reordered_opaque;
        else if( pkt->dts != AV_NOPTS_VALUE )
            p_pic->i_pts = pkt->dts; // for AVI files
        else if( info )
        {
            h->vfr_input = info->vfr = 0;
            goto exit;
        }
        if( !h->pts_offset_flag )
        {
            h->pts_offset = p_pic->i_pts;
            h->pts_offset_flag = 1;
        }
        p_pic->i_pts -= h->pts_offset;
    }

exit:
    if( pkt->destruct )
        pkt->destruct( pkt );
    avcodec_get_frame_defaults( frame );
    return 0;
}

static int open_file( char *psz_filename, hnd_t *p_handle, video_info_t *info, cli_input_opt_t *opt )
{
    lavf_hnd_t *h = malloc( sizeof(lavf_hnd_t) );
    if( !h )
        return -1;
    av_register_all();
    h->scaler = NULL;
    if( !strcmp( psz_filename, "-" ) )
        psz_filename = "pipe:";

    if( av_open_input_file( &h->lavf, psz_filename, NULL, 0, NULL ) )
    {
        fprintf( stderr, "lavf [error]: could not open input file\n" );
        return -1;
    }

    if( av_find_stream_info( h->lavf ) < 0 )
    {
        fprintf( stderr, "lavf [error]: could not find input stream info\n" );
        return -1;
    }

    int i = 0;
    while( i < h->lavf->nb_streams && h->lavf->streams[i]->codec->codec_type != CODEC_TYPE_VIDEO )
        i++;
    if( i == h->lavf->nb_streams )
    {
        fprintf( stderr, "lavf [error]: could not find video stream\n" );
        return -1;
    }
    h->stream_id       = i;
    h->next_frame      = 0;
    h->pts_offset_flag = 0;
    h->pts_offset      = 0;
    AVCodecContext *c  = h->lavf->streams[i]->codec;
    h->init_width      = h->cur_width  = info->width  = c->width;
    h->init_height     = h->cur_height = info->height = c->height;
    h->cur_pix_fmt     = c->pix_fmt;
    info->fps_num      = h->lavf->streams[i]->r_frame_rate.num;
    info->fps_den      = h->lavf->streams[i]->r_frame_rate.den;
    info->timebase_num = h->lavf->streams[i]->time_base.num;
    info->timebase_den = h->lavf->streams[i]->time_base.den;
    h->vfr_input       = info->vfr;
    h->vertical_flip   = 0;

    /* avisynth stores rgb data vertically flipped. */
    if( !strcasecmp( get_filename_extension( psz_filename ), "avs" ) &&
        (h->cur_pix_fmt == PIX_FMT_BGRA || h->cur_pix_fmt == PIX_FMT_BGR24) )
        info->csp |= X264_CSP_VFLIP;

    if( h->cur_pix_fmt != PIX_FMT_YUV420P )
        fprintf( stderr, "lavf [warning]: converting from %s to YV12\n",
                 avcodec_get_pix_fmt_name( h->cur_pix_fmt ) );

    if( avcodec_open( c, avcodec_find_decoder( c->codec_id ) ) )
    {
        fprintf( stderr, "lavf [error]: could not find decoder for video stream\n" );
        return -1;
    }

    /* prefetch the first frame and set/confirm flags */
    h->first_pic = malloc( sizeof(x264_picture_t) );
    if( !h->first_pic || lavf_input.picture_alloc( h->first_pic, info->csp, info->width, info->height ) )
    {
        fprintf( stderr, "lavf [error]: malloc failed\n" );
        return -1;
    }
    else if( read_frame_internal( h->first_pic, h, 0, info ) )
        return -1;

    info->sar_height = c->sample_aspect_ratio.den;
    info->sar_width  = c->sample_aspect_ratio.num;
    *p_handle = h;

    return 0;
}

static int picture_alloc( x264_picture_t *pic, int i_csp, int i_width, int i_height )
{
    if( x264_picture_alloc( pic, i_csp, i_width, i_height ) )
        return -1;
    lavf_pic_t *pic_h = pic->opaque = malloc( sizeof(lavf_pic_t) );
    if( !pic_h )
        return -1;
    avcodec_get_frame_defaults( &pic_h->frame );
    av_init_packet( &pic_h->packet );
    return 0;
}

/* FIXME */
static int get_frame_total( hnd_t handle )
{
    return 0;
}

static int read_frame( x264_picture_t *p_pic, hnd_t handle, int i_frame )
{
    return read_frame_internal( p_pic, handle, i_frame, NULL );
}

static void picture_clean( x264_picture_t *pic )
{
    free( pic->opaque );
    x264_picture_clean( pic );
}

static int close_file( hnd_t handle )
{
    lavf_hnd_t *h = handle;
    sws_freeContext( h->scaler );
    avcodec_close( h->lavf->streams[h->stream_id]->codec );
    av_close_input_file( h->lavf );
    free( h );
    return 0;
}

cli_input_t lavf_input = { open_file, get_frame_total, picture_alloc, read_frame, NULL, picture_clean, close_file };
