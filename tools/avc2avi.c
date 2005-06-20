/*****************************************************************************
 * avc2avi.c: raw h264 -> AVI
 *****************************************************************************
 * Copyright (C) 2004 Laurent Aimar
 * $Id: avc2avi.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
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
#include <stdint.h>

#include <signal.h>
#define _GNU_SOURCE
#include <getopt.h>

#ifdef _MSC_VER
#include <io.h>     /* _setmode() */
#include <fcntl.h>  /* _O_BINARY */
#endif

#include "../common/bs.h"

#define DATA_MAX 3000000
uint8_t data[DATA_MAX];

/* Ctrl-C handler */
static int     i_ctrl_c = 0;
static void    SigIntHandler( int a )
{
    i_ctrl_c = 1;
}

typedef struct
{
    char *psz_fin;
    char *psz_fout;

    float f_fps;
    char  fcc[4];
} cfg_t;

typedef struct
{
    int i_data;
    int i_data_max;
    uint8_t *p_data;
} vbuf_t;

void vbuf_init( vbuf_t * );
void vbuf_add( vbuf_t *, int i_data, void *p_data );
void vbuf_reset( vbuf_t * );

typedef struct
{
    FILE *f;

    float f_fps;
    char  fcc[4];

    int   i_width;
    int   i_height;

    int64_t i_movi;
    int64_t i_movi_end;
    int64_t i_riff;

    int      i_frame;
    int      i_idx_max;
    uint32_t *idx;
} avi_t;

void avi_init( avi_t *, FILE *, float, char fcc[4] );
void avi_write( avi_t *, vbuf_t *, int  );
void avi_end( avi_t * );

enum nal_unit_type_e
{
    NAL_UNKNOWN = 0,
    NAL_SLICE   = 1,
    NAL_SLICE_DPA   = 2,
    NAL_SLICE_DPB   = 3,
    NAL_SLICE_DPC   = 4,
    NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */
    NAL_SEI         = 6,    /* ref_idc == 0 */
    NAL_SPS         = 7,
    NAL_PPS         = 8
    /* ref_idc == 0 for 6,9,10,11,12 */
};
enum nal_priority_e
{
    NAL_PRIORITY_DISPOSABLE = 0,
    NAL_PRIORITY_LOW        = 1,
    NAL_PRIORITY_HIGH       = 2,
    NAL_PRIORITY_HIGHEST    = 3,
};

typedef struct
{
    int i_ref_idc;  /* nal_priority_e */
    int i_type;     /* nal_unit_type_e */

    /* This data are raw payload */
    int     i_payload;
    uint8_t *p_payload;
} nal_t;

typedef struct
{
    int i_width;
    int i_height;

    int i_nal_type;
    int i_ref_idc;
    int i_idr_pic_id;
    int i_frame_num;
    int i_poc;

    int b_key;
    int i_log2_max_frame_num;
    int i_poc_type;
    int i_log2_max_poc_lsb;
} h264_t;

void h264_parser_init( h264_t * );
void h264_parser_parse( h264_t *h, nal_t *n, int *pb_nal_start );


static int nal_decode( nal_t *nal, void *p_data, int i_data );

static void Help( void );
static int  Parse( int argc, char **argv, cfg_t * );
static int  ParseNAL( nal_t *nal, avi_t *a, h264_t *h, int *pb_slice );

/****************************************************************************
 * main:
 ****************************************************************************/
int main( int argc, char **argv )
{
    cfg_t cfg;

    FILE    *fout;
    FILE    *fin;

    vbuf_t  vb;
    avi_t   avi;
    h264_t  h264;

    nal_t nal;
    int i_frame;
    int i_data;
    int b_eof;
    int b_key;
    int b_slice;

#ifdef _MSC_VER
    _setmode(_fileno(stdin), _O_BINARY);    /* thanks to Marcos Morais <morais at dee.ufcg.edu.br> */
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    /* Parse command line */
    if( Parse( argc, argv, &cfg ) < 0 )
    {
        return -1;
    }

    /* Open input */
    if( cfg.psz_fin == NULL || *cfg.psz_fin == '\0' || !strcmp( cfg.psz_fin, "-" ) )
        fin = stdin;
    else
        fin = fopen( cfg.psz_fin, "rb" );
    if( fin == NULL )
    {
        fprintf( stderr, "cannot open input file\n" );
        return -1;
    }

    /* Open output */
    if( cfg.psz_fout == NULL || *cfg.psz_fout == '\0' || !strcmp( cfg.psz_fout, "-" ) )
        fout = stdin;
    else
        fout = fopen( cfg.psz_fout, "wb" );
    if( fout == NULL )
    {
        fprintf( stderr, "cannot open output file\n" );
        return -1;
    }

    /* Init avi */
    avi_init( &avi, fout, cfg.f_fps, cfg.fcc );

    /* Init parser */
    h264_parser_init( &h264 );

    /* Control-C handler */
    signal( SIGINT, SigIntHandler );

    /* Init data */
    b_eof = 0;
    b_key = 0;
    b_slice = 0;
    i_frame = 0;
    i_data  = 0;

    /* Alloc space for a nal, used for decoding pps/sps/slice header */
    nal.p_payload = malloc( DATA_MAX );

    vbuf_init( &vb );

    /* split frame */
    while( !i_ctrl_c )
    {
        uint8_t *p, *p_next, *end;
        int i_size;

        /* fill buffer */
        if( i_data < DATA_MAX && !b_eof )
        {
            int i_read = fread( &data[i_data], 1, DATA_MAX - i_data, fin );
            if( i_read <= 0 )
                b_eof = 1;
            else
                i_data += i_read;
        }
        if( i_data < 3 )
            break;

        end = &data[i_data];

        /* Search begin of a NAL */
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

        /* Search end of NAL */
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
            p_next = end;

        /* Compute NAL size */
        i_size = p_next - p - 3;
        if( i_size <= 0 )
        {
            if( b_eof )
                break;

            fprintf( stderr, "nal too large (FIXME) ?\n" );
            i_data = 0;
            continue;
        }

        /* Nal start at p+3 with i_size length */
        nal_decode( &nal, p +3, i_size < 2048 ? i_size : 2048 );

        b_key = h264.b_key;

        if( b_slice && vb.i_data && ( nal.i_type == NAL_SPS || nal.i_type == NAL_PPS ) )
        {
            avi_write( &avi, &vb, b_key );
            vbuf_reset( &vb );
            b_slice = 0;
        }

        /* Parse SPS/PPS/Slice */
        if( ParseNAL( &nal, &avi, &h264, &b_slice ) && vb.i_data > 0 )
        {
            avi_write( &avi, &vb, b_key );
            vbuf_reset( &vb );
        }

        /* fprintf( stderr, "nal:%d ref:%d\n", nal.i_type, nal.i_ref_idc ); */

        /* Append NAL to buffer */
        vbuf_add( &vb, i_size + 3, p );

        /* Remove this nal */
        memmove( &data[0], p_next, end - p_next );
        i_data -= p_next - &data[0];
    }

    if( vb.i_data > 0 )
    {
        avi_write( &avi, &vb, h264.b_key );
    }

    avi.i_width  = h264.i_width;
    avi.i_height = h264.i_height;

    avi_end( &avi );

    /* free mem */
    free( nal.p_payload );

    fclose( fin );
    fclose( fout );

    return 0;
}

/*****************************************************************************
 * Help:
 *****************************************************************************/
static void Help( void )
{
    fprintf( stderr,
             "avc2avi\n"
             "Syntax: avc2avi [options] [ -i input.h264 ] [ -o output.avi ]\n"
             "\n"
             "  -h, --help                  Print this help\n"
             "\n"
             "  -i, --input                 Specify input file (default: stdin)\n"
             "  -o, --output                Specify output file (default: stdout)\n"
             "\n"
             "  -f, --fps <float>           Set FPS (default: 25.0)\n"
             "  -c, --codec <string>        Set the codec fourcc (default: 'h264')\n"
             "\n" );
}

/*****************************************************************************
 * Parse:
 *****************************************************************************/
static int  Parse( int argc, char **argv, cfg_t *cfg )
{
    /* Set default values */
    cfg->psz_fin = NULL;
    cfg->psz_fout = NULL;
    cfg->f_fps = 25.0;
    memcpy( cfg->fcc, "h264", 4 );

    /* Parse command line options */
    opterr = 0; // no error message
    for( ;; )
    {
        int long_options_index;
        static struct option long_options[] =
        {
            { "help",   no_argument,       NULL, 'h' },
            { "input",  required_argument, NULL, 'i' },
            { "output", required_argument, NULL, 'o' },
            { "fps",    required_argument, NULL, 'f' },
            { "codec",  required_argument, NULL, 'c' },
            {0, 0, 0, 0}
        };

        int c;

        c = getopt_long( argc, argv, "hi:o:f:c:",
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
            case 'i':
                cfg->psz_fin = strdup( optarg );
                break;
            case 'o':
                cfg->psz_fout = strdup( optarg );
                break;
            case 'f':
                cfg->f_fps = atof( optarg );
                break;
            case 'c':
                memset( cfg->fcc, ' ', 4 );
                memcpy( cfg->fcc, optarg, strlen( optarg ) < 4 ? strlen( optarg ) : 4 );
                break;

            default:
                fprintf( stderr, "unknown option (%c)\n", optopt );
                return -1;
        }
    }


    return 0;
}

/*****************************************************************************
 * h264_parser_*:
 *****************************************************************************/
void h264_parser_init( h264_t *h )
{
    h->i_width = 0;
    h->i_height = 0;
    h->b_key = 0;
    h->i_nal_type = -1;
    h->i_ref_idc = -1;
    h->i_idr_pic_id = -1;
    h->i_frame_num = -1;
    h->i_log2_max_frame_num = 0;
    h->i_poc = -1;
    h->i_poc_type = -1;
}
void h264_parser_parse( h264_t *h, nal_t *nal, int *pb_nal_start )
{
    bs_t s;
    *pb_nal_start = 0;

    if( nal->i_type == NAL_SPS || nal->i_type == NAL_PPS )
        *pb_nal_start = 1;

    bs_init( &s, nal->p_payload, nal->i_payload );
    if( nal->i_type == NAL_SPS )
    {
        int i_tmp;

        i_tmp = bs_read( &s, 8 );
        bs_skip( &s, 1+1+1 + 5 + 8 );
        /* sps id */
        bs_read_ue( &s );

        if( i_tmp >= 100 )
        {
            bs_read_ue( &s ); // chroma_format_idc
            bs_read_ue( &s ); // bit_depth_luma_minus8
            bs_read_ue( &s ); // bit_depth_chroma_minus8
            bs_skip( &s, 1 ); // qpprime_y_zero_transform_bypass_flag
            if( bs_read( &s, 1 ) ) // seq_scaling_matrix_present_flag
            {
                int i, j;
                for( i = 0; i < 8; i++ )
                {
                    if( bs_read( &s, 1 ) ) // seq_scaling_list_present_flag[i]
                    {
                        uint8_t i_tmp = 8;
                        for( j = 0; j < (i<6?16:64); j++ )
                        {
                            i_tmp += bs_read_se( &s );
                            if( i_tmp == 0 )
                                break;
                        }
                    }
                }
            }
        }

        /* Skip i_log2_max_frame_num */
        h->i_log2_max_frame_num = bs_read_ue( &s ) + 4;
        /* Read poc_type */
        h->i_poc_type = bs_read_ue( &s );
        if( h->i_poc_type == 0 )
        {
            h->i_log2_max_poc_lsb = bs_read_ue( &s ) + 4;
        }
        else if( h->i_poc_type == 1 )
        {
            int i_cycle;
            /* skip b_delta_pic_order_always_zero */
            bs_skip( &s, 1 );
            /* skip i_offset_for_non_ref_pic */
            bs_read_se( &s );
            /* skip i_offset_for_top_to_bottom_field */
            bs_read_se( &s );
            /* read i_num_ref_frames_in_poc_cycle */
            i_cycle = bs_read_ue( &s ); 
            if( i_cycle > 256 ) i_cycle = 256;
            while( i_cycle > 0 )
            {
                /* skip i_offset_for_ref_frame */
                bs_read_se(&s );
            }
        }
        /* i_num_ref_frames */
        bs_read_ue( &s );
        /* b_gaps_in_frame_num_value_allowed */
        bs_skip( &s, 1 );

        /* Read size */
        h->i_width  = 16 * ( bs_read_ue( &s ) + 1 );
        h->i_height = 16 * ( bs_read_ue( &s ) + 1 );

        /* b_frame_mbs_only */
        i_tmp = bs_read( &s, 1 );
        if( i_tmp == 0 )
        {
            bs_skip( &s, 1 );
        }
        /* b_direct8x8_inference */
        bs_skip( &s, 1 );

        /* crop ? */
        i_tmp = bs_read( &s, 1 );
        if( i_tmp )
        {
            /* left */
            h->i_width -= 2 * bs_read_ue( &s );
            /* right */
            h->i_width -= 2 * bs_read_ue( &s );
            /* top */
            h->i_height -= 2 * bs_read_ue( &s );
            /* bottom */
            h->i_height -= 2 * bs_read_ue( &s );
        }

        /* vui: ignored */
    }
    else if( nal->i_type >= NAL_SLICE && nal->i_type <= NAL_SLICE_IDR )
    {
        int i_tmp;

        /* i_first_mb */
        bs_read_ue( &s );
        /* picture type */
        switch( bs_read_ue( &s ) )
        {
            case 0: case 5: /* P */
            case 1: case 6: /* B */
            case 3: case 8: /* SP */
                h->b_key = 0;
                break;
            case 2: case 7: /* I */
            case 4: case 9: /* SI */
                h->b_key = (nal->i_type == NAL_SLICE_IDR);
                break;
        }
        /* pps id */
        bs_read_ue( &s );

        /* frame num */
        i_tmp = bs_read( &s, h->i_log2_max_frame_num );

        if( i_tmp != h->i_frame_num )
            *pb_nal_start = 1;

        h->i_frame_num = i_tmp;

        if( nal->i_type == NAL_SLICE_IDR )
        {
            i_tmp = bs_read_ue( &s );
            if( h->i_nal_type == NAL_SLICE_IDR && h->i_idr_pic_id != i_tmp )
                *pb_nal_start = 1;

            h->i_idr_pic_id = i_tmp;
        }

        if( h->i_poc_type == 0 )
        {
            i_tmp = bs_read( &s, h->i_log2_max_poc_lsb );
            if( i_tmp != h->i_poc )
                *pb_nal_start = 1;
            h->i_poc = i_tmp;
        }
    }
    h->i_nal_type = nal->i_type;
    h->i_ref_idc = nal->i_ref_idc;
}


static int  ParseNAL( nal_t *nal, avi_t *a, h264_t *h, int *pb_slice )
{
    int b_flush = 0;
    int b_start;

    h264_parser_parse( h, nal, &b_start );

    if( b_start && *pb_slice )
    {
        b_flush = 1;
        *pb_slice = 0;
    }

    if( nal->i_type >= NAL_SLICE && nal->i_type <= NAL_SLICE_IDR )
        *pb_slice = 1;

    return b_flush;
}

/*****************************************************************************
 * vbuf: variable buffer
 *****************************************************************************/
void vbuf_init( vbuf_t *v )
{
    v->i_data = 0;
    v->i_data_max = 10000;
    v->p_data = malloc( v->i_data_max );
}
void vbuf_add( vbuf_t *v, int i_data, void *p_data )
{
    if( i_data + v->i_data >= v->i_data_max )
    {
        v->i_data_max += i_data;
        v->p_data = realloc( v->p_data, v->i_data_max );
    }
    memcpy( &v->p_data[v->i_data], p_data, i_data );

    v->i_data += i_data;
}
void vbuf_reset( vbuf_t *v )
{
    v->i_data = 0;
}

/*****************************************************************************
 * avi:
 *****************************************************************************/
void avi_write_uint16( avi_t *a, uint16_t w )
{
    fputc( ( w      ) & 0xff, a->f );
    fputc( ( w >> 8 ) & 0xff, a->f );
}

void avi_write_uint32( avi_t *a, uint32_t dw )
{
    fputc( ( dw      ) & 0xff, a->f );
    fputc( ( dw >> 8 ) & 0xff, a->f );
    fputc( ( dw >> 16) & 0xff, a->f );
    fputc( ( dw >> 24) & 0xff, a->f );
}

void avi_write_fourcc( avi_t *a, char fcc[4] )
{
    fputc( fcc[0], a->f );
    fputc( fcc[1], a->f );
    fputc( fcc[2], a->f );
    fputc( fcc[3], a->f );
}

/* Flags in avih */
#define AVIF_HASINDEX       0x00000010  // Index at end of file?
#define AVIF_ISINTERLEAVED  0x00000100
#define AVIF_TRUSTCKTYPE    0x00000800  // Use CKType to find key frames?

#define AVIIF_KEYFRAME      0x00000010L /* this frame is a key frame.*/

void avi_write_header( avi_t *a )
{
    avi_write_fourcc( a, "RIFF" );
    avi_write_uint32( a, a->i_riff > 0 ? a->i_riff - 8 : 0xFFFFFFFF );
    avi_write_fourcc( a, "AVI " );

    avi_write_fourcc( a, "LIST" );
    avi_write_uint32( a,  4 + 4*16 + 12 + 4*16 + 4*12 );
    avi_write_fourcc( a, "hdrl" );

    avi_write_fourcc( a, "avih" );
    avi_write_uint32( a, 4*16 - 8 );
    avi_write_uint32( a, 1000000 / a->f_fps );
    avi_write_uint32( a, 0xffffffff );
    avi_write_uint32( a, 0 );
    avi_write_uint32( a, AVIF_HASINDEX|AVIF_ISINTERLEAVED|AVIF_TRUSTCKTYPE);
    avi_write_uint32( a, a->i_frame );
    avi_write_uint32( a, 0 );
    avi_write_uint32( a, 1 );
    avi_write_uint32( a, 1000000 );
    avi_write_uint32( a, a->i_width );
    avi_write_uint32( a, a->i_height );
    avi_write_uint32( a, 0 );
    avi_write_uint32( a, 0 );
    avi_write_uint32( a, 0 );
    avi_write_uint32( a, 0 );

    avi_write_fourcc( a, "LIST" );
    avi_write_uint32( a,  4 + 4*16 + 4*12 );
    avi_write_fourcc( a, "strl" );

    avi_write_fourcc( a, "strh" );
    avi_write_uint32( a,  4*16 - 8 );
    avi_write_fourcc( a, "vids" );
    avi_write_fourcc( a, a->fcc );
    avi_write_uint32( a, 0 );
    avi_write_uint32( a, 0 );
    avi_write_uint32( a, 0 );
    avi_write_uint32( a, 1000 );
    avi_write_uint32( a, a->f_fps * 1000 );
    avi_write_uint32( a, 0 );
    avi_write_uint32( a, a->i_frame );
    avi_write_uint32( a, 1024*1024 );
    avi_write_uint32( a, -1 );
    avi_write_uint32( a, a->i_width * a->i_height );
    avi_write_uint32( a, 0 );
    avi_write_uint16( a, a->i_width );
    avi_write_uint16( a, a->i_height );

    avi_write_fourcc( a, "strf" );
    avi_write_uint32( a,  4*12 - 8 );
    avi_write_uint32( a,  4*12 - 8 );
    avi_write_uint32( a,  a->i_width );
    avi_write_uint32( a,  a->i_height );
    avi_write_uint16( a,  1 );
    avi_write_uint16( a,  24 );
    avi_write_fourcc( a,  a->fcc );
    avi_write_uint32( a, a->i_width * a->i_height );
    avi_write_uint32( a,  0 );
    avi_write_uint32( a,  0 );
    avi_write_uint32( a,  0 );
    avi_write_uint32( a,  0 );

    avi_write_fourcc( a, "LIST" );
    avi_write_uint32( a,  a->i_movi_end > 0 ? a->i_movi_end - a->i_movi + 4: 0xFFFFFFFF );
    avi_write_fourcc( a, "movi" );
}

void avi_write_idx( avi_t *a )
{
    avi_write_fourcc( a, "idx1" );
    avi_write_uint32( a,  a->i_frame * 16 );
    fwrite( a->idx, a->i_frame * 16, 1, a->f );
}

void avi_init( avi_t *a, FILE *f, float f_fps, char fcc[4] )
{
    a->f = f;
    a->f_fps = f_fps;
    memcpy( a->fcc, fcc, 4 );
    a->i_width = 0;
    a->i_height = 0;
    a->i_frame = 0;
    a->i_movi = 0;
    a->i_riff = 0;
    a->i_movi_end = 0;
    a->i_idx_max = 0;
    a->idx = NULL;

    avi_write_header( a );

    a->i_movi = ftell( a->f );
}

static void avi_set_dw( void *_p, uint32_t dw )
{
    uint8_t *p = _p;

    p[0] = ( dw      )&0xff;
    p[1] = ( dw >> 8 )&0xff;
    p[2] = ( dw >> 16)&0xff;
    p[3] = ( dw >> 24)&0xff;
}

void avi_write( avi_t *a, vbuf_t *v, int b_key )
{
    int64_t i_pos = ftell( a->f );

    /* chunk header */
    avi_write_fourcc( a, "00dc" );
    avi_write_uint32( a, v->i_data );

    fwrite( v->p_data, v->i_data, 1, a->f );

    if( v->i_data&0x01 )
    {
        /* pad */
        fputc( 0, a->f );
    }

    /* Append idx chunk */
    if( a->i_idx_max <= a->i_frame )
    {
        a->i_idx_max += 1000;
        a->idx = realloc( a->idx, a->i_idx_max * 16 );
    }

    memcpy( &a->idx[4*a->i_frame+0], "00dc", 4 );
    avi_set_dw( &a->idx[4*a->i_frame+1], b_key ? AVIIF_KEYFRAME : 0 );
    avi_set_dw( &a->idx[4*a->i_frame+2], i_pos );
    avi_set_dw( &a->idx[4*a->i_frame+3], v->i_data );

    a->i_frame++;
}

void avi_end( avi_t *a )
{
    a->i_movi_end = ftell( a->f );

    /* write index */
    avi_write_idx( a );

    a->i_riff = ftell( a->f );

    /* Fix header */
    fseek( a->f, 0, SEEK_SET );
    avi_write_header( a );

    fprintf( stderr, "avi file written\n" );
    fprintf( stderr, "  - codec: %4.4s\n", a->fcc );
    fprintf( stderr, "  - size: %dx%d\n", a->i_width, a->i_height );
    fprintf( stderr, "  - fps: %.3f\n", a->f_fps );
    fprintf( stderr, "  - frames: %d\n", a->i_frame );
}

/*****************************************************************************
 * nal:
 *****************************************************************************/
int nal_decode( nal_t *nal, void *p_data, int i_data )
{
    uint8_t *src = p_data;
    uint8_t *end = &src[i_data];
    uint8_t *dst = nal->p_payload;

    nal->i_type    = src[0]&0x1f;
    nal->i_ref_idc = (src[0] >> 5)&0x03;

    src++;

    while( src < end )
    {
        if( src < end - 3 && src[0] == 0x00 && src[1] == 0x00  && src[2] == 0x03 )
        {
            *dst++ = 0x00;
            *dst++ = 0x00;

            src += 3;
            continue;
        }
        *dst++ = *src++;
    }

    nal->i_payload = dst - (uint8_t*)p_data;
    return 0;
}

