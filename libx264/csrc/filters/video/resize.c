/*****************************************************************************
 * resize.c: resize video filter
 *****************************************************************************
 * Copyright (C) 2010-2011 x264 project
 *
 * Authors: Steven Walters <kemuri9@gmail.com>
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

#include "video.h"
#define NAME "resize"
#define FAIL_IF_ERROR( cond, ... ) FAIL_IF_ERR( cond, NAME, __VA_ARGS__ )

cli_vid_filter_t resize_filter;

static int full_check( video_info_t *info, x264_param_t *param )
{
    int required = 0;
    required |= info->csp    != param->i_csp;
    required |= info->width  != param->i_width;
    required |= info->height != param->i_height;
    return required;
}

#if HAVE_SWSCALE
#undef DECLARE_ALIGNED
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

typedef struct
{
    int width;
    int height;
    int pix_fmt;
} frame_prop_t;

typedef struct
{
    hnd_t prev_hnd;
    cli_vid_filter_t prev_filter;

    cli_pic_t buffer;
    int buffer_allocated;
    int dst_csp;
    struct SwsContext *ctx;
    uint32_t ctx_flags;
    /* state of swapping chroma planes pre and post resize */
    int pre_swap_chroma;
    int post_swap_chroma;
    int variable_input; /* input is capable of changing properties */
    int working;        /* we have already started working with frames */
    frame_prop_t dst;   /* desired output properties */
    frame_prop_t scale; /* properties of the SwsContext input */
} resizer_hnd_t;

static void help( int longhelp )
{
    printf( "      "NAME":[width,height][,sar][,fittobox][,csp][,method]\n" );
    if( !longhelp )
        return;
    printf( "            resizes frames based on the given criteria:\n"
            "            - resolution only: resizes and adapts sar to avoid stretching\n"
            "            - sar only: sets the sar and resizes to avoid stretching\n"
            "            - resolution and sar: resizes to given resolution and sets the sar\n"
            "            - fittobox: resizes the video based on the desired constraints\n"
            "               - width, height, both\n"
            "            - fittobox and sar: same as above except with specified sar\n"
            "            - csp: convert to the given csp. syntax: [name][:depth]\n"
            "               - valid csp names [keep current]: " );

    for( int i = X264_CSP_NONE+1; i < X264_CSP_CLI_MAX; i++ )
    {
        printf( "%s", x264_cli_csps[i].name );
        if( i+1 < X264_CSP_CLI_MAX )
            printf( ", " );
    }
    printf( "\n"
            "               - depth: 8 or 16 bits per pixel [keep current]\n"
            "            note: not all depths are supported by all csps.\n"
            "            - method: use resizer method [\"bicubic\"]\n"
            "               - fastbilinear, bilinear, bicubic, experimental, point,\n"
            "               - area, bicublin, gauss, sinc, lanczos, spline\n" );
}

static uint32_t convert_method_to_flag( const char *name )
{
    uint32_t flag = 0;
    if( !strcasecmp( name, "fastbilinear" ) )
        flag = SWS_FAST_BILINEAR;
    else if( !strcasecmp( name, "bilinear" ) )
        flag = SWS_BILINEAR;
    else if( !strcasecmp( name, "bicubic" ) )
        flag = SWS_BICUBIC;
    else if( !strcasecmp( name, "experimental" ) )
        flag = SWS_X;
    else if( !strcasecmp( name, "point" ) )
        flag = SWS_POINT;
    else if( !strcasecmp( name, "area" ) )
        flag = SWS_AREA;
    else if( !strcasecmp( name, "bicublin" ) )
        flag = SWS_BICUBLIN;
    else if( !strcasecmp( name, "guass" ) )
        flag = SWS_GAUSS;
    else if( !strcasecmp( name, "sinc" ) )
        flag = SWS_SINC;
    else if( !strcasecmp( name, "lanczos" ) )
        flag = SWS_LANCZOS;
    else if( !strcasecmp( name, "spline" ) )
        flag = SWS_SPLINE;
    else // default
        flag = SWS_BICUBIC;
    return flag;
}

static int convert_csp_to_pix_fmt( int csp )
{
    if( csp&X264_CSP_OTHER )
        return csp&X264_CSP_MASK;
    switch( csp&X264_CSP_MASK )
    {
        case X264_CSP_YV12: /* specially handled via swapping chroma */
        case X264_CSP_I420: return csp&X264_CSP_HIGH_DEPTH ? PIX_FMT_YUV420P16 : PIX_FMT_YUV420P;
        case X264_CSP_I422: return csp&X264_CSP_HIGH_DEPTH ? PIX_FMT_YUV422P16 : PIX_FMT_YUV422P;
        case X264_CSP_YV24: /* specially handled via swapping chroma */
        case X264_CSP_I444: return csp&X264_CSP_HIGH_DEPTH ? PIX_FMT_YUV444P16 : PIX_FMT_YUV444P;
        case X264_CSP_RGB:  return csp&X264_CSP_HIGH_DEPTH ? PIX_FMT_RGB48     : PIX_FMT_RGB24;
        /* the next 3 csps have no equivalent 16bit depth in swscale */
        case X264_CSP_NV12: return csp&X264_CSP_HIGH_DEPTH ? PIX_FMT_NONE      : PIX_FMT_NV12;
        case X264_CSP_BGR:  return csp&X264_CSP_HIGH_DEPTH ? PIX_FMT_NONE      : PIX_FMT_BGR24;
        case X264_CSP_BGRA: return csp&X264_CSP_HIGH_DEPTH ? PIX_FMT_NONE      : PIX_FMT_BGRA;
        default:            return PIX_FMT_NONE;
    }
}

static int pick_closest_supported_csp( int csp )
{
    int pix_fmt = convert_csp_to_pix_fmt( csp );
    switch( pix_fmt )
    {
        case PIX_FMT_YUV420P16LE:
        case PIX_FMT_YUV420P16BE:
            return X264_CSP_I420 | X264_CSP_HIGH_DEPTH;
        case PIX_FMT_YUV422P:
        case PIX_FMT_YUYV422:
        case PIX_FMT_UYVY422:
        case PIX_FMT_YUVJ422P:
            return X264_CSP_I422;
        case PIX_FMT_YUV422P16LE:
        case PIX_FMT_YUV422P16BE:
            return X264_CSP_I422 | X264_CSP_HIGH_DEPTH;
        case PIX_FMT_YUV444P:
        case PIX_FMT_YUVJ444P:
            return X264_CSP_I444;
        case PIX_FMT_YUV444P16LE:
        case PIX_FMT_YUV444P16BE:
            return X264_CSP_I444 | X264_CSP_HIGH_DEPTH;
        case PIX_FMT_RGB24:
        case PIX_FMT_RGB565BE:
        case PIX_FMT_RGB565LE:
        case PIX_FMT_RGB555BE:
        case PIX_FMT_RGB555LE:
            return X264_CSP_RGB;
        case PIX_FMT_RGB48BE:
        case PIX_FMT_RGB48LE:
            return X264_CSP_RGB | X264_CSP_HIGH_DEPTH;
        case PIX_FMT_BGR24:
        case PIX_FMT_BGR565BE:
        case PIX_FMT_BGR565LE:
        case PIX_FMT_BGR555BE:
        case PIX_FMT_BGR555LE:
            return X264_CSP_BGR;
        case PIX_FMT_ARGB:
        case PIX_FMT_RGBA:
        case PIX_FMT_ABGR:
        case PIX_FMT_BGRA:
            return X264_CSP_BGRA;
        case PIX_FMT_NV12:
        case PIX_FMT_NV21:
             return X264_CSP_NV12;
        default:
            return X264_CSP_I420;
    }
}

static int handle_opts( const char **optlist, char **opts, video_info_t *info, resizer_hnd_t *h )
{
    uint32_t out_sar_w, out_sar_h;

    char *str_width  = x264_get_option( optlist[0], opts );
    char *str_height = x264_get_option( optlist[1], opts );
    char *str_sar    = x264_get_option( optlist[2], opts );
    char *fittobox   = x264_get_option( optlist[3], opts );
    char *str_csp    = x264_get_option( optlist[4], opts );
    int width        = x264_otoi( str_width, -1 );
    int height       = x264_otoi( str_height, -1 );

    int csp_only = 0;
    uint32_t in_sar_w = info->sar_width;
    uint32_t in_sar_h = info->sar_height;

    if( str_csp )
    {
        /* output csp was specified, first check if optional depth was provided */
        char *str_depth = strchr( str_csp, ':' );
        int depth = x264_cli_csp_depth_factor( info->csp ) * 8;
        if( str_depth )
        {
            /* csp bit depth was specified */
            *str_depth++ = '\0';
            depth = x264_otoi( str_depth, -1 );
            FAIL_IF_ERROR( depth != 8 && depth != 16, "unsupported bit depth %d\n", depth );
        }
        /* now lookup against the list of valid csps */
        int csp;
        if( strlen( str_csp ) == 0 )
            csp = info->csp & X264_CSP_MASK;
        else
            for( csp = X264_CSP_CLI_MAX-1; x264_cli_csps[csp].name && strcasecmp( x264_cli_csps[csp].name, str_csp ); )
                csp--;
        FAIL_IF_ERROR( csp == X264_CSP_NONE, "unsupported colorspace `%s'\n", str_csp );
        h->dst_csp = csp;
        if( depth == 16 )
            h->dst_csp |= X264_CSP_HIGH_DEPTH;
    }

    /* if the input sar is currently invalid, set it to 1:1 so it can be used in math */
    if( !in_sar_w || !in_sar_h )
        in_sar_w = in_sar_h = 1;
    if( str_sar )
    {
        FAIL_IF_ERROR( 2 != sscanf( str_sar, "%u:%u", &out_sar_w, &out_sar_h ) &&
                       2 != sscanf( str_sar, "%u/%u", &out_sar_w, &out_sar_h ),
                       "invalid sar `%s'\n", str_sar )
    }
    else
        out_sar_w = out_sar_h = 1;
    if( fittobox )
    {
        /* resize the video to fit the box as much as possible */
        if( !strcasecmp( fittobox, "both" ) )
        {
            FAIL_IF_ERROR( width <= 0 || height <= 0, "invalid box resolution %sx%s\n",
                           x264_otos( str_width, "<unset>" ), x264_otos( str_height, "<unset>" ) )
        }
        else if( !strcasecmp( fittobox, "width" ) )
        {
            FAIL_IF_ERROR( width <= 0, "invalid box width `%s'\n", x264_otos( str_width, "<unset>" ) )
            height = INT_MAX;
        }
        else if( !strcasecmp( fittobox, "height" ) )
        {
            FAIL_IF_ERROR( height <= 0, "invalid box height `%s'\n", x264_otos( str_height, "<unset>" ) )
            width = INT_MAX;
        }
        else FAIL_IF_ERROR( 1, "invalid fittobox mode `%s'\n", fittobox )

        /* maximally fit the new coded resolution to the box */
        const x264_cli_csp_t *csp = x264_cli_get_csp( h->dst_csp );
        double width_units = (double)info->height * in_sar_h * out_sar_w;
        double height_units = (double)info->width * in_sar_w * out_sar_h;
        width = width / csp->mod_width * csp->mod_width;
        height = height / csp->mod_height * csp->mod_height;
        if( width * width_units > height * height_units )
        {
            int new_width = round( height * height_units / (width_units * csp->mod_width) );
            new_width *= csp->mod_width;
            width = X264_MIN( new_width, width );
        }
        else
        {
            int new_height = round( width * width_units / (height_units * csp->mod_height) );
            new_height *= csp->mod_height;
            height = X264_MIN( new_height, height );
        }
    }
    else
    {
        if( str_width || str_height )
        {
            FAIL_IF_ERROR( width <= 0 || height <= 0, "invalid resolution %sx%s\n",
                           x264_otos( str_width, "<unset>" ), x264_otos( str_height, "<unset>" ) )
            if( !str_sar ) /* res only -> adjust sar */
            {
                /* new_sar = (new_h * old_w * old_sar_w) / (old_h * new_w * old_sar_h) */
                uint64_t num = (uint64_t)info->width  * height;
                uint64_t den = (uint64_t)info->height * width;
                x264_reduce_fraction64( &num, &den );
                out_sar_w = num * in_sar_w;
                out_sar_h = den * in_sar_h;
                x264_reduce_fraction( &out_sar_w, &out_sar_h );
            }
        }
        else if( str_sar ) /* sar only -> adjust res */
        {
             const x264_cli_csp_t *csp = x264_cli_get_csp( h->dst_csp );
             double width_units = (double)in_sar_h * out_sar_w;
             double height_units = (double)in_sar_w * out_sar_h;
             width  = info->width;
             height = info->height;
             if( width_units > height_units ) // SAR got wider, decrease width
             {
                 width = round( info->width * height_units / (width_units * csp->mod_width) );
                 width *= csp->mod_width;
             }
             else // SAR got thinner, decrease height
             {
                 height = round( info->height * width_units / (height_units * csp->mod_height) );
                 height *= csp->mod_height;
             }
        }
        else /* csp only */
        {
            h->dst.width  = info->width;
            h->dst.height = info->height;
            csp_only = 1;
        }
    }
    if( !csp_only )
    {
        info->sar_width  = out_sar_w;
        info->sar_height = out_sar_h;
        h->dst.width  = width;
        h->dst.height = height;
    }
    return 0;
}

static int handle_jpeg( int *format )
{
    switch( *format )
    {
        case PIX_FMT_YUVJ420P:
            *format = PIX_FMT_YUV420P;
            return 1;
        case PIX_FMT_YUVJ422P:
            *format = PIX_FMT_YUV422P;
            return 1;
        case PIX_FMT_YUVJ444P:
            *format = PIX_FMT_YUV444P;
            return 1;
        case PIX_FMT_YUVJ440P:
            *format = PIX_FMT_YUV440P;
            return 1;
        default:
            return 0;
    }
}

static int x264_init_sws_context( resizer_hnd_t *h )
{
    if( !h->ctx )
    {
        h->ctx = sws_alloc_context();
        if( !h->ctx )
            return -1;

        /* set flags that will not change */
        int dst_format = h->dst.pix_fmt;
        int dst_range  = handle_jpeg( &dst_format );
        av_set_int( h->ctx, "sws_flags",  h->ctx_flags );
        av_set_int( h->ctx, "dstw",       h->dst.width );
        av_set_int( h->ctx, "dsth",       h->dst.height );
        av_set_int( h->ctx, "dst_format", dst_format );
        av_set_int( h->ctx, "dst_range",  dst_range ); /* FIXME: use the correct full range value */
    }

    int src_format = h->scale.pix_fmt;
    int src_range  = handle_jpeg( &src_format );
    av_set_int( h->ctx, "srcw",       h->scale.width );
    av_set_int( h->ctx, "srch",       h->scale.height );
    av_set_int( h->ctx, "src_format", src_format );
    av_set_int( h->ctx, "src_range",  src_range ); /* FIXME: use the correct full range value */

    /* FIXME: use the correct full range values
     * FIXME: use the correct matrix coefficients (only YUV -> RGB conversions are supported) */
    sws_setColorspaceDetails( h->ctx,
                              sws_getCoefficients( SWS_CS_DEFAULT ), src_range,
                              sws_getCoefficients( SWS_CS_DEFAULT ), av_get_int( h->ctx, "dst_range", NULL ),
                              0, 1<<16, 1<<16 );

    return sws_init_context( h->ctx, NULL, NULL ) < 0;
}

static int check_resizer( resizer_hnd_t *h, cli_pic_t *in )
{
    frame_prop_t input_prop = { in->img.width, in->img.height, convert_csp_to_pix_fmt( in->img.csp ) };
    if( !memcmp( &input_prop, &h->scale, sizeof(frame_prop_t) ) )
        return 0;
    /* also warn if the resizer was initialized after the first frame */
    if( h->ctx || h->working )
        x264_cli_log( NAME, X264_LOG_WARNING, "stream properties changed at pts %"PRId64"\n", in->pts );
    h->scale = input_prop;
    if( !h->buffer_allocated )
    {
        if( x264_cli_pic_alloc( &h->buffer, h->dst_csp, h->dst.width, h->dst.height ) )
            return -1;
        h->buffer_allocated = 1;
    }
    FAIL_IF_ERROR( x264_init_sws_context( h ), "swscale init failed\n" )
    return 0;
}

static int init( hnd_t *handle, cli_vid_filter_t *filter, video_info_t *info, x264_param_t *param, char *opt_string )
{
    /* if called for normalizing the csp to known formats and the format is not unknown, exit */
    if( opt_string && !strcmp( opt_string, "normcsp" ) && !(info->csp&X264_CSP_OTHER) )
        return 0;
    /* if called by x264cli and nothing needs to be done, exit */
    if( !opt_string && !full_check( info, param ) )
        return 0;

    static const char *optlist[] = { "width", "height", "sar", "fittobox", "csp", "method", NULL };
    char **opts = x264_split_options( opt_string, optlist );
    if( !opts && opt_string )
        return -1;

    resizer_hnd_t *h = calloc( 1, sizeof(resizer_hnd_t) );
    if( !h )
        return -1;
    if( opts )
    {
        h->dst_csp    = info->csp;
        h->dst.width  = info->width;
        h->dst.height = info->height;
        if( !strcmp( opt_string, "normcsp" ) )
        {
            /* only in normalization scenarios is the input capable of changing properties */
            h->variable_input = 1;
            h->dst_csp = pick_closest_supported_csp( info->csp );
            /* now fix the catch-all i420 choice if it does not allow for the current input resolution dimensions. */
            if( h->dst_csp == X264_CSP_I420 && info->width&1 )
                h->dst_csp = X264_CSP_I444;
            if( h->dst_csp == X264_CSP_I420 && info->height&1 )
                h->dst_csp = X264_CSP_I422;
        }
        else if( handle_opts( optlist, opts, info, h ) )
            return -1;
    }
    else
    {
        h->dst_csp    = param->i_csp;
        h->dst.width  = param->i_width;
        h->dst.height = param->i_height;
    }
    h->ctx_flags = convert_method_to_flag( x264_otos( x264_get_option( optlist[5], opts ), "" ) );
    x264_free_string_array( opts );

    if( h->ctx_flags != SWS_FAST_BILINEAR )
        h->ctx_flags |= SWS_FULL_CHR_H_INT | SWS_FULL_CHR_H_INP | SWS_ACCURATE_RND;
    h->dst.pix_fmt = convert_csp_to_pix_fmt( h->dst_csp );
    h->scale = h->dst;

    /* swap chroma planes if YV12/YV24 is involved, as libswscale works with I420/I444 */
    int src_csp = info->csp & (X264_CSP_MASK | X264_CSP_OTHER);
    int dst_csp = h->dst_csp & (X264_CSP_MASK | X264_CSP_OTHER);
    h->pre_swap_chroma  = src_csp == X264_CSP_YV12 || src_csp == X264_CSP_YV24;
    h->post_swap_chroma = dst_csp == X264_CSP_YV12 || dst_csp == X264_CSP_YV24;

    int src_pix_fmt = convert_csp_to_pix_fmt( info->csp );

    int src_pix_fmt_inv = convert_csp_to_pix_fmt( info->csp ^ X264_CSP_HIGH_DEPTH );
    int dst_pix_fmt_inv = convert_csp_to_pix_fmt( h->dst_csp ^ X264_CSP_HIGH_DEPTH );

    /* confirm swscale can support this conversion */
    FAIL_IF_ERROR( src_pix_fmt == PIX_FMT_NONE && src_pix_fmt_inv != PIX_FMT_NONE,
                   "input colorspace %s with bit depth %d is not supported\n", av_get_pix_fmt_name( src_pix_fmt_inv ),
                   info->csp & X264_CSP_HIGH_DEPTH ? 16 : 8 );
    FAIL_IF_ERROR( !sws_isSupportedInput( src_pix_fmt ), "input colorspace %s is not supported\n", av_get_pix_fmt_name( src_pix_fmt ) )
    FAIL_IF_ERROR( h->dst.pix_fmt == PIX_FMT_NONE && dst_pix_fmt_inv != PIX_FMT_NONE,
                   "input colorspace %s with bit depth %d is not supported\n", av_get_pix_fmt_name( dst_pix_fmt_inv ),
                   h->dst_csp & X264_CSP_HIGH_DEPTH ? 16 : 8 );
    FAIL_IF_ERROR( !sws_isSupportedOutput( h->dst.pix_fmt ), "output colorspace %s is not supported\n", av_get_pix_fmt_name( h->dst.pix_fmt ) )
    FAIL_IF_ERROR( h->dst.height != info->height && info->interlaced,
                   "swscale is not compatible with interlaced vertical resizing\n" )
    /* confirm that the desired resolution meets the colorspace requirements */
    const x264_cli_csp_t *csp = x264_cli_get_csp( h->dst_csp );
    FAIL_IF_ERROR( h->dst.width % csp->mod_width || h->dst.height % csp->mod_height,
                   "resolution %dx%d is not compliant with colorspace %s\n", h->dst.width, h->dst.height, csp->name )

    if( h->dst.width != info->width || h->dst.height != info->height )
        x264_cli_log( NAME, X264_LOG_INFO, "resizing to %dx%d\n", h->dst.width, h->dst.height );
    if( h->dst.pix_fmt != src_pix_fmt )
        x264_cli_log( NAME, X264_LOG_WARNING, "converting from %s to %s\n",
                      av_get_pix_fmt_name( src_pix_fmt ), av_get_pix_fmt_name( h->dst.pix_fmt ) );
    h->dst_csp |= info->csp & X264_CSP_VFLIP; // preserve vflip

    /* if the input is not variable, initialize the context */
    if( !h->variable_input )
    {
        cli_pic_t input_pic = {{info->csp, info->width, info->height, 0}, 0};
        if( check_resizer( h, &input_pic ) )
            return -1;
    }

    /* finished initing, overwrite values */
    info->csp    = h->dst_csp;
    info->width  = h->dst.width;
    info->height = h->dst.height;

    h->prev_filter = *filter;
    h->prev_hnd = *handle;
    *handle = h;
    *filter = resize_filter;

    return 0;
}

static int get_frame( hnd_t handle, cli_pic_t *output, int frame )
{
    resizer_hnd_t *h = handle;
    if( h->prev_filter.get_frame( h->prev_hnd, output, frame ) )
        return -1;
    if( h->variable_input && check_resizer( h, output ) )
        return -1;
    h->working = 1;
    if( h->pre_swap_chroma )
        XCHG( uint8_t*, output->img.plane[1], output->img.plane[2] );
    if( h->ctx )
    {
        sws_scale( h->ctx, (const uint8_t* const*)output->img.plane, output->img.stride,
                   0, output->img.height, h->buffer.img.plane, h->buffer.img.stride );
        output->img = h->buffer.img; /* copy img data */
    }
    else
        output->img.csp = h->dst_csp;
    if( h->post_swap_chroma )
        XCHG( uint8_t*, output->img.plane[1], output->img.plane[2] );

    return 0;
}

static int release_frame( hnd_t handle, cli_pic_t *pic, int frame )
{
    resizer_hnd_t *h = handle;
    return h->prev_filter.release_frame( h->prev_hnd, pic, frame );
}

static void free_filter( hnd_t handle )
{
    resizer_hnd_t *h = handle;
    h->prev_filter.free( h->prev_hnd );
    if( h->ctx )
        sws_freeContext( h->ctx );
    if( h->buffer_allocated )
        x264_cli_pic_clean( &h->buffer );
    free( h );
}

#else /* no swscale */
static int init( hnd_t *handle, cli_vid_filter_t *filter, video_info_t *info, x264_param_t *param, char *opt_string )
{
    int ret = 0;

    if( !opt_string )
        ret = full_check( info, param );
    else
    {
        if( !strcmp( opt_string, "normcsp" ) )
            ret = info->csp & X264_CSP_OTHER;
        else
            ret = -1;
    }

    /* pass if nothing needs to be done, otherwise fail */
    FAIL_IF_ERROR( ret, "not compiled with swscale support\n" )
    return 0;
}

#define help NULL
#define get_frame NULL
#define release_frame NULL
#define free_filter NULL
#define convert_csp_to_pix_fmt(x) (x & X264_CSP_MASK)

#endif

cli_vid_filter_t resize_filter = { NAME, help, init, get_frame, release_frame, free_filter, NULL };
