/***************************************************-*- coding: iso-8859-1 -*-
 * ratecontrol.c: h264 encoder library (Rate Control)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: ratecontrol.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
 *
 * Authors: Måns Rullgård <mru@mru.ath.cx>
 * 2 pass code: Michael Niedermayer <michaelni@gmx.at>
 *              Loren Merritt <lorenm@u.washington.edu>
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

#define _ISOC99_SOURCE
#undef NDEBUG // always check asserts, the speed effect is far too small to disable them
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

#include "common/common.h"
#include "common/cpu.h"
#include "common/macroblock.h"
#include "ratecontrol.h"

#ifdef SYS_MACOSX
#define exp2f(x) ( (float) exp2( (x) ) )
#endif
#ifdef SYS_FREEBSD
#define exp2f(x) powf( 2, (x) )
#endif


typedef struct
{
    int pict_type;
    int idr;
    float qscale;
    int mv_bits;
    int i_tex_bits;
    int p_tex_bits;
    int misc_bits;
    uint64_t expected_bits;
    float new_qscale;
    int new_qp;
    int i_count;
    int p_count;
    int s_count;
    float blurred_complexity;
} ratecontrol_entry_t;

struct x264_ratecontrol_t
{
    /* constants */
    double fps;
    int gop_size;
    int bitrate;
    int nmb;                    /* number of macroblocks in a frame */
    int buffer_size;
    int rcbufrate;
    int init_qp;
    int qp_constant[5];

    /* 1st pass stuff */
    int gop_qp;
    int buffer_fullness;
    int frames;                 /* frames in current gop */
    int pframes;
    int slice_type;
    int mb;                     /* MBs processed in current frame */
    int bits_gop;               /* allocated bits current gop */
    int bits_last_gop;          /* bits consumed in gop */
    int qp;                     /* qp for current frame */
    int qpm;                    /* qp for next MB */
    float qpa;                  /* average qp for last frame */
    int qps;
    float qp_avg_p;             /* average QP for P frames */
    float qp_last_p;
    int fbits;                  /* bits allocated for current frame */
    int ufbits;                 /* bits used for current frame */
    int nzcoeffs;               /* # of 0-quantized coefficients */
    int ncoeffs;                /* total # of coefficients */
    int overhead;

    /* 2pass stuff */
    FILE *p_stat_file_out;

    int num_entries;            /* number of ratecontrol_entry_ts */
    ratecontrol_entry_t *entry; /* FIXME: copy needed data and free this once init is done */
    double last_qscale;
    double last_qscale_for[5];  /* last qscale for a specific pict type, used for max_diff & ipb factor stuff  */
    int last_non_b_pict_type;
    double accum_p_qp;          /* for determining I-frame quant */
    double accum_p_norm;
    double last_accum_p_norm;
    double lmin[5];             /* min qscale by frame type */
    double lmax[5];
    double lstep;               /* max change (multiply) in qscale per frame */
    double i_cplx_sum[5];       /* estimated total texture bits in intra MBs at qscale=1 */
    double p_cplx_sum[5];
    double mv_bits_sum[5];
    int frame_count[5];         /* number of frames of each type */
};


static int init_pass2(x264_t *);
static float rate_estimate_qscale( x264_t *h, int pict_type );

/* Terminology:
 * qp = h.264's quantizer
 * qscale = linearized quantizer = Lagrange multiplier
 */
static inline double qp2qscale(double qp)
{
    return 0.85 * pow(2.0, ( qp - 12.0 ) / 6.0);
}
static inline double qscale2qp(double qscale)
{
    return 12.0 + 6.0 * log(qscale/0.85) / log(2.0);
}

static inline double qscale2bits(ratecontrol_entry_t *rce, double qscale)
{
    if(qscale<0.1)
    {
//      fprintf(stderr, "qscale<0.1\n");
        qscale = 0.1;
    }
    return (double)(rce->i_tex_bits + rce->p_tex_bits + .1) * rce->qscale / qscale;
}

static inline double bits2qscale(ratecontrol_entry_t *rce, double bits)
{
    if(bits<0.9)
    {
//      fprintf(stderr, "bits<0.9\n");
        bits = 1.0;
    }
    return rce->qscale * (double)(rce->i_tex_bits + rce->p_tex_bits + .1) / bits;
}


int x264_ratecontrol_new( x264_t *h )
{
    x264_ratecontrol_t *rc;
    float bpp;
    int i;

    /* Needed(?) for 2 pass */
    x264_cpu_restore( h->param.cpu );

    h->rc = rc = x264_malloc( sizeof( x264_ratecontrol_t ) );
    memset(rc, 0, sizeof(*rc));

    /* FIXME: use integers */
    if(h->param.i_fps_num > 0 && h->param.i_fps_den > 0)
        rc->fps = (float) h->param.i_fps_num / h->param.i_fps_den;
    else
        rc->fps = 25.0;

    rc->gop_size = h->param.i_keyint_max;
    rc->bitrate = h->param.rc.i_bitrate * 1000;
    rc->nmb = h->mb.i_mb_count;

    rc->qp_constant[SLICE_TYPE_P] = h->param.rc.i_qp_constant;
    rc->qp_constant[SLICE_TYPE_I] = x264_clip3( (int)( qscale2qp( qp2qscale( h->param.rc.i_qp_constant ) / fabs( h->param.rc.f_ip_factor )) + 0.5 ), 0, 51 );
    rc->qp_constant[SLICE_TYPE_B] = x264_clip3( (int)( qscale2qp( qp2qscale( h->param.rc.i_qp_constant ) * fabs( h->param.rc.f_pb_factor )) + 0.5 ), 0, 51 );

    /* Init 1pass CBR algo */
    if( h->param.rc.b_cbr ){
        rc->buffer_size = h->param.rc.i_rc_buffer_size * 1000;
        rc->buffer_fullness = h->param.rc.i_rc_init_buffer;
        rc->rcbufrate = rc->bitrate / rc->fps;

        if(rc->buffer_size < rc->rcbufrate){
            x264_log(h, X264_LOG_WARNING, "rc buffer size %i too small\n",
                     rc->buffer_size);
            rc->buffer_size = 0;
        }

        if(rc->buffer_size <= 0)
            rc->buffer_size = rc->bitrate / 2;

        if(rc->buffer_fullness > rc->buffer_size || rc->buffer_fullness < 0){
            x264_log(h, X264_LOG_WARNING, "invalid initial buffer fullness %i\n",
                     rc->buffer_fullness);
            rc->buffer_fullness = 0;
        }

        bpp = rc->bitrate / (rc->fps * h->param.i_width * h->param.i_height);
        if(bpp <= 0.6)
            rc->init_qp = 31;
        else if(bpp <= 1.4)
            rc->init_qp = 25;
        else if(bpp <= 2.4)
            rc->init_qp = 20;
        else
            rc->init_qp = 10;
        rc->gop_qp = rc->init_qp;

        rc->bits_last_gop = 0;

        x264_log(h, X264_LOG_DEBUG, "%f fps, %i bps, bufsize %i\n",
                 rc->fps, rc->bitrate, rc->buffer_size);
    }


    rc->lstep = exp2f(h->param.rc.i_qp_step / 6.0);
    for( i = 0; i < 5; i++ )
    {
        rc->last_qscale_for[i] = qp2qscale(26);
        rc->lmin[i] = qp2qscale( h->param.rc.i_qp_min );
        rc->lmax[i] = qp2qscale( h->param.rc.i_qp_max );
    }
#if 0 // FIXME: do we want to assign lmin/lmax based on ip_factor, or leave them all the same?
    rc->lmin[SLICE_TYPE_I] /= fabs(h->param.f_ip_factor);
    rc->lmax[SLICE_TYPE_I] /= fabs(h->param.f_ip_factor);
    rc->lmin[SLICE_TYPE_B] *= fabs(h->param.f_pb_factor);
    rc->lmax[SLICE_TYPE_B] *= fabs(h->param.f_pb_factor);
#endif

    /* Load stat file and init 2pass algo */
    if( h->param.rc.b_stat_read )
    {
        int stats_size;
        char *p, *stats_in;
        FILE *stats_file;

        /* read 1st pass stats */
        assert( h->param.rc.psz_stat_in );
        stats_file = fopen( h->param.rc.psz_stat_in, "rb");
        if(!stats_file)
        {
            x264_log(h, X264_LOG_ERROR, "ratecontrol_init: can't open stats file\n");
            return -1;
        }
        // FIXME: error checking
        fseek(stats_file, 0, SEEK_END);
        stats_size = ftell(stats_file);
        fseek(stats_file, 0, SEEK_SET);
        stats_in = x264_malloc(stats_size+10);
        fread(stats_in, 1, stats_size, stats_file);
        fclose(stats_file);

        /* find number of pics */
        p = stats_in;
        for(i=-1; p; i++){
            p = strchr(p+1, ';');
        }
        i += h->param.i_bframe;
        rc->entry = (ratecontrol_entry_t*) x264_malloc(i*sizeof(ratecontrol_entry_t));
        memset(rc->entry, 0, i*sizeof(ratecontrol_entry_t));
        rc->num_entries= i;

        /* init all to skipped p frames */
        for(i=0; i<rc->num_entries; i++){
            ratecontrol_entry_t *rce = &rc->entry[i];
            rce->pict_type = SLICE_TYPE_P;
            rce->qscale = rce->new_qscale = qp2qscale(20);
            rce->misc_bits = rc->nmb + 10;
            rce->new_qp = 0;
        }

        /* read stats */
        p = stats_in;
        for(i=0; i < rc->num_entries - h->param.i_bframe; i++){
            ratecontrol_entry_t *rce;
            int frame_number;
            char pict_type;
            int e;
            char *next;
            float qp;

            next= strchr(p, ';');
            if(next){
                (*next)=0; //sscanf is unbelievably slow on looong strings
                next++;
            }
            e = sscanf(p, " in:%d ", &frame_number);

            assert(frame_number >= 0);
            assert(frame_number < rc->num_entries);
            rce = &rc->entry[frame_number];

            e += sscanf(p, " in:%*d out:%*d type:%c q:%f itex:%d ptex:%d mv:%d misc:%d imb:%d pmb:%d smb:%d",
                   &pict_type, &qp, &rce->i_tex_bits, &rce->p_tex_bits,
                   &rce->mv_bits, &rce->misc_bits, &rce->i_count, &rce->p_count, &rce->s_count);

            switch(pict_type){
                case 'I': rce->idr = 1;
                case 'i': rce->pict_type = SLICE_TYPE_I; break;
                case 'P': rce->pict_type = SLICE_TYPE_P; break;
                case 'B': rce->pict_type = SLICE_TYPE_B; break;
                default:  e = -1; break;
            }
            if(e != 10){
                x264_log(h, X264_LOG_ERROR, "statistics are damaged at line %d, parser out=%d\n", i, e);
                return -1;
            }
            rce->qscale = qp2qscale(qp);
            p = next;
        }

        x264_free(stats_in);

        if(init_pass2(h) < 0) return -1;
    }

    /* Open output file */
    if( h->param.rc.b_stat_write )
    {
        rc->p_stat_file_out = fopen( h->param.rc.psz_stat_out, "wb" );
        if( rc->p_stat_file_out == NULL )
        {
            x264_log(h, X264_LOG_ERROR, "ratecontrol_init: can't open stats file\n");
            return -1;
        }
    }

    return 0;
}

void x264_ratecontrol_delete( x264_t *h )
{
    x264_ratecontrol_t *rc = h->rc;

    if( rc->p_stat_file_out )
        fclose( rc->p_stat_file_out );
    if( rc->entry )
        x264_free(rc->entry);
    x264_free( rc );
}

void x264_ratecontrol_start( x264_t *h, int i_slice_type )
{
    x264_ratecontrol_t *rc = h->rc;
    int gframes, iframes, pframes, bframes;
    int minbits, maxbits;
    int gbits, fbits;
    int zn = 0;
    float kp;
    int gbuf;

    rc->slice_type = i_slice_type;

    x264_cpu_restore( h->param.cpu );

    if( !h->param.rc.b_cbr )
    {
        rc->qpm = rc->qpa = rc->qp =
            rc->qp_constant[ i_slice_type ];
        return;
    }
    else if( h->param.rc.b_stat_read )
    {
        int frame = h->fenc->i_frame;
        ratecontrol_entry_t *rce;
        assert( frame >= 0 && frame < rc->num_entries );
        rce = &h->rc->entry[frame];

        rce->new_qscale = rate_estimate_qscale( h, i_slice_type );
        rc->qpm = rc->qpa = rc->qp = rce->new_qp =
            (int)(qscale2qp(rce->new_qscale) + 0.5);
        return;
    }

    switch(i_slice_type){
    case SLICE_TYPE_I:
        gbuf = rc->buffer_fullness + (rc->gop_size-1) * rc->rcbufrate;
        rc->bits_gop = gbuf - rc->buffer_size / 2;

        if(!rc->mb && rc->pframes){
            int qp = rc->qp_avg_p / rc->pframes + 0.5;
#if 0 /* JM does this without explaining why */
            int gdq = (float) rc->gop_size / 15 + 0.5;
            if(gdq > 2)
                gdq = 2;
            qp -= gdq;
            if(qp > rc->qp_last_p - 2)
                qp--;
#endif
            qp = x264_clip3(qp, rc->gop_qp - 4, rc->gop_qp + 4);
            qp = x264_clip3(qp, h->param.rc.i_qp_min, h->param.rc.i_qp_max);
            rc->gop_qp = qp;
        } else if(rc->frames > 4){
            rc->gop_qp = rc->init_qp;
        }

        kp = h->param.rc.f_ip_factor * h->param.rc.f_pb_factor;

        x264_log(h, X264_LOG_DEBUG,"gbuf=%i bits_gop=%i frames=%i gop_qp=%i\n",
                 gbuf, rc->bits_gop, rc->frames, rc->gop_qp);

        rc->bits_last_gop = 0;
        rc->frames = 0;
        rc->pframes = 0;
        rc->qp_avg_p = 0;
        break;

    case SLICE_TYPE_P:
        kp = h->param.rc.f_pb_factor;
        break;

    case SLICE_TYPE_B:
        kp = 1.0;
        break;

    default:
        x264_log(h, X264_LOG_WARNING, "ratecontrol: unknown slice type %i\n",
                 i_slice_type);
        kp = 1.0;
        break;
    }

    gframes = rc->gop_size - rc->frames;
    iframes = gframes / rc->gop_size;
    pframes = gframes / (h->param.i_bframe + 1) - iframes;
    bframes = gframes - pframes - iframes;

    gbits = rc->bits_gop - rc->bits_last_gop;
    fbits = kp * gbits /
        (h->param.rc.f_ip_factor * h->param.rc.f_pb_factor * iframes +
         h->param.rc.f_pb_factor * pframes + bframes);

    minbits = rc->buffer_fullness + rc->rcbufrate - rc->buffer_size;
    if(minbits < 0)
        minbits = 0;
    maxbits = rc->buffer_fullness;
    rc->fbits = x264_clip3(fbits, minbits, maxbits);

    if(i_slice_type == SLICE_TYPE_I){
        rc->qp = rc->gop_qp;
    } else if(rc->ncoeffs && rc->ufbits){
        int dqp, nonzc;

        nonzc = (rc->ncoeffs - rc->nzcoeffs);
        if(nonzc == 0)
            zn = rc->ncoeffs;
        else if(rc->fbits < INT_MAX / nonzc)
            zn = rc->ncoeffs - rc->fbits * nonzc / rc->ufbits;
        else
            zn = 0;
        zn = x264_clip3(zn, 0, rc->ncoeffs);
        dqp = h->param.rc.i_rc_sens * exp2f(rc->qpa / 6) *
            (zn - rc->nzcoeffs) / rc->nzcoeffs;
        dqp = x264_clip3(dqp, -h->param.rc.i_qp_step, h->param.rc.i_qp_step);
        rc->qp = (int)(rc->qpa + dqp + .5);
    }

    if(rc->fbits > 0.9 * maxbits)
        rc->qp += 2;
    else if(rc->fbits > 0.8 * maxbits)
        rc->qp += 1;
    else if(rc->fbits < 1.1 * minbits)
        rc->qp -= 2;
    else if(rc->fbits < 1.2 * minbits)
        rc->qp -= 1;

    rc->qp = x264_clip3(rc->qp, h->param.rc.i_qp_min, h->param.rc.i_qp_max);
    rc->qpm = rc->qp;

    x264_log(h, X264_LOG_DEBUG, "fbits=%i, qp=%i, z=%i, min=%i, max=%i\n",
             rc->fbits, rc->qpm, zn, minbits, maxbits);

    rc->fbits -= rc->overhead;
    rc->ufbits = 0;
    rc->ncoeffs = 0;
    rc->nzcoeffs = 0;
    rc->mb = 0;
    rc->qps = 0;
}

void x264_ratecontrol_mb( x264_t *h, int bits )
{
    x264_ratecontrol_t *rc = h->rc;
    int rbits;
    int zn, enz, nonz;
    int rcoeffs;
    int dqp;
    int i;

    if( !h->param.rc.b_cbr || h->param.rc.b_stat_read )
        return;

    x264_cpu_restore( h->param.cpu );

    rc->qps += rc->qpm;
    rc->ufbits += bits;
    rc->mb++;

    for(i = 0; i < 16 + 8; i++)
        rc->nzcoeffs += 16 - h->mb.cache.non_zero_count[x264_scan8[i]];
    rc->ncoeffs += 16 * (16 + 8);

    if(rc->mb < rc->nmb / 16)
        return;
    else if(rc->mb == rc->nmb)
        return;

    rcoeffs = (rc->nmb - rc->mb) * 16 * 24;
    rbits = rc->fbits - rc->ufbits;
/*     if(rbits < 0) */
/*      rbits = 0; */

/*     zn = (rc->nmb - rc->mb) * 16 * 24; */
    nonz = (rc->ncoeffs - rc->nzcoeffs);
    if(nonz == 0)
        zn = rcoeffs;
    else if(rc->ufbits && rbits < INT_MAX / nonz)
        zn = rcoeffs - rbits * nonz / rc->ufbits;
    else
        zn = 0;
    zn = x264_clip3(zn, 0, rcoeffs);
    enz = rc->nzcoeffs * (rc->nmb - rc->mb) / rc->mb;
    dqp = (float) 2*h->param.rc.i_rc_sens * exp2f((float) rc->qps / rc->mb / 6) *
        (zn - enz) / enz;
    rc->qpm = x264_clip3(rc->qpm + dqp, rc->qp - 3, rc->qp + 3);
    if(rbits <= 0)
        rc->qpm++;
    rc->qpm = x264_clip3(rc->qpm, h->param.rc.i_qp_min, h->param.rc.i_qp_max);
}

int  x264_ratecontrol_qp( x264_t *h )
{
    return h->rc->qpm;
}

int x264_ratecontrol_slice_type( x264_t *h, int frame_num )
{
    if( h->param.rc.b_stat_read )
    {
        if( frame_num >= h->rc->num_entries )
        {
            x264_log(h, X264_LOG_ERROR, "More input frames than in the 1st pass\n");
            return X264_TYPE_P;
        }
        switch( h->rc->entry[frame_num].pict_type )
        {
            case SLICE_TYPE_I:
                return h->rc->entry[frame_num].idr ? X264_TYPE_IDR : X264_TYPE_I;

            case SLICE_TYPE_B:
                return X264_TYPE_B;

            case SLICE_TYPE_P:
            default:
                return X264_TYPE_P;
        }
    }
    else
    {
        return X264_TYPE_AUTO;
    }
}

void x264_ratecontrol_end( x264_t *h, int bits )
{
    x264_ratecontrol_t *rc = h->rc;
    int i;

    x264_cpu_restore( h->param.cpu );

    h->stat.frame.i_mb_count_skip = h->stat.frame.i_mb_count[P_SKIP] + h->stat.frame.i_mb_count[B_SKIP];
    h->stat.frame.i_mb_count_p = h->stat.frame.i_mb_count[P_L0] + h->stat.frame.i_mb_count[P_8x8];
    for( i = B_DIRECT; i < B_8x8; i++ )
        h->stat.frame.i_mb_count_p += h->stat.frame.i_mb_count[i];

    if( h->param.rc.b_stat_write )
    {
        char c_type = rc->slice_type==SLICE_TYPE_I ? (h->fenc->i_poc==0 ? 'I' : 'i')
                    : rc->slice_type==SLICE_TYPE_P ? 'P' : 'B';
        fprintf( rc->p_stat_file_out,
                 "in:%d out:%d type:%c q:%.3f itex:%d ptex:%d mv:%d misc:%d imb:%d pmb:%d smb:%d;\n",
                 h->fenc->i_frame, h->i_frame-1,
                 c_type, rc->qpa,
                 h->stat.frame.i_itex_bits, h->stat.frame.i_ptex_bits,
                 h->stat.frame.i_hdr_bits, h->stat.frame.i_misc_bits,
                 h->stat.frame.i_mb_count[I_4x4] + h->stat.frame.i_mb_count[I_16x16],
                 h->stat.frame.i_mb_count_p,
                 h->stat.frame.i_mb_count_skip);
    }

    if( !h->param.rc.b_cbr || h->param.rc.b_stat_read )
        return;

    rc->buffer_fullness += rc->rcbufrate - bits;
    if(rc->buffer_fullness < 0){
        x264_log(h, X264_LOG_WARNING, "buffer underflow %i\n",
                 rc->buffer_fullness);
        rc->buffer_fullness = 0;
    }

    rc->qpa = (float)rc->qps / rc->mb;
    if(rc->slice_type == SLICE_TYPE_P){
        rc->qp_avg_p += rc->qpa;
        rc->qp_last_p = rc->qpa;
        rc->pframes++;
    } else if(rc->slice_type == SLICE_TYPE_I){
        float err = (float) rc->ufbits / rc->fbits;
        if(err > 1.1)
            rc->gop_qp++;
        else if(err < 0.9)
            rc->gop_qp--;
    }

    rc->overhead = bits - rc->ufbits;

    x264_log(h, X264_LOG_DEBUG, "bits=%i, qp=%.1f, z=%i, zr=%6.3f, buf=%i\n",
             bits, rc->qpa, rc->nzcoeffs, (float) rc->nzcoeffs / rc->ncoeffs,
             rc->buffer_fullness);

    rc->bits_last_gop += bits;
    rc->frames++;
    rc->mb = 0;
}

/****************************************************************************
 * 2 pass functions
 ***************************************************************************/
double x264_eval( char *s, double *const_value, const char **const_name,
                  double (**func1)(void *, double), const char **func1_name,
                  double (**func2)(void *, double, double), char **func2_name,
                  void *opaque );

/**
 * modifies the bitrate curve from pass1 for one frame
 */
static double get_qscale(x264_t *h, ratecontrol_entry_t *rce, double rate_factor)
{
    x264_ratecontrol_t *rcc= h->rc;
    double bits;
    const int pict_type = rce->pict_type;

    double const_values[]={
        rce->i_tex_bits * rce->qscale,
        rce->p_tex_bits * rce->qscale,
        (rce->i_tex_bits + rce->p_tex_bits) * rce->qscale,
        rce->mv_bits / rcc->nmb,
        (double)rce->i_count / rcc->nmb,
        (double)rce->p_count / rcc->nmb,
        (double)rce->s_count / rcc->nmb,
        rce->pict_type == SLICE_TYPE_I,
        rce->pict_type == SLICE_TYPE_P,
        rce->pict_type == SLICE_TYPE_B,
        h->param.rc.f_qcompress,
        rcc->i_cplx_sum[SLICE_TYPE_I] / rcc->frame_count[SLICE_TYPE_I],
        rcc->i_cplx_sum[SLICE_TYPE_P] / rcc->frame_count[SLICE_TYPE_P],
        rcc->p_cplx_sum[SLICE_TYPE_P] / rcc->frame_count[SLICE_TYPE_P],
        rcc->p_cplx_sum[SLICE_TYPE_B] / rcc->frame_count[SLICE_TYPE_B],
        (rcc->i_cplx_sum[pict_type] + rcc->p_cplx_sum[pict_type]) / rcc->frame_count[pict_type],
        rce->blurred_complexity,
        0
    };
    static const char *const_names[]={
        "iTex",
        "pTex",
        "tex",
        "mv",
        "iCount",
        "pCount",
        "sCount",
        "isI",
        "isP",
        "isB",
        "qComp",
        "avgIITex",
        "avgPITex",
        "avgPPTex",
        "avgBPTex",
        "avgTex",
        "blurTex",
        NULL
    };
    static double (*func1[])(void *, double)={
        (void *)bits2qscale,
        (void *)qscale2bits,
        NULL
    };
    static const char *func1_names[]={
        "bits2qp",
        "qp2bits",
        NULL
    };

    bits = x264_eval((char*)h->param.rc.psz_rc_eq, const_values, const_names, func1, func1_names, NULL, NULL, rce);

    // avoid NaN's in the rc_eq
    if(bits != bits || rce->i_tex_bits + rce->p_tex_bits == 0)
        bits = 0;

    bits *= rate_factor;
    if(bits<0.0) bits=0.0;
    bits += 1.0; //avoid 1/0 issues

    return bits2qscale(rce, bits);
}

static double get_diff_limited_q(x264_t *h, ratecontrol_entry_t *rce, double q)
{
    x264_ratecontrol_t *rcc = h->rc;
    const int pict_type = rce->pict_type;

    // force I/B quants as a function of P quants
    const double last_p_q    = rcc->last_qscale_for[SLICE_TYPE_P];
    const double last_non_b_q= rcc->last_qscale_for[rcc->last_non_b_pict_type];
    if( pict_type == SLICE_TYPE_I )
    {
        double iq = q;
        double pq = qp2qscale( rcc->accum_p_qp / rcc->accum_p_norm );
        double ip_factor = fabs( h->param.rc.f_ip_factor );
        /* don't apply ip_factor if the following frame is also I */
        if( rcc->accum_p_norm <= 0 )
            q = iq;
        else if( h->param.rc.f_ip_factor < 0 )
            q = iq / ip_factor;
        else if( rcc->accum_p_norm >= 1 )
            q = pq / ip_factor;
        else
            q = rcc->accum_p_norm * pq / ip_factor + (1 - rcc->accum_p_norm) * iq;
    }
    else if( pict_type == SLICE_TYPE_B )
    {
        if( h->param.rc.f_pb_factor > 0 )
            q = last_non_b_q;
        q *= fabs( h->param.rc.f_pb_factor );
    }
    else if( pict_type == SLICE_TYPE_P
             && rcc->last_non_b_pict_type == SLICE_TYPE_P
             && rce->i_tex_bits + rce->p_tex_bits == 0 )
    {
        q = last_p_q;
    }

    /* last qscale / qdiff stuff */
    /* TODO take intro account whether the I-frame is a scene cut
     * or just a seek point */
    if(rcc->last_non_b_pict_type==pict_type
       && (pict_type!=SLICE_TYPE_I || rcc->last_accum_p_norm < 1))
    {
        double last_q = rcc->last_qscale_for[pict_type];
        double max_qscale = last_q * rcc->lstep;
        double min_qscale = last_q / rcc->lstep;

        if     (q > max_qscale) q = max_qscale;
        else if(q < min_qscale) q = min_qscale;
    }

    rcc->last_qscale_for[pict_type] = q;
    if(pict_type!=SLICE_TYPE_B)
        rcc->last_non_b_pict_type = pict_type;
    if(pict_type==SLICE_TYPE_I)
    {
        rcc->last_accum_p_norm = rcc->accum_p_norm;
        rcc->accum_p_norm = 0;
        rcc->accum_p_qp = 0;
    }
    if(pict_type==SLICE_TYPE_P)
    {
        float mask = 1 - pow( (float)rce->i_count / rcc->nmb, 2 );
        rcc->accum_p_qp   = mask * (qscale2qp(q) + rcc->accum_p_qp);
        rcc->accum_p_norm = mask * (1 + rcc->accum_p_norm);
    }
    return q;
}

// clip a qscale to between lmin and lmax
static double clip_qscale( x264_t *h, ratecontrol_entry_t *rce, double q )
{
    double lmin = h->rc->lmin[rce->pict_type];
    double lmax = h->rc->lmax[rce->pict_type];

    if(lmin==lmax){
        return lmin;
    }else{
        double min2 = log(lmin);
        double max2 = log(lmax);
        q = (log(q) - min2)/(max2-min2) - 0.5;
        q = 1.0/(1.0 + exp(-4*q));
        q = q*(max2-min2) + min2;
        return exp(q);
    }
}

// update qscale for 1 frame based on actual bits used so far
static float rate_estimate_qscale(x264_t *h, int pict_type)
{
    float q;
    float br_compensation;
    double diff;
    int picture_number = h->fenc->i_frame;
    x264_ratecontrol_t *rcc = h->rc;
    ratecontrol_entry_t *rce;
    double lmin = rcc->lmin[pict_type];
    double lmax = rcc->lmax[pict_type];
    int64_t total_bits = 8*(h->stat.i_slice_size[SLICE_TYPE_I]
                          + h->stat.i_slice_size[SLICE_TYPE_P]
                          + h->stat.i_slice_size[SLICE_TYPE_B]);

//printf("input_pic_num:%d pic_num:%d frame_rate:%d\n", s->input_picture_number, s->picture_number, s->frame_rate);

    rce = &rcc->entry[picture_number];

    assert(pict_type == rce->pict_type);

    if(rce->pict_type == SLICE_TYPE_B)
    {
        return rcc->last_qscale * h->param.rc.f_pb_factor;
    }
    else
    {
        diff = (int64_t)total_bits - (int64_t)rce->expected_bits;
        br_compensation = (rcc->buffer_size - diff) / rcc->buffer_size;
        br_compensation = x264_clip3f(br_compensation, .5, 2);

        q = rce->new_qscale / br_compensation;
        q = x264_clip3f(q, lmin, lmax);
        rcc->last_qscale = q;
        return q;
    }
}

static int init_pass2( x264_t *h )
{
    x264_ratecontrol_t *rcc = h->rc;
    uint64_t all_const_bits = 0;
    uint64_t all_available_bits = (uint64_t)(h->param.rc.i_bitrate * 1000 * (double)rcc->num_entries / rcc->fps);
    double rate_factor, step, step_mult;
    double qblur = h->param.rc.f_qblur;
    double cplxblur = h->param.rc.f_complexity_blur;
    const int filter_size = (int)(qblur*4) | 1;
    double expected_bits;
    double *qscale, *blurred_qscale;
    int i;

    /* find total/average complexity & const_bits */
    for(i=0; i<rcc->num_entries; i++){
        ratecontrol_entry_t *rce = &rcc->entry[i];
        all_const_bits += rce->mv_bits + rce->misc_bits;
        rcc->i_cplx_sum[rce->pict_type] += rce->i_tex_bits * rce->qscale;
        rcc->p_cplx_sum[rce->pict_type] += rce->p_tex_bits * rce->qscale;
        rcc->mv_bits_sum[rce->pict_type] += rce->mv_bits;
        rcc->frame_count[rce->pict_type] ++;
    }

    if( all_available_bits < all_const_bits)
    {
        x264_log(h, X264_LOG_ERROR, "requested bitrate is too low. estimated minimum is %d kbps\n",
                 (int)(all_const_bits * rcc->fps / (rcc->num_entries * 1000)));
        return -1;
    }

    for(i=0; i<rcc->num_entries; i++){
        ratecontrol_entry_t *rce = &rcc->entry[i];
        double weight_sum = 0;
        double cplx_sum = 0;
        double weight = 1.0;
        int j;
        /* weighted average of cplx of future frames */
        for(j=1; j<cplxblur*2 && j<rcc->num_entries-i; j++){
            ratecontrol_entry_t *rcj = &rcc->entry[i+j];
            weight *= 1 - pow( (float)rcj->i_count / rcc->nmb, 2 );
            if(weight < .0001)
                break;
            weight_sum += weight;
            cplx_sum += weight * (rcj->i_tex_bits + rcj->p_tex_bits) * rce->qscale;
        }
        /* weighted average of cplx of past frames */
        weight = 1.0;
        for(j=0; j<cplxblur*2 && j<=i; j++){
            ratecontrol_entry_t *rcj = &rcc->entry[i-j];
            weight_sum += weight;
            cplx_sum += weight * (rcj->i_tex_bits + rcj->p_tex_bits) * rce->qscale;
            weight *= 1 - pow( (float)rcj->i_count / rcc->nmb, 2 );
            if(weight < .0001)
                break;
        }
        rce->blurred_complexity = cplx_sum / weight_sum;
    }

    qscale = x264_malloc(sizeof(double)*rcc->num_entries);
    if(filter_size > 1)
        blurred_qscale = x264_malloc(sizeof(double)*rcc->num_entries);
    else
        blurred_qscale = qscale;

    expected_bits = 1;
    for(i=0; i<rcc->num_entries; i++)
        expected_bits += qscale2bits(&rcc->entry[i], get_qscale(h, &rcc->entry[i], 1.0));
    step_mult = all_available_bits / expected_bits;

    rate_factor = 0;
    for(step = 1E4 * step_mult; step > 1E-7 * step_mult; step *= 0.5){
        expected_bits = 0;
        rate_factor += step;

        rcc->last_non_b_pict_type = -1;
        rcc->last_accum_p_norm = 1;

        /* find qscale */
        for(i=0; i<rcc->num_entries; i++){
            qscale[i] = get_qscale(h, &rcc->entry[i], rate_factor);
        }

        /* fixed I/B QP relative to P mode */
        for(i=rcc->num_entries-1; i>=0; i--){
            qscale[i] = get_diff_limited_q(h, &rcc->entry[i], qscale[i]);
            assert(qscale[i] >= 0);
        }

        /* smooth curve */
        if(filter_size > 1){
            assert(filter_size%2==1);
            for(i=0; i<rcc->num_entries; i++){
                ratecontrol_entry_t *rce = &rcc->entry[i];
                int j;
                double q=0.0, sum=0.0;

                for(j=0; j<filter_size; j++){
                    int index = i+j-filter_size/2;
                    double d = index-i;
                    double coeff = qblur==0 ? 1.0 : exp(-d*d/(qblur*qblur));
                    if(index < 0 || index >= rcc->num_entries) continue;
                    if(rce->pict_type != rcc->entry[index].pict_type) continue;
                    q += qscale[index] * coeff;
                    sum += coeff;
                }
                blurred_qscale[i] = q/sum;
            }
        }

        /* find expected bits */
        for(i=0; i<rcc->num_entries; i++){
            ratecontrol_entry_t *rce = &rcc->entry[i];
            double bits;
            rce->new_qscale = clip_qscale(h, rce, blurred_qscale[i]);
            assert(rce->new_qscale >= 0);
            bits = qscale2bits(rce, rce->new_qscale) + rce->mv_bits + rce->misc_bits;

            rce->expected_bits = expected_bits;
            expected_bits += bits;
        }

//printf("expected:%llu available:%llu factor:%lf avgQ:%lf\n", (uint64_t)expected_bits, all_available_bits, rate_factor);
        if(expected_bits > all_available_bits) rate_factor -= step;
    }

    x264_free(qscale);
    if(filter_size > 1)
        x264_free(blurred_qscale);

    if(fabs(expected_bits/all_available_bits - 1.0) > 0.01)
    {
        double avgq = 0;
        for(i=0; i<rcc->num_entries; i++)
            avgq += rcc->entry[i].new_qscale;
        avgq = qscale2qp(avgq / rcc->num_entries);

        x264_log(h, X264_LOG_ERROR, "Error: 2pass curve failed to converge\n");
        x264_log(h, X264_LOG_ERROR, "expected bits: %llu, available: %llu, avg QP: %.4lf\n", (uint64_t)expected_bits, all_available_bits, avgq);
        if(expected_bits < all_available_bits && avgq < h->param.rc.i_qp_min + 1)
            x264_log(h, X264_LOG_ERROR, "try reducing bitrate or reducing qp_min\n");
        if(expected_bits > all_available_bits && avgq > h->param.rc.i_qp_min - 1)
            x264_log(h, X264_LOG_ERROR, "try increasing bitrate or increasing qp_max\n");
        return -1;
    }

    return 0;
}


