/*****************************************************************************
 * resize.c: x264 video resize filter
 *****************************************************************************
 * Copyright (C) 2010 Steven Walters <kemuri9@gmail.com>
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

/* this function is not a part of the swscale API but is defined in swscale_internal.h */
const char *sws_format_name( enum PixelFormat format );

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
    int ctx_flags;
    int swap_chroma;    /* state of swapping chroma planes */
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
            "            - fittobox: resizes the video based on the desired contraints\n"
            "               - width, height, both\n"
            "            - fittobox and sar: same as above except with specified sar\n"
            "            simultaneously converting to the given colorspace\n"
            "            using resizer method [\"bicubic\"]\n"
            "             - fastbilinear, bilinear, bicubic, experimental, point,\n"
            "             - area, bicublin, gauss, sinc, lanczos, spline\n" );
}

static uint32_t convert_cpu_to_flag( uint32_t cpu )
{
    uint32_t swscale_cpu = 0;
    if( cpu & X264_CPU_ALTIVEC )
        swscale_cpu |= SWS_CPU_CAPS_ALTIVEC;
    if( cpu & X264_CPU_MMXEXT )
        swscale_cpu |= SWS_CPU_CAPS_MMX | SWS_CPU_CAPS_MMX2;
    return swscale_cpu;
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
        case X264_CSP_I420: return PIX_FMT_YUV420P;
        case X264_CSP_I422: return PIX_FMT_YUV422P;
        case X264_CSP_I444: return PIX_FMT_YUV444P;
        case X264_CSP_NV12: return PIX_FMT_NV12;
        case X264_CSP_YV12: return PIX_FMT_YUV420P; /* specially handled via swapping chroma */
        case X264_CSP_BGR:  return PIX_FMT_BGR24;
        case X264_CSP_BGRA: return PIX_FMT_BGRA;
        default:            return PIX_FMT_NONE;
    }
}

static int pick_closest_supported_csp( int csp )
{
    int pix_fmt = convert_csp_to_pix_fmt( csp );
    switch( pix_fmt )
    {
        case PIX_FMT_YUV422P:
        case PIX_FMT_YUV422P16LE:
        case PIX_FMT_YUV422P16BE:
        case PIX_FMT_YUYV422:
        case PIX_FMT_UYVY422:
            return X264_CSP_I422;
        case PIX_FMT_YUV444P:
        case PIX_FMT_YUV444P16LE:
        case PIX_FMT_YUV444P16BE:
            return X264_CSP_I444;
        case PIX_FMT_RGB24:    // convert rgb to bgr
        case PIX_FMT_RGB48BE:
        case PIX_FMT_RGB48LE:
        case PIX_FMT_RGB565BE:
        case PIX_FMT_RGB565LE:
        case PIX_FMT_RGB555BE:
        case PIX_FMT_RGB555LE:
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

static int round_dbl( double val, int precision, int b_truncate )
{
    int ret = (int)(val / precision) * precision;
    if( !b_truncate && (val - ret) >= (precision/2) ) // use the remainder if we're not truncating it
        ret += precision;
    return ret;
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
        /* output csp was specified, lookup against valid values */
        int csp;
        for( csp = X264_CSP_CLI_MAX-1; x264_cli_csps[csp].name && strcasecmp( x264_cli_csps[csp].name, str_csp ); )
            csp--;
        FAIL_IF_ERROR( csp == X264_CSP_NONE, "unsupported colorspace `%s'\n", str_csp );
        h->dst_csp = csp;
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
        double box_width = width;
        double box_height = height;
        if( !strcasecmp( fittobox, "both" ) )
        {
            FAIL_IF_ERROR( box_width <= 0 || box_height <= 0, "invalid box resolution %sx%s\n",
                           x264_otos( str_width, "unset" ), x264_otos( str_height, "unset" ) )
        }
        else if( !strcasecmp( fittobox, "width" ) )
        {
            FAIL_IF_ERROR( box_width <= 0, "invalid box width `%s'\n", x264_otos( str_width, "unset" ) )
            box_height = INT_MAX;
        }
        else if( !strcasecmp( fittobox, "height" ) )
        {
            FAIL_IF_ERROR( box_height <= 0, "invalid box height `%s'\n", x264_otos( str_height, "unset" ) )
            box_width = INT_MAX;
        }
        else FAIL_IF_ERROR( 1, "invalid fittobox mode `%s'\n", fittobox )

        /* we now have the requested bounding box display dimensions, now adjust them for output sar */
        if( out_sar_w > out_sar_h ) // SAR is wide, decrease width
            box_width  *= (double)out_sar_h / out_sar_w;
        else // SAR is thin, decrease height
            box_height *= (double)out_sar_w  / out_sar_h;

        /* get the display resolution of the clip as it is now */
        double d_width  = info->width;
        double d_height = info->height;
        if( in_sar_w > in_sar_h )
            d_width  *= (double)in_sar_w / in_sar_h;
        else
            d_height *= (double)in_sar_h / in_sar_w;
        /* now convert it to the coded resolution in accordance with the output sar */
        if( out_sar_w > out_sar_h )
            d_width  *= (double)out_sar_h / out_sar_w;
        else
            d_height *= (double)out_sar_w / out_sar_h;

        /* maximally fit the new coded resolution to the box */
        double scale = X264_MIN( box_width / d_width, box_height / d_height );
        const x264_cli_csp_t *csp = x264_cli_get_csp( h->dst_csp );
        width  = round_dbl( scale * d_width,  csp->mod_width,  1 );
        height = round_dbl( scale * d_height, csp->mod_height, 1 );
    }
    else
    {
        if( str_width || str_height )
        {
            FAIL_IF_ERROR( width <= 0 || height <= 0, "invalid resolution %sx%s\n",
                           x264_otos( str_width, "unset" ), x264_otos( str_height, "unset" ) )
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
             width  = info->width;
             height = info->height;
             if( (out_sar_w * in_sar_h) > (out_sar_h * in_sar_w) ) // SAR got wider, decrease width
                 width = round_dbl( (double)info->width * in_sar_w * out_sar_h
                                  / in_sar_h / out_sar_w, csp->mod_width, 0 );
             else // SAR got thinner, decrease height
                 height = round_dbl( (double)info->height * in_sar_h * out_sar_w
                                   / in_sar_w / out_sar_h, csp->mod_height, 0 );
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
            h->dst_csp = pick_closest_supported_csp( info->csp );
        else if( handle_opts( optlist, opts, info, h ) )
            return -1;
    }
    else
    {
        h->dst_csp    = param->i_csp;
        h->dst.width  = param->i_width;
        h->dst.height = param->i_height;
    }
    uint32_t method = convert_method_to_flag( x264_otos( x264_get_option( optlist[5], opts ), "" ) );
    x264_free_string_array( opts );

    h->ctx_flags = convert_cpu_to_flag( param->cpu ) | method;
    if( method != SWS_FAST_BILINEAR )
        h->ctx_flags |= SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND;
    h->dst.pix_fmt = convert_csp_to_pix_fmt( h->dst_csp );
    h->scale = h->dst;
    /* swap chroma planes for yv12 to have it become i420 */
    h->swap_chroma = (info->csp & X264_CSP_MASK) == X264_CSP_YV12;
    int src_pix_fmt = convert_csp_to_pix_fmt( info->csp );

    /* confirm swscale can support this conversion */
    FAIL_IF_ERROR( !sws_isSupportedInput( src_pix_fmt ), "input colorspace %s is not supported\n", sws_format_name( src_pix_fmt ) )
    FAIL_IF_ERROR( !sws_isSupportedOutput( h->dst.pix_fmt ), "output colorspace %s is not supported\n", sws_format_name( h->dst.pix_fmt ) )
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
                      sws_format_name( src_pix_fmt ), sws_format_name( h->dst.pix_fmt ) );
    h->dst_csp |= info->csp & X264_CSP_VFLIP; // preserve vflip
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

static int check_resizer( resizer_hnd_t *h, cli_pic_t *in )
{
    frame_prop_t input_prop = { in->img.width, in->img.height, convert_csp_to_pix_fmt( in->img.csp ) };
    if( !memcmp( &input_prop, &h->scale, sizeof(frame_prop_t) ) )
        return 0;
    if( h->ctx )
    {
        sws_freeContext( h->ctx );
        x264_cli_log( NAME, X264_LOG_WARNING, "stream properties changed at pts %"PRId64"\n", in->pts );
    }
    h->scale = input_prop;
    if( !h->buffer_allocated )
    {
        if( x264_cli_pic_alloc( &h->buffer, h->dst_csp, h->dst.width, h->dst.height ) )
            return -1;
        h->buffer_allocated = 1;
    }
    h->ctx = sws_getContext( h->scale.width, h->scale.height, h->scale.pix_fmt, h->dst.width,
                             h->dst.height, h->dst.pix_fmt, h->ctx_flags, NULL, NULL, NULL );
    FAIL_IF_ERROR( !h->ctx, "swscale init failed\n" )
    return 0;
}

static int get_frame( hnd_t handle, cli_pic_t *output, int frame )
{
    resizer_hnd_t *h = handle;
    if( h->prev_filter.get_frame( h->prev_hnd, output, frame ) )
        return -1;
    if( check_resizer( h, output ) )
        return -1;
    if( h->ctx )
    {
        sws_scale( h->ctx, (const uint8_t* const*)output->img.plane, output->img.stride,
                   0, output->img.height, h->buffer.img.plane, h->buffer.img.stride );
        output->img = h->buffer.img; /* copy img data */
    }
    else
        output->img.csp = h->dst_csp;
    if( h->swap_chroma )
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
