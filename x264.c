/*****************************************************************************
 * x264: h264 encoder testing program.
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *          Laurent Aimar <fenrir@via.ecp.fr>
 *          Steven Walters <kemuri9@gmail.com>
 *          Kieran Kunhya <kieran@kunhya.com>
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

#include <stdlib.h>
#include <math.h>

#include <signal.h>
#define _GNU_SOURCE
#include <getopt.h>

#include "common/common.h"
#include "common/cpu.h"
#include "x264.h"
#include "muxers.h"

#ifdef _WIN32
#include <windows.h>
#else
#define SetConsoleTitle(t)
#endif

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
    int b_progress;
    int i_seek;
    hnd_t hin;
    hnd_t hout;
    FILE *qpfile;
} cli_opt_t;

/* i/o file operation function pointer structs */
cli_input_t input;
static cli_output_t output;

static const char * const demuxer_names[] =
{
    "auto",
    "yuv",
    "y4m",
#ifdef AVS_INPUT
    "avs",
#endif
#ifdef LAVF_INPUT
    "lavf",
#endif
#ifdef FFMS_INPUT
    "ffms",
#endif
    0
};

static const char * const muxer_names[] =
{
    "auto",
    "raw",
    "mkv",
    "flv",
#ifdef MP4_OUTPUT
    "mp4",
#endif
    0
};

static void Help( x264_param_t *defaults, int longhelp );
static int  Parse( int argc, char **argv, x264_param_t *param, cli_opt_t *opt );
static int  Encode( x264_param_t *param, cli_opt_t *opt );

/****************************************************************************
 * main:
 ****************************************************************************/
int main( int argc, char **argv )
{
    x264_param_t param;
    cli_opt_t opt;
    int ret;

#ifdef PTW32_STATIC_LIB
    pthread_win32_process_attach_np();
    pthread_win32_thread_attach_np();
#endif

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    x264_param_default( &param );

    /* Parse command line */
    if( Parse( argc, argv, &param, &opt ) < 0 )
        return -1;

    /* Control-C handler */
    signal( SIGINT, SigIntHandler );

    ret = Encode( &param, &opt );

#ifdef PTW32_STATIC_LIB
    pthread_win32_thread_detach_np();
    pthread_win32_process_detach_np();
#endif

    return ret;
}

static char const *strtable_lookup( const char * const table[], int index )
{
    int i = 0; while( table[i] ) i++;
    return ( ( index >= 0 && index < i ) ? table[ index ] : "???" );
}

static char *stringify_names( char *buf, const char * const names[] )
{
    int i = 0;
    char *p = buf;
    for( p[0] = 0; names[i]; i++ )
    {
        p += sprintf( p, "%s", names[i] );
        if( names[i+1] )
            p += sprintf( p, ", " );
    }
    return buf;
}

/*****************************************************************************
 * Help:
 *****************************************************************************/
static void Help( x264_param_t *defaults, int longhelp )
{
    char buf[50];
#define H0 printf
#define H1 if(longhelp>=1) printf
#define H2 if(longhelp==2) printf
    H0( "x264 core:%d%s\n"
        "Syntax: x264 [options] -o outfile infile [widthxheight]\n"
        "\n"
        "Infile can be raw YUV 4:2:0 (in which case resolution is required),\n"
        "  or YUV4MPEG 4:2:0 (*.y4m),\n"
        "  or Avisynth if compiled with support (%s).\n"
        "  or libav* formats if compiled with lavf support (%s) or ffms support (%s).\n"
        "Outfile type is selected by filename:\n"
        " .264 -> Raw bytestream\n"
        " .mkv -> Matroska\n"
        " .flv -> Flash Video\n"
        " .mp4 -> MP4 if compiled with GPAC support (%s)\n"
        "\n"
        "Options:\n"
        "\n"
        "  -h, --help                  List basic options\n"
        "      --longhelp              List more options\n"
        "      --fullhelp              List all options\n"
        "\n",
        X264_BUILD, X264_VERSION,
#ifdef AVS_INPUT
        "yes",
#else
        "no",
#endif
#ifdef LAVF_INPUT
        "yes",
#else
        "no",
#endif
#ifdef FFMS_INPUT
        "yes",
#else
        "no",
#endif
#ifdef MP4_OUTPUT
        "yes"
#else
        "no"
#endif
      );
    H0( "Example usage:\n" );
    H0( "\n" );
    H0( "      Constant quality mode:\n" );
    H0( "            x264 --crf 24 -o <output> <input>\n" );
    H0( "\n" );
    H0( "      Two-pass with a bitrate of 1000kbps:\n" );
    H0( "            x264 --pass 1 --bitrate 1000 -o <output> <input>\n" );
    H0( "            x264 --pass 2 --bitrate 1000 -o <output> <input>\n" );
    H0( "\n" );
    H0( "      Lossless:\n" );
    H0( "            x264 --crf 0 -o <output> <input>\n" );
    H0( "\n" );
    H0( "      Maximum PSNR at the cost of speed and visual quality:\n" );
    H0( "            x264 --preset placebo --tune psnr -o <output> <input>\n" );
    H0( "\n" );
    H0( "      Constant bitrate at 1000kbps with a 2 second-buffer:\n");
    H0( "            x264 --vbv-bufsize 2000 --bitrate 1000 -o <output> <input>\n" );
    H0( "\n" );
    H0( "Presets:\n" );
    H0( "\n" );
    H0( "      --profile               Force the limits of an H.264 profile [high]\n"
        "                                  Overrides all settings.\n" );
    H2( "                                  - baseline:\n"
        "                                    --no-8x8dct --bframes 0 --no-cabac\n"
        "                                    --cqm flat --weightp 0 No interlaced\n"
        "                                    No lossless\n"
        "                                  - main:\n"
        "                                    --no-8x8dct --cqm flat No lossless\n"
        "                                  - high:\n"
        "                                    No lossless\n" );
        else H0( "                                  - baseline,main,high\n" );
    H0( "      --preset                Use a preset to select encoding settings [medium]\n"
        "                                  Overridden by user settings.\n" );
    H2( "                                  - ultrafast:\n"
        "                                    --no-8x8dct --aq-mode 0 --b-adapt 0\n"
        "                                    --bframes 0 --no-cabac --no-deblock\n"
        "                                    --no-mbtree --me dia --no-mixed-refs\n"
        "                                    --partitions none --ref 1 --scenecut 0\n"
        "                                    --subme 0 --trellis 0 --no-weightb\n"
        "                                    --weightp 0\n"
        "                                  - veryfast:\n"
        "                                    --no-mbtree --me dia --no-mixed-refs\n"
        "                                    --partitions i8x8,i4x4 --ref 1\n"
        "                                    --subme 1 --trellis 0 --weightp 0\n"
        "                                  - faster:\n"
        "                                    --no-mbtree --no-mixed-refs --ref 2\n"
        "                                    --subme 4 --weightp 1\n"
        "                                  - fast:\n"
        "                                    --rc-lookahead 30 --ref 2 --subme 6\n"
        "                                  - medium:\n"
        "                                    Default settings apply.\n"
        "                                  - slow:\n"
        "                                    --b-adapt 2 --direct auto --me umh\n"
        "                                    --rc-lookahead 50 --ref 5 --subme 8\n"
        "                                  - slower:\n"
        "                                    --b-adapt 2 --direct auto --me umh\n"
        "                                    --partitions all --rc-lookahead 60\n"
        "                                    --ref 8 --subme 9 --trellis 2\n"
        "                                  - veryslow:\n"
        "                                    --b-adapt 2 --bframes 8 --direct auto\n"
        "                                    --me umh --merange 24 --partitions all\n"
        "                                    --ref 16 --subme 10 --trellis 2\n"
        "                                    --rc-lookahead 60\n"
        "                                  - placebo:\n"
        "                                    --bframes 16 --b-adapt 2 --direct auto\n"
        "                                    --slow-firstpass --no-fast-pskip\n"
        "                                    --me tesa --merange 24 --partitions all\n"
        "                                    --rc-lookahead 60 --ref 16 --subme 10\n"
        "                                    --trellis 2\n" );
    else H0( "                                  - ultrafast,veryfast,faster,fast,medium\n"
             "                                  - slow,slower,veryslow,placebo\n" );
    H0( "      --tune                  Tune the settings for a particular type of source\n"
        "                              or situation\n"
        "                                  Overridden by user settings.\n"
        "                                  Multiple tunings are separated by commas.\n"
        "                                  Only one psy tuning can be used at a time.\n" );
    H2( "                                  - film (psy tuning):\n"
        "                                    --deblock -1:-1 --psy-rd <unset>:0.15\n"
        "                                  - animation (psy tuning):\n"
        "                                    --bframes {+2} --deblock 1:1\n"
        "                                    --psy-rd 0.4:<unset> --aq-strength 0.6\n"
        "                                    --ref {Double if >1 else 1}\n"
        "                                  - grain (psy tuning):\n"
        "                                    --aq-strength 0.5 --no-dct-decimate\n"
        "                                    --deadzone-inter 6 --deadzone-intra 6\n"
        "                                    --deblock -2:-2 --ipratio 1.1 \n"
        "                                    --pbratio 1.1 --psy-rd <unset>:0.25\n"
        "                                    --qcomp 0.8\n"
        "                                  - psnr (psy tuning):\n"
        "                                    --aq-mode 0 --no-psy\n"
        "                                  - ssim (psy tuning):\n"
        "                                    --aq-mode 2 --no-psy\n"
        "                                  - fastdecode:\n"
        "                                    --no-cabac --no-deblock --no-weightb\n"
        "                                    --weightp 0\n"
        "                                  - zerolatency:\n"
        "                                    --bframes 0 --rc-lookahead 0\n"
        "                                    --sync-lookahead 0 --sliced-threads\n" );
    else H0( "                                  - psy tunings: film,animation,grain,psnr,ssim\n"
             "                                  - other tunings: fastdecode,zerolatency\n" );
    H1( "      --slow-firstpass        Don't use faster settings with --pass 1\n" );
    H0( "\n" );
    H0( "Frame-type options:\n" );
    H0( "\n" );
    H0( "  -I, --keyint <integer>      Maximum GOP size [%d]\n", defaults->i_keyint_max );
    H2( "  -i, --min-keyint <integer>  Minimum GOP size [%d]\n", defaults->i_keyint_min );
    H2( "      --no-scenecut           Disable adaptive I-frame decision\n" );
    H2( "      --scenecut <integer>    How aggressively to insert extra I-frames [%d]\n", defaults->i_scenecut_threshold );
    H2( "      --intra-refresh         Use Periodic Intra Refresh instead of IDR frames\n" );
    H1( "  -b, --bframes <integer>     Number of B-frames between I and P [%d]\n", defaults->i_bframe );
    H1( "      --b-adapt <integer>     Adaptive B-frame decision method [%d]\n"
        "                                  Higher values may lower threading efficiency.\n"
        "                                  - 0: Disabled\n"
        "                                  - 1: Fast\n"
        "                                  - 2: Optimal (slow with high --bframes)\n", defaults->i_bframe_adaptive );
    H2( "      --b-bias <integer>      Influences how often B-frames are used [%d]\n", defaults->i_bframe_bias );
    H1( "      --b-pyramid <string>    Keep some B-frames as references [%s]\n"
        "                                  - none: Disabled\n"
        "                                  - strict: Strictly hierarchical pyramid\n"
        "                                  - normal: Non-strict (not Blu-ray compatible)\n",
        strtable_lookup( x264_b_pyramid_names, defaults->i_bframe_pyramid ) );
    H1( "      --no-cabac              Disable CABAC\n" );
    H1( "  -r, --ref <integer>         Number of reference frames [%d]\n", defaults->i_frame_reference );
    H1( "      --no-deblock            Disable loop filter\n" );
    H1( "  -f, --deblock <alpha:beta>  Loop filter parameters [%d:%d]\n",
                                       defaults->i_deblocking_filter_alphac0, defaults->i_deblocking_filter_beta );
    H2( "      --slices <integer>      Number of slices per frame; forces rectangular\n"
        "                              slices and is overridden by other slicing options\n" );
    else H1( "      --slices <integer>      Number of slices per frame\n" );
    H2( "      --slice-max-size <integer> Limit the size of each slice in bytes\n");
    H2( "      --slice-max-mbs <integer> Limit the size of each slice in macroblocks\n");
    H0( "      --interlaced            Enable pure-interlaced mode\n" );
    H2( "      --constrained-intra     Enable constrained intra prediction.\n" );
    H0( "\n" );
    H0( "Ratecontrol:\n" );
    H0( "\n" );
    H1( "  -q, --qp <integer>          Force constant QP (0-51, 0=lossless)\n" );
    H0( "  -B, --bitrate <integer>     Set bitrate (kbit/s)\n" );
    H0( "      --crf <float>           Quality-based VBR (0-51, 0=lossless) [%.1f]\n", defaults->rc.f_rf_constant );
    H1( "      --rc-lookahead <integer> Number of frames for frametype lookahead [%d]\n", defaults->rc.i_lookahead );
    H0( "      --vbv-maxrate <integer> Max local bitrate (kbit/s) [%d]\n", defaults->rc.i_vbv_max_bitrate );
    H0( "      --vbv-bufsize <integer> Set size of the VBV buffer (kbit) [%d]\n", defaults->rc.i_vbv_buffer_size );
    H2( "      --vbv-init <float>      Initial VBV buffer occupancy [%.1f]\n", defaults->rc.f_vbv_buffer_init );
    H2( "      --qpmin <integer>       Set min QP [%d]\n", defaults->rc.i_qp_min );
    H2( "      --qpmax <integer>       Set max QP [%d]\n", defaults->rc.i_qp_max );
    H2( "      --qpstep <integer>      Set max QP step [%d]\n", defaults->rc.i_qp_step );
    H2( "      --ratetol <float>       Tolerance of ABR ratecontrol and VBV [%.1f]\n", defaults->rc.f_rate_tolerance );
    H2( "      --ipratio <float>       QP factor between I and P [%.2f]\n", defaults->rc.f_ip_factor );
    H2( "      --pbratio <float>       QP factor between P and B [%.2f]\n", defaults->rc.f_pb_factor );
    H2( "      --chroma-qp-offset <integer>  QP difference between chroma and luma [%d]\n", defaults->analyse.i_chroma_qp_offset );
    H2( "      --aq-mode <integer>     AQ method [%d]\n"
        "                                  - 0: Disabled\n"
        "                                  - 1: Variance AQ (complexity mask)\n"
        "                                  - 2: Auto-variance AQ (experimental)\n", defaults->rc.i_aq_mode );
    H1( "      --aq-strength <float>   Reduces blocking and blurring in flat and\n"
        "                              textured areas. [%.1f]\n", defaults->rc.f_aq_strength );
    H1( "\n" );
    H0( "  -p, --pass <integer>        Enable multipass ratecontrol\n"
        "                                  - 1: First pass, creates stats file\n"
        "                                  - 2: Last pass, does not overwrite stats file\n" );
    H2( "                                  - 3: Nth pass, overwrites stats file\n" );
    H1( "      --stats <string>        Filename for 2 pass stats [\"%s\"]\n", defaults->rc.psz_stat_out );
    H2( "      --no-mbtree             Disable mb-tree ratecontrol.\n");
    H2( "      --qcomp <float>         QP curve compression [%.2f]\n", defaults->rc.f_qcompress );
    H2( "      --cplxblur <float>      Reduce fluctuations in QP (before curve compression) [%.1f]\n", defaults->rc.f_complexity_blur );
    H2( "      --qblur <float>         Reduce fluctuations in QP (after curve compression) [%.1f]\n", defaults->rc.f_qblur );
    H2( "      --zones <zone0>/<zone1>/...  Tweak the bitrate of regions of the video\n" );
    H2( "                              Each zone is of the form\n"
        "                                  <start frame>,<end frame>,<option>\n"
        "                                  where <option> is either\n"
        "                                      q=<integer> (force QP)\n"
        "                                  or  b=<float> (bitrate multiplier)\n" );
    H2( "      --qpfile <string>       Force frametypes and QPs for some or all frames\n"
        "                              Format of each line: framenumber frametype QP\n"
        "                              QP of -1 lets x264 choose. Frametypes: I,i,P,B,b.\n"
        "                              QPs are restricted by qpmin/qpmax.\n" );
    H1( "\n" );
    H1( "Analysis:\n" );
    H1( "\n" );
    H1( "  -A, --partitions <string>   Partitions to consider [\"p8x8,b8x8,i8x8,i4x4\"]\n"
        "                                  - p8x8, p4x4, b8x8, i8x8, i4x4\n"
        "                                  - none, all\n"
        "                                  (p4x4 requires p8x8. i8x8 requires --8x8dct.)\n" );
    H1( "      --direct <string>       Direct MV prediction mode [\"%s\"]\n"
        "                                  - none, spatial, temporal, auto\n",
                                       strtable_lookup( x264_direct_pred_names, defaults->analyse.i_direct_mv_pred ) );
    H2( "      --no-weightb            Disable weighted prediction for B-frames\n" );
    H1( "      --weightp <integer>     Weighted prediction for P-frames [%d]\n"
        "                              - 0: Disabled\n"
        "                              - 1: Blind offset\n"
        "                              - 2: Smart analysis\n", defaults->analyse.i_weighted_pred );
    H1( "      --me <string>           Integer pixel motion estimation method [\"%s\"]\n",
                                       strtable_lookup( x264_motion_est_names, defaults->analyse.i_me_method ) );
    H2( "                                  - dia: diamond search, radius 1 (fast)\n"
        "                                  - hex: hexagonal search, radius 2\n"
        "                                  - umh: uneven multi-hexagon search\n"
        "                                  - esa: exhaustive search\n"
        "                                  - tesa: hadamard exhaustive search (slow)\n" );
    else H1( "                                  - dia, hex, umh\n" );
    H2( "      --merange <integer>     Maximum motion vector search range [%d]\n", defaults->analyse.i_me_range );
    H2( "      --mvrange <integer>     Maximum motion vector length [-1 (auto)]\n" );
    H2( "      --mvrange-thread <int>  Minimum buffer between threads [-1 (auto)]\n" );
    H1( "  -m, --subme <integer>       Subpixel motion estimation and mode decision [%d]\n", defaults->analyse.i_subpel_refine );
    H2( "                                  - 0: fullpel only (not recommended)\n"
        "                                  - 1: SAD mode decision, one qpel iteration\n"
        "                                  - 2: SATD mode decision\n"
        "                                  - 3-5: Progressively more qpel\n"
        "                                  - 6: RD mode decision for I/P-frames\n"
        "                                  - 7: RD mode decision for all frames\n"
        "                                  - 8: RD refinement for I/P-frames\n"
        "                                  - 9: RD refinement for all frames\n"
        "                                  - 10: QP-RD - requires trellis=2, aq-mode>0\n" );
    else H1( "                                  decision quality: 1=fast, 10=best.\n"  );
    H1( "      --psy-rd                Strength of psychovisual optimization [\"%.1f:%.1f\"]\n"
        "                                  #1: RD (requires subme>=6)\n"
        "                                  #2: Trellis (requires trellis, experimental)\n",
                                       defaults->analyse.f_psy_rd, defaults->analyse.f_psy_trellis );
    H2( "      --no-psy                Disable all visual optimizations that worsen\n"
        "                              both PSNR and SSIM.\n" );
    H2( "      --no-mixed-refs         Don't decide references on a per partition basis\n" );
    H2( "      --no-chroma-me          Ignore chroma in motion estimation\n" );
    H1( "      --no-8x8dct             Disable adaptive spatial transform size\n" );
    H1( "  -t, --trellis <integer>     Trellis RD quantization. Requires CABAC. [%d]\n"
        "                                  - 0: disabled\n"
        "                                  - 1: enabled only on the final encode of a MB\n"
        "                                  - 2: enabled on all mode decisions\n", defaults->analyse.i_trellis );
    H2( "      --no-fast-pskip         Disables early SKIP detection on P-frames\n" );
    H2( "      --no-dct-decimate       Disables coefficient thresholding on P-frames\n" );
    H1( "      --nr <integer>          Noise reduction [%d]\n", defaults->analyse.i_noise_reduction );
    H2( "\n" );
    H2( "      --deadzone-inter <int>  Set the size of the inter luma quantization deadzone [%d]\n", defaults->analyse.i_luma_deadzone[0] );
    H2( "      --deadzone-intra <int>  Set the size of the intra luma quantization deadzone [%d]\n", defaults->analyse.i_luma_deadzone[1] );
    H2( "                                  Deadzones should be in the range 0 - 32.\n" );
    H2( "      --cqm <string>          Preset quant matrices [\"flat\"]\n"
        "                                  - jvt, flat\n" );
    H1( "      --cqmfile <string>      Read custom quant matrices from a JM-compatible file\n" );
    H2( "                                  Overrides any other --cqm* options.\n" );
    H2( "      --cqm4 <list>           Set all 4x4 quant matrices\n"
        "                                  Takes a comma-separated list of 16 integers.\n" );
    H2( "      --cqm8 <list>           Set all 8x8 quant matrices\n"
        "                                  Takes a comma-separated list of 64 integers.\n" );
    H2( "      --cqm4i, --cqm4p, --cqm8i, --cqm8p\n"
        "                              Set both luma and chroma quant matrices\n" );
    H2( "      --cqm4iy, --cqm4ic, --cqm4py, --cqm4pc\n"
        "                              Set individual quant matrices\n" );
    H2( "\n" );
    H2( "Video Usability Info (Annex E):\n" );
    H2( "The VUI settings are not used by the encoder but are merely suggestions to\n" );
    H2( "the playback equipment. See doc/vui.txt for details. Use at your own risk.\n" );
    H2( "\n" );
    H2( "      --overscan <string>     Specify crop overscan setting [\"%s\"]\n"
        "                                  - undef, show, crop\n",
                                       strtable_lookup( x264_overscan_names, defaults->vui.i_overscan ) );
    H2( "      --videoformat <string>  Specify video format [\"%s\"]\n"
        "                                  - component, pal, ntsc, secam, mac, undef\n",
                                       strtable_lookup( x264_vidformat_names, defaults->vui.i_vidformat ) );
    H2( "      --fullrange <string>    Specify full range samples setting [\"%s\"]\n"
        "                                  - off, on\n",
                                       strtable_lookup( x264_fullrange_names, defaults->vui.b_fullrange ) );
    H2( "      --colorprim <string>    Specify color primaries [\"%s\"]\n"
        "                                  - undef, bt709, bt470m, bt470bg\n"
        "                                    smpte170m, smpte240m, film\n",
                                       strtable_lookup( x264_colorprim_names, defaults->vui.i_colorprim ) );
    H2( "      --transfer <string>     Specify transfer characteristics [\"%s\"]\n"
        "                                  - undef, bt709, bt470m, bt470bg, linear,\n"
        "                                    log100, log316, smpte170m, smpte240m\n",
                                       strtable_lookup( x264_transfer_names, defaults->vui.i_transfer ) );
    H2( "      --colormatrix <string>  Specify color matrix setting [\"%s\"]\n"
        "                                  - undef, bt709, fcc, bt470bg\n"
        "                                    smpte170m, smpte240m, GBR, YCgCo\n",
                                       strtable_lookup( x264_colmatrix_names, defaults->vui.i_colmatrix ) );
    H2( "      --chromaloc <integer>   Specify chroma sample location (0 to 5) [%d]\n",
                                       defaults->vui.i_chroma_loc );
    H0( "\n" );
    H0( "Input/Output:\n" );
    H0( "\n" );
    H0( "  -o, --output                Specify output file\n" );
    H1( "      --muxer <string>        Specify output container format [\"%s\"]\n"
        "                                  - %s\n", muxer_names[0], stringify_names( buf, muxer_names ) );
    H1( "      --demuxer <string>      Specify input container format [\"%s\"]\n"
        "                                  - %s\n", demuxer_names[0], stringify_names( buf, demuxer_names ) );
    H1( "      --index <string>        Filename for input index file\n" );
    H0( "      --sar width:height      Specify Sample Aspect Ratio\n" );
    H0( "      --fps <float|rational>  Specify framerate\n" );
    H0( "      --seek <integer>        First frame to encode\n" );
    H0( "      --frames <integer>      Maximum number of frames to encode\n" );
    H0( "      --level <string>        Specify level (as defined by Annex A)\n" );
    H1( "\n" );
    H1( "  -v, --verbose               Print stats for each frame\n" );
    H1( "      --no-progress           Don't show the progress indicator while encoding\n" );
    H0( "      --quiet                 Quiet Mode\n" );
    H1( "      --psnr                  Enable PSNR computation\n" );
    H1( "      --ssim                  Enable SSIM computation\n" );
    H1( "      --threads <integer>     Force a specific number of threads\n" );
    H2( "      --sliced-threads        Low-latency but lower-efficiency threading\n" );
    H2( "      --thread-input          Run Avisynth in its own thread\n" );
    H2( "      --sync-lookahead <integer> Number of buffer frames for threaded lookahead\n" );
    H2( "      --non-deterministic     Slightly improve quality of SMP, at the cost of repeatability\n" );
    H2( "      --asm <integer>         Override CPU detection\n" );
    H2( "      --no-asm                Disable all CPU optimizations\n" );
    H2( "      --visualize             Show MB types overlayed on the encoded video\n" );
    H2( "      --dump-yuv <string>     Save reconstructed frames\n" );
    H2( "      --sps-id <integer>      Set SPS and PPS id numbers [%d]\n", defaults->i_sps_id );
    H2( "      --aud                   Use access unit delimiters\n" );
    H2( "      --force-cfr             Force constant framerate timestamp generation\n" );
    H0( "\n" );
}

#define OPT_FRAMES 256
#define OPT_SEEK 257
#define OPT_QPFILE 258
#define OPT_THREAD_INPUT 259
#define OPT_QUIET 260
#define OPT_NOPROGRESS 261
#define OPT_VISUALIZE 262
#define OPT_LONGHELP 263
#define OPT_PROFILE 264
#define OPT_PRESET 265
#define OPT_TUNE 266
#define OPT_SLOWFIRSTPASS 267
#define OPT_FULLHELP 268
#define OPT_FPS 269
#define OPT_MUXER 270
#define OPT_DEMUXER 271
#define OPT_INDEX 272
#define OPT_INTERLACED 273

static char short_options[] = "8A:B:b:f:hI:i:m:o:p:q:r:t:Vvw";
static struct option long_options[] =
{
    { "help",              no_argument, NULL, 'h' },
    { "longhelp",          no_argument, NULL, OPT_LONGHELP },
    { "fullhelp",          no_argument, NULL, OPT_FULLHELP },
    { "version",           no_argument, NULL, 'V' },
    { "profile",     required_argument, NULL, OPT_PROFILE },
    { "preset",      required_argument, NULL, OPT_PRESET },
    { "tune",        required_argument, NULL, OPT_TUNE },
    { "slow-firstpass",    no_argument, NULL, OPT_SLOWFIRSTPASS },
    { "bitrate",     required_argument, NULL, 'B' },
    { "bframes",     required_argument, NULL, 'b' },
    { "b-adapt",     required_argument, NULL, 0 },
    { "no-b-adapt",        no_argument, NULL, 0 },
    { "b-bias",      required_argument, NULL, 0 },
    { "b-pyramid",   required_argument, NULL, 0 },
    { "min-keyint",  required_argument, NULL, 'i' },
    { "keyint",      required_argument, NULL, 'I' },
    { "intra-refresh",     no_argument, NULL, 0 },
    { "scenecut",    required_argument, NULL, 0 },
    { "no-scenecut",       no_argument, NULL, 0 },
    { "nf",                no_argument, NULL, 0 },
    { "no-deblock",        no_argument, NULL, 0 },
    { "filter",      required_argument, NULL, 0 },
    { "deblock",     required_argument, NULL, 'f' },
    { "interlaced",        no_argument, NULL, OPT_INTERLACED },
    { "no-interlaced",     no_argument, NULL, OPT_INTERLACED },
    { "constrained-intra", no_argument, NULL, 0 },
    { "cabac",             no_argument, NULL, 0 },
    { "no-cabac",          no_argument, NULL, 0 },
    { "qp",          required_argument, NULL, 'q' },
    { "qpmin",       required_argument, NULL, 0 },
    { "qpmax",       required_argument, NULL, 0 },
    { "qpstep",      required_argument, NULL, 0 },
    { "crf",         required_argument, NULL, 0 },
    { "rc-lookahead",required_argument, NULL, 0 },
    { "ref",         required_argument, NULL, 'r' },
    { "asm",         required_argument, NULL, 0 },
    { "no-asm",            no_argument, NULL, 0 },
    { "sar",         required_argument, NULL, 0 },
    { "fps",         required_argument, NULL, OPT_FPS },
    { "frames",      required_argument, NULL, OPT_FRAMES },
    { "seek",        required_argument, NULL, OPT_SEEK },
    { "output",      required_argument, NULL, 'o' },
    { "muxer",       required_argument, NULL, OPT_MUXER },
    { "demuxer",     required_argument, NULL, OPT_DEMUXER },
    { "stdout",      required_argument, NULL, OPT_MUXER },
    { "stdin",       required_argument, NULL, OPT_DEMUXER },
    { "index",       required_argument, NULL, OPT_INDEX },
    { "analyse",     required_argument, NULL, 0 },
    { "partitions",  required_argument, NULL, 'A' },
    { "direct",      required_argument, NULL, 0 },
    { "weightb",           no_argument, NULL, 'w' },
    { "no-weightb",        no_argument, NULL, 0 },
    { "weightp",     required_argument, NULL, 0 },
    { "me",          required_argument, NULL, 0 },
    { "merange",     required_argument, NULL, 0 },
    { "mvrange",     required_argument, NULL, 0 },
    { "mvrange-thread", required_argument, NULL, 0 },
    { "subme",       required_argument, NULL, 'm' },
    { "psy-rd",      required_argument, NULL, 0 },
    { "no-psy",            no_argument, NULL, 0 },
    { "psy",               no_argument, NULL, 0 },
    { "mixed-refs",        no_argument, NULL, 0 },
    { "no-mixed-refs",     no_argument, NULL, 0 },
    { "no-chroma-me",      no_argument, NULL, 0 },
    { "8x8dct",            no_argument, NULL, 0 },
    { "no-8x8dct",         no_argument, NULL, 0 },
    { "trellis",     required_argument, NULL, 't' },
    { "fast-pskip",        no_argument, NULL, 0 },
    { "no-fast-pskip",     no_argument, NULL, 0 },
    { "no-dct-decimate",   no_argument, NULL, 0 },
    { "aq-strength", required_argument, NULL, 0 },
    { "aq-mode",     required_argument, NULL, 0 },
    { "deadzone-inter", required_argument, NULL, '0' },
    { "deadzone-intra", required_argument, NULL, '0' },
    { "level",       required_argument, NULL, 0 },
    { "ratetol",     required_argument, NULL, 0 },
    { "vbv-maxrate", required_argument, NULL, 0 },
    { "vbv-bufsize", required_argument, NULL, 0 },
    { "vbv-init",    required_argument, NULL,  0 },
    { "ipratio",     required_argument, NULL, 0 },
    { "pbratio",     required_argument, NULL, 0 },
    { "chroma-qp-offset", required_argument, NULL, 0 },
    { "pass",        required_argument, NULL, 'p' },
    { "stats",       required_argument, NULL, 0 },
    { "qcomp",       required_argument, NULL, 0 },
    { "mbtree",            no_argument, NULL, 0 },
    { "no-mbtree",         no_argument, NULL, 0 },
    { "qblur",       required_argument, NULL, 0 },
    { "cplxblur",    required_argument, NULL, 0 },
    { "zones",       required_argument, NULL, 0 },
    { "qpfile",      required_argument, NULL, OPT_QPFILE },
    { "threads",     required_argument, NULL, 0 },
    { "sliced-threads",    no_argument, NULL, 0 },
    { "no-sliced-threads", no_argument, NULL, 0 },
    { "slice-max-size",    required_argument, NULL, 0 },
    { "slice-max-mbs",     required_argument, NULL, 0 },
    { "slices",            required_argument, NULL, 0 },
    { "thread-input",      no_argument, NULL, OPT_THREAD_INPUT },
    { "sync-lookahead",    required_argument, NULL, 0 },
    { "non-deterministic", no_argument, NULL, 0 },
    { "psnr",              no_argument, NULL, 0 },
    { "ssim",              no_argument, NULL, 0 },
    { "quiet",             no_argument, NULL, OPT_QUIET },
    { "verbose",           no_argument, NULL, 'v' },
    { "no-progress",       no_argument, NULL, OPT_NOPROGRESS },
    { "visualize",         no_argument, NULL, OPT_VISUALIZE },
    { "dump-yuv",    required_argument, NULL, 0 },
    { "sps-id",      required_argument, NULL, 0 },
    { "aud",               no_argument, NULL, 0 },
    { "nr",          required_argument, NULL, 0 },
    { "cqm",         required_argument, NULL, 0 },
    { "cqmfile",     required_argument, NULL, 0 },
    { "cqm4",        required_argument, NULL, 0 },
    { "cqm4i",       required_argument, NULL, 0 },
    { "cqm4iy",      required_argument, NULL, 0 },
    { "cqm4ic",      required_argument, NULL, 0 },
    { "cqm4p",       required_argument, NULL, 0 },
    { "cqm4py",      required_argument, NULL, 0 },
    { "cqm4pc",      required_argument, NULL, 0 },
    { "cqm8",        required_argument, NULL, 0 },
    { "cqm8i",       required_argument, NULL, 0 },
    { "cqm8p",       required_argument, NULL, 0 },
    { "overscan",    required_argument, NULL, 0 },
    { "videoformat", required_argument, NULL, 0 },
    { "fullrange",   required_argument, NULL, 0 },
    { "colorprim",   required_argument, NULL, 0 },
    { "transfer",    required_argument, NULL, 0 },
    { "colormatrix", required_argument, NULL, 0 },
    { "chromaloc",   required_argument, NULL, 0 },
    { "force-cfr",         no_argument, NULL, 0 },
    {0, 0, 0, 0}
};

static int select_output( const char *muxer, char *filename, x264_param_t *param )
{
    const char *ext = get_filename_extension( filename );
    if( !strcmp( filename, "-" ) || strcasecmp( muxer, "auto" ) )
        ext = muxer;

    if( !strcasecmp( ext, "mp4" ) )
    {
#ifdef MP4_OUTPUT
        output = mp4_output;
        param->b_annexb = 0;
        param->b_aud = 0;
        param->b_dts_compress = 0;
        param->b_repeat_headers = 0;
#else
        fprintf( stderr, "x264 [error]: not compiled with MP4 output support\n" );
        return -1;
#endif
    }
    else if( !strcasecmp( ext, "mkv" ) )
    {
        output = mkv_output;
        param->b_annexb = 0;
        param->b_aud = 0;
        param->b_dts_compress = 0;
        param->b_repeat_headers = 0;
    }
    else if( !strcasecmp( ext, "flv" ) )
    {
        output = flv_output;
        param->b_annexb = 0;
        param->b_aud = 0;
        param->b_dts_compress = 1;
        param->b_repeat_headers = 0;
    }
    else
        output = raw_output;
    return 0;
}

static int select_input( const char *demuxer, char *used_demuxer, char *filename,
                         hnd_t *p_handle, video_info_t *info, cli_input_opt_t *opt )
{
    const char *ext = get_filename_extension( filename );
    int b_regular = strcmp( filename, "-" );
    int b_auto = !strcasecmp( demuxer, "auto" );
    if( !b_regular && b_auto )
        ext = "yuv";
    if( b_regular )
    {
        FILE *f = fopen( filename, "r" );
        if( f )
        {
            b_regular = x264_is_regular_file( f );
            fclose( f );
        }
    }
    const char *module = b_auto ? ext : demuxer;

    if( !strcasecmp( module, "avs" ) || !strcasecmp( ext, "d2v" ) || !strcasecmp( ext, "dga" ) )
    {
#ifdef AVS_INPUT
        input = avs_input;
        module = "avs";
#else
        fprintf( stderr, "x264 [error]: not compiled with AVS input support\n" );
        return -1;
#endif
    }
    else if( !strcasecmp( module, "y4m" ) )
        input = y4m_input;
    else if( !strcasecmp( module, "yuv" ) )
        input = yuv_input;
    else
    {
#ifdef FFMS_INPUT
        if( b_regular && (b_auto || !strcasecmp( demuxer, "ffms" )) &&
            !ffms_input.open_file( filename, p_handle, info, opt ) )
        {
            module = "ffms";
            b_auto = 0;
            input = ffms_input;
        }
#endif
#ifdef LAVF_INPUT
        if( (b_auto || !strcasecmp( demuxer, "lavf" )) &&
            !lavf_input.open_file( filename, p_handle, info, opt ) )
        {
            module = "lavf";
            b_auto = 0;
            input = lavf_input;
        }
#endif
#ifdef AVS_INPUT
        if( b_regular && (b_auto || !strcasecmp( demuxer, "avs" )) &&
            !avs_input.open_file( filename, p_handle, info, opt ) )
        {
            module = "avs";
            b_auto = 0;
            input = avs_input;
        }
#endif
        if( b_auto && !yuv_input.open_file( filename, p_handle, info, opt ) )
        {
            module = "yuv";
            b_auto = 0;
            input = yuv_input;
        }

        if( !(*p_handle) )
        {
            fprintf( stderr, "x264 [error]: could not open input file `%s' via any method!\n", filename );
            return -1;
        }
    }
    strcpy( used_demuxer, module );

    return 0;
}

/*****************************************************************************
 * Parse:
 *****************************************************************************/
static int Parse( int argc, char **argv, x264_param_t *param, cli_opt_t *opt )
{
    char *input_filename = NULL;
    const char *demuxer = demuxer_names[0];
    char *output_filename = NULL;
    const char *muxer = muxer_names[0];
    x264_param_t defaults = *param;
    char *profile = NULL;
    int b_thread_input = 0;
    int b_turbo = 1;
    int b_pass1 = 0;
    int b_user_ref = 0;
    int b_user_fps = 0;
    int b_user_interlaced = 0;
    int i;
    cli_input_opt_t input_opt;

    memset( opt, 0, sizeof(cli_opt_t) );
    memset( &input_opt, 0, sizeof(cli_input_opt_t) );
    opt->b_progress = 1;

    /* Presets are applied before all other options. */
    for( optind = 0;; )
    {
        int c = getopt_long( argc, argv, short_options, long_options, NULL );
        if( c == -1 )
            break;

        if( c == OPT_PRESET )
        {
            if( !strcasecmp( optarg, "ultrafast" ) )
            {
                param->i_frame_reference = 1;
                param->i_scenecut_threshold = 0;
                param->b_deblocking_filter = 0;
                param->b_cabac = 0;
                param->i_bframe = 0;
                param->analyse.intra = 0;
                param->analyse.inter = 0;
                param->analyse.b_transform_8x8 = 0;
                param->analyse.i_me_method = X264_ME_DIA;
                param->analyse.i_subpel_refine = 0;
                param->rc.i_aq_mode = 0;
                param->analyse.b_mixed_references = 0;
                param->analyse.i_trellis = 0;
                param->i_bframe_adaptive = X264_B_ADAPT_NONE;
                param->rc.b_mb_tree = 0;
                param->analyse.i_weighted_pred = X264_WEIGHTP_NONE;
            }
            else if( !strcasecmp( optarg, "veryfast" ) )
            {
                param->analyse.inter = X264_ANALYSE_I8x8|X264_ANALYSE_I4x4;
                param->analyse.i_me_method = X264_ME_DIA;
                param->analyse.i_subpel_refine = 1;
                param->i_frame_reference = 1;
                param->analyse.b_mixed_references = 0;
                param->analyse.i_trellis = 0;
                param->rc.b_mb_tree = 0;
                param->analyse.i_weighted_pred = X264_WEIGHTP_NONE;
            }
            else if( !strcasecmp( optarg, "faster" ) )
            {
                param->analyse.b_mixed_references = 0;
                param->i_frame_reference = 2;
                param->analyse.i_subpel_refine = 4;
                param->rc.b_mb_tree = 0;
                param->analyse.i_weighted_pred = X264_WEIGHTP_BLIND;
            }
            else if( !strcasecmp( optarg, "fast" ) )
            {
                param->i_frame_reference = 2;
                param->analyse.i_subpel_refine = 6;
                param->rc.i_lookahead = 30;
            }
            else if( !strcasecmp( optarg, "medium" ) )
            {
                /* Default is medium */
            }
            else if( !strcasecmp( optarg, "slow" ) )
            {
                param->analyse.i_me_method = X264_ME_UMH;
                param->analyse.i_subpel_refine = 8;
                param->i_frame_reference = 5;
                param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
                param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;
                param->rc.i_lookahead = 50;
            }
            else if( !strcasecmp( optarg, "slower" ) )
            {
                param->analyse.i_me_method = X264_ME_UMH;
                param->analyse.i_subpel_refine = 9;
                param->i_frame_reference = 8;
                param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
                param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;
                param->analyse.inter |= X264_ANALYSE_PSUB8x8;
                param->analyse.i_trellis = 2;
                param->rc.i_lookahead = 60;
            }
            else if( !strcasecmp( optarg, "veryslow" ) )
            {
                param->analyse.i_me_method = X264_ME_UMH;
                param->analyse.i_subpel_refine = 10;
                param->analyse.i_me_range = 24;
                param->i_frame_reference = 16;
                param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
                param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;
                param->analyse.inter |= X264_ANALYSE_PSUB8x8;
                param->analyse.i_trellis = 2;
                param->i_bframe = 8;
                param->rc.i_lookahead = 60;
            }
            else if( !strcasecmp( optarg, "placebo" ) )
            {
                param->analyse.i_me_method = X264_ME_TESA;
                param->analyse.i_subpel_refine = 10;
                param->analyse.i_me_range = 24;
                param->i_frame_reference = 16;
                param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
                param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;
                param->analyse.inter |= X264_ANALYSE_PSUB8x8;
                param->analyse.b_fast_pskip = 0;
                param->analyse.i_trellis = 2;
                param->i_bframe = 16;
                param->rc.i_lookahead = 60;
                b_turbo = 0;
            }
            else
            {
                fprintf( stderr, "x264 [error]: invalid preset '%s'\n", optarg );
                return -1;
            }
        }
        else if( c == '?' )
            return -1;
    }

    /* Tunings are applied next. */
    for( optind = 0;; )
    {
        int c = getopt_long( argc, argv, short_options, long_options, NULL );
        if( c == -1 )
            break;

        if( c == OPT_TUNE )
        {
            char *s = strtok( optarg, ",./-+" );
            int psy_tuning_used = 0;
            while( s )
            {
                if( !strncasecmp( s, "film", 4 ) )
                {
                    if( psy_tuning_used ) goto psy_failure;
                    param->i_deblocking_filter_alphac0 = -1;
                    param->i_deblocking_filter_beta = -1;
                    param->analyse.f_psy_trellis = 0.15;
                    psy_tuning_used = 1;
                }
                else if( !strncasecmp( s, "animation", 9 ) )
                {
                    if( psy_tuning_used ) goto psy_failure;
                    param->i_frame_reference = param->i_frame_reference > 1 ? param->i_frame_reference*2 : 1;
                    param->i_deblocking_filter_alphac0 = 1;
                    param->i_deblocking_filter_beta = 1;
                    param->analyse.f_psy_rd = 0.4;
                    param->rc.f_aq_strength = 0.6;
                    param->i_bframe += 2;
                    psy_tuning_used = 1;
                }
                else if( !strncasecmp( s, "grain", 5 ) )
                {
                    if( psy_tuning_used ) goto psy_failure;
                    param->i_deblocking_filter_alphac0 = -2;
                    param->i_deblocking_filter_beta = -2;
                    param->analyse.f_psy_trellis = 0.25;
                    param->analyse.b_dct_decimate = 0;
                    param->rc.f_pb_factor = 1.1;
                    param->rc.f_ip_factor = 1.1;
                    param->rc.f_aq_strength = 0.5;
                    param->analyse.i_luma_deadzone[0] = 6;
                    param->analyse.i_luma_deadzone[1] = 6;
                    param->rc.f_qcompress = 0.8;
                    psy_tuning_used = 1;
                }
                else if( !strncasecmp( s, "psnr", 4 ) )
                {
                    if( psy_tuning_used ) goto psy_failure;
                    param->rc.i_aq_mode = X264_AQ_NONE;
                    param->analyse.b_psy = 0;
                    psy_tuning_used = 1;
                }
                else if( !strncasecmp( s, "ssim", 4 ) )
                {
                    if( psy_tuning_used ) goto psy_failure;
                    param->rc.i_aq_mode = X264_AQ_AUTOVARIANCE;
                    param->analyse.b_psy = 0;
                    psy_tuning_used = 1;
                }
                else if( !strncasecmp( s, "fastdecode", 10 ) )
                {
                    param->b_deblocking_filter = 0;
                    param->b_cabac = 0;
                    param->analyse.b_weighted_bipred = 0;
                    param->analyse.i_weighted_pred = X264_WEIGHTP_NONE;
                }
                else if( !strncasecmp( s, "zerolatency", 11 ) )
                {
                    param->rc.i_lookahead = 0;
                    param->i_sync_lookahead = 0;
                    param->i_bframe = 0;
                    param->b_sliced_threads = 1;
                }
                else if( !strncasecmp( s, "touhou", 6 ) )
                {
                    if( psy_tuning_used ) goto psy_failure;
                    param->i_frame_reference = param->i_frame_reference > 1 ? param->i_frame_reference*2 : 1;
                    param->i_deblocking_filter_alphac0 = -1;
                    param->i_deblocking_filter_beta = -1;
                    param->analyse.f_psy_trellis = 0.2;
                    param->rc.f_aq_strength = 1.3;
                    if( param->analyse.inter & X264_ANALYSE_PSUB16x16 )
                        param->analyse.inter |= X264_ANALYSE_PSUB8x8;
                    psy_tuning_used = 1;
                }
                else
                {
                    fprintf( stderr, "x264 [error]: invalid tune '%s'\n", s );
                    return -1;
                }
                if( 0 )
                {
psy_failure:
                    fprintf( stderr, "x264 [warning]: only 1 psy tuning can be used: ignoring tune %s\n", s );
                }
                s = strtok( NULL, ",./-+" );
            }
        }
        else if( c == '?' )
            return -1;
    }

    /* Parse command line options */
    for( optind = 0;; )
    {
        int b_error = 0;
        int long_options_index = -1;

        int c = getopt_long( argc, argv, short_options, long_options, &long_options_index );

        if( c == -1 )
        {
            break;
        }

        switch( c )
        {
            case 'h':
                Help( &defaults, 0 );
                exit(0);
            case OPT_LONGHELP:
                Help( &defaults, 1 );
                exit(0);
            case OPT_FULLHELP:
                Help( &defaults, 2 );
                exit(0);
            case 'V':
#ifdef X264_POINTVER
                printf( "x264 "X264_POINTVER"\n" );
#else
                printf( "x264 0.%d.X\n", X264_BUILD );
#endif
                printf( "built on " __DATE__ ", " );
#ifdef __GNUC__
                printf( "gcc: " __VERSION__ "\n" );
#else
                printf( "using a non-gcc compiler\n" );
#endif
                exit(0);
            case OPT_FRAMES:
                param->i_frame_total = X264_MAX( atoi( optarg ), 0 );
                break;
            case OPT_SEEK:
                opt->i_seek = input_opt.seek = X264_MAX( atoi( optarg ), 0 );
                break;
            case 'o':
                output_filename = optarg;
                break;
            case OPT_MUXER:
                for( i = 0; muxer_names[i] && strcasecmp( muxer_names[i], optarg ); )
                    i++;
                if( !muxer_names[i] )
                {
                    fprintf( stderr, "x264 [error]: invalid muxer '%s'\n", optarg );
                    return -1;
                }
                muxer = optarg;
                break;
            case OPT_DEMUXER:
                for( i = 0; demuxer_names[i] && strcasecmp( demuxer_names[i], optarg ); )
                    i++;
                if( !demuxer_names[i] )
                {
                    fprintf( stderr, "x264 [error]: invalid demuxer '%s'\n", optarg );
                    return -1;
                }
                demuxer = optarg;
                break;
            case OPT_INDEX:
                input_opt.index = optarg;
                break;
            case OPT_QPFILE:
                opt->qpfile = fopen( optarg, "rb" );
                if( !opt->qpfile )
                {
                    fprintf( stderr, "x264 [error]: can't open qpfile `%s'\n", optarg );
                    return -1;
                }
                else if( !x264_is_regular_file( opt->qpfile ) )
                {
                    fprintf( stderr, "x264 [error]: qpfile incompatible with non-regular file `%s'\n", optarg );
                    fclose( opt->qpfile );
                    return -1;
                }
                break;
            case OPT_THREAD_INPUT:
                b_thread_input = 1;
                break;
            case OPT_QUIET:
                param->i_log_level = X264_LOG_NONE;
                break;
            case 'v':
                param->i_log_level = X264_LOG_DEBUG;
                break;
            case OPT_NOPROGRESS:
                opt->b_progress = 0;
                break;
            case OPT_VISUALIZE:
#ifdef VISUALIZE
                param->b_visualize = 1;
                b_exit_on_ctrl_c = 1;
#else
                fprintf( stderr, "x264 [warning]: not compiled with visualization support\n" );
#endif
                break;
            case OPT_TUNE:
            case OPT_PRESET:
                break;
            case OPT_PROFILE:
                profile = optarg;
                break;
            case OPT_SLOWFIRSTPASS:
                b_turbo = 0;
                break;
            case 'r':
                b_user_ref = 1;
                goto generic_option;
            case 'p':
                b_pass1 = atoi( optarg ) == 1;
                goto generic_option;
            case OPT_FPS:
                b_user_fps = 1;
                param->b_vfr_input = 0;
                goto generic_option;
            case OPT_INTERLACED:
                b_user_interlaced = 1;
                goto generic_option;
            default:
generic_option:
            {
                int i;
                if( long_options_index < 0 )
                {
                    for( i = 0; long_options[i].name; i++ )
                        if( long_options[i].val == c )
                        {
                            long_options_index = i;
                            break;
                        }
                    if( long_options_index < 0 )
                    {
                        /* getopt_long already printed an error message */
                        return -1;
                    }
                }

                b_error |= x264_param_parse( param, long_options[long_options_index].name, optarg );
            }
        }

        if( b_error )
        {
            const char *name = long_options_index > 0 ? long_options[long_options_index].name : argv[optind-2];
            fprintf( stderr, "x264 [error]: invalid argument: %s = %s\n", name, optarg );
            return -1;
        }
    }

    /* Set faster options in case of turbo firstpass. */
    if( b_turbo && b_pass1 )
    {
        param->i_frame_reference = 1;
        param->analyse.b_transform_8x8 = 0;
        param->analyse.inter = 0;
        param->analyse.i_me_method = X264_ME_DIA;
        param->analyse.i_subpel_refine = X264_MIN( 2, param->analyse.i_subpel_refine );
        param->analyse.i_trellis = 0;
    }

    /* Apply profile restrictions. */
    if( profile )
    {
        if( !strcasecmp( profile, "baseline" ) )
        {
            param->analyse.b_transform_8x8 = 0;
            param->b_cabac = 0;
            param->i_cqm_preset = X264_CQM_FLAT;
            param->i_bframe = 0;
            param->analyse.i_weighted_pred = X264_WEIGHTP_NONE;
            if( param->b_interlaced )
            {
                fprintf( stderr, "x264 [error]: baseline profile doesn't support interlacing\n" );
                return -1;
            }
        }
        else if( !strcasecmp( profile, "main" ) )
        {
            param->analyse.b_transform_8x8 = 0;
            param->i_cqm_preset = X264_CQM_FLAT;
        }
        else if( !strcasecmp( profile, "high" ) )
        {
            /* Default */
        }
        else
        {
            fprintf( stderr, "x264 [error]: invalid profile: %s\n", profile );
            return -1;
        }
        if( (param->rc.i_rc_method == X264_RC_CQP && param->rc.i_qp_constant == 0) ||
            (param->rc.i_rc_method == X264_RC_CRF && param->rc.f_rf_constant == 0) )
        {
            fprintf( stderr, "x264 [error]: %s profile doesn't support lossless\n", profile );
            return -1;
        }
    }

    /* Get the file name */
    if( optind > argc - 1 || !output_filename )
    {
        fprintf( stderr, "x264 [error]: No %s file. Run x264 --help for a list of options.\n",
                 optind > argc - 1 ? "input" : "output" );
        return -1;
    }

    if( select_output( muxer, output_filename, param ) )
        return -1;
    if( output.open_file( output_filename, &opt->hout ) )
    {
        fprintf( stderr, "x264 [error]: could not open output file `%s'\n", output_filename );
        return -1;
    }

    input_filename = argv[optind++];
    input_opt.resolution = optind < argc ? argv[optind++] : NULL;
    video_info_t info = {0};
    char demuxername[5];

    /* set info flags to param flags to be overwritten by demuxer as necessary. */
    info.csp        = param->i_csp;
    info.fps_num    = param->i_fps_num;
    info.fps_den    = param->i_fps_den;
    info.interlaced = param->b_interlaced;
    info.sar_width  = param->vui.i_sar_width;
    info.sar_height = param->vui.i_sar_height;
    info.vfr        = param->b_vfr_input;

    if( select_input( demuxer, demuxername, input_filename, &opt->hin, &info, &input_opt ) )
        return -1;

    if( !opt->hin && input.open_file( input_filename, &opt->hin, &info, &input_opt ) )
    {
        fprintf( stderr, "x264 [error]: could not open input file `%s'\n", input_filename );
        return -1;
    }

    x264_reduce_fraction( &info.sar_width, &info.sar_height );
    x264_reduce_fraction( &info.fps_num, &info.fps_den );
    if( param->i_log_level >= X264_LOG_INFO )
        fprintf( stderr, "%s [info]: %dx%d%c %d:%d @ %d/%d fps (%cfr)\n", demuxername, info.width,
                 info.height, info.interlaced ? 'i' : 'p', info.sar_width, info.sar_height,
                 info.fps_num, info.fps_den, info.vfr ? 'v' : 'c' );

    /* set param flags from the info flags as necessary */
    param->i_csp       = info.csp;
    param->i_height    = info.height;
    param->b_vfr_input = info.vfr;
    param->i_width     = info.width;
    if( !b_user_interlaced && info.interlaced )
    {
        fprintf( stderr, "x264 [warning]: input appears to be interlaced, enabling interlaced mode.\n"
                         "                If you want otherwise, use --no-interlaced\n" );
        param->b_interlaced = 1;
    }
    if( !b_user_fps )
    {
        param->i_fps_num = info.fps_num;
        param->i_fps_den = info.fps_den;
    }
    if( param->b_vfr_input )
    {
        param->i_timebase_num = info.timebase_num;
        param->i_timebase_den = info.timebase_den;
    }
    else
    {
        param->i_timebase_den = param->i_fps_num;
        param->i_timebase_num = param->i_fps_den;
    }
    if( !param->vui.i_sar_width || !param->vui.i_sar_height )
    {
        param->vui.i_sar_width  = info.sar_width;
        param->vui.i_sar_height = info.sar_height;
    }

#ifdef HAVE_PTHREAD
    if( b_thread_input || param->i_threads > 1
        || (param->i_threads == X264_THREADS_AUTO && x264_cpu_num_processors() > 1) )
    {
        if( thread_input.open_file( NULL, &opt->hin, &info, NULL ) )
        {
            fprintf( stderr, "x264 [error]: threaded input failed\n" );
            return -1;
        }
        else
            input = thread_input;
    }
#endif


    /* Automatically reduce reference frame count to match the user's target level
     * if the user didn't explicitly set a reference frame count. */
    if( !b_user_ref )
    {
        int mbs = (((param->i_width)+15)>>4) * (((param->i_height)+15)>>4);
        int i;
        for( i = 0; x264_levels[i].level_idc != 0; i++ )
            if( param->i_level_idc == x264_levels[i].level_idc )
            {
                while( mbs * 384 * param->i_frame_reference > x264_levels[i].dpb
                       && param->i_frame_reference > 1 )
                {
                    param->i_frame_reference--;
                }
                break;
            }
    }


    return 0;
}

static void parse_qpfile( cli_opt_t *opt, x264_picture_t *pic, int i_frame )
{
    int num = -1, qp, ret;
    char type;
    uint64_t file_pos;
    while( num < i_frame )
    {
        file_pos = ftell( opt->qpfile );
        ret = fscanf( opt->qpfile, "%d %c %d\n", &num, &type, &qp );
        if( num > i_frame || ret == EOF )
        {
            pic->i_type = X264_TYPE_AUTO;
            pic->i_qpplus1 = 0;
            fseek( opt->qpfile, file_pos, SEEK_SET );
            break;
        }
        if( num < i_frame && ret == 3 )
            continue;
        pic->i_qpplus1 = qp+1;
        if     ( type == 'I' ) pic->i_type = X264_TYPE_IDR;
        else if( type == 'i' ) pic->i_type = X264_TYPE_I;
        else if( type == 'P' ) pic->i_type = X264_TYPE_P;
        else if( type == 'B' ) pic->i_type = X264_TYPE_BREF;
        else if( type == 'b' ) pic->i_type = X264_TYPE_B;
        else ret = 0;
        if( ret != 3 || qp < -1 || qp > 51 )
        {
            fprintf( stderr, "x264 [error]: can't parse qpfile for frame %d\n", i_frame );
            fclose( opt->qpfile );
            opt->qpfile = NULL;
            pic->i_type = X264_TYPE_AUTO;
            pic->i_qpplus1 = 0;
            break;
        }
    }
}

/*****************************************************************************
 * Encode:
 *****************************************************************************/

static int  Encode_frame( x264_t *h, hnd_t hout, x264_picture_t *pic, int64_t *last_pts )
{
    x264_picture_t pic_out;
    x264_nal_t *nal;
    int i_nal;
    int i_frame_size = 0;

    i_frame_size = x264_encoder_encode( h, &nal, &i_nal, pic, &pic_out );

    if( i_frame_size < 0 )
    {
        fprintf( stderr, "x264 [error]: x264_encoder_encode failed\n" );
        return -1;
    }

    if( i_frame_size )
    {
        i_frame_size = output.write_frame( hout, nal[0].p_payload, i_frame_size, &pic_out );
        *last_pts = pic_out.i_pts;
    }

    return i_frame_size;
}

static void Print_status( int64_t i_start, int i_frame, int i_frame_total, int64_t i_file, x264_param_t *param, int64_t last_pts )
{
    char    buf[200];
    int64_t i_elapsed = x264_mdate() - i_start;
    double fps = i_elapsed > 0 ? i_frame * 1000000. / i_elapsed : 0;
    double bitrate = (double) i_file * 8 / ( (double) last_pts * 1000 * param->i_timebase_num / param->i_timebase_den );
    if( i_frame_total )
    {
        int eta = i_elapsed * (i_frame_total - i_frame) / ((int64_t)i_frame * 1000000);
        sprintf( buf, "x264 [%.1f%%] %d/%d frames, %.2f fps, %.2f kb/s, eta %d:%02d:%02d",
                 100. * i_frame / i_frame_total, i_frame, i_frame_total, fps, bitrate,
                 eta/3600, (eta/60)%60, eta%60 );
    }
    else
    {
        sprintf( buf, "x264 %d frames: %.2f fps, %.2f kb/s", i_frame, fps, bitrate );
    }
    fprintf( stderr, "%s  \r", buf+5 );
    SetConsoleTitle( buf );
    fflush( stderr ); // needed in windows
}

static int  Encode( x264_param_t *param, cli_opt_t *opt )
{
    x264_t *h;
    x264_picture_t pic;

    int     i_frame, i_frame_total, i_frame_output;
    int64_t i_start, i_end;
    int64_t i_file = 0;
    int     i_frame_size;
    int     i_update_interval;
    int64_t last_pts = 0;
#   define  MAX_PTS_WARNING 3 /* arbitrary */
    int     pts_warning_cnt = 0;
    int64_t largest_pts = -1;
    int64_t second_largest_pts = -1;
    int64_t ticks_per_frame;
    double  duration;
    int     prev_timebase_den = param->i_timebase_den / gcd( param->i_timebase_num, param->i_timebase_den );
    int     dts_compress_multiplier;

    opt->b_progress &= param->i_log_level < X264_LOG_DEBUG;
    i_frame_total = input.get_frame_total( opt->hin );
    i_frame_total = X264_MAX( i_frame_total - opt->i_seek, 0 );
    if( ( i_frame_total == 0 || param->i_frame_total < i_frame_total )
        && param->i_frame_total > 0 )
        i_frame_total = param->i_frame_total;
    param->i_frame_total = i_frame_total;
    i_update_interval = i_frame_total ? x264_clip3( i_frame_total / 1000, 1, 10 ) : 10;

    if( ( h = x264_encoder_open( param ) ) == NULL )
    {
        fprintf( stderr, "x264 [error]: x264_encoder_open failed\n" );
        input.close_file( opt->hin );
        return -1;
    }

    x264_encoder_parameters( h, param );

    dts_compress_multiplier = param->i_timebase_den / prev_timebase_den;

    if( output.set_param( opt->hout, param ) )
    {
        fprintf( stderr, "x264 [error]: can't set outfile param\n" );
        input.close_file( opt->hin );
        output.close_file( opt->hout, largest_pts, second_largest_pts );
        return -1;
    }

    /* Create a new pic */
    if( input.picture_alloc( &pic, param->i_csp, param->i_width, param->i_height ) )
    {
        fprintf( stderr, "x264 [error]: malloc failed\n" );
        return -1;
    }

    i_start = x264_mdate();
    /* ticks/frame = ticks/second / frames/second */
    ticks_per_frame = (int64_t)param->i_timebase_den * param->i_fps_den / param->i_timebase_num / param->i_fps_num;
    if( ticks_per_frame < 1 )
    {
        fprintf( stderr, "x264 [error]: ticks_per_frame invalid: %"PRId64"\n", ticks_per_frame );
        return -1;
    }

    if( !param->b_repeat_headers )
    {
        // Write SPS/PPS/SEI
        x264_nal_t *headers;
        int i_nal;

        if( x264_encoder_headers( h, &headers, &i_nal ) < 0 )
        {
            fprintf( stderr, "x264 [error]: x264_encoder_headers failed\n" );
            return -1;
        }

        if( (i_file = output.write_headers( opt->hout, headers )) < 0 )
            return -1;
    }

    /* Encode frames */
    for( i_frame = 0, i_frame_output = 0; b_ctrl_c == 0 && (i_frame < i_frame_total || i_frame_total == 0); )
    {
        if( input.read_frame( &pic, opt->hin, i_frame + opt->i_seek ) )
            break;

        if( !param->b_vfr_input )
            pic.i_pts = i_frame;
        if( pic.i_pts <= largest_pts )
        {
            if( param->i_log_level >= X264_LOG_WARNING )
            {
                if( param->i_log_level >= X264_LOG_DEBUG || pts_warning_cnt < MAX_PTS_WARNING )
                    fprintf( stderr, "x264 [warning]: non-strictly-monotonic pts at frame %d (%"PRId64" <= %"PRId64")\n",
                             i_frame, pic.i_pts * dts_compress_multiplier, largest_pts * dts_compress_multiplier );
                else if( pts_warning_cnt == MAX_PTS_WARNING )
                    fprintf( stderr, "x264 [warning]: too many nonmonotonic pts warnings, suppressing further ones\n" );
                pts_warning_cnt++;
            }
            pic.i_pts = largest_pts + ticks_per_frame;
        }
        second_largest_pts = largest_pts;
        largest_pts = pic.i_pts;

        if( opt->qpfile )
            parse_qpfile( opt, &pic, i_frame + opt->i_seek );
        else
        {
            /* Do not force any parameters */
            pic.i_type = X264_TYPE_AUTO;
            pic.i_qpplus1 = 0;
        }

        i_frame_size = Encode_frame( h, opt->hout, &pic, &last_pts );
        if( i_frame_size < 0 )
            return -1;
        i_file += i_frame_size;
        if( i_frame_size )
            i_frame_output++;

        i_frame++;

        if( input.release_frame && input.release_frame( &pic, opt->hin ) )
            break;

        /* update status line (up to 1000 times per input file) */
        if( opt->b_progress && i_frame_output % i_update_interval == 0 && i_frame_output )
            Print_status( i_start, i_frame_output, i_frame_total, i_file, param, last_pts );
    }
    /* Flush delayed frames */
    while( !b_ctrl_c && x264_encoder_delayed_frames( h ) )
    {
        i_frame_size = Encode_frame( h, opt->hout, NULL, &last_pts );
        if( i_frame_size < 0 )
            return -1;
        i_file += i_frame_size;
        if( i_frame_size )
            i_frame_output++;
        if( opt->b_progress && i_frame_output % i_update_interval == 0 && i_frame_output )
            Print_status( i_start, i_frame_output, i_frame_total, i_file, param, last_pts );
    }
    if( pts_warning_cnt >= MAX_PTS_WARNING && param->i_log_level < X264_LOG_DEBUG )
        fprintf( stderr, "x264 [warning]: %d suppressed nonmonotonic pts warnings\n", pts_warning_cnt-MAX_PTS_WARNING );

    /* duration algorithm fails when only 1 frame is output */
    if( i_frame_output == 1 )
        duration = (double)param->i_fps_den / param->i_fps_num;
    else
        duration = (double)(2 * largest_pts - second_largest_pts) * param->i_timebase_num / param->i_timebase_den;
    duration *= dts_compress_multiplier;

    i_end = x264_mdate();
    input.picture_clean( &pic );
    /* Erase progress indicator before printing encoding stats. */
    if( opt->b_progress )
        fprintf( stderr, "                                                                               \r" );
    x264_encoder_close( h );
    fprintf( stderr, "\n" );

    if( b_ctrl_c )
        fprintf( stderr, "aborted at input frame %d, output frame %d\n", opt->i_seek + i_frame, i_frame_output );

    input.close_file( opt->hin );
    output.close_file( opt->hout, largest_pts, second_largest_pts );

    if( i_frame_output > 0 )
    {
        double fps = (double)i_frame_output * (double)1000000 /
                     (double)( i_end - i_start );

        fprintf( stderr, "encoded %d frames, %.2f fps, %.2f kb/s\n", i_frame_output, fps,
                 (double) i_file * 8 / ( 1000 * duration ) );
    }

    return 0;
}
