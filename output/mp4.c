/*****************************************************************************
 * mp4.c: x264 mp4 output module
 *****************************************************************************
 * Copyright (C) 2003-2009 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
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
#include <gpac/isomedia.h>

typedef struct
{
    GF_ISOFile *p_file;
    GF_AVCConfig *p_config;
    GF_ISOSample *p_sample;
    int i_track;
    uint32_t i_descidx;
    int i_time_inc;
    int i_time_res;
    int i_numframe;
    int i_init_delay;
    uint8_t b_sps;
    uint8_t b_pps;
} mp4_hnd_t;

static void recompute_bitrate_mp4( GF_ISOFile *p_file, int i_track )
{
    u32 i, count, di, timescale, time_wnd, rate;
    u64 offset;
    Double br;
    GF_ESD *esd;

    esd = gf_isom_get_esd( p_file, i_track, 1 );
    if( !esd )
        return;

    esd->decoderConfig->avgBitrate = 0;
    esd->decoderConfig->maxBitrate = 0;
    rate = time_wnd = 0;

    timescale = gf_isom_get_media_timescale( p_file, i_track );
    count = gf_isom_get_sample_count( p_file, i_track );
    for( i = 0; i < count; i++ )
    {
        GF_ISOSample *samp = gf_isom_get_sample_info( p_file, i_track, i+1, &di, &offset );

        if( samp->dataLength>esd->decoderConfig->bufferSizeDB )
            esd->decoderConfig->bufferSizeDB = samp->dataLength;

        if( esd->decoderConfig->bufferSizeDB < samp->dataLength )
            esd->decoderConfig->bufferSizeDB = samp->dataLength;

        esd->decoderConfig->avgBitrate += samp->dataLength;
        rate += samp->dataLength;
        if( samp->DTS > time_wnd + timescale )
        {
            if( rate > esd->decoderConfig->maxBitrate )
                esd->decoderConfig->maxBitrate = rate;
            time_wnd = samp->DTS;
            rate = 0;
        }

        gf_isom_sample_del( &samp );
    }

    br = (Double)(s64)gf_isom_get_media_duration( p_file, i_track );
    br /= timescale;
    esd->decoderConfig->avgBitrate = (u32)(esd->decoderConfig->avgBitrate / br);
    /*move to bps*/
    esd->decoderConfig->avgBitrate *= 8;
    esd->decoderConfig->maxBitrate *= 8;

    gf_isom_change_mpeg4_description( p_file, i_track, 1, esd );
    gf_odf_desc_del( (GF_Descriptor*)esd );
}

static int close_file( hnd_t handle )
{
    mp4_hnd_t *p_mp4 = handle;

    if( !p_mp4 )
        return 0;

    if( p_mp4->p_config )
        gf_odf_avc_cfg_del( p_mp4->p_config );

    if( p_mp4->p_sample )
    {
        if( p_mp4->p_sample->data )
            free( p_mp4->p_sample->data );

        gf_isom_sample_del( &p_mp4->p_sample );
    }

    if( p_mp4->p_file )
    {
        recompute_bitrate_mp4( p_mp4->p_file, p_mp4->i_track );
        gf_isom_set_pl_indication( p_mp4->p_file, GF_ISOM_PL_VISUAL, 0x15 );
        gf_isom_set_storage_mode( p_mp4->p_file, GF_ISOM_STORE_FLAT );
        gf_isom_close( p_mp4->p_file );
    }

    free( p_mp4 );

    return 0;
}

static int open_file( char *psz_filename, hnd_t *p_handle )
{
    mp4_hnd_t *p_mp4;

    *p_handle = NULL;
    FILE *fh = fopen( psz_filename, "w" );
    if( !fh )
        return -1;
    else if( !x264_is_regular_file( fh ) )
    {
        fprintf( stderr, "mp4 [error]: MP4 output is incompatible with non-regular file `%s'\n", psz_filename );
        return -1;
    }
    fclose( fh );

    if( !(p_mp4 = malloc( sizeof(mp4_hnd_t) )) )
        return -1;

    memset( p_mp4, 0, sizeof(mp4_hnd_t) );
    p_mp4->p_file = gf_isom_open( psz_filename, GF_ISOM_OPEN_WRITE, NULL );

    if( !(p_mp4->p_sample = gf_isom_sample_new()) )
    {
        close_file( p_mp4 );
        return -1;
    }

    gf_isom_set_brand_info( p_mp4->p_file, GF_ISOM_BRAND_AVC1, 0 );

    *p_handle = p_mp4;

    return 0;
}

static int set_param( hnd_t handle, x264_param_t *p_param )
{
    mp4_hnd_t *p_mp4 = handle;

    p_mp4->i_track = gf_isom_new_track( p_mp4->p_file, 0, GF_ISOM_MEDIA_VISUAL,
                                        p_param->i_fps_num );

    p_mp4->p_config = gf_odf_avc_cfg_new();
    gf_isom_avc_config_new( p_mp4->p_file, p_mp4->i_track, p_mp4->p_config,
                            NULL, NULL, &p_mp4->i_descidx );

    gf_isom_set_track_enabled( p_mp4->p_file, p_mp4->i_track, 1 );

    gf_isom_set_visual_info( p_mp4->p_file, p_mp4->i_track, p_mp4->i_descidx,
                             p_param->i_width, p_param->i_height );

    if( p_param->vui.i_sar_width && p_param->vui.i_sar_height )
    {
        uint64_t dw = p_param->i_width << 16;
        uint64_t dh = p_param->i_height << 16;
        double sar = (double)p_param->vui.i_sar_width / p_param->vui.i_sar_height;
        if( sar > 1.0 )
            dw *= sar ;
        else
            dh /= sar;
        gf_isom_set_track_layout_info( p_mp4->p_file, p_mp4->i_track, dw, dh, 0, 0, 0 );
    }

    p_mp4->p_sample->data = malloc( p_param->i_width * p_param->i_height * 3 / 2 );
    if( !p_mp4->p_sample->data )
        return -1;

    p_mp4->i_time_res = p_param->i_fps_num;
    p_mp4->i_time_inc = p_param->i_fps_den;
    p_mp4->i_init_delay = p_param->i_bframe ? (p_param->i_bframe_pyramid ? 2 : 1) : 0;
    p_mp4->i_init_delay *= p_mp4->i_time_inc;
    fprintf( stderr, "mp4 [info]: initial delay %d (scale %d)\n",
             p_mp4->i_init_delay, p_mp4->i_time_res );

    return 0;
}

static int write_nalu( hnd_t handle, uint8_t *p_nalu, int i_size )
{
    mp4_hnd_t *p_mp4 = handle;
    GF_AVCConfigSlot *p_slot;
    uint8_t type = p_nalu[4] & 0x1f;
    int psize;

    switch( type )
    {
        // sps
        case 0x07:
            if( !p_mp4->b_sps )
            {
                p_mp4->p_config->configurationVersion = 1;
                p_mp4->p_config->AVCProfileIndication = p_nalu[5];
                p_mp4->p_config->profile_compatibility = p_nalu[6];
                p_mp4->p_config->AVCLevelIndication = p_nalu[7];
                p_slot = malloc( sizeof(GF_AVCConfigSlot) );
                if( !p_slot )
                    return -1;
                p_slot->size = i_size - 4;
                p_slot->data = malloc( p_slot->size );
                if( !p_slot->data )
                    return -1;
                memcpy( p_slot->data, p_nalu + 4, i_size - 4 );
                gf_list_add( p_mp4->p_config->sequenceParameterSets, p_slot );
                p_slot = NULL;
                p_mp4->b_sps = 1;
            }
            break;

        // pps
        case 0x08:
            if( !p_mp4->b_pps )
            {
                p_slot = malloc( sizeof(GF_AVCConfigSlot) );
                if( !p_slot )
                    return -1;
                p_slot->size = i_size - 4;
                p_slot->data = malloc( p_slot->size );
                if( !p_slot->data )
                    return -1;
                memcpy( p_slot->data, p_nalu + 4, i_size - 4 );
                gf_list_add( p_mp4->p_config->pictureParameterSets, p_slot );
                p_slot = NULL;
                p_mp4->b_pps = 1;
                if( p_mp4->b_sps )
                    gf_isom_avc_config_update( p_mp4->p_file, p_mp4->i_track, 1, p_mp4->p_config );
            }
            break;

        // slice, sei
        case 0x1:
        case 0x5:
        case 0x6:
            psize = i_size - 4;
            memcpy( p_mp4->p_sample->data + p_mp4->p_sample->dataLength, p_nalu, i_size );
            p_mp4->p_sample->data[p_mp4->p_sample->dataLength + 0] = psize >> 24;
            p_mp4->p_sample->data[p_mp4->p_sample->dataLength + 1] = psize >> 16;
            p_mp4->p_sample->data[p_mp4->p_sample->dataLength + 2] = psize >>  8;
            p_mp4->p_sample->data[p_mp4->p_sample->dataLength + 3] = psize >>  0;
            p_mp4->p_sample->dataLength += i_size;
            break;
    }

    return i_size;
}

static int set_eop( hnd_t handle, x264_picture_t *p_picture )
{
    mp4_hnd_t *p_mp4 = handle;
    uint64_t dts = (uint64_t)p_mp4->i_numframe * p_mp4->i_time_inc;
    uint64_t pts = (uint64_t)p_picture->i_pts;
    int32_t offset = p_mp4->i_init_delay + pts - dts;

    p_mp4->p_sample->IsRAP = p_picture->i_type == X264_TYPE_IDR ? 1 : 0;
    p_mp4->p_sample->DTS = dts;
    p_mp4->p_sample->CTS_Offset = offset;
    gf_isom_add_sample( p_mp4->p_file, p_mp4->i_track, p_mp4->i_descidx, p_mp4->p_sample );

    p_mp4->p_sample->dataLength = 0;
    p_mp4->i_numframe++;

    return 0;
}

cli_output_t mp4_output = { open_file, set_param, write_nalu, set_eop, close_file };
