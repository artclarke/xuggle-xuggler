/*****************************************************************************
 * x264: h264 encoder/decoder testing program.
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: x264.c,v 1.1 2004/06/03 19:24:12 fenrir Exp $
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
#include <string.h>

#include <math.h>

#include <signal.h>
#define _GNU_SOURCE
#include <getopt.h>

#ifdef _MSC_VER
#include <io.h>     /* _setmode() */
#include <fcntl.h>  /* _O_BINARY */
#endif

#ifdef AVIS_INPUT
#include <windows.h>
#include <vfw.h>
#endif

#ifdef MP4_OUTPUT
#include <gpac/m4_author.h>
#endif

#include "common/common.h"
#include "x264.h"
#ifndef _MSC_VER
#include "config.h"
#endif

#define DATA_MAX 3000000
uint8_t data[DATA_MAX];

typedef void *hnd_t;

/* Ctrl-C handler */
static int     b_ctrl_c = 0;
static int     b_exit_on_ctrl_c = 0;
static void    SigIntHandler( int a )
{
    if( b_exit_on_ctrl_c )
        exit(0);
    b_ctrl_c = 1;
}

typedef struct {
    int b_decompress;
    int b_progress;
    int i_maxframes;
    int i_seek;
    hnd_t hin;
    hnd_t hout;
} cli_opt_t;

/* input file operation function pointers */
static int (*p_open_infile)( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param );
static int (*p_get_frame_total)( hnd_t handle, int i_width, int i_height );
static int (*p_read_frame)( x264_picture_t *p_pic, hnd_t handle, int i_frame, int i_width, int i_height );
static int (*p_close_infile)( hnd_t handle );

static int open_file_yuv( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param );
static int get_frame_total_yuv( hnd_t handle, int i_width, int i_height );
static int read_frame_yuv( x264_picture_t *p_pic, hnd_t handle, int i_frame, int i_width, int i_height );
static int close_file_yuv( hnd_t handle );

#ifdef AVIS_INPUT
static int open_file_avis( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param );
static int get_frame_total_avis( hnd_t handle, int i_width, int i_height );
static int read_frame_avis( x264_picture_t *p_pic, hnd_t handle, int i_frame, int i_width, int i_height );
static int close_file_avis( hnd_t handle );
#endif

/* output file operation function pointers */
static int (*p_open_outfile)( char *psz_filename, hnd_t *p_handle );
static int (*p_set_outfile_param)( hnd_t handle, x264_param_t *p_param );
static int (*p_write_nalu)( hnd_t handle, uint8_t *p_nal, int i_size );
static int (*p_set_eop)( hnd_t handle, x264_picture_t *p_picture );
static int (*p_close_outfile)( hnd_t handle );

static int open_file_bsf( char *psz_filename, hnd_t *p_handle );
static int set_param_bsf( hnd_t handle, x264_param_t *p_param );
static int write_nalu_bsf( hnd_t handle, uint8_t *p_nal, int i_size );
static int set_eop_bsf( hnd_t handle,  x264_picture_t *p_picture );
static int close_file_bsf( hnd_t handle );

#ifdef MP4_OUTPUT
static int open_file_mp4( char *psz_filename, hnd_t *p_handle );
static int set_param_mp4( hnd_t handle, x264_param_t *p_param );
static int write_nalu_mp4( hnd_t handle, uint8_t *p_nal, int i_size );
static int set_eop_mp4( hnd_t handle, x264_picture_t *p_picture );
static int close_file_mp4( hnd_t handle );
#endif

static void Help( x264_param_t *defaults );
static int  Parse( int argc, char **argv, x264_param_t *param, cli_opt_t *opt );
static int  Encode( x264_param_t *param, cli_opt_t *opt );


/****************************************************************************
 * main:
 ****************************************************************************/
int main( int argc, char **argv )
{
    x264_param_t param;
    cli_opt_t opt;

#ifdef _MSC_VER
    _setmode(_fileno(stdin), _O_BINARY);    /* thanks to Marcos Morais <morais at dee.ufcg.edu.br> */
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    x264_param_default( &param );

    /* Parse command line */
    if( Parse( argc, argv, &param, &opt ) < 0 )
        return -1;

    /* Control-C handler */
    signal( SIGINT, SigIntHandler );

    return Encode( &param, &opt );
}

/*****************************************************************************
 * Help:
 *****************************************************************************/
static void Help( x264_param_t *defaults )
{
    fprintf( stderr,
             "x264 core:%d%s\n"
             "Syntax: x264 [options] -o outfile infile [widthxheight]\n"
             "\n"
             "Infile can be raw YUV 4:2:0 (in which case resolution is required),\n"
             "  or AVI or Avisynth if compiled with AVIS support (%s).\n"
             "Outfile type is selected by filename:\n"
             " .264 -> Raw bytestream\n"
             " .mp4 -> MP4 if compiled with GPAC support (%s)\n"
             "\n"
             "Options:\n"
             "\n"
             "  -h, --help                  Print this help\n"
             "\n"
             "Frame-type options:\n"
             "\n"
             "  -I, --keyint <integer>      Maximum GOP size [%d]\n"
             "  -i, --min-keyint <integer>  Minimum GOP size [%d]\n"
             "      --scenecut <integer>    How aggressively to insert extra I-frames [%d]\n"
             "  -b, --bframes <integer>     Number of B-frames between I and P [%d]\n"
             "      --no-b-adapt            Disable adaptive B-frame decision\n"
             "      --b-bias <integer>      Influences how often B-frames are used [%d]\n"
             "      --b-pyramid             Keep some B-frames as references\n"
             "\n"
             "      --no-cabac              Disable CABAC\n"
             "  -r, --ref <integer>         Number of reference frames [%d]\n"
             "      --nf                    Disable loop filter\n"
             "  -f, --filter <alpha:beta>   Loop filter AlphaC0 and Beta parameters [%d:%d]\n"
             "\n"
             "Ratecontrol:\n"
             "\n"
             "  -q, --qp <integer>          Set QP (0=lossless) [%d]\n"
             "  -B, --bitrate <integer>     Set bitrate\n"
             "      --qpmin <integer>       Set min QP [%d]\n"
             "      --qpmax <integer>       Set max QP [%d]\n"
             "      --qpstep <integer>      Set max QP step [%d]\n"
             "      --ratetol <float>       Allowed variance of average bitrate [%.1f]\n"
             "      --vbv-maxrate <integer> Max local bitrate [%d]\n"
             "      --vbv-bufsize <integer> Size of VBV buffer [%d]\n"
             "      --vbv-init <float>      Initial VBV buffer occupancy [%.1f]\n"
             "\n"
             "      --ipratio <float>       QP factor between I and P [%.2f]\n"
             "      --pbratio <float>       QP factor between P and B [%.2f]\n"
             "      --chroma-qp-offset <integer>  QP difference between chroma and luma [%d]\n"
             "\n"
             "  -p, --pass <1|2|3>          Enable multipass ratecontrol:\n"
             "                                  - 1: First pass, creates stats file\n"
             "                                  - 2: Last pass, does not overwrite stats file\n"
             "                                  - 3: Nth pass, overwrites stats file\n"
             "      --stats <string>        Filename for 2 pass stats [\"%s\"]\n"
             "      --rceq <string>         Ratecontrol equation [\"%s\"]\n"
             "      --qcomp <float>         QP curve compression: 0.0 => CBR, 1.0 => CQP [%.2f]\n"
             "      --cplxblur <float>      Reduce fluctuations in QP (before curve compression) [%.1f]\n"
             "      --qblur <float>         Reduce fluctuations in QP (after curve compression) [%.1f]\n"
             "\n"
             "      --zones <zone0>/<zone1>/...\n"
             "                              Tweak the bitrate of some regions of the video\n"
             "                              Each zone is of the form\n"
             "                                  <start frame>,<end frame>,<option>\n"
             "                                  where <option> is either\n"
             "                                      q=<integer> (force QP)\n"
             "                                  or  b=<float> (bitrate multiplier)\n"
             "\n"
             "Analysis:\n"
             "\n"
             "  -A, --analyse <string>      Partitions to consider [\"p8x8,b8x8,i8x8,i4x4\"]\n"
             "                                  - p8x8, p4x4, b8x8, i8x8, i4x4\n"
             "                                  - none, all\n"
             "                                  (p4x4 requires p8x8. i8x8 requires --8x8dct.)\n"
             "      --direct <string>       Direct MV prediction mode [\"temporal\"]\n"
             "                                  - none, spatial, temporal\n"
             "  -w, --weightb               Weighted prediction for B-frames\n"
             "      --me <string>           Integer pixel motion estimation method [\"%s\"]\n"
             "                                  - dia: diamond search, radius 1 (fast)\n"
             "                                  - hex: hexagonal search, radius 2\n"
             "                                  - umh: uneven multi-hexagon search\n"
             "                                  - esa: exhaustive search (slow)\n"
             "      --merange <integer>     Maximum motion vector search range [%d]\n"
             "  -m, --subme <integer>       Subpixel motion estimation and partition\n"
             "                                  decision quality: 1=fast, 6=best. [%d]\n"
             "      --no-chroma-me          Ignore chroma in motion estimation\n"
             "  -8, --8x8dct                Adaptive spatial transform size\n"
             "\n"
             "      --cqm <string>          Preset quant matrices [\"flat\"]\n"
             "                                  - jvt, flat\n"
             "      --cqmfile <string>      Read quant matrices from a JM-compatible file\n"
             "                                  Overrides any other --cqm* options.\n"
             "      --cqm4 <list>           Set all 4x4 quant matrices\n"
             "                                  Takes a comma-separated list of 16 integers.\n"
             "      --cqm8 <list>           Set all 8x8 quant matrices\n"
             "                                  Takes a comma-separated list of 64 integers.\n"
             "      --cqm4i, --cqm4p, --cqm8i, --cqm8p\n"
             "                              Set both luma and chroma quant matrices\n"
             "      --cqm4iy, --cqm4ic, --cqm4py, --cqm4pc\n"
             "                              Set individual quant matrices\n"
             "\n"
             "Input/Output:\n"
             "\n"
             "      --level <integer>       Specify level (as defined by Annex A)\n"
             "      --sar width:height      Specify Sample Aspect Ratio\n"
             "      --fps <float|rational>  Specify framerate\n"
             "      --seek <integer>        First frame to encode\n"
             "      --frames <integer>      Maximum number of frames to encode\n"
             "  -o, --output                Specify output file\n"
             "\n"
             "      --threads <integer>     Parallel encoding (uses slices)\n"
             "      --no-asm                Disable all CPU optimizations\n"
             "      --no-psnr               Disable PSNR computation\n"
             "      --quiet                 Quiet Mode\n"
             "  -v, --verbose               Print stats for each frame\n"
             "      --progress              Show a progress indicator while encoding\n"
             "      --visualize             Show MB types overlayed on the encoded video\n"
             "      --aud                   Use access unit delimiters\n"
             "\n",
            X264_BUILD, X264_VERSION,
#ifdef AVIS_INPUT
            "yes",
#else
            "no",
#endif
#ifdef MP4_OUTPUT
            "yes",
#else
            "no",
#endif
            defaults->i_keyint_max,
            defaults->i_keyint_min,
            defaults->i_scenecut_threshold,
            defaults->i_bframe,
            defaults->i_bframe_bias,
            defaults->i_frame_reference,
            defaults->i_deblocking_filter_alphac0,
            defaults->i_deblocking_filter_beta,
            defaults->rc.i_qp_constant,
            defaults->rc.i_qp_min,
            defaults->rc.i_qp_max,
            defaults->rc.i_qp_step,
            defaults->rc.f_rate_tolerance,
            defaults->rc.i_vbv_max_bitrate,
            defaults->rc.i_vbv_buffer_size,
            defaults->rc.f_vbv_buffer_init,
            defaults->rc.f_ip_factor,
            defaults->rc.f_pb_factor,
            defaults->analyse.i_chroma_qp_offset,
            defaults->rc.psz_stat_out,
            defaults->rc.psz_rc_eq,
            defaults->rc.f_qcompress,
            defaults->rc.f_complexity_blur,
            defaults->rc.f_qblur,
            defaults->analyse.i_me_method==X264_ME_DIA ? "dia"
            : defaults->analyse.i_me_method==X264_ME_HEX ? "hex"
            : defaults->analyse.i_me_method==X264_ME_UMH ? "umh"
            : defaults->analyse.i_me_method==X264_ME_ESA ? "esa" : NULL,
            defaults->analyse.i_me_range,
            defaults->analyse.i_subpel_refine
           );
}

static int parse_cqm( const char *str, uint8_t *cqm, int length )
{
    int i = 0;
    do {
        int coef;
        if( !sscanf( str, "%d", &coef ) || coef < 1 || coef > 255 )
            return -1;
        cqm[i++] = coef;
    } while( i < length && (str = strchr( str, ',' )) && str++ );
    return (i == length) ? 0 : -1;
}

/*****************************************************************************
 * Parse:
 *****************************************************************************/
static int  Parse( int argc, char **argv,
                   x264_param_t *param, cli_opt_t *opt )
{
    char *psz_filename = NULL;
    x264_param_t defaults = *param;
    char *psz;
    char b_avis = 0;

    memset( opt, 0, sizeof(cli_opt_t) );

    /* Default input file driver */
    p_open_infile = open_file_yuv;
    p_get_frame_total = get_frame_total_yuv;
    p_read_frame = read_frame_yuv;
    p_close_infile = close_file_yuv;

    /* Default output file driver */
    p_open_outfile = open_file_bsf;
    p_set_outfile_param = set_param_bsf;
    p_write_nalu = write_nalu_bsf;
    p_set_eop = set_eop_bsf;
    p_close_outfile = close_file_bsf;

    /* Parse command line options */
    opterr = 0; // no error message
    for( ;; )
    {
        int b_error = 0;
        int long_options_index;
#define OPT_QPMIN 256
#define OPT_QPMAX 257
#define OPT_QPSTEP 258
#define OPT_IPRATIO 260
#define OPT_PBRATIO 261
#define OPT_RATETOL 262
#define OPT_RCSTATS 264
#define OPT_RCEQ 265
#define OPT_QCOMP 266
#define OPT_NOPSNR 267
#define OPT_QUIET 268
#define OPT_SCENECUT 270
#define OPT_QBLUR 271
#define OPT_CPLXBLUR 272
#define OPT_FRAMES 273
#define OPT_FPS 274
#define OPT_DIRECT 275
#define OPT_LEVEL 276
#define OPT_NOBADAPT 277
#define OPT_BBIAS 278
#define OPT_BPYRAMID 279
#define OPT_CHROMA_QP 280
#define OPT_NO_CHROMA_ME 281
#define OPT_NO_CABAC 282
#define OPT_AUD 283
#define OPT_PROGRESS 284
#define OPT_ME 285
#define OPT_MERANGE 286
#define OPT_VBVMAXRATE 287
#define OPT_VBVBUFSIZE 288
#define OPT_VBVINIT 289
#define OPT_VISUALIZE 290
#define OPT_SEEK 291
#define OPT_ZONES 292
#define OPT_THREADS 293
#define OPT_CQM 294
#define OPT_CQM4 295
#define OPT_CQM4I 296
#define OPT_CQM4IY 297
#define OPT_CQM4IC 298
#define OPT_CQM4P 299
#define OPT_CQM4PY 300
#define OPT_CQM4PC 301
#define OPT_CQM8 302
#define OPT_CQM8I 303
#define OPT_CQM8P 304
#define OPT_CQMFILE 305

        static struct option long_options[] =
        {
            { "help",    no_argument,       NULL, 'h' },
            { "bitrate", required_argument, NULL, 'B' },
            { "bframes", required_argument, NULL, 'b' },
            { "no-b-adapt", no_argument,    NULL, OPT_NOBADAPT },
            { "b-bias",  required_argument, NULL, OPT_BBIAS },
            { "b-pyramid", no_argument,     NULL, OPT_BPYRAMID },
            { "min-keyint",required_argument,NULL,'i' },
            { "keyint",  required_argument, NULL, 'I' },
            { "scenecut",required_argument, NULL, OPT_SCENECUT },
            { "nf",      no_argument,       NULL, 'n' },
            { "filter",  required_argument, NULL, 'f' },
            { "no-cabac",no_argument,       NULL, OPT_NO_CABAC },
            { "qp",      required_argument, NULL, 'q' },
            { "qpmin",   required_argument, NULL, OPT_QPMIN },
            { "qpmax",   required_argument, NULL, OPT_QPMAX },
            { "qpstep",  required_argument, NULL, OPT_QPSTEP },
            { "ref",     required_argument, NULL, 'r' },
            { "no-asm",  no_argument,       NULL, 'C' },
            { "sar",     required_argument, NULL, 's' },
            { "fps",     required_argument, NULL, OPT_FPS },
            { "frames",  required_argument, NULL, OPT_FRAMES },
            { "seek",    required_argument, NULL, OPT_SEEK },
            { "output",  required_argument, NULL, 'o' },
            { "analyse", required_argument, NULL, 'A' },
            { "direct",  required_argument, NULL, OPT_DIRECT },
            { "weightb", no_argument,       NULL, 'w' },
            { "me",      required_argument, NULL, OPT_ME },
            { "merange", required_argument, NULL, OPT_MERANGE },
            { "subme",   required_argument, NULL, 'm' },
            { "no-chroma-me", no_argument,  NULL, OPT_NO_CHROMA_ME },
            { "8x8dct",  no_argument,       NULL, '8' },
            { "level",   required_argument, NULL, OPT_LEVEL },
            { "ratetol", required_argument, NULL, OPT_RATETOL },
            { "vbv-maxrate", required_argument, NULL, OPT_VBVMAXRATE },
            { "vbv-bufsize", required_argument, NULL, OPT_VBVBUFSIZE },
            { "vbv-init", required_argument,NULL,  OPT_VBVINIT },
            { "ipratio", required_argument, NULL, OPT_IPRATIO },
            { "pbratio", required_argument, NULL, OPT_PBRATIO },
            { "chroma-qp-offset", required_argument, NULL, OPT_CHROMA_QP },
            { "pass",    required_argument, NULL, 'p' },
            { "stats",   required_argument, NULL, OPT_RCSTATS },
            { "rceq",    required_argument, NULL, OPT_RCEQ },
            { "qcomp",   required_argument, NULL, OPT_QCOMP },
            { "qblur",   required_argument, NULL, OPT_QBLUR },
            { "cplxblur",required_argument, NULL, OPT_CPLXBLUR },
            { "zones",   required_argument, NULL, OPT_ZONES },
            { "threads", required_argument, NULL, OPT_THREADS },
            { "no-psnr", no_argument,       NULL, OPT_NOPSNR },
            { "quiet",   no_argument,       NULL, OPT_QUIET },
            { "verbose", no_argument,       NULL, 'v' },
            { "progress",no_argument,       NULL, OPT_PROGRESS },
            { "visualize",no_argument,      NULL, OPT_VISUALIZE },
            { "aud",     no_argument,       NULL, OPT_AUD },
            { "cqm",     required_argument, NULL, OPT_CQM },
            { "cqmfile", required_argument, NULL, OPT_CQMFILE },
            { "cqm4",    required_argument, NULL, OPT_CQM4 },
            { "cqm4i",   required_argument, NULL, OPT_CQM4I },
            { "cqm4iy",  required_argument, NULL, OPT_CQM4IY },
            { "cqm4ic",  required_argument, NULL, OPT_CQM4IC },
            { "cqm4p",   required_argument, NULL, OPT_CQM4P },
            { "cqm4py",  required_argument, NULL, OPT_CQM4PY },
            { "cqm4pc",  required_argument, NULL, OPT_CQM4PC },
            { "cqm8",    required_argument, NULL, OPT_CQM8 },
            { "cqm8i",   required_argument, NULL, OPT_CQM8I },
            { "cqm8p",   required_argument, NULL, OPT_CQM8P },
            {0, 0, 0, 0}
        };

        int c;

        c = getopt_long( argc, argv, "hi:I:b:r:cxB:q:f:o:A:m:p:vw8",
                         long_options, &long_options_index);

        if( c == -1 )
        {
            break;
        }

        switch( c )
        {
            case 'h':
                Help( &defaults );
                return -1;

            case 0:
                break;
            case 'B':
                param->rc.i_bitrate = atol( optarg );
                param->rc.b_cbr = 1;
                break;
            case 'b':
                param->i_bframe = atol( optarg );
                break;
            case OPT_NOBADAPT:
                param->b_bframe_adaptive = 0;
                break;
            case OPT_BBIAS:
                param->i_bframe_bias = atol( optarg );
                break;
            case OPT_BPYRAMID:
                param->b_bframe_pyramid = 1;
                break;
            case 'i':
                param->i_keyint_min = atol( optarg );
                if( param->i_keyint_max < param->i_keyint_min )
                    param->i_keyint_max = param->i_keyint_min;
                break;
            case 'I':
                param->i_keyint_max = atol( optarg );
                if( param->i_keyint_min > param->i_keyint_max )
                    param->i_keyint_min = param->i_keyint_max;
                break;
            case OPT_SCENECUT:
                param->i_scenecut_threshold = atol( optarg );
                break;
            case 'n':
                param->b_deblocking_filter = 0;
                break;
            case 'f':
            {
                char *p = strchr( optarg, ':' );
                param->i_deblocking_filter_alphac0 = atoi( optarg );
                param->i_deblocking_filter_beta = p ? atoi( p+1 ) : param->i_deblocking_filter_alphac0;
                break;
            }
            case 'q':
                param->rc.i_qp_constant = atoi( optarg );
                break;
            case OPT_QPMIN:
                param->rc.i_qp_min = atoi( optarg );
                break;
            case OPT_QPMAX:
                param->rc.i_qp_max = atoi( optarg );
                break;
            case OPT_QPSTEP:
                param->rc.i_qp_step = atoi( optarg );
                break;
            case 'r':
                param->i_frame_reference = atoi( optarg );
                break;
            case OPT_NO_CABAC:
                param->b_cabac = 0;
                break;
            case 'x':
                opt->b_decompress = 1;
                break;
            case 'C':
                param->cpu = 0;
                break;
            case OPT_FRAMES:
                opt->i_maxframes = atoi( optarg );
                break;
            case OPT_SEEK:
                opt->i_seek = atoi( optarg );
                break;
            case'o':
                if( !strncasecmp(optarg + strlen(optarg) - 4, ".mp4", 4) )
                {
#ifdef MP4_OUTPUT
                    p_open_outfile = open_file_mp4;
                    p_write_nalu = write_nalu_mp4;
                    p_set_outfile_param = set_param_mp4;
                    p_set_eop = set_eop_mp4;
                    p_close_outfile = close_file_mp4;
#else
                    fprintf( stderr, "not compiled with MP4 output support\n" );
                    return -1;
#endif
                }
                if( !strcmp(optarg, "-") )
                    opt->hout = stdout;
                else if( p_open_outfile( optarg, &opt->hout ) )
                {
                    fprintf( stderr, "cannot open output file `%s'\n", optarg );
                    return -1;
                }
                break;
            case 's':
            {
                char *p = strchr( optarg, ':' );
                if( p )
                {
                    param->vui.i_sar_width = atoi( optarg );
                    param->vui.i_sar_height = atoi( p + 1 );
                }
                break;
            }
            case OPT_FPS:
            {
                float fps;
                if( sscanf( optarg, "%d/%d", &param->i_fps_num, &param->i_fps_den ) == 2 )
                    ;
                else if( sscanf( optarg, "%f", &fps ) )
                {
                    param->i_fps_num = (int)(fps * 1000 + .5);
                    param->i_fps_den = 1000;
                }
                else
                {
                    fprintf( stderr, "bad fps `%s'\n", optarg );
                    return -1;
                }
                break;
            }
            case 'A':
                param->analyse.inter = 0;
                if( strstr( optarg, "none" ) )  param->analyse.inter =  0;
                if( strstr( optarg, "all" ) )   param->analyse.inter = ~0;

                if( strstr( optarg, "i4x4" ) )  param->analyse.inter |= X264_ANALYSE_I4x4;
                if( strstr( optarg, "i8x8" ) )  param->analyse.inter |= X264_ANALYSE_I8x8;
                if( strstr( optarg, "p8x8" ) )  param->analyse.inter |= X264_ANALYSE_PSUB16x16;
                if( strstr( optarg, "p4x4" ) )  param->analyse.inter |= X264_ANALYSE_PSUB8x8;
                if( strstr( optarg, "b8x8" ) )  param->analyse.inter |= X264_ANALYSE_BSUB16x16;
                break;
            case OPT_DIRECT:
                if( strstr( optarg, "temporal" ) )
                    param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_TEMPORAL;
                else if( strstr( optarg, "spatial" ) )
                    param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_SPATIAL;
                else if( strstr( optarg, "none" ) )
                    param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_NONE;
                else
                    param->analyse.i_direct_mv_pred = atoi( optarg );
                break;
            case 'w':
                param->analyse.b_weighted_bipred = 1;
                break;
            case OPT_ME:
                param->analyse.i_me_method = 
                    strstr( optarg, "dia" ) ? X264_ME_DIA :
                    strstr( optarg, "hex" ) ? X264_ME_HEX :
                    strstr( optarg, "umh" ) ? X264_ME_UMH :
                    strstr( optarg, "esa" ) ? X264_ME_ESA : -1;
                if( param->analyse.i_me_method == -1 )
                {
                    fprintf( stderr, "bad ME method `%s'\n", optarg );
                    return -1;
                }
                break;
            case OPT_MERANGE:
                param->analyse.i_me_range = atoi(optarg);
                break;
            case 'm':
                param->analyse.i_subpel_refine = atoi(optarg);
                break;
            case OPT_NO_CHROMA_ME:
                param->analyse.b_chroma_me = 0;
                break;
            case '8':
                param->analyse.b_transform_8x8 = 1;
                break;
            case OPT_LEVEL:
                param->i_level_idc = atoi(optarg);
                break;
            case OPT_RATETOL:
                param->rc.f_rate_tolerance = atof(optarg);
                break;
            case OPT_VBVMAXRATE:
                param->rc.i_vbv_max_bitrate = atoi( optarg );
                break;
            case OPT_VBVBUFSIZE:
                param->rc.i_vbv_buffer_size = atoi( optarg );
                break;
            case OPT_VBVINIT:
                param->rc.f_vbv_buffer_init = atof(optarg);
                break;
            case OPT_IPRATIO:
                param->rc.f_ip_factor = atof(optarg);
                break;
            case OPT_PBRATIO:
                param->rc.f_pb_factor = atof(optarg);
                break;
            case OPT_CHROMA_QP:
                param->analyse.i_chroma_qp_offset = atoi(optarg);
                break;
            case 'p':
            {
                int i_pass = atoi(optarg);
                if( i_pass == 1 )
                    param->rc.b_stat_write = 1;
                else if( i_pass == 2 )
                    param->rc.b_stat_read = 1;
                else if( i_pass > 2 )
                    param->rc.b_stat_read =
                    param->rc.b_stat_write = 1;
                break;
            }
            case OPT_RCSTATS:
                param->rc.psz_stat_in = optarg;
                param->rc.psz_stat_out = optarg;
                break;
            case OPT_RCEQ:
                param->rc.psz_rc_eq = optarg;
               break;
            case OPT_QCOMP:
                param->rc.f_qcompress = atof(optarg);
                break;
            case OPT_QBLUR:
                param->rc.f_qblur = atof(optarg);
                break;
            case OPT_CPLXBLUR:
                param->rc.f_complexity_blur = atof(optarg);
                break;
            case OPT_ZONES:
                param->rc.psz_zones = optarg;
                break;
            case OPT_THREADS:
                param->i_threads = atoi(optarg);
                break;
            case OPT_NOPSNR:
                param->analyse.b_psnr = 0;
                break;
            case OPT_QUIET:
                param->i_log_level = X264_LOG_NONE;
                break;
            case 'v':
                param->i_log_level = X264_LOG_DEBUG;
                break;
            case OPT_AUD:
                param->b_aud = 1;
                break;
            case OPT_PROGRESS:
                opt->b_progress = 1;
                break;
            case OPT_VISUALIZE:
#ifdef VISUALIZE
                param->b_visualize = 1;
                b_exit_on_ctrl_c = 1;
#else
                fprintf( stderr, "not compiled with visualization support\n" );
#endif
                break;
            case OPT_CQM:
                if( strstr( optarg, "flat" ) )
                    param->i_cqm_preset = X264_CQM_FLAT;
                else if( strstr( optarg, "jvt" ) )
                    param->i_cqm_preset = X264_CQM_JVT;
                else
                {
                    fprintf( stderr, "bad CQM preset `%s'\n", optarg );
                    return -1;
                }
                break;
            case OPT_CQMFILE:
                param->psz_cqm_file = optarg;
                break;
            case OPT_CQM4:
                param->i_cqm_preset = X264_CQM_CUSTOM;
                b_error |= parse_cqm( optarg, param->cqm_4iy, 16 );
                b_error |= parse_cqm( optarg, param->cqm_4ic, 16 );
                b_error |= parse_cqm( optarg, param->cqm_4py, 16 );
                b_error |= parse_cqm( optarg, param->cqm_4pc, 16 );
                break;
            case OPT_CQM8:
                param->i_cqm_preset = X264_CQM_CUSTOM;
                b_error |= parse_cqm( optarg, param->cqm_8iy, 64 );
                b_error |= parse_cqm( optarg, param->cqm_8py, 64 );
                break;
            case OPT_CQM4I:
                param->i_cqm_preset = X264_CQM_CUSTOM;
                b_error |= parse_cqm( optarg, param->cqm_4iy, 16 );
                b_error |= parse_cqm( optarg, param->cqm_4ic, 16 );
                break;
            case OPT_CQM4P:
                param->i_cqm_preset = X264_CQM_CUSTOM;
                b_error |= parse_cqm( optarg, param->cqm_4py, 16 );
                b_error |= parse_cqm( optarg, param->cqm_4pc, 16 );
                break;
            case OPT_CQM4IY:
                param->i_cqm_preset = X264_CQM_CUSTOM;
                b_error |= parse_cqm( optarg, param->cqm_4iy, 16 );
                break;
            case OPT_CQM4IC:
                param->i_cqm_preset = X264_CQM_CUSTOM;
                b_error |= parse_cqm( optarg, param->cqm_4ic, 16 );
                break;
            case OPT_CQM4PY:
                param->i_cqm_preset = X264_CQM_CUSTOM;
                b_error |= parse_cqm( optarg, param->cqm_4iy, 16 );
                break;
            case OPT_CQM4PC:
                param->i_cqm_preset = X264_CQM_CUSTOM;
                b_error |= parse_cqm( optarg, param->cqm_4ic, 16 );
                break;
            case OPT_CQM8I:
                param->i_cqm_preset = X264_CQM_CUSTOM;
                b_error |= parse_cqm( optarg, param->cqm_8iy, 64 );
                break;
            case OPT_CQM8P:
                param->i_cqm_preset = X264_CQM_CUSTOM;
                b_error |= parse_cqm( optarg, param->cqm_8py, 64 );
                break;
            default:
                fprintf( stderr, "unknown option (%c)\n", optopt );
                return -1;
        }

        if( b_error )
        {
            fprintf( stderr, "bad argument (%s)\n", optarg );
            return -1;
        }
    }

    /* Get the file name */
    if( optind > argc - 1 || !opt->hout )
    {
        Help( &defaults );
        return -1;
    }
    psz_filename = argv[optind++];

    if( !opt->b_decompress )
    {
        if( optind > argc - 1 )
        {
            /* try to parse the file name */
            for( psz = psz_filename; *psz; psz++ )
            {
                if( *psz >= '0' && *psz <= '9'
                    && sscanf( psz, "%ux%u", &param->i_width, &param->i_height ) == 2 )
                {
                    if( param->i_log_level > X264_LOG_NONE )
                        fprintf( stderr, "x264: file name gives %dx%d\n", param->i_width, param->i_height );
                    break;
                }
            }
        }
        else
        {
            sscanf( argv[optind++], "%ux%u", &param->i_width, &param->i_height );
        }
        
        /* check avis input */
        psz = psz_filename + strlen(psz_filename) - 1;
        while( psz > psz_filename && *psz != '.' )
            psz--;

        if( !strncasecmp( psz, ".avi", 4 ) || !strncasecmp( psz, ".avs", 4 ) )
            b_avis = 1;

        if( !b_avis && ( !param->i_width || !param->i_height ) )
        {
            Help( &defaults );
            return -1;
        }
    }

    /* open the input */
    if( !strcmp( psz_filename, "-" ) )
    {
        opt->hin = stdin;
        optind++;
    }
    else
    {
        if( b_avis )
        {
#ifdef AVIS_INPUT
            p_open_infile = open_file_avis;
            p_get_frame_total = get_frame_total_avis;
            p_read_frame = read_frame_avis;
            p_close_infile = close_file_avis;
#else
            fprintf( stderr, "not compiled with AVIS input support\n" );
            return -1;
#endif
        }
        if( p_open_infile( psz_filename, &opt->hin, param ) )
        {
            fprintf( stderr, "could not open input file '%s'\n", psz_filename );
            return -1;
        }
    }

    return 0;
}

/*****************************************************************************
 * Decode:
 *****************************************************************************/
#if 0
static int  Decode( x264_param_t  *param, FILE *fh26l, hnd_t hout )
{
    fprintf( stderr, "decompressor not working (help is welcome)\n" );
    return -1;
    x264_nal_t nal;
    int i_data;
    int b_eof;

    //param.cpu = 0;
    if( ( h = x264_decoder_open( &param ) ) == NULL )
    {
        fprintf( stderr, "x264_decoder_open failed\n" );
        return -1;
    }

    i_start = x264_mdate();
    b_eof = 0;
    i_frame = 0;
    i_data  = 0;
    nal.p_payload = malloc( DATA_MAX );

    while( !b_ctrl_c )
    {
        uint8_t *p, *p_next, *end;
        int i_size;
        /* fill buffer */
        if( i_data < DATA_MAX && !b_eof )
        {
            int i_read = fread( &data[i_data], 1, DATA_MAX - i_data, fh26l );
            if( i_read <= 0 )
            {
                b_eof = 1;
            }
            else
            {
                i_data += i_read;
            }
        }

        if( i_data < 3 )
        {
            break;
        }

        end = &data[i_data];

        /* extract one nal */
        p = &data[0];
        while( p < end - 3 )
        {
            if( p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x01 )
            {
                break;
            }
            p++;
        }

        if( p >= end - 3 )
        {
            fprintf( stderr, "garbage (i_data = %d)\n", i_data );
            i_data = 0;
            continue;
        }

        p_next = p + 3;
        while( p_next < end - 3 )
        {
            if( p_next[0] == 0x00 && p_next[1] == 0x00 && p_next[2] == 0x01 )
            {
                break;
            }
            p_next++;
        }

        if( p_next == end - 3 && i_data < DATA_MAX )
        {
            p_next = end;
        }

        /* decode this nal */
        i_size = p_next - p - 3;
        if( i_size <= 0 )
        {
            if( b_eof )
            {
                break;
            }
            fprintf( stderr, "nal too large (FIXME) ?\n" );
            i_data = 0;
            continue;
        }

        x264_nal_decode( &nal, p +3, i_size );

        /* decode the content of the nal */
        x264_decoder_decode( h, &pic, &nal );

        if( pic != NULL )
        {
            int i;

            i_frame++;

            for( i = 0; i < pic->i_plane;i++ )
            {
                int i_line;
                int i_div;

                i_div = i==0 ? 1 : 2;
                for( i_line = 0; i_line < pic->i_height/i_div; i_line++ )
                {
                    fwrite( pic->plane[i]+i_line*pic->i_stride[i], 1, pic->i_width/i_div, hout );
                }
            }
        }

        memmove( &data[0], p_next, end - p_next );
        i_data -= p_next - &data[0];
    }

    i_end = x264_mdate();
    free( nal.p_payload );
    fprintf( stderr, "\n" );

    x264_decoder_close( h );

    fclose( fh26l );
    if( hout != stdout )
    {
        fclose( hout );
    }
    if( i_frame > 0 )
    {
        double fps = (double)i_frame * (double)1000000 /
                     (double)( i_end - i_start );
        fprintf( stderr, "decoded %d frames %ffps\n", i_frame, fps );
    }
}
#endif

static int  Encode_frame( x264_t *h, hnd_t hout, x264_picture_t *pic )
{
    x264_picture_t pic_out;
    x264_nal_t *nal;
    int i_nal, i;
    int i_file = 0;

    /* Do not force any parameters */
    if( pic )
    {
        pic->i_type = X264_TYPE_AUTO;
        pic->i_qpplus1 = 0;
    }
    if( x264_encoder_encode( h, &nal, &i_nal, pic, &pic_out ) < 0 )
    {
        fprintf( stderr, "x264_encoder_encode failed\n" );
    }

    for( i = 0; i < i_nal; i++ )
    {
        int i_size;
        int i_data;

        i_data = DATA_MAX;
        if( ( i_size = x264_nal_encode( data, &i_data, 1, &nal[i] ) ) > 0 )
        {
            i_file += p_write_nalu( hout, data, i_size );
        }
        else if( i_size < 0 )
        {
            fprintf( stderr, "need to increase buffer size (size=%d)\n", -i_size );
        }
    }
    if (i_nal)
        p_set_eop( hout, &pic_out );

    return i_file;
}

/*****************************************************************************
 * Encode:
 *****************************************************************************/
static int  Encode( x264_param_t *param, cli_opt_t *opt )
{
    x264_t *h;
    x264_picture_t pic;

    int     i_frame, i_frame_total;
    int64_t i_start, i_end;
    int64_t i_file;
    int     i_frame_size;
    int     i_progress;

    i_frame_total = p_get_frame_total( opt->hin, param->i_width, param->i_height );

    if( ( h = x264_encoder_open( param ) ) == NULL )
    {
        fprintf( stderr, "x264_encoder_open failed\n" );
        p_close_infile( opt->hin );
        p_close_outfile( opt->hout );
        return -1;
    }

    if( p_set_outfile_param( opt->hout, param ) )
    {
        fprintf( stderr, "can't set outfile param\n" );
        p_close_infile( opt->hin );
        p_close_outfile( opt->hout );
        return -1;
    }

    /* Create a new pic */
    x264_picture_alloc( &pic, X264_CSP_I420, param->i_width, param->i_height );

    i_start = x264_mdate();
    /* Encode frames */
    i_frame_total -= opt->i_seek;
    if( opt->i_maxframes > 0 && opt->i_maxframes < i_frame_total )
        i_frame_total = opt->i_maxframes;
    for( i_frame = 0, i_file = 0, i_progress = 0;
         b_ctrl_c == 0 && (i_frame < i_frame_total || i_frame_total == 0); )
    {
        if( p_read_frame( &pic, opt->hin, i_frame + opt->i_seek, param->i_width, param->i_height ) )
            break;

        pic.i_pts = i_frame * param->i_fps_den;

        i_file += Encode_frame( h, opt->hout, &pic );

        i_frame++;

        /* update status line (up to 1000 times per input file) */
        if( opt->b_progress && param->i_log_level < X264_LOG_DEBUG && 
            i_frame * 1000 / i_frame_total > i_progress )
        {
            int64_t i_elapsed = x264_mdate() - i_start;
            double fps = i_elapsed > 0 ? i_frame * 1000000. / i_elapsed : 0;
            i_progress = i_frame * 1000 / i_frame_total;
            fprintf( stderr, "encoded frames: %d/%d (%.1f%%), %.2f fps   \r", i_frame,
                     i_frame_total, (float)i_progress / 10, fps );
            fflush( stderr ); // needed in windows
        }
    }
    /* Flush delayed B-frames */
    do {
        i_file +=
        i_frame_size = Encode_frame( h, opt->hout, NULL );
    } while( i_frame_size );

    i_end = x264_mdate();
    x264_picture_clean( &pic );
    x264_encoder_close( h );
    fprintf( stderr, "\n" );

    if( b_ctrl_c )
        fprintf( stderr, "aborted at input frame %d\n", opt->i_seek + i_frame );

    p_close_infile( opt->hin );
    p_close_outfile( opt->hout );

    if( i_frame > 0 )
    {
        double fps = (double)i_frame * (double)1000000 /
                     (double)( i_end - i_start );

        fprintf( stderr, "encoded %d frames, %.2f fps, %.2f kb/s\n", i_frame, fps,
                 (double) i_file * 8 * param->i_fps_num / ( param->i_fps_den * i_frame * 1000 ) );
    }

    return 0;
}

/* raw 420 yuv file operation */
static int open_file_yuv( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param )
{
    if ((*p_handle = fopen(psz_filename, "rb")) == NULL)
        return -1;
    return 0;
}

static int get_frame_total_yuv( hnd_t handle, int i_width, int i_height )
{
    FILE *f = (FILE *)handle;
    int i_frame_total = 0;

    if( !fseek( f, 0, SEEK_END ) )
    {
        int64_t i_size = ftell( f );
        fseek( f, 0, SEEK_SET );
        i_frame_total = (int)(i_size / ( i_width * i_height * 3 / 2 ));
    }

    return i_frame_total;
}

static int read_frame_yuv( x264_picture_t *p_pic, hnd_t handle, int i_frame, int i_width, int i_height )
{
    static int prev_frame = -1;
    FILE *f = (FILE *)handle;

    if( i_frame != prev_frame+1 )
        if( fseek( f, i_frame * i_width * i_height * 3 / 2, SEEK_SET ) )
            return -1;

    if( fread( p_pic->img.plane[0], 1, i_width * i_height, f ) <= 0
            || fread( p_pic->img.plane[1], 1, i_width * i_height / 4, f ) <= 0
            || fread( p_pic->img.plane[2], 1, i_width * i_height / 4, f ) <= 0 )
        return -1;

    prev_frame = i_frame;

    return 0;
}

static int close_file_yuv(hnd_t handle)
{
    if (handle == NULL)
        return 0;
    return fclose((FILE *)handle);
}


/* avs/avi input file support under cygwin */

#ifdef AVIS_INPUT

static int gcd(int a, int b)
{
    int c;

    while (1)
    {
        c = a % b;
        if (!c)
            return b;
        a = b;
        b = c;
    }
}

static int open_file_avis( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param )
{
    AVISTREAMINFO info;
    PAVISTREAM p_avi = NULL;
    int i;

    *p_handle = NULL;

    AVIFileInit();
    if( AVIStreamOpenFromFile( &p_avi, psz_filename, streamtypeVIDEO, 0, OF_READ, NULL ) )
    {
        AVIFileExit();
        return -1;
    }

    if( AVIStreamInfo(p_avi, &info, sizeof(AVISTREAMINFO)) )
    {
        AVIStreamRelease(p_avi);
        AVIFileExit();
        return -1;
    }

    // check input format
    if (info.fccHandler != MAKEFOURCC('Y', 'V', '1', '2'))
    {
        fprintf( stderr, "avis [error]: unsupported input format (%c%c%c%c)\n",
            (char)(info.fccHandler & 0xff), (char)((info.fccHandler >> 8) & 0xff),
            (char)((info.fccHandler >> 16) & 0xff), (char)((info.fccHandler >> 24)) );

        AVIStreamRelease(p_avi);
        AVIFileExit();

        return -1;
    }

    p_param->i_width = info.rcFrame.right - info.rcFrame.left;
    p_param->i_height = info.rcFrame.bottom - info.rcFrame.top;
    i = gcd(info.dwRate, info.dwScale);
    p_param->i_fps_den = info.dwScale / i;
    p_param->i_fps_num = info.dwRate / i;

    fprintf( stderr, "avis [info]: %dx%d @ %.2f fps (%d frames)\n",  
        p_param->i_width, p_param->i_height,
        (double)p_param->i_fps_num / (double)p_param->i_fps_den,
        (int)info.dwLength );

    *p_handle = (hnd_t)p_avi;

    return 0;
}

static int get_frame_total_avis( hnd_t handle, int i_width, int i_height )
{
    PAVISTREAM p_avi = (PAVISTREAM)handle;
    AVISTREAMINFO info;

    if( AVIStreamInfo(p_avi, &info, sizeof(AVISTREAMINFO)) )
        return -1;

    return info.dwLength;
}

static int read_frame_avis( x264_picture_t *p_pic, hnd_t handle, int i_frame, int i_width, int i_height )
{
    PAVISTREAM p_avi = (PAVISTREAM)handle;
    
    p_pic->img.i_csp = X264_CSP_YV12;
    
    if( AVIStreamRead(p_avi, i_frame, 1, p_pic->img.plane[0], i_width * i_height * 3 / 2, NULL, NULL ) )
        return -1;

    return 0;
}

static int close_file_avis( hnd_t handle )
{
    PAVISTREAM p_avi = (PAVISTREAM)handle;

    AVIStreamRelease(p_avi);
    AVIFileExit();

    return 0;
}

#endif


static int open_file_bsf( char *psz_filename, hnd_t *p_handle )
{
    if ((*p_handle = fopen(psz_filename, "w+b")) == NULL)
        return -1;

    return 0;
}

static int set_param_bsf( hnd_t handle, x264_param_t *p_param )
{
    return 0;
}

static int write_nalu_bsf( hnd_t handle, uint8_t *p_nalu, int i_size )
{
    if (fwrite(p_nalu, i_size, 1, (FILE *)handle) > 0)
        return i_size;
    return -1;
}

static int set_eop_bsf( hnd_t handle,  x264_picture_t *p_picture )
{
    return 0;
}

static int close_file_bsf( hnd_t handle )
{
    if ((handle == NULL) || (handle == stdout))
        return 0;

    return fclose((FILE *)handle);
}

/* -- mp4 muxing support ------------------------------------------------- */
#ifdef MP4_OUTPUT

typedef struct
{
    M4File *p_file;
    AVCConfig *p_config;
    M4Sample *p_sample;
    int i_track;
    int i_descidx;
    int i_time_inc;
    int i_time_res;
    int i_numframe;
    int i_init_delay;
    uint8_t b_sps;
    uint8_t b_pps;
} mp4_t;


static void recompute_bitrate_mp4(M4File *p_file, int i_track)
{
    u32 i, count, di, timescale, time_wnd, rate;
    u64 offset;
    Double br;
    ESDescriptor *esd;

    esd = M4_GetStreamDescriptor(p_file, i_track, 1);
    if (!esd) return;

    esd->decoderConfig->avgBitrate = 0;
    esd->decoderConfig->maxBitrate = 0;
    rate = time_wnd = 0;

    timescale = M4_GetMediaTimeScale(p_file, i_track);
    count = M4_GetSampleCount(p_file, i_track);
    for (i=0; i<count; i++) {
        M4Sample *samp = M4_GetSampleInfo(p_file, i_track, i+1, &di, &offset);

        if (samp->dataLength>esd->decoderConfig->bufferSizeDB) esd->decoderConfig->bufferSizeDB = samp->dataLength;

        if (esd->decoderConfig->bufferSizeDB < samp->dataLength) esd->decoderConfig->bufferSizeDB = samp->dataLength;
        esd->decoderConfig->avgBitrate += samp->dataLength;
        rate += samp->dataLength;
        if (samp->DTS > time_wnd + timescale) {
            if (rate > esd->decoderConfig->maxBitrate) esd->decoderConfig->maxBitrate = rate;
            time_wnd = samp->DTS;
            rate = 0;
        }

        M4_DeleteSample(&samp);
    }

    br = (Double) (s64) M4_GetMediaDuration(p_file, i_track);
    br /= timescale;
    esd->decoderConfig->avgBitrate = (u32) (esd->decoderConfig->avgBitrate / br);
    /*move to bps*/
    esd->decoderConfig->avgBitrate *= 8;
    esd->decoderConfig->maxBitrate *= 8;

    M4_ChangeStreamDescriptor(p_file, i_track, 1, esd);
    OD_DeleteDescriptor((Descriptor **)&esd);
}


static int close_file_mp4( hnd_t handle )
{
    mp4_t *p_mp4 = (mp4_t *)handle;

    if (p_mp4 == NULL)
        return 0;

    if (p_mp4->p_config)
        AVC_DeleteConfig(p_mp4->p_config);

    if (p_mp4->p_sample)
    {
        if (p_mp4->p_sample->data)
            free(p_mp4->p_sample->data);

        M4_DeleteSample(&p_mp4->p_sample);
    }

    if (p_mp4->p_file)
    {
        recompute_bitrate_mp4(p_mp4->p_file, p_mp4->i_track);
        M4_SetMoviePLIndication(p_mp4->p_file, M4_PL_VISUAL, 0x15);
        M4_SetStorageMode(p_mp4->p_file, M4_FLAT);
        M4_MovieClose(p_mp4->p_file);
    }

    free(p_mp4);

    return 0;
}

static int open_file_mp4( char *psz_filename, hnd_t *p_handle )
{
    mp4_t *p_mp4;

    *p_handle = NULL;

    if ((p_mp4 = (mp4_t *)malloc(sizeof(mp4_t))) == NULL)
        return -1;

    memset(p_mp4, 0, sizeof(mp4_t));
    p_mp4->p_file = M4_MovieOpen(psz_filename, M4_OPEN_WRITE);

    if ((p_mp4->p_sample = M4_NewSample()) == NULL)
    {
        close_file_mp4( p_mp4 );
        return -1;
    }

    M4_SetMovieVersionInfo(p_mp4->p_file, H264_AVC_File, 0);

    *p_handle = p_mp4;

    return 0;
}


static int set_param_mp4( hnd_t handle, x264_param_t *p_param )
{
    mp4_t *p_mp4 = (mp4_t *)handle;

    p_mp4->i_track = M4_NewTrack(p_mp4->p_file, 0, M4_VisualMediaType, 
        p_param->i_fps_num);

    p_mp4->p_config = AVC_NewConfig();
    M4_AVC_NewStreamConfig(p_mp4->p_file, p_mp4->i_track, p_mp4->p_config, 
        NULL, NULL, &p_mp4->i_descidx);

    M4_SetVisualEntrySize(p_mp4->p_file, p_mp4->i_track, p_mp4->i_descidx, 
        p_param->i_width, p_param->i_height);

    p_mp4->p_sample->data = (char *)malloc(p_param->i_width * p_param->i_height * 3 / 2);
    if (p_mp4->p_sample->data == NULL)
        return -1;

    p_mp4->i_time_res = p_param->i_fps_num;
    p_mp4->i_time_inc = p_param->i_fps_den;
    p_mp4->i_init_delay = p_param->i_bframe ? (p_param->b_bframe_pyramid ? 2 : 1) : 0;
    p_mp4->i_init_delay *= p_mp4->i_time_inc;
    fprintf(stderr, "mp4 [info]: initial delay %d (scale %d)\n", 
        p_mp4->i_init_delay, p_mp4->i_time_res);

    return 0;
}


static int write_nalu_mp4( hnd_t handle, uint8_t *p_nalu, int i_size )
{
    mp4_t *p_mp4 = (mp4_t *)handle;
    AVCConfigSlot *p_slot;
    uint8_t type = p_nalu[4] & 0x1f;
    int psize;

    switch(type)
    {
    // sps
    case 0x07:
        if (!p_mp4->b_sps)
        {
            p_mp4->p_config->configurationVersion = 1;
            p_mp4->p_config->AVCProfileIndication = p_nalu[5];
            p_mp4->p_config->profile_compatibility = p_nalu[6];
            p_mp4->p_config->AVCLevelIndication = p_nalu[7];
            p_slot = (AVCConfigSlot *)malloc(sizeof(AVCConfigSlot));
            p_slot->size = i_size - 4;
            p_slot->data = (char *)malloc(p_slot->size);
            memcpy(p_slot->data, p_nalu + 4, i_size - 4);
            ChainAddEntry(p_mp4->p_config->sequenceParameterSets, p_slot);
            p_slot = NULL;
            p_mp4->b_sps = 1;
        }
        break;

    // pps      
    case 0x08:
        if (!p_mp4->b_pps)
        {
            p_slot = (AVCConfigSlot *)malloc(sizeof(AVCConfigSlot));
            p_slot->size = i_size - 4;
            p_slot->data = (char *)malloc(p_slot->size);
            memcpy(p_slot->data, p_nalu + 4, i_size - 4);
            ChainAddEntry(p_mp4->p_config->pictureParameterSets, p_slot);
            p_slot = NULL;
            p_mp4->b_pps = 1;
            if (p_mp4->b_sps)
                M4_AVC_UpdateStreamConfig(p_mp4->p_file, p_mp4->i_track, 1, p_mp4->p_config);
        }
        break;

    // slice, sei
    case 0x1:
    case 0x5:
    case 0x6:
        psize = i_size - 4 ;
        memcpy(p_mp4->p_sample->data + p_mp4->p_sample->dataLength, p_nalu, i_size);
        p_mp4->p_sample->data[p_mp4->p_sample->dataLength + 0] = (psize >> 24) & 0xff;
        p_mp4->p_sample->data[p_mp4->p_sample->dataLength + 1] = (psize >> 16) & 0xff;
        p_mp4->p_sample->data[p_mp4->p_sample->dataLength + 2] = (psize >> 8) & 0xff;
        p_mp4->p_sample->data[p_mp4->p_sample->dataLength + 3] = (psize >> 0) & 0xff;
        p_mp4->p_sample->dataLength += i_size;
        break;
    }

    return i_size;
}

static int set_eop_mp4( hnd_t handle, x264_picture_t *p_picture )
{
    mp4_t *p_mp4 = (mp4_t *)handle;
    uint32_t dts = p_mp4->i_numframe * p_mp4->i_time_inc;
    uint32_t pts = p_picture->i_pts;
    int offset = p_mp4->i_init_delay + pts - dts;

    p_mp4->p_sample->IsRAP = p_picture->i_type == X264_TYPE_IDR ? 1 : 0;
    p_mp4->p_sample->DTS = dts;
    p_mp4->p_sample->CTS_Offset = offset;
    M4_AddSample(p_mp4->p_file, p_mp4->i_track, p_mp4->i_descidx, p_mp4->p_sample);

    p_mp4->p_sample->dataLength = 0;
    p_mp4->i_numframe++;

    return 0;
}

#endif
