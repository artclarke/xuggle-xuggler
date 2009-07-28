/*****************************************************************************
 * x264: h264 encoder testing program.
 *****************************************************************************
 * Copyright (C) 2003-2008 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
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

#ifndef _MSC_VER
#include "config.h"
#endif

#ifdef _WIN32
#include <windows.h>
#else
#define SetConsoleTitle(t)
#endif

uint8_t *mux_buffer = NULL;
int mux_buffer_size = 0;

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

/* input file operation function pointers */
int (*p_open_infile)( char *psz_filename, hnd_t *p_handle, x264_param_t *p_param );
int (*p_get_frame_total)( hnd_t handle );
int (*p_read_frame)( x264_picture_t *p_pic, hnd_t handle, int i_frame );
int (*p_close_infile)( hnd_t handle );

/* output file operation function pointers */
static int (*p_open_outfile)( char *psz_filename, hnd_t *p_handle );
static int (*p_set_outfile_param)( hnd_t handle, x264_param_t *p_param );
static int (*p_write_nalu)( hnd_t handle, uint8_t *p_nal, int i_size );
static int (*p_set_eop)( hnd_t handle, x264_picture_t *p_picture );
static int (*p_close_outfile)( hnd_t handle );

static void Help( x264_param_t *defaults, int b_longhelp );
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

/*****************************************************************************
 * Help:
 *****************************************************************************/
static void Help( x264_param_t *defaults, int b_longhelp )
{
#define H0 printf
#define H1 if(b_longhelp) printf
    H0( "x264 core:%d%s\n"
        "Syntax: x264 [options] -o outfile infile [widthxheight]\n"
        "\n"
        "Infile can be raw YUV 4:2:0 (in which case resolution is required),\n"
        "  or YUV4MPEG 4:2:0 (*.y4m),\n"
        "  or AVI or Avisynth if compiled with AVIS support (%s).\n"
        "Outfile type is selected by filename:\n"
        " .264 -> Raw bytestream\n"
        " .mkv -> Matroska\n"
        " .mp4 -> MP4 if compiled with GPAC support (%s)\n"
        "\n"
        "Options:\n"
        "\n"
        "  -h, --help                  List the more commonly used options\n"
        "      --longhelp              List all options\n"
        "\n",
        X264_BUILD, X264_VERSION,
#ifdef AVIS_INPUT
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
    H0( "Presets:\n" );
    H0( "\n" );
    H0( "      --profile               Force H.264 profile [high]\n" );
    H0( "                                  Overrides all settings\n");
    H0( "                                  - baseline,main,high\n" );
    H0( "      --preset                Use a preset to select encoding settings [medium]\n" );
    H0( "                                  Overridden by user settings\n");
    H1( "                                  - ultrafast,veryfast,fast,medium\n"
        "                                  - slow,slower,placebo\n" );
    else H0( "                                  - ultrafast,veryfast,fast,medium,slow,slower\n" );
    H0( "      --tune                  Tune the settings for a particular type of source\n" );
    H0( "                                  Overridden by user settings\n");
    H1( "                                  - film,animation,grain,psnr,ssim\n"
        "                                  - fastdecode,touhou\n");
    else H0( "                                  - film,animation,grain,psnr,ssim,fastdecode\n");
    H0( "      --slow-firstpass        Don't use faster settings with --pass 1\n" );
    H0( "\n" );
    H0( "Frame-type options:\n" );
    H0( "\n" );
    H0( "  -I, --keyint <integer>      Maximum GOP size [%d]\n", defaults->i_keyint_max );
    H1( "  -i, --min-keyint <integer>  Minimum GOP size [%d]\n", defaults->i_keyint_min );
    H1( "      --no-scenecut           Disable adaptive I-frame decision\n" );
    H1( "      --scenecut <integer>    How aggressively to insert extra I-frames [%d]\n", defaults->i_scenecut_threshold );
    H0( "  -b, --bframes <integer>     Number of B-frames between I and P [%d]\n", defaults->i_bframe );
    H1( "      --b-adapt               Adaptive B-frame decision method [%d]\n"
        "                                  Higher values may lower threading efficiency.\n"
        "                                  - 0: Disabled\n"
        "                                  - 1: Fast\n"
        "                                  - 2: Optimal (slow with high --bframes)\n", defaults->i_bframe_adaptive );
    H1( "      --b-bias <integer>      Influences how often B-frames are used [%d]\n", defaults->i_bframe_bias );
    H0( "      --b-pyramid             Keep some B-frames as references\n" );
    H0( "      --no-cabac              Disable CABAC\n" );
    H0( "  -r, --ref <integer>         Number of reference frames [%d]\n", defaults->i_frame_reference );
    H1( "      --no-deblock            Disable loop filter\n" );
    H0( "  -f, --deblock <alpha:beta>  Loop filter AlphaC0 and Beta parameters [%d:%d]\n",
                                       defaults->i_deblocking_filter_alphac0, defaults->i_deblocking_filter_beta );
    H0( "      --interlaced            Enable pure-interlaced mode\n" );
    H0( "\n" );
    H0( "Ratecontrol:\n" );
    H0( "\n" );
    H0( "  -q, --qp <integer>          Set QP (0-51, 0=lossless)\n" );
    H0( "  -B, --bitrate <integer>     Set bitrate (kbit/s)\n" );
    H0( "      --crf <float>           Quality-based VBR (0-51, 0=lossless) [%.1f]\n", defaults->rc.f_rf_constant );
    H0( "      --vbv-maxrate <integer> Max local bitrate (kbit/s) [%d]\n", defaults->rc.i_vbv_max_bitrate );
    H0( "      --vbv-bufsize <integer> Set size of the VBV buffer (kbit) [%d]\n", defaults->rc.i_vbv_buffer_size );
    H1( "      --vbv-init <float>      Initial VBV buffer occupancy [%.1f]\n", defaults->rc.f_vbv_buffer_init );
    H1( "      --qpmin <integer>       Set min QP [%d]\n", defaults->rc.i_qp_min );
    H1( "      --qpmax <integer>       Set max QP [%d]\n", defaults->rc.i_qp_max );
    H1( "      --qpstep <integer>      Set max QP step [%d]\n", defaults->rc.i_qp_step );
    H0( "      --ratetol <float>       Allowed variance of average bitrate [%.1f]\n", defaults->rc.f_rate_tolerance );
    H0( "      --ipratio <float>       QP factor between I and P [%.2f]\n", defaults->rc.f_ip_factor );
    H0( "      --pbratio <float>       QP factor between P and B [%.2f]\n", defaults->rc.f_pb_factor );
    H1( "      --chroma-qp-offset <integer>  QP difference between chroma and luma [%d]\n", defaults->analyse.i_chroma_qp_offset );
    H1( "      --aq-mode <integer>     AQ method [%d]\n"
        "                                  - 0: Disabled\n"
        "                                  - 1: Variance AQ (complexity mask)\n"
        "                                  - 2: Auto-variance AQ (experimental)\n", defaults->rc.i_aq_mode );
    H0( "      --aq-strength <float>   Reduces blocking and blurring in flat and\n"
        "                              textured areas. [%.1f]\n"
        "                                  - 0.5: weak AQ\n"
        "                                  - 1.5: strong AQ\n", defaults->rc.f_aq_strength );
    H0( "\n" );
    H0( "  -p, --pass <1|2|3>          Enable multipass ratecontrol\n"
        "                                  - 1: First pass, creates stats file\n"
        "                                  - 2: Last pass, does not overwrite stats file\n"
        "                                  - 3: Nth pass, overwrites stats file\n" );
    H0( "      --stats <string>        Filename for 2 pass stats [\"%s\"]\n", defaults->rc.psz_stat_out );
    H0( "      --qcomp <float>         QP curve compression: 0.0 => CBR, 1.0 => CQP [%.2f]\n", defaults->rc.f_qcompress );
    H1( "      --cplxblur <float>      Reduce fluctuations in QP (before curve compression) [%.1f]\n", defaults->rc.f_complexity_blur );
    H1( "      --qblur <float>         Reduce fluctuations in QP (after curve compression) [%.1f]\n", defaults->rc.f_qblur );
    H0( "      --zones <zone0>/<zone1>/...  Tweak the bitrate of some regions of the video\n" );
    H1( "                              Each zone is of the form\n"
        "                                  <start frame>,<end frame>,<option>\n"
        "                                  where <option> is either\n"
        "                                      q=<integer> (force QP)\n"
        "                                  or  b=<float> (bitrate multiplier)\n" );
    H1( "      --qpfile <string>       Force frametypes and QPs for some or all frames\n"
        "                              Format of each line: framenumber frametype QP\n"
        "                              QP of -1 lets x264 choose. Frametypes: I,i,P,B,b.\n" );
    H0( "\n" );
    H0( "Analysis:\n" );
    H0( "\n" );
    H0( "  -A, --partitions <string>   Partitions to consider [\"p8x8,b8x8,i8x8,i4x4\"]\n"
        "                                  - p8x8, p4x4, b8x8, i8x8, i4x4\n"
        "                                  - none, all\n"
        "                                  (p4x4 requires p8x8. i8x8 requires --8x8dct.)\n" );
    H0( "      --direct <string>       Direct MV prediction mode [\"%s\"]\n"
        "                                  - none, spatial, temporal, auto\n",
                                       strtable_lookup( x264_direct_pred_names, defaults->analyse.i_direct_mv_pred ) );
    H0( "      --no-weightb            Disable weighted prediction for B-frames\n" );
    H0( "      --me <string>           Integer pixel motion estimation method [\"%s\"]\n",
                                       strtable_lookup( x264_motion_est_names, defaults->analyse.i_me_method ) );
    H1( "                                  - dia: diamond search, radius 1 (fast)\n"
        "                                  - hex: hexagonal search, radius 2\n"
        "                                  - umh: uneven multi-hexagon search\n"
        "                                  - esa: exhaustive search\n"
        "                                  - tesa: hadamard exhaustive search (slow)\n" );
    else H0( "                                  - dia, hex, umh\n" );
    H0( "      --merange <integer>     Maximum motion vector search range [%d]\n", defaults->analyse.i_me_range );
    H1( "      --mvrange <integer>     Maximum motion vector length [-1 (auto)]\n" );
    H1( "      --mvrange-thread <int>  Minimum buffer between threads [-1 (auto)]\n" );
    H0( "  -m, --subme <integer>       Subpixel motion estimation and mode decision [%d]\n", defaults->analyse.i_subpel_refine );
    H1( "                                  - 0: fullpel only (not recommended)\n"
        "                                  - 1: SAD mode decision, one qpel iteration\n"
        "                                  - 2: SATD mode decision\n"
        "                                  - 3-5: Progressively more qpel\n"
        "                                  - 6: RD mode decision for I/P-frames\n"
        "                                  - 7: RD mode decision for all frames\n"
        "                                  - 8: RD refinement for I/P-frames\n"
        "                                  - 9: RD refinement for all frames\n"
        "                                  - 10: QP-RD - requires trellis=2, aq-mode>0\n" );
    else H0( "                                  decision quality: 1=fast, 10=best.\n"  );
    H0( "      --psy-rd                Strength of psychovisual optimization [\"%.1f:%.1f\"]\n"
        "                                  #1: RD (requires subme>=6)\n"
        "                                  #2: Trellis (requires trellis, experimental)\n",
                                       defaults->analyse.f_psy_rd, defaults->analyse.f_psy_trellis );
    H0( "      --no-mixed-refs         Don't decide references on a per partition basis\n" );
    H1( "      --no-chroma-me          Ignore chroma in motion estimation\n" );
    H0( "      --no-8x8dct             Disable adaptive spatial transform size\n" );
    H0( "  -t, --trellis <integer>     Trellis RD quantization. Requires CABAC. [%d]\n"
        "                                  - 0: disabled\n"
        "                                  - 1: enabled only on the final encode of a MB\n"
        "                                  - 2: enabled on all mode decisions\n", defaults->analyse.i_trellis );
    H0( "      --no-fast-pskip         Disables early SKIP detection on P-frames\n" );
    H0( "      --no-dct-decimate       Disables coefficient thresholding on P-frames\n" );
    H0( "      --nr <integer>          Noise reduction [%d]\n", defaults->analyse.i_noise_reduction );
    H1( "\n" );
    H1( "      --deadzone-inter <int>  Set the size of the inter luma quantization deadzone [%d]\n", defaults->analyse.i_luma_deadzone[0] );
    H1( "      --deadzone-intra <int>  Set the size of the intra luma quantization deadzone [%d]\n", defaults->analyse.i_luma_deadzone[1] );
    H1( "                                  Deadzones should be in the range 0 - 32.\n" );
    H1( "      --cqm <string>          Preset quant matrices [\"flat\"]\n"
        "                                  - jvt, flat\n" );
    H0( "      --cqmfile <string>      Read custom quant matrices from a JM-compatible file\n" );
    H1( "                                  Overrides any other --cqm* options.\n" );
    H1( "      --cqm4 <list>           Set all 4x4 quant matrices\n"
        "                                  Takes a comma-separated list of 16 integers.\n" );
    H1( "      --cqm8 <list>           Set all 8x8 quant matrices\n"
        "                                  Takes a comma-separated list of 64 integers.\n" );
    H1( "      --cqm4i, --cqm4p, --cqm8i, --cqm8p\n"
        "                              Set both luma and chroma quant matrices\n" );
    H1( "      --cqm4iy, --cqm4ic, --cqm4py, --cqm4pc\n"
        "                              Set individual quant matrices\n" );
    H1( "\n" );
    H1( "Video Usability Info (Annex E):\n" );
    H1( "The VUI settings are not used by the encoder but are merely suggestions to\n" );
    H1( "the playback equipment. See doc/vui.txt for details. Use at your own risk.\n" );
    H1( "\n" );
    H1( "      --overscan <string>     Specify crop overscan setting [\"%s\"]\n"
        "                                  - undef, show, crop\n",
                                       strtable_lookup( x264_overscan_names, defaults->vui.i_overscan ) );
    H1( "      --videoformat <string>  Specify video format [\"%s\"]\n"
        "                                  - component, pal, ntsc, secam, mac, undef\n",
                                       strtable_lookup( x264_vidformat_names, defaults->vui.i_vidformat ) );
    H1( "      --fullrange <string>    Specify full range samples setting [\"%s\"]\n"
        "                                  - off, on\n",
                                       strtable_lookup( x264_fullrange_names, defaults->vui.b_fullrange ) );
    H1( "      --colorprim <string>    Specify color primaries [\"%s\"]\n"
        "                                  - undef, bt709, bt470m, bt470bg\n"
        "                                    smpte170m, smpte240m, film\n",
                                       strtable_lookup( x264_colorprim_names, defaults->vui.i_colorprim ) );
    H1( "      --transfer <string>     Specify transfer characteristics [\"%s\"]\n"
        "                                  - undef, bt709, bt470m, bt470bg, linear,\n"
        "                                    log100, log316, smpte170m, smpte240m\n",
                                       strtable_lookup( x264_transfer_names, defaults->vui.i_transfer ) );
    H1( "      --colormatrix <string>  Specify color matrix setting [\"%s\"]\n"
        "                                  - undef, bt709, fcc, bt470bg\n"
        "                                    smpte170m, smpte240m, GBR, YCgCo\n",
                                       strtable_lookup( x264_colmatrix_names, defaults->vui.i_colmatrix ) );
    H1( "      --chromaloc <integer>   Specify chroma sample location (0 to 5) [%d]\n",
                                       defaults->vui.i_chroma_loc );
    H0( "\n" );
    H0( "Input/Output:\n" );
    H0( "\n" );
    H0( "  -o, --output                Specify output file\n" );
    H0( "      --sar width:height      Specify Sample Aspect Ratio\n" );
    H0( "      --fps <float|rational>  Specify framerate\n" );
    H0( "      --seek <integer>        First frame to encode\n" );
    H0( "      --frames <integer>      Maximum number of frames to encode\n" );
    H0( "      --level <string>        Specify level (as defined by Annex A)\n" );
    H0( "\n" );
    H0( "  -v, --verbose               Print stats for each frame\n" );
    H0( "      --no-progress           Don't show the progress indicator while encoding\n" );
    H0( "      --quiet                 Quiet Mode\n" );
    H0( "      --psnr                  Enable PSNR computation\n" );
    H0( "      --ssim                  Enable SSIM computation\n" );
    H0( "      --threads <integer>     Force a specific number of threads\n" );
    H1( "      --thread-input          Run Avisynth in its own thread\n" );
    H1( "      --non-deterministic     Slightly improve quality of SMP, at the cost of repeatability\n" );
    H1( "      --asm <integer>         Override CPU detection\n" );
    H1( "      --no-asm                Disable all CPU optimizations\n" );
    H1( "      --visualize             Show MB types overlayed on the encoded video\n" );
    H1( "      --dump-yuv <string>     Save reconstructed frames\n" );
    H1( "      --sps-id <integer>      Set SPS and PPS id numbers [%d]\n", defaults->i_sps_id );
    H1( "      --aud                   Use access unit delimiters\n" );
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

static char short_options[] = "8A:B:b:f:hI:i:m:o:p:q:r:t:Vvw";
static struct option long_options[] =
{
    { "help",              no_argument, NULL, 'h' },
    { "longhelp",          no_argument, NULL, OPT_LONGHELP },
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
    { "b-pyramid",         no_argument, NULL, 0 },
    { "min-keyint",  required_argument, NULL, 'i' },
    { "keyint",      required_argument, NULL, 'I' },
    { "scenecut",    required_argument, NULL, 0 },
    { "no-scenecut",       no_argument, NULL, 0 },
    { "nf",                no_argument, NULL, 0 },
    { "no-deblock",        no_argument, NULL, 0 },
    { "filter",      required_argument, NULL, 0 },
    { "deblock",     required_argument, NULL, 'f' },
    { "interlaced",        no_argument, NULL, 0 },
    { "cabac",             no_argument, NULL, 0 },
    { "no-cabac",          no_argument, NULL, 0 },
    { "qp",          required_argument, NULL, 'q' },
    { "qpmin",       required_argument, NULL, 0 },
    { "qpmax",       required_argument, NULL, 0 },
    { "qpstep",      required_argument, NULL, 0 },
    { "crf",         required_argument, NULL, 0 },
    { "ref",         required_argument, NULL, 'r' },
    { "asm",         required_argument, NULL, 0 },
    { "no-asm",            no_argument, NULL, 0 },
    { "sar",         required_argument, NULL, 0 },
    { "fps",         required_argument, NULL, 0 },
    { "frames",      required_argument, NULL, OPT_FRAMES },
    { "seek",        required_argument, NULL, OPT_SEEK },
    { "output",      required_argument, NULL, 'o' },
    { "analyse",     required_argument, NULL, 0 },
    { "partitions",  required_argument, NULL, 'A' },
    { "direct",      required_argument, NULL, 0 },
    { "weightb",           no_argument, NULL, 'w' },
    { "no-weightb",        no_argument, NULL, 0 },
    { "me",          required_argument, NULL, 0 },
    { "merange",     required_argument, NULL, 0 },
    { "mvrange",     required_argument, NULL, 0 },
    { "mvrange-thread", required_argument, NULL, 0 },
    { "subme",       required_argument, NULL, 'm' },
    { "psy-rd",      required_argument, NULL, 0 },
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
    { "qblur",       required_argument, NULL, 0 },
    { "cplxblur",    required_argument, NULL, 0 },
    { "zones",       required_argument, NULL, 0 },
    { "qpfile",      required_argument, NULL, OPT_QPFILE },
    { "threads",     required_argument, NULL, 0 },
    { "thread-input",      no_argument, NULL, OPT_THREAD_INPUT },
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
    {0, 0, 0, 0}
};

/*****************************************************************************
 * Parse:
 *****************************************************************************/
static int  Parse( int argc, char **argv,
                   x264_param_t *param, cli_opt_t *opt )
{
    char *psz_filename = NULL;
    x264_param_t defaults = *param;
    char *psz;
    char *profile = NULL;
    int b_avis = 0;
    int b_y4m = 0;
    int b_thread_input = 0;
    int b_turbo = 1;
    int b_pass1 = 0;
    int b_user_ref = 0;

    memset( opt, 0, sizeof(cli_opt_t) );
    opt->b_progress = 1;

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
            }
            else if( !strcasecmp( optarg, "veryfast" ) )
            {
                param->analyse.inter = X264_ANALYSE_I8x8|X264_ANALYSE_I4x4;
                param->analyse.i_me_method = X264_ME_DIA;
                param->analyse.i_subpel_refine = 1;
                param->i_frame_reference = 1;
                param->analyse.b_mixed_references = 0;
                param->analyse.i_trellis = 0;
            }
            else if( !strcasecmp( optarg, "fast" ) )
            {
                param->analyse.b_mixed_references = 0;
                param->i_frame_reference = 2;
                param->analyse.i_subpel_refine = 4;
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
            }
            else
            {
                fprintf( stderr, "x264 [error]: invalid preset: %s\n", optarg );
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
            if( !strcasecmp( optarg, "film" ) )
            {
                param->i_deblocking_filter_alphac0 = -1;
                param->i_deblocking_filter_beta = -1;
                param->analyse.f_psy_trellis = 0.15;
            }
            else if( !strcasecmp( optarg, "animation" ) )
            {
                param->i_frame_reference = param->i_frame_reference > 1 ? param->i_frame_reference*2 : 1;
                param->i_deblocking_filter_alphac0 = 1;
                param->i_deblocking_filter_beta = 1;
                param->analyse.f_psy_rd = 0.4;
                param->rc.f_aq_strength = 0.6;
                param->i_bframe += 2;
            }
            else if( !strcasecmp( optarg, "grain" ) )
            {
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
            }
            else if( !strcasecmp( optarg, "psnr" ) )
            {
                param->analyse.f_psy_rd = 0;
                param->rc.i_aq_mode = X264_AQ_NONE;
            }
            else if( !strcasecmp( optarg, "ssim" ) )
            {
                param->analyse.f_psy_rd = 0;
                param->rc.i_aq_mode = X264_AQ_AUTOVARIANCE;
            }
            else if( !strcasecmp( optarg, "fastdecode" ) )
            {
                param->b_deblocking_filter = 0;
                param->b_cabac = 0;
                param->analyse.b_weighted_bipred = 0;
            }
            else if( !strcasecmp( optarg, "touhou" ) )
            {
                param->i_frame_reference = param->i_frame_reference > 1 ? param->i_frame_reference*2 : 1;
                param->i_deblocking_filter_alphac0 = -1;
                param->i_deblocking_filter_beta = -1;
                param->analyse.f_psy_trellis = 0.2;
                param->rc.f_ip_factor = 2.1;
                param->rc.f_aq_strength = 1.3;
                if( param->analyse.inter & X264_ANALYSE_PSUB16x16 )
                    param->analyse.inter |= X264_ANALYSE_PSUB8x8;
            }
            else
            {
                fprintf( stderr, "x264 [error]: invalid tune: %s\n", optarg );
                return -1;
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
                param->i_frame_total = atoi( optarg );
                break;
            case OPT_SEEK:
                opt->i_seek = atoi( optarg );
                break;
            case 'o':
                if( !strncasecmp(optarg + strlen(optarg) - 4, ".mp4", 4) )
                {
#ifdef MP4_OUTPUT
                    p_open_outfile = open_file_mp4;
                    p_write_nalu = write_nalu_mp4;
                    p_set_outfile_param = set_param_mp4;
                    p_set_eop = set_eop_mp4;
                    p_close_outfile = close_file_mp4;
#else
                    fprintf( stderr, "x264 [error]: not compiled with MP4 output support\n" );
                    return -1;
#endif
                }
                else if( !strncasecmp(optarg + strlen(optarg) - 4, ".mkv", 4) )
                {
                    p_open_outfile = open_file_mkv;
                    p_write_nalu = write_nalu_mkv;
                    p_set_outfile_param = set_param_mkv;
                    p_set_eop = set_eop_mkv;
                    p_close_outfile = close_file_mkv;
                }
                if( !strcmp(optarg, "-") )
                    opt->hout = stdout;
                else if( p_open_outfile( optarg, &opt->hout ) )
                {
                    fprintf( stderr, "x264 [error]: can't open output file `%s'\n", optarg );
                    return -1;
                }
                break;
            case OPT_QPFILE:
                opt->qpfile = fopen( optarg, "r" );
                if( !opt->qpfile )
                {
                    fprintf( stderr, "x264 [error]: can't open `%s'\n", optarg );
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
    if( optind > argc - 1 || !opt->hout )
    {
        fprintf( stderr, "x264 [error]: No %s file. Run x264 --help for a list of options.\n",
                 optind > argc - 1 ? "input" : "output" );
        return -1;
    }
    psz_filename = argv[optind++];

    /* check demuxer type */
    psz = psz_filename + strlen(psz_filename) - 1;
    while( psz > psz_filename && *psz != '.' )
        psz--;
    if( !strncasecmp( psz, ".avi", 4 ) || !strncasecmp( psz, ".avs", 4 ) )
        b_avis = 1;
    if( !strncasecmp( psz, ".y4m", 4 ) )
        b_y4m = 1;

    if( !(b_avis || b_y4m) ) // raw yuv
    {
        if( optind > argc - 1 )
        {
            /* try to parse the file name */
            for( psz = psz_filename; *psz; psz++ )
            {
                if( *psz >= '0' && *psz <= '9'
                    && sscanf( psz, "%ux%u", &param->i_width, &param->i_height ) == 2 )
                {
                    if( param->i_log_level >= X264_LOG_INFO )
                        fprintf( stderr, "x264 [info]: %dx%d (given by file name) @ %.2f fps\n", param->i_width, param->i_height, (double)param->i_fps_num / (double)param->i_fps_den);
                    break;
                }
            }
        }
        else
        {
            sscanf( argv[optind++], "%ux%u", &param->i_width, &param->i_height );
            if( param->i_log_level >= X264_LOG_INFO )
                fprintf( stderr, "x264 [info]: %dx%d @ %.2f fps\n", param->i_width, param->i_height, (double)param->i_fps_num / (double)param->i_fps_den);
        }
    }

    if( !(b_avis || b_y4m) && ( !param->i_width || !param->i_height ) )
    {
        fprintf( stderr, "x264 [error]: Rawyuv input requires a resolution.\n" );
        return -1;
    }

    /* open the input */
    {
        if( b_avis )
        {
#ifdef AVIS_INPUT
            p_open_infile = open_file_avis;
            p_get_frame_total = get_frame_total_avis;
            p_read_frame = read_frame_avis;
            p_close_infile = close_file_avis;
#else
            fprintf( stderr, "x264 [error]: not compiled with AVIS input support\n" );
            return -1;
#endif
        }
        if ( b_y4m )
        {
            p_open_infile = open_file_y4m;
            p_get_frame_total = get_frame_total_y4m;
            p_read_frame = read_frame_y4m;
            p_close_infile = close_file_y4m;
        }

        if( p_open_infile( psz_filename, &opt->hin, param ) )
        {
            fprintf( stderr, "x264 [error]: could not open input file '%s'\n", psz_filename );
            return -1;
        }
    }

#ifdef HAVE_PTHREAD
    if( b_thread_input || param->i_threads > 1
        || (param->i_threads == 0 && x264_cpu_num_processors() > 1) )
    {
        if( open_file_thread( NULL, &opt->hin, param ) )
        {
            fprintf( stderr, "x264 [warning]: threaded input failed\n" );
        }
        else
        {
            p_open_infile = open_file_thread;
            p_get_frame_total = get_frame_total_thread;
            p_read_frame = read_frame_thread;
            p_close_infile = close_file_thread;
        }
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
			fseek( opt->qpfile , file_pos , SEEK_SET );
			break;
		}
        if( num < i_frame )
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

static int  Encode_frame( x264_t *h, hnd_t hout, x264_picture_t *pic )
{
    x264_picture_t pic_out;
    x264_nal_t *nal;
    int i_nal, i;
    int i_file = 0;

    if( x264_encoder_encode( h, &nal, &i_nal, pic, &pic_out ) < 0 )
    {
        fprintf( stderr, "x264 [error]: x264_encoder_encode failed\n" );
    }

    for( i = 0; i < i_nal; i++ )
    {
        int i_size;

        if( mux_buffer_size < nal[i].i_payload * 3/2 + 4 )
        {
            mux_buffer_size = nal[i].i_payload * 2 + 4;
            x264_free( mux_buffer );
            mux_buffer = x264_malloc( mux_buffer_size );
        }

        i_size = mux_buffer_size;
        x264_nal_encode( mux_buffer, &i_size, 1, &nal[i] );
        i_file += p_write_nalu( hout, mux_buffer, i_size );
    }
    if (i_nal)
        p_set_eop( hout, &pic_out );

    return i_file;
}

static int  Encode( x264_param_t *param, cli_opt_t *opt )
{
    x264_t *h;
    x264_picture_t pic;

    int     i_frame, i_frame_total;
    int64_t i_start, i_end;
    int64_t i_file;
    int     i_frame_size;
    int     i_update_interval;
    char    buf[200];

    opt->b_progress &= param->i_log_level < X264_LOG_DEBUG;
    i_frame_total = p_get_frame_total( opt->hin );
    i_frame_total -= opt->i_seek;
    if( ( i_frame_total == 0 || param->i_frame_total < i_frame_total )
        && param->i_frame_total > 0 )
        i_frame_total = param->i_frame_total;
    param->i_frame_total = i_frame_total;
    i_update_interval = i_frame_total ? x264_clip3( i_frame_total / 1000, 1, 10 ) : 10;

    if( ( h = x264_encoder_open( param ) ) == NULL )
    {
        fprintf( stderr, "x264 [error]: x264_encoder_open failed\n" );
        p_close_infile( opt->hin );
        return -1;
    }

    if( p_set_outfile_param( opt->hout, param ) )
    {
        fprintf( stderr, "x264 [error]: can't set outfile param\n" );
        p_close_infile( opt->hin );
        p_close_outfile( opt->hout );
        return -1;
    }

    /* Create a new pic */
    x264_picture_alloc( &pic, X264_CSP_I420, param->i_width, param->i_height );

    i_start = x264_mdate();

    /* Encode frames */
    for( i_frame = 0, i_file = 0; b_ctrl_c == 0 && (i_frame < i_frame_total || i_frame_total == 0); )
    {
        if( p_read_frame( &pic, opt->hin, i_frame + opt->i_seek ) )
            break;

        pic.i_pts = (int64_t)i_frame * param->i_fps_den;

        if( opt->qpfile )
            parse_qpfile( opt, &pic, i_frame + opt->i_seek );
        else
        {
            /* Do not force any parameters */
            pic.i_type = X264_TYPE_AUTO;
            pic.i_qpplus1 = 0;
        }

        i_file += Encode_frame( h, opt->hout, &pic );

        i_frame++;

        /* update status line (up to 1000 times per input file) */
        if( opt->b_progress && i_frame % i_update_interval == 0 )
        {
            int64_t i_elapsed = x264_mdate() - i_start;
            double fps = i_elapsed > 0 ? i_frame * 1000000. / i_elapsed : 0;
            double bitrate = (double) i_file * 8 * param->i_fps_num / ( (double) param->i_fps_den * i_frame * 1000 );
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
    }
    /* Flush delayed B-frames */
    do {
        i_file +=
        i_frame_size = Encode_frame( h, opt->hout, NULL );
    } while( i_frame_size );

    i_end = x264_mdate();
    x264_picture_clean( &pic );
    /* Erase progress indicator before printing encoding stats. */
    if( opt->b_progress )
        fprintf( stderr, "                                                                               \r" );
    x264_encoder_close( h );
    x264_free( mux_buffer );
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
                 (double) i_file * 8 * param->i_fps_num /
                 ( (double) param->i_fps_den * i_frame * 1000 ) );
    }

    return 0;
}
