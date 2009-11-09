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

/* i/o file operation function pointer structs */
cli_input_t input;
static cli_output_t output;

/* i/o modules that work with pipes (and fifos) */
static const char * const stdin_format_names[] = { "yuv", "y4m", 0 };
static const char * const stdout_format_names[] = { "raw", "mkv", 0 };

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

/*****************************************************************************
 * Help:
 *****************************************************************************/
static void Help( x264_param_t *defaults, int longhelp )
{
#define H0 printf
#define H1 if(longhelp>=1) printf
#define H2 if(longhelp==2) printf
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
        "  -h, --help                  List basic options\n"
        "      --longhelp              List more options\n"
        "      --fullhelp              List all options\n"
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
    H0( "Example usage:\n" );
    H0( "\n" );
    H0( "      Constant quality mode:\n" );
    H0( "            x264 --crf 24 -o output input\n" );
    H0( "\n" );
    H0( "      Two-pass with a bitrate of 1000kbps:\n" );
    H0( "            x264 --pass 1 --bitrate 1000 -o output input\n" );
    H0( "            x264 --pass 2 --bitrate 1000 -o output input\n" );
    H0( "\n" );
    H0( "      Lossless:\n" );
    H0( "            x264 --crf 0 -o output input\n" );
    H0( "\n" );
    H0( "      Maximum PSNR at the cost of speed and visual quality:\n" );
    H0( "            x264 --preset placebo --tune psnr -o output input\n" );
    H0( "\n" );
    H0( "      Constant bitrate at 1000kbps with a 2 second-buffer:\n");
    H0( "            x264 --vbv-bufsize 2000 --bitrate 1000 -o output input\n" );
    H0( "\n" );
    H0( "Presets:\n" );
    H0( "\n" );
    H0( "      --profile               Force H.264 profile [high]\n" );
    H0( "                                  Overrides all settings\n");
    H0( "                                  - baseline,main,high\n" );
    H0( "      --preset                Use a preset to select encoding settings [medium]\n" );
    H0( "                                  Overridden by user settings\n");
    H0( "                                  - ultrafast,veryfast,faster,fast,medium\n"
        "                                  - slow,slower,veryslow,placebo\n" );
    H0( "      --tune                  Tune the settings for a particular type of source\n" );
    H0( "                                  Overridden by user settings\n");
    H2( "                                  - film,animation,grain,psnr,ssim\n"
        "                                  - fastdecode,touhou\n");
    else H0( "                                  - film,animation,grain,psnr,ssim,fastdecode\n");
    H1( "      --slow-firstpass        Don't use faster settings with --pass 1\n" );
    H0( "\n" );
    H0( "Frame-type options:\n" );
    H0( "\n" );
    H0( "  -I, --keyint <integer>      Maximum GOP size [%d]\n", defaults->i_keyint_max );
    H2( "  -i, --min-keyint <integer>  Minimum GOP size [%d]\n", defaults->i_keyint_min );
    H2( "      --no-scenecut           Disable adaptive I-frame decision\n" );
    H2( "      --scenecut <integer>    How aggressively to insert extra I-frames [%d]\n", defaults->i_scenecut_threshold );
    H1( "  -b, --bframes <integer>     Number of B-frames between I and P [%d]\n", defaults->i_bframe );
    H1( "      --b-adapt               Adaptive B-frame decision method [%d]\n"
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
    H2( "  -p, --pass <1|2|3>          Enable multipass ratecontrol\n" );
    else H0( "  -p, --pass <1|2>            Enable multipass ratecontrol\n" );
    H0( "                                  - 1: First pass, creates stats file\n"
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
    H1( "      --weightp               Weighted prediction for P-frames [2]\n"
        "                              - 0: Disabled\n"
        "                              - 1: Blind offset\n"
        "                              - 2: Smart analysis\n");
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
    H1( "      --stdout                Specify stdout format [\"%s\"]\n"
        "                                  - raw, mkv\n", stdout_format_names[0] );
    H1( "      --stdin                 Specify stdin format [\"%s\"]\n"
        "                                  - yuv, y4m\n", stdin_format_names[0] );
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
    H2( "      --thread-input          Run Avisynth in its own thread\n" );
    H2( "      --sync-lookahead <integer> Number of buffer frames for threaded lookahead\n" );
    H2( "      --non-deterministic     Slightly improve quality of SMP, at the cost of repeatability\n" );
    H2( "      --asm <integer>         Override CPU detection\n" );
    H2( "      --no-asm                Disable all CPU optimizations\n" );
    H2( "      --visualize             Show MB types overlayed on the encoded video\n" );
    H2( "      --dump-yuv <string>     Save reconstructed frames\n" );
    H2( "      --sps-id <integer>      Set SPS and PPS id numbers [%d]\n", defaults->i_sps_id );
    H2( "      --aud                   Use access unit delimiters\n" );
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
#define OPT_STDOUT_FORMAT 270
#define OPT_STDIN_FORMAT 271

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
    { "scenecut",    required_argument, NULL, 0 },
    { "no-scenecut",       no_argument, NULL, 0 },
    { "nf",                no_argument, NULL, 0 },
    { "no-deblock",        no_argument, NULL, 0 },
    { "filter",      required_argument, NULL, 0 },
    { "deblock",     required_argument, NULL, 'f' },
    { "interlaced",        no_argument, NULL, 0 },
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
    { "stdout",      required_argument, NULL, OPT_STDOUT_FORMAT },
    { "stdin",       required_argument, NULL, OPT_STDIN_FORMAT },
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
    {0, 0, 0, 0}
};

static int select_output( char *filename, const char *pipe_format )
{
    char *ext = filename + strlen( filename ) - 1;
    while( *ext != '.' && ext > filename )
        ext--;

    if( !strcasecmp( ext, ".mp4" ) )
    {
#ifdef MP4_OUTPUT
        output = mp4_output;
#else
        fprintf( stderr, "x264 [error]: not compiled with MP4 output support\n" );
        return -1;
#endif
    }
    else if( !strcasecmp( ext, ".mkv" ) || (!strcmp( filename, "-" ) && !strcasecmp( pipe_format, "mkv" )) )
        output = mkv_output;
    else
        output = raw_output;
    return 0;
}

static int select_input( char *filename, char *resolution, const char *pipe_format, x264_param_t *param )
{
    char *ext = filename + strlen( filename ) - 1;
    while( ext > filename && *ext != '.' )
        ext--;

    if( !strcasecmp( ext, ".avi" ) || !strcasecmp( ext, ".avs" ) )
    {
#ifdef AVIS_INPUT
        input = avis_input;
#else
        fprintf( stderr, "x264 [error]: not compiled with AVIS input support\n" );
        return -1;
#endif
    }
    else if( !strcasecmp( ext, ".y4m" ) || (!strcmp( filename, "-" ) && !strcasecmp( pipe_format, "y4m" )) )
        input = y4m_input;
    else // yuv
    {
        if( !resolution )
        {
            /* try to parse the file name */
            char *p;
            for( p = filename; *p; p++ )
                if( *p >= '0' && *p <= '9' &&
                    sscanf( p, "%ux%u", &param->i_width, &param->i_height ) == 2 )
                {
                    if( param->i_log_level >= X264_LOG_INFO )
                        fprintf( stderr, "x264 [info]: %dx%d (given by file name) @ %.2f fps\n", param->i_width,
                                 param->i_height, (double)param->i_fps_num / param->i_fps_den );
                    break;
                }
        }
        else
        {
            sscanf( resolution, "%ux%u", &param->i_width, &param->i_height );
            if( param->i_log_level >= X264_LOG_INFO )
                fprintf( stderr, "x264 [info]: %dx%d @ %.2f fps\n", param->i_width, param->i_height,
                         (double)param->i_fps_num / param->i_fps_den );
        }
        if( !param->i_width || !param->i_height )
        {
            fprintf( stderr, "x264 [error]: Rawyuv input requires a resolution.\n" );
            return -1;
        }
        input = yuv_input;
    }

    return 0;
}

/*****************************************************************************
 * Parse:
 *****************************************************************************/
static int  Parse( int argc, char **argv,
                   x264_param_t *param, cli_opt_t *opt )
{
    char *input_filename = NULL;
    const char *stdin_format = stdin_format_names[0];
    char *output_filename = NULL;
    const char *stdout_format = stdout_format_names[0];
    x264_param_t defaults = *param;
    char *profile = NULL;
    int b_thread_input = 0;
    int b_turbo = 1;
    int b_pass1 = 0;
    int b_user_ref = 0;
    int b_user_fps = 0;
    int i;

    memset( opt, 0, sizeof(cli_opt_t) );
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
                param->rc.i_aq_mode = X264_AQ_NONE;
                param->analyse.b_psy = 0;
            }
            else if( !strcasecmp( optarg, "ssim" ) )
            {
                param->rc.i_aq_mode = X264_AQ_AUTOVARIANCE;
                param->analyse.b_psy = 0;
            }
            else if( !strcasecmp( optarg, "fastdecode" ) )
            {
                param->b_deblocking_filter = 0;
                param->b_cabac = 0;
                param->analyse.b_weighted_bipred = 0;
                param->analyse.i_weighted_pred = X264_WEIGHTP_NONE;
            }
            else if( !strcasecmp( optarg, "touhou" ) )
            {
                param->i_frame_reference = param->i_frame_reference > 1 ? param->i_frame_reference*2 : 1;
                param->i_deblocking_filter_alphac0 = -1;
                param->i_deblocking_filter_beta = -1;
                param->analyse.f_psy_trellis = 0.2;
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
                param->i_frame_total = atoi( optarg );
                break;
            case OPT_SEEK:
                opt->i_seek = atoi( optarg );
                break;
            case 'o':
                output_filename = optarg;
                break;
            case OPT_STDOUT_FORMAT:
                for( i = 0; stdout_format_names[i] && strcasecmp( stdout_format_names[i], optarg ); )
                    i++;
                if( !stdout_format_names[i] )
                {
                    fprintf( stderr, "x264 [error]: invalid stdout format `%s'\n", optarg );
                    return -1;
                }
                stdout_format = optarg;
                break;
            case OPT_STDIN_FORMAT:
                for( i = 0; stdin_format_names[i] && strcasecmp( stdin_format_names[i], optarg ); )
                    i++;
                if( !stdin_format_names[i] )
                {
                    fprintf( stderr, "x264 [error]: invalid stdin format `%s'\n", optarg );
                    return -1;
                }
                stdin_format = optarg;
                break;
            case OPT_QPFILE:
                opt->qpfile = fopen( optarg, "rb" );
                if( !opt->qpfile )
                {
                    fprintf( stderr, "x264 [error]: can't open `%s'\n", optarg );
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
    input_filename = argv[optind++];

    if( select_output( output_filename, stdout_format ) )
        return -1;
    if( output.open_file( output_filename, &opt->hout ) )
    {
        fprintf( stderr, "x264 [error]: could not open output file `%s'\n", output_filename );
        return -1;
    }

    if( select_input( input_filename, optind < argc ? argv[optind++] : NULL, stdin_format, param ) )
        return -1;

    {
        int i_fps_num = param->i_fps_num;
        int i_fps_den = param->i_fps_den;

        if( input.open_file( input_filename, &opt->hin, param ) )
        {
            fprintf( stderr, "x264 [error]: could not open input file `%s'\n", input_filename );
            return -1;
        }
        /* Restore the user's frame rate if fps has been explicitly set on the commandline. */
        if( b_user_fps )
        {
            param->i_fps_num = i_fps_num;
            param->i_fps_den = i_fps_den;
        }
    }

#ifdef HAVE_PTHREAD
    if( b_thread_input || param->i_threads > 1
        || (param->i_threads == X264_THREADS_AUTO && x264_cpu_num_processors() > 1) )
    {
        if( thread_input.open_file( NULL, &opt->hin, param ) )
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

static int  Encode_frame( x264_t *h, hnd_t hout, x264_picture_t *pic )
{
    x264_picture_t pic_out;
    x264_nal_t *nal;
    int i_nal, i, i_nalu_size;
    int i_file = 0;

    if( x264_encoder_encode( h, &nal, &i_nal, pic, &pic_out ) < 0 )
    {
        fprintf( stderr, "x264 [error]: x264_encoder_encode failed\n" );
        return -1;
    }

    for( i = 0; i < i_nal; i++ )
    {
        i_nalu_size = output.write_nalu( hout, nal[i].p_payload, nal[i].i_payload );
        if( i_nalu_size < 0 )
            return -1;
        i_file += i_nalu_size;
    }
    if( i_nal )
        output.set_eop( hout, &pic_out );

    return i_file;
}

static void Print_status( int64_t i_start, int i_frame, int i_frame_total, int64_t i_file, x264_param_t *param )
{
    char    buf[200];
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

static int  Encode( x264_param_t *param, cli_opt_t *opt )
{
    x264_t *h;
    x264_picture_t pic;

    int     i_frame, i_frame_total, i_frame_output;
    int64_t i_start, i_end;
    int64_t i_file;
    int     i_frame_size;
    int     i_update_interval;

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

    if( output.set_param( opt->hout, param ) )
    {
        fprintf( stderr, "x264 [error]: can't set outfile param\n" );
        input.close_file( opt->hin );
        output.close_file( opt->hout );
        return -1;
    }

    /* Create a new pic */
    if( x264_picture_alloc( &pic, X264_CSP_I420, param->i_width, param->i_height ) < 0 )
    {
        fprintf( stderr, "x264 [error]: malloc failed\n" );
        return -1;
    }

    i_start = x264_mdate();

    /* Encode frames */
    for( i_frame = 0, i_file = 0, i_frame_output = 0; b_ctrl_c == 0 && (i_frame < i_frame_total || i_frame_total == 0); )
    {
        if( input.read_frame( &pic, opt->hin, i_frame + opt->i_seek ) )
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

        i_frame_size = Encode_frame( h, opt->hout, &pic );
        if( i_frame_size < 0 )
            return -1;
        i_file += i_frame_size;
        if( i_frame_size )
            i_frame_output++;

        i_frame++;

        /* update status line (up to 1000 times per input file) */
        if( opt->b_progress && i_frame_output % i_update_interval == 0 && i_frame_output )
            Print_status( i_start, i_frame_output, i_frame_total, i_file, param );
    }
    /* Flush delayed frames */
    while( !b_ctrl_c && x264_encoder_delayed_frames( h ) )
    {
        i_frame_size = Encode_frame( h, opt->hout, NULL );
        if( i_frame_size < 0 )
            return -1;
        i_file += i_frame_size;
        if( i_frame_size )
            i_frame_output++;
        if( opt->b_progress && i_frame_output % i_update_interval == 0 && i_frame_output )
            Print_status( i_start, i_frame_output, i_frame_total, i_file, param );
    }

    i_end = x264_mdate();
    x264_picture_clean( &pic );
    /* Erase progress indicator before printing encoding stats. */
    if( opt->b_progress )
        fprintf( stderr, "                                                                               \r" );
    x264_encoder_close( h );
    x264_free( mux_buffer );
    fprintf( stderr, "\n" );

    if( b_ctrl_c )
        fprintf( stderr, "aborted at input frame %d, output frame %d\n", opt->i_seek + i_frame, i_frame_output );

    input.close_file( opt->hin );
    output.close_file( opt->hout );

    if( i_frame_output > 0 )
    {
        double fps = (double)i_frame_output * (double)1000000 /
                     (double)( i_end - i_start );

        fprintf( stderr, "encoded %d frames, %.2f fps, %.2f kb/s\n", i_frame_output, fps,
                 (double) i_file * 8 * param->i_fps_num /
                 ( (double) param->i_fps_den * i_frame_output * 1000 ) );
    }

    return 0;
}
