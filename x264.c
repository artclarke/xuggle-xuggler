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

#include "core/common.h"
#include "x264.h"

#define DATA_MAX 3000000
uint8_t data[DATA_MAX];

/* Ctrl-C handler */
static int     i_ctrl_c = 0;
static void    SigIntHandler( int a )
{
    i_ctrl_c = 1;
}

static void Help( void );
static int  Parse( int argc, char **argv, x264_param_t  *param, FILE **p_fin, FILE **p_fout, int *pb_decompress );
static int  Encode( x264_param_t  *param, FILE *fyuv,  FILE *fout );
static int  Decode( x264_param_t  *param, FILE *fh26l, FILE *fout );

/****************************************************************************
 * main:
 ****************************************************************************/
int main( int argc, char **argv )
{
    x264_param_t param;

    FILE    *fout;
    FILE    *fin;

    int     b_decompress;
    int     i_ret;

#ifdef _MSC_VER
    _setmode(_fileno(stdin), _O_BINARY);    /* thanks to Marcos Morais <morais at dee.ufcg.edu.br> */
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    x264_param_default( &param );
    param.f_fps = 25.0;

    /* Parse command line */
    if( Parse( argc, argv, &param, &fin, &fout, &b_decompress ) < 0 )
    {
        return -1;
    }

    /* Control-C handler */
    signal( SIGINT, SigIntHandler );

    if( b_decompress )
        i_ret = Decode( &param, fin, fout );
    else
        i_ret = Encode( &param, fin, fout );

    return i_ret;
}

/*****************************************************************************
 * Help:
 *****************************************************************************/
static void Help( void )
{
    fprintf( stderr,
             "x264 build:0x%4.4x\n"
             "Syntax: x264 [options] [-o out.h26l] in.yuv widthxheigh\n"
             "\n"
             "  -h, --help                  Print this help\n"
             "\n"
             "  -I, --idrframe <integer>    Each 'number' I frames are IDR frames\n"
             "  -i, --iframe <integer>      Frequency of I frames\n"
             "  -b, --bframe <integer>      Number of B-frames between I and P\n"
             "\n"
             "  -c, --cabac                 Enable CABAC\n"
             "  -r, --ref <integer>         Number of references\n"
             "  -n, --nf                    Disable loop filter\n"
             "  -f, --filter <alpha:beta>   Loop filter AplhaCO and Beta parameters\n"
             "  -q, --qp <integer>          Set QP\n"
             "  -B, --bitrate <integer>     Set bitrate [broken]\n"
             "\n"
             "  -A, --analyse <string>      Analyse options:\n"
             "                                  - i4x4\n"
             "                                  - psub16x16,psub8x8\n"
             "                                  - none, all\n"
             "\n"
             "  -s, --sar width:height      Specify Sample Aspect Ratio\n"
             "  -o, --output                Specify output file\n"
             "\n"
             "      --no-asm                Disable any CPU optims\n"
             "\n",
            X264_BUILD
           );
}

/*****************************************************************************
 * Parse:
 *****************************************************************************/
static int  Parse( int argc, char **argv,
                   x264_param_t  *param,
                   FILE **p_fin, FILE **p_fout, int *pb_decompress )
{
    char *psz_filename = NULL;

    /* Default output */
    *p_fout = stdout;
    *p_fin  = stdin;
    *pb_decompress = 0;

    /* Parse command line options */
    opterr = 0; // no error message
    for( ;; )
    {
        int long_options_index;
        static struct option long_options[] =
        {
            { "help",    no_argument,       NULL, 'h' },
            { "bitrate", required_argument, NULL, 'B' },
            { "bframe",  required_argument, NULL, 'b' },
            { "iframe",  required_argument, NULL, 'i' },
            { "idrframe",required_argument, NULL, 'I' },
            { "nf",      no_argument,       NULL, 'n' },
            { "filter",  required_argument, NULL, 'f' },
            { "cabac",   no_argument,       NULL, 'c' },
            { "qp",      required_argument, NULL, 'q' },
            { "ref",     required_argument, NULL, 'r' },
            { "no-asm",  no_argument,       NULL, 'C' },
            { "sar",     required_argument, NULL, 's' },
            { "output",  required_argument, NULL, 'o' },
            { "analyse", required_argument, NULL, 'A' },
            {0, 0, 0, 0}
        };

        int c;

        c = getopt_long( argc, argv, "hi:I:b:r:cxB:q:no:s:A:",
                         long_options, &long_options_index);

        if( c == -1 )
        {
            break;
        }

        switch( c )
        {
            case 'h':
                Help();
                return -1;

            case 0:
                break;
            case 'B':
                param->i_bitrate = atol( optarg );
                break;
            case 'b':
                param->i_bframe = atol( optarg );
                break;
            case 'i':
                param->i_iframe = atol( optarg );
                break;
            case 'I':
                param->i_idrframe = atol( optarg );
                break;
            case 'n':
                param->b_deblocking_filter = 0;
                break;
            case 'f':
            {
                char *p = strchr( optarg, ':' );
                if( p )
                {
                    param->i_deblocking_filter_alphac0 = atoi( optarg );
                    param->i_deblocking_filter_beta = atoi( p );
                }
                break;
            }
            case 'q':
                param->i_qp_constant = atoi( optarg );
                break;
            case 'r':
                param->i_frame_reference = atoi( optarg );
                break;
            case 'c':
                param->b_cabac = 1;
                break;
            case 'x':
                *pb_decompress = 1;
                break;
            case 'C':
                param->cpu = 0;
                break;
            case'o':
                if( ( *p_fout = fopen( optarg, "wb" ) ) == NULL )
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
            case 'A':
                param->analyse.inter = 0;
                if( strstr( optarg, "none" ) )  param->analyse.inter = 0x000000;
                if( strstr( optarg, "all" ) )   param->analyse.inter = X264_ANALYSE_I4x4|X264_ANALYSE_PSUB16x16|X264_ANALYSE_PSUB8x8;

                if( strstr( optarg, "i4x4" ) )      param->analyse.inter |= X264_ANALYSE_I4x4;
                if( strstr( optarg, "psub16x16" ) ) param->analyse.inter |= X264_ANALYSE_PSUB16x16;
                if( strstr( optarg, "psub8x8" ) )   param->analyse.inter |= X264_ANALYSE_PSUB8x8;
                break;

            default:
                fprintf( stderr, "unknown option (%c)\n", optopt );
                return -1;
        }
    }

    /* Get the file name */
    if( optind > argc - 1 )
    {
        Help();
        return -1;
    }
    psz_filename = argv[optind++];

    if( !(*pb_decompress) )
    {
        char *psz_size = NULL;
        char *p;


        if( optind > argc - 1 )
        {
            char *psz = psz_filename;
            char *x = NULL;
            /* try to parse the file name */
            while( *psz )
            {
                while( *psz && ( *psz < '0' || *psz > '9' ) ) psz++;
                x = strchr( psz, 'x' );
                if( !x )
                    break;
                if( ( x[1] >= '0' && x[1] <= '9' ) )
                {
                    psz_size = psz;
                    break;
                }
            }
            if( psz_size == NULL )
            {
                Help();
                return -1;
            }
            fprintf( stderr, "x264: file name gives %dx%d\n", atoi(psz), atoi(x+1) );
        }
        else
        {
            psz_size = argv[optind++];
        }

        param->i_width           = strtol( psz_size, &p, 0 );
        param->i_height          = strtol( p+1, &p, 0 );
    }

    /* open the input */
    if( !strcmp( psz_filename, "-" ) )
    {
        *p_fin = stdin;
        optind++;
    }
    else if( ( *p_fin = fopen( psz_filename, "rb" ) ) == NULL )
    {
        fprintf( stderr, "could not open input file '%s'\n", psz_filename );
        return -1;
    }

    return 0;
}

/*****************************************************************************
 * Decode:
 *****************************************************************************/
static int  Decode( x264_param_t  *param, FILE *fh26l, FILE *fout )
{
    fprintf( stderr, "decompressor not working (help is welcome)\n" );
    return -1;
#if 0
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

    while( !i_ctrl_c )
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
                    fwrite( pic->plane[i]+i_line*pic->i_stride[i], 1, pic->i_width/i_div, fout );
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
    if( fout != stdout )
    {
        fclose( fout );
    }
    if( i_frame > 0 )
    {
        double fps = (double)i_frame * (double)1000000 /
                     (double)( i_end - i_start );
        fprintf( stderr, "decoded %d frames %ffps\n", i_frame, fps );
    }
#endif
}

/*****************************************************************************
 * Encode:
 *****************************************************************************/
static int  Encode( x264_param_t  *param, FILE *fyuv, FILE *fout )
{
    x264_t *h;
    x264_picture_t pic;

    int     i_frame, i_frame_total;
    int64_t i_start, i_end;
    int64_t i_file;

    i_frame_total = 0;
    if( !fseek( fyuv, 0, SEEK_END ) )
    {
        int64_t i_size = ftell( fyuv );
        fseek( fyuv, 0, SEEK_SET );
        i_frame_total = (int)(i_size / ( param->i_width * param->i_height * 3 / 2 ));
    }

    if( ( h = x264_encoder_open( param ) ) == NULL )
    {
        fprintf( stderr, "x264_encoder_open failed\n" );
        return -1;
    }

    /* Create a new pic */
    x264_picture_alloc( &pic, X264_CSP_I420, param->i_width, param->i_height );

    i_start = x264_mdate();
    for( i_frame = 0, i_file = 0; i_ctrl_c == 0 ; i_frame++ )
    {
        int         i_nal;
        x264_nal_t  *nal;

        int         i;

        /* read a frame */
        if( fread( pic.img.plane[0], 1, param->i_width * param->i_height, fyuv ) <= 0 ||
            fread( pic.img.plane[1], 1, param->i_width * param->i_height / 4, fyuv ) <= 0 ||
            fread( pic.img.plane[2], 1, param->i_width * param->i_height / 4, fyuv ) <= 0 )
        {
            break;
        }

        /* Do not force any parameters */
        pic.i_type = X264_TYPE_AUTO;
        if( x264_encoder_encode( h, &nal, &i_nal, &pic ) < 0 )
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
                i_file += fwrite( data, 1, i_size, fout );
            }
            else if( i_size < 0 )
            {
                fprintf( stderr,
                         "need to increase buffer size (size=%d)\n", -i_size );
            }
        }
    }
    i_end = x264_mdate();
    x264_picture_clean( &pic );
    x264_encoder_close( h );
    fprintf( stderr, "\n" );

    fclose( fyuv );
    if( fout != stdout )
    {
        fclose( fout );
    }

    if( i_frame > 0 )
    {
        double fps = (double)i_frame * (double)1000000 /
                     (double)( i_end - i_start );

        fprintf( stderr, "encoded %d frames %ffps %lld kb/s\n", i_frame, fps, i_file * 8 * 25 / i_frame / 1000 );
    }

    return 0;
}


