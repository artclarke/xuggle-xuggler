/*****************************************************************************
 * flv.c:
 *****************************************************************************
 * Copyright (C) 2009 Kieran Kunhya
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
#include "flv_bytestream.h"
#define CHECK(x)\
do {\
    if( (x) < 0 )\
        return -1;\
} while( 0 )

typedef struct
{
    flv_buffer *c;

    uint8_t b_sps;
    uint8_t b_pps;
    uint8_t *sei;
    int sei_len;

    int64_t i_fps_num;
    int64_t i_fps_den;
    int64_t i_init_delay;
    int64_t i_framenum;
    double d_mspf;

    uint64_t i_duration_pos;
    uint64_t i_filesize_pos;
    uint64_t i_bitrate_pos;

    uint8_t b_write_length;

    unsigned start;
} flv_hnd_t;

static int write_header( flv_buffer *c )
{
    x264_put_tag( c, "FLV" ); // Signature
    x264_put_byte( c, 1 );    // Version
    x264_put_byte( c, 1 );    // Video Only
    x264_put_be32( c, 9 );    // DataOffset
    x264_put_be32( c, 0 );    // PreviousTagSize0

    return flv_flush_data( c );
}

static int open_file( char *psz_filename, hnd_t *p_handle )
{
    flv_hnd_t *p_flv = malloc( sizeof(*p_flv) );
    *p_handle = NULL;
    if( !p_flv )
        return -1;
    memset( p_flv, 0, sizeof(*p_flv) );

    p_flv->c = flv_create_writer( psz_filename );
    if( !p_flv->c )
        return -1;

    CHECK( write_header( p_flv->c ) );
    *p_handle = p_flv;

    return 0;
}

static int set_param( hnd_t handle, x264_param_t *p_param )
{
    flv_hnd_t *p_flv = handle;
    flv_buffer *c = p_flv->c;

    x264_put_byte( c, FLV_TAG_TYPE_META ); // Tag Type "script data"

    int start = c->d_cur;
    x264_put_be24( c, 0 ); // data length
    x264_put_be24( c, 0 ); // timestamp
    x264_put_be32( c, 0 ); // reserved

    x264_put_byte( c, AMF_DATA_TYPE_STRING );
    x264_put_amf_string( c, "onMetaData" );

    x264_put_byte( c, AMF_DATA_TYPE_MIXEDARRAY );
    x264_put_be32( c, 7 );

    x264_put_amf_string( c, "width" );
    x264_put_amf_double( c, p_param->i_width );

    x264_put_amf_string( c, "height" );
    x264_put_amf_double( c, p_param->i_height );

    x264_put_amf_string( c, "framerate" );
    x264_put_amf_double( c, (double)p_param->i_fps_num / p_param->i_fps_den );

    x264_put_amf_string( c, "videocodecid" );
    x264_put_amf_double( c, FLV_CODECID_H264 );

    x264_put_amf_string( c, "duration" );
    p_flv->i_duration_pos = c->d_cur + c->d_total + 1; // + 1 because of the following AMF_DATA_TYPE_NUMBER byte
    x264_put_amf_double( c, 0 ); // written at end of encoding

    x264_put_amf_string( c, "filesize" );
    p_flv->i_filesize_pos = c->d_cur + c->d_total + 1;
    x264_put_amf_double( c, 0 ); // written at end of encoding

    x264_put_amf_string( c, "videodatarate" );
    p_flv->i_bitrate_pos = c->d_cur + c->d_total + 1;
    x264_put_amf_double( c, 0 ); // written at end of encoding

    x264_put_amf_string( c, "" );
    x264_put_byte( c, AMF_END_OF_OBJECT );

    unsigned length = c->d_cur - start;
    rewrite_amf_be24( c, length - 10, start );

    x264_put_be32( c, length + 1 ); // tag length

    p_flv->i_fps_num = p_param->i_fps_num;
    p_flv->i_fps_den = p_param->i_fps_den;
    p_flv->i_init_delay = p_param->i_bframe ? (p_param->i_bframe_pyramid ? 2 : 1) : 0;
    p_flv->d_mspf = 1000 * (double)p_flv->i_fps_den / p_flv->i_fps_num;

    fprintf( stderr, "flv [info]: initial delay %i frames\n",
             (int)p_flv->i_init_delay );

    return 0;
}

static int write_nalu( hnd_t handle, uint8_t *p_nalu, int i_size, x264_picture_t *p_picture )
{
    flv_hnd_t *p_flv = handle;
    flv_buffer *c = p_flv->c;
    uint64_t dts = (uint64_t)p_flv->i_framenum * p_flv->d_mspf;
    uint64_t pts = (uint64_t)p_picture->i_pts * p_flv->d_mspf / p_flv->i_fps_den;
    uint64_t offset = p_flv->i_init_delay * p_flv->d_mspf + pts - dts;
    uint8_t type = p_nalu[4] & 0x1f;

    switch( type )
    {
        // sps
        case 0x07:
            if( !p_flv->b_sps )
            {
                uint8_t *sps = p_nalu + 4;

                x264_put_byte( c, FLV_TAG_TYPE_VIDEO );
                x264_put_be24( c, 0 ); // rewrite later, pps size unknown
                x264_put_be24( c, 0 ); // timestamp
                x264_put_byte( c, 0 ); // timestamp extended
                x264_put_be24( c, 0 ); // StreamID - Always 0
                p_flv->start = c->d_cur; // needed for overwriting length

                x264_put_byte( c, 7 | FLV_FRAME_KEY ); // Frametype and CodecID
                x264_put_byte( c, 0 ); // AVC sequence header
                x264_put_be24( c, 0 ); // composition time

                x264_put_byte( c, 1 );      // version
                x264_put_byte( c, sps[1] ); // profile
                x264_put_byte( c, sps[2] ); // profile
                x264_put_byte( c, sps[3] ); // level
                x264_put_byte( c, 0xff );   // 6 bits reserved (111111) + 2 bits nal size length - 1 (11)
                x264_put_byte( c, 0xe1 );   // 3 bits reserved (111) + 5 bits number of sps (00001)

                x264_put_be16( c, i_size - 4 );
                flv_append_data( c, sps, i_size - 4 );

                p_flv->b_sps = 1;
            }
            break;

        // pps
        case 0x08:
            if( !p_flv->b_pps )
            {
                x264_put_byte( c, 1 ); // number of pps
                x264_put_be16( c, i_size - 4 );
                flv_append_data( c, p_nalu + 4, i_size - 4 );

                // rewrite data length info
                unsigned length = c->d_cur - p_flv->start;
                rewrite_amf_be24( c, length, p_flv->start - 10 );
                x264_put_be32( c, length + 11 ); // Last tag size

                p_flv->b_pps = 1;
            }
            break;

        // slice
        case 0x1:
        case 0x5:
            if( !p_flv->b_write_length )
            {
                // A new frame - write packet header
                x264_put_byte( c, FLV_TAG_TYPE_VIDEO );
                x264_put_be24( c, 0 ); // calculated later
                x264_put_be24( c, dts );
                x264_put_byte( c, dts >> 24 );
                x264_put_be24( c, 0 );

                p_flv->start = c->d_cur;
                x264_put_byte( c, p_picture->i_type == X264_TYPE_IDR ? FLV_FRAME_KEY : FLV_FRAME_INTER );
                x264_put_byte( c, 1 ); // AVC NALU
                x264_put_be24( c, offset );

                p_flv->b_write_length = 1;
            }
            if( p_flv->sei )
            {
                flv_append_data( c, p_flv->sei, p_flv->sei_len );
                free( p_flv->sei );
                p_flv->sei = NULL;
            }
            flv_append_data( c, p_nalu, i_size );
            break;
        // sei
        case 0x6:
            /* It is within the spec to write this as-is but for
             * mplayer/ffmpeg playback this is deferred until before the first frame */

            p_flv->sei = malloc( i_size );
            if( !p_flv->sei )
                return -1;
            p_flv->sei_len = i_size;

            memcpy( p_flv->sei, p_nalu, i_size );
            break;
    }
    return i_size;
}

static int set_eop( hnd_t handle, x264_picture_t *p_picture )
{
    flv_hnd_t *p_flv = handle;
    flv_buffer *c = p_flv->c;

    if( p_flv->b_write_length )
    {
        unsigned length = c->d_cur - p_flv->start;
        rewrite_amf_be24( c, length, p_flv->start - 10 );
        x264_put_be32( c, 11 + length ); // Last tag size
        CHECK( flv_flush_data( c ) );
        p_flv->b_write_length = 0;
    }
    p_flv->i_framenum++;

    return 0;
}

static void rewrite_amf_double( FILE *fp, uint64_t position, double value )
{
    uint64_t x = endian_fix64( dbl2int( value ) );
    fseek( fp, position, SEEK_SET );
    fwrite( &x, 8, 1, fp );
}

static int close_file( hnd_t handle )
{
    flv_hnd_t *p_flv = handle;
    flv_buffer *c = p_flv->c;

    CHECK( flv_flush_data( c ) );

    if( x264_is_regular_file( c->fp ) )
    {
        double duration = p_flv->i_fps_den * p_flv->i_framenum / p_flv->i_fps_num;
        uint64_t filesize = ftell( c->fp );
        rewrite_amf_double( c->fp, p_flv->i_duration_pos, duration );
        rewrite_amf_double( c->fp, p_flv->i_filesize_pos, filesize );
        rewrite_amf_double( c->fp, p_flv->i_bitrate_pos, filesize * 8 / ( duration * 1000 ) );
    }

    fclose( c->fp );
    free( p_flv );
    free( c );

    return 0;
}

cli_output_t flv_output = { open_file, set_param, write_nalu, set_eop, close_file };
