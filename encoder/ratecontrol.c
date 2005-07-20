/***************************************************-*- coding: iso-8859-1 -*-
 * ratecontrol.c: h264 encoder library (Rate Control)
 *****************************************************************************
 * Copyright (C) 2005 x264 project
 * $Id: ratecontrol.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *          Michael Niedermayer <michaelni@gmx.at>
 *          Måns Rullgård <mru@mru.ath.cx>
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

#if defined(SYS_FREEBSD) || defined(SYS_BEOS) || defined(SYS_NETBSD)
#define exp2f(x) powf( 2, (x) )
#endif
#if defined(_MSC_VER) || defined(SYS_SunOS)
#define exp2f(x) pow( 2, (x) )
#define sqrtf sqrt
#endif
#ifdef WIN32 // POSIX says that rename() removes the destination, but win32 doesn't.
#define rename(src,dst) (unlink(dst), rename(src,dst))
#endif

typedef struct
{
    int pict_type;
    int kept_as_ref;
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

typedef struct
{
    double coeff;
    double count;
    double decay;
} predictor_t;

struct x264_ratecontrol_t
{
    /* constants */
    int b_abr;
    int b_2pass;
    double fps;
    double bitrate;
    double rate_tolerance;
    int nmb;                    /* number of macroblocks in a frame */
    int qp_constant[5];

    /* current frame */
    ratecontrol_entry_t *rce;
    int qp;                     /* qp for current frame */
    float qpa;                  /* average of macroblocks' qp (same as qp if no adaptive quant) */
    int slice_type;
    int qp_force;

    /* VBV stuff */
    double buffer_size;
    double buffer_fill;
    double buffer_rate;         /* # of bits added to buffer_fill after each frame */
    predictor_t pred[5];        /* predict frame size from satd */

    /* ABR stuff */
    int    last_satd;
    double last_rceq;
    double cplxr_sum;           /* sum of bits*qscale/rceq */
    double expected_bits_sum;   /* sum of qscale2bits after rceq, ratefactor, and overflow */
    double wanted_bits_window;  /* target bitrate * window */
    double cbr_decay;
    double short_term_cplxsum;
    double short_term_cplxcount;

    /* 2pass stuff */
    FILE *p_stat_file_out;
    char *psz_stat_file_tmpname;

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

    int i_zones;
    x264_zone_t *zones;
};


static int parse_zones( x264_t *h );
static int init_pass2(x264_t *);
static float rate_estimate_qscale( x264_t *h, int pict_type );
static void update_vbv( x264_t *h, int bits );
int  x264_rc_analyse_slice( x264_t *h );

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

/* Texture bitrate is not quite inversely proportional to qscale,
 * probably due the the changing number of SKIP blocks.
 * MV bits level off at about qp<=12, because the lambda used
 * for motion estimation is constant there. */
static inline double qscale2bits(ratecontrol_entry_t *rce, double qscale)
{
    if(qscale<0.1)
        qscale = 0.1;
    return (rce->i_tex_bits + rce->p_tex_bits + .1) * pow( rce->qscale / qscale, 1.1 )
           + rce->mv_bits * pow( X264_MAX(rce->qscale, 12) / X264_MAX(qscale, 12), 0.5 );
}

/* There is no analytical inverse to the above formula. */
#if 0
static inline double bits2qscale(ratecontrol_entry_t *rce, double bits)
{
    if(bits<1.0)
        bits = 1.0;
    return (rce->i_tex_bits + rce->p_tex_bits + rce->mv_bits + .1) * rce->qscale / bits;
}
#endif


int x264_ratecontrol_new( x264_t *h )
{
    x264_ratecontrol_t *rc;
    int i;

    x264_cpu_restore( h->param.cpu );

    h->rc = rc = x264_malloc( sizeof( x264_ratecontrol_t ) );
    memset(rc, 0, sizeof(*rc));

    rc->b_abr = h->param.rc.b_cbr && !h->param.rc.b_stat_read;
    rc->b_2pass = h->param.rc.b_cbr && h->param.rc.b_stat_read;
    h->mb.b_variable_qp = 0;
    
    /* FIXME: use integers */
    if(h->param.i_fps_num > 0 && h->param.i_fps_den > 0)
        rc->fps = (float) h->param.i_fps_num / h->param.i_fps_den;
    else
        rc->fps = 25.0;

    rc->bitrate = h->param.rc.i_bitrate * 1000;
    rc->rate_tolerance = h->param.rc.f_rate_tolerance;
    rc->nmb = h->mb.i_mb_count;
    rc->last_non_b_pict_type = -1;
    rc->cbr_decay = 1.0;

    if( h->param.rc.i_vbv_max_bitrate < h->param.rc.i_bitrate &&
        h->param.rc.i_vbv_max_bitrate > 0)
        x264_log(h, X264_LOG_ERROR, "max bitrate less than average bitrate, ignored.\n");
    else if( h->param.rc.i_vbv_max_bitrate > 0 &&
             h->param.rc.i_vbv_buffer_size > 0 )
    {
        if( h->param.rc.i_vbv_buffer_size < 10 * h->param.rc.i_vbv_max_bitrate / rc->fps ) {
            h->param.rc.i_vbv_buffer_size = 10 * h->param.rc.i_vbv_max_bitrate / rc->fps;
            x264_log( h, X264_LOG_ERROR, "VBV buffer size too small, using %d kbit\n",
                      h->param.rc.i_vbv_buffer_size );
        }
        rc->buffer_rate = h->param.rc.i_vbv_max_bitrate * 1000 / rc->fps;
        rc->buffer_size = h->param.rc.i_vbv_buffer_size * 1000;
        rc->buffer_fill = rc->buffer_size * h->param.rc.f_vbv_buffer_init;
        rc->cbr_decay = 1.0 - rc->buffer_rate / rc->buffer_size
                      * 0.5 * X264_MAX(0, 1.5 - rc->buffer_rate * rc->fps / rc->bitrate);
    }
    else if( h->param.rc.i_vbv_max_bitrate || h->param.rc.i_vbv_buffer_size )
        x264_log(h, X264_LOG_ERROR, "VBV maxrate or buffer size specified, but not both.\n");
    if(rc->rate_tolerance < 0.01) {
        x264_log(h, X264_LOG_ERROR, "bitrate tolerance too small, using .01\n");
        rc->rate_tolerance = 0.01;
    }

    if( rc->b_abr )
    {
        /* FIXME shouldn't need to arbitrarily specify a QP,
         * but this is more robust than BPP measures */
#define ABR_INIT_QP 24
        rc->accum_p_norm = .01;
        rc->accum_p_qp = ABR_INIT_QP * rc->accum_p_norm;
        rc->cplxr_sum = .01;
        rc->wanted_bits_window = .01;
    }

    rc->qp_constant[SLICE_TYPE_P] = h->param.rc.i_qp_constant;
    rc->qp_constant[SLICE_TYPE_I] = x264_clip3( (int)( qscale2qp( qp2qscale( h->param.rc.i_qp_constant ) / fabs( h->param.rc.f_ip_factor )) + 0.5 ), 0, 51 );
    rc->qp_constant[SLICE_TYPE_B] = x264_clip3( (int)( qscale2qp( qp2qscale( h->param.rc.i_qp_constant ) * fabs( h->param.rc.f_pb_factor )) + 0.5 ), 0, 51 );

    rc->lstep = exp2f(h->param.rc.i_qp_step / 6.0);
    rc->last_qscale = qp2qscale(26);
    for( i = 0; i < 5; i++ )
    {
        rc->last_qscale_for[i] = qp2qscale(26);
        rc->lmin[i] = qp2qscale( h->param.rc.i_qp_min );
        rc->lmax[i] = qp2qscale( h->param.rc.i_qp_max );
        rc->pred[i].coeff= 2.0;
        rc->pred[i].count= 1.0;
        rc->pred[i].decay= 0.5;
    }
#if 0 // FIXME: do we want to assign lmin/lmax based on ip_factor, or leave them all the same?
    rc->lmin[SLICE_TYPE_I] /= fabs(h->param.f_ip_factor);
    rc->lmax[SLICE_TYPE_I] /= fabs(h->param.f_ip_factor);
    rc->lmin[SLICE_TYPE_B] *= fabs(h->param.f_pb_factor);
    rc->lmax[SLICE_TYPE_B] *= fabs(h->param.f_pb_factor);
#endif

    if( parse_zones( h ) < 0 )
        return -1;

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
        for(i=-1; p; i++)
            p = strchr(p+1, ';');
        if(i==0)
        {
            x264_log(h, X264_LOG_ERROR, "empty stats file\n");
            return -1;
        }
        i += h->param.i_bframe;
        rc->entry = (ratecontrol_entry_t*) x264_malloc(i*sizeof(ratecontrol_entry_t));
        memset(rc->entry, 0, i*sizeof(ratecontrol_entry_t));
        /* FIXME: num_entries is sometimes treated as number of frames in the video */
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

            if(frame_number < 0 || frame_number >= rc->num_entries)
            {
                x264_log(h, X264_LOG_ERROR, "bad frame number (%d) at stats line %d\n", frame_number, i);
                return -1;
            }
            rce = &rc->entry[frame_number];

            e += sscanf(p, " in:%*d out:%*d type:%c q:%f itex:%d ptex:%d mv:%d misc:%d imb:%d pmb:%d smb:%d",
                   &pict_type, &qp, &rce->i_tex_bits, &rce->p_tex_bits,
                   &rce->mv_bits, &rce->misc_bits, &rce->i_count, &rce->p_count, &rce->s_count);

            switch(pict_type){
                case 'I': rce->kept_as_ref = 1;
                case 'i': rce->pict_type = SLICE_TYPE_I; break;
                case 'P': rce->pict_type = SLICE_TYPE_P; break;
                case 'B': rce->kept_as_ref = 1;
                case 'b': rce->pict_type = SLICE_TYPE_B; break;
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

        if(h->param.rc.b_cbr)
        {
            if(init_pass2(h) < 0) return -1;
        } /* else we're using constant quant, so no need to run the bitrate allocation */
    }

    /* Open output file */
    /* If input and output files are the same, output to a temp file
     * and move it to the real name only when it's complete */
    if( h->param.rc.b_stat_write )
    {
        rc->psz_stat_file_tmpname = x264_malloc( strlen(h->param.rc.psz_stat_out) + 6 );
        strcpy( rc->psz_stat_file_tmpname, h->param.rc.psz_stat_out );
        strcat( rc->psz_stat_file_tmpname, ".temp" );

        rc->p_stat_file_out = fopen( rc->psz_stat_file_tmpname, "wb" );
        if( rc->p_stat_file_out == NULL )
        {
            x264_log(h, X264_LOG_ERROR, "ratecontrol_init: can't open stats file\n");
            return -1;
        }
    }

    return 0;
}

static int parse_zones( x264_t *h )
{
    x264_ratecontrol_t *rc = h->rc;
    int i;
    if( h->param.rc.psz_zones && !h->param.rc.i_zones )
    {
        char *p;
        h->param.rc.i_zones = 1;
        for( p = h->param.rc.psz_zones; *p; p++ )
            h->param.rc.i_zones += (*p == '/');
        h->param.rc.zones = x264_malloc( h->param.rc.i_zones * sizeof(x264_zone_t) );
        p = h->param.rc.psz_zones;
        for( i = 0; i < h->param.rc.i_zones; i++)
        {
            x264_zone_t *z = &h->param.rc.zones[i];
            if( 3 == sscanf(p, "%u,%u,q=%u", &z->i_start, &z->i_end, &z->i_qp) )
                z->b_force_qp = 1;
            else if( 3 == sscanf(p, "%u,%u,b=%f", &z->i_start, &z->i_end, &z->f_bitrate_factor) )
                z->b_force_qp = 0;
            else
            {
                char *slash = strchr(p, '/');
                if(slash) *slash = '\0';
                x264_log( h, X264_LOG_ERROR, "invalid zone: \"%s\"\n", p );
                return -1;
            }
            p = strchr(p, '/') + 1;
        }
    }

    if( h->param.rc.i_zones > 0 )
    {
        for( i = 0; i < h->param.rc.i_zones; i++ )
        {
            x264_zone_t z = h->param.rc.zones[i];
            if( z.i_start < 0 || z.i_start > z.i_end )
            {
                x264_log( h, X264_LOG_ERROR, "invalid zone: start=%d end=%d\n",
                          z.i_start, z.i_end );
                return -1;
            }
            else if( !z.b_force_qp && z.f_bitrate_factor <= 0 )
            {
                x264_log( h, X264_LOG_ERROR, "invalid zone: bitrate_factor=%f\n",
                          z.f_bitrate_factor );
                return -1;
            }
        }

        rc->i_zones = h->param.rc.i_zones;
        rc->zones = x264_malloc( rc->i_zones * sizeof(x264_zone_t) );
        memcpy( rc->zones, h->param.rc.zones, rc->i_zones * sizeof(x264_zone_t) );
    }

    return 0;
}

void x264_ratecontrol_delete( x264_t *h )
{
    x264_ratecontrol_t *rc = h->rc;

    if( rc->p_stat_file_out )
    {
        fclose( rc->p_stat_file_out );
        if( h->i_frame >= rc->num_entries - h->param.i_bframe )
            if( rename( rc->psz_stat_file_tmpname, h->param.rc.psz_stat_out ) != 0 )
            {
                x264_log( h, X264_LOG_ERROR, "failed to rename \"%s\" to \"%s\"\n",
                          rc->psz_stat_file_tmpname, h->param.rc.psz_stat_out );
            }
        x264_free( rc->psz_stat_file_tmpname );
    }
    x264_free( rc->entry );
    x264_free( rc->zones );
    x264_free( rc );
}

/* Before encoding a frame, choose a QP for it */
void x264_ratecontrol_start( x264_t *h, int i_slice_type, int i_force_qp )
{
    x264_ratecontrol_t *rc = h->rc;

    x264_cpu_restore( h->param.cpu );

    rc->qp_force = i_force_qp;
    rc->slice_type = i_slice_type;

    if( i_force_qp )
    {
        rc->qpa = rc->qp = i_force_qp - 1;
    }
    else if( rc->b_abr )
    {
        rc->qpa = rc->qp =
            x264_clip3( (int)(qscale2qp( rate_estimate_qscale( h, i_slice_type ) ) + .5), 0, 51 );
    }
    else if( rc->b_2pass )
    {
        int frame = h->fenc->i_frame;
        ratecontrol_entry_t *rce;
        assert( frame >= 0 && frame < rc->num_entries );
        rce = h->rc->rce = &h->rc->entry[frame];

        rce->new_qscale = rate_estimate_qscale( h, i_slice_type );
        rc->qpa = rc->qp = rce->new_qp =
            x264_clip3( (int)(qscale2qp(rce->new_qscale) + 0.5), 0, 51 );
    }
    else /* CQP */
    {
        int q;
        if( i_slice_type == SLICE_TYPE_B && h->fdec->b_kept_as_ref )
            q = ( rc->qp_constant[ SLICE_TYPE_B ] + rc->qp_constant[ SLICE_TYPE_P ] ) / 2;
        else
            q = rc->qp_constant[ i_slice_type ];
        rc->qpa = rc->qp = q;
    }
}

void x264_ratecontrol_mb( x264_t *h, int bits )
{
    /* currently no adaptive quant */
}

int x264_ratecontrol_qp( x264_t *h )
{
    return h->rc->qp;
}

/* In 2pass, force the same frame types as in the 1st pass */
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
                return h->rc->entry[frame_num].kept_as_ref ? X264_TYPE_IDR : X264_TYPE_I;

            case SLICE_TYPE_B:
                return h->rc->entry[frame_num].kept_as_ref ? X264_TYPE_BREF : X264_TYPE_B;

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

/* After encoding one frame, save stats and update ratecontrol state */
void x264_ratecontrol_end( x264_t *h, int bits )
{
    x264_ratecontrol_t *rc = h->rc;
    const int *mbs = h->stat.frame.i_mb_count;
    int i;

    x264_cpu_restore( h->param.cpu );

    h->stat.frame.i_mb_count_skip = mbs[P_SKIP] + mbs[B_SKIP];
    h->stat.frame.i_mb_count_i = mbs[I_16x16] + mbs[I_8x8] + mbs[I_4x4];
    h->stat.frame.i_mb_count_p = mbs[P_L0] + mbs[P_8x8];
    for( i = B_DIRECT; i < B_8x8; i++ )
        h->stat.frame.i_mb_count_p += mbs[i];

    if( h->param.rc.b_stat_write )
    {
        char c_type = rc->slice_type==SLICE_TYPE_I ? (h->fenc->i_poc==0 ? 'I' : 'i')
                    : rc->slice_type==SLICE_TYPE_P ? 'P'
                    : h->fenc->b_kept_as_ref ? 'B' : 'b';
        fprintf( rc->p_stat_file_out,
                 "in:%d out:%d type:%c q:%.2f itex:%d ptex:%d mv:%d misc:%d imb:%d pmb:%d smb:%d;\n",
                 h->fenc->i_frame, h->i_frame-1,
                 c_type, rc->qpa,
                 h->stat.frame.i_itex_bits, h->stat.frame.i_ptex_bits,
                 h->stat.frame.i_hdr_bits, h->stat.frame.i_misc_bits,
                 h->stat.frame.i_mb_count_i,
                 h->stat.frame.i_mb_count_p,
                 h->stat.frame.i_mb_count_skip);
    }

    if( rc->b_abr )
    {
        if( rc->slice_type != SLICE_TYPE_B )
            rc->cplxr_sum += bits * qp2qscale(rc->qpa) / rc->last_rceq;
        else
        {
            /* Depends on the fact that B-frame's QP is an offset from the following P-frame's.
             * Not perfectly accurate with B-refs, but good enough. */
            rc->cplxr_sum += bits * qp2qscale(rc->qpa) / (rc->last_rceq * fabs(h->param.rc.f_pb_factor));
        }
        rc->cplxr_sum *= rc->cbr_decay;
        rc->wanted_bits_window += rc->bitrate / rc->fps;
        rc->wanted_bits_window *= rc->cbr_decay;

        rc->accum_p_qp   *= .95;
        rc->accum_p_norm *= .95;
        rc->accum_p_norm += 1;
        if( rc->slice_type == SLICE_TYPE_I )
            rc->accum_p_qp += rc->qpa * fabs(h->param.rc.f_ip_factor);
        else
            rc->accum_p_qp += rc->qpa;
    }

    if( rc->b_2pass )
    {
        rc->expected_bits_sum += qscale2bits( rc->rce, qp2qscale(rc->rce->new_qp) );
    }

    update_vbv( h, bits );

    if( rc->slice_type != SLICE_TYPE_B )
        rc->last_non_b_pict_type = rc->slice_type;
}

/****************************************************************************
 * 2 pass functions
 ***************************************************************************/

double x264_eval( char *s, double *const_value, const char **const_name,
                  double (**func1)(void *, double), const char **func1_name,
                  double (**func2)(void *, double, double), char **func2_name,
                  void *opaque );

/**
 * modify the bitrate curve from pass1 for one frame
 */
static double get_qscale(x264_t *h, ratecontrol_entry_t *rce, double rate_factor, int frame_num)
{
    x264_ratecontrol_t *rcc= h->rc;
    const int pict_type = rce->pict_type;
    double q;
    int i;

    double const_values[]={
        rce->i_tex_bits * rce->qscale,
        rce->p_tex_bits * rce->qscale,
        (rce->i_tex_bits + rce->p_tex_bits) * rce->qscale,
        rce->mv_bits * rce->qscale,
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
        "blurCplx",
        NULL
    };
    static double (*func1[])(void *, double)={
//      (void *)bits2qscale,
        (void *)qscale2bits,
        NULL
    };
    static const char *func1_names[]={
//      "bits2qp",
        "qp2bits",
        NULL
    };

    q = x264_eval((char*)h->param.rc.psz_rc_eq, const_values, const_names, func1, func1_names, NULL, NULL, rce);

    // avoid NaN's in the rc_eq
    if(q != q || rce->i_tex_bits + rce->p_tex_bits + rce->mv_bits == 0)
        q = rcc->last_qscale;
    else {
        rcc->last_rceq = q;
        q /= rate_factor;
        rcc->last_qscale = q;
    }

    for( i = rcc->i_zones-1; i >= 0; i-- )
    {
        x264_zone_t *z = &rcc->zones[i];
        if( frame_num >= z->i_start && frame_num <= z->i_end )
        {
            if( z->b_force_qp )
                q = qp2qscale(z->i_qp);
            else
                q /= z->f_bitrate_factor;
            break;
        }
    }

    return q;
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
        if( !rce->kept_as_ref )
            q *= fabs( h->param.rc.f_pb_factor );
    }
    else if( pict_type == SLICE_TYPE_P
             && rcc->last_non_b_pict_type == SLICE_TYPE_P
             && rce->i_tex_bits + rce->p_tex_bits == 0 )
    {
        q = last_p_q;
    }

    /* last qscale / qdiff stuff */
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

static double predict_size( predictor_t *p, double q, double var )
{
     return p->coeff*var / (q*p->count);
}

static void update_predictor( predictor_t *p, double q, double var, double bits )
{
    p->count *= p->decay;
    p->coeff *= p->decay;
    p->count ++;
    p->coeff += bits*q / var;
}

static void update_vbv( x264_t *h, int bits )
{
    x264_ratecontrol_t *rcc = h->rc;
    if( !rcc->buffer_size )
        return;

    rcc->buffer_fill += rcc->buffer_rate - bits;
    if( rcc->buffer_fill < 0 && !rcc->b_2pass )
        x264_log( h, X264_LOG_WARNING, "VBV underflow (%.0f bits)\n", rcc->buffer_fill );
    rcc->buffer_fill = x264_clip3( rcc->buffer_fill, 0, rcc->buffer_size );

    if(rcc->last_satd > 100)
        update_predictor( &rcc->pred[rcc->slice_type], qp2qscale(rcc->qpa), rcc->last_satd, bits );
}

// apply VBV constraints and clip qscale to between lmin and lmax
static double clip_qscale( x264_t *h, int pict_type, double q )
{
    x264_ratecontrol_t *rcc = h->rc;
    double lmin = rcc->lmin[pict_type];
    double lmax = rcc->lmax[pict_type];
    double q0 = q;

    /* B-frames are not directly subject to VBV,
     * since they are controlled by the P-frames' QPs.
     * FIXME: in 2pass we could modify previous frames' QP too,
     *        instead of waiting for the buffer to fill */
    if( rcc->buffer_size &&
        ( pict_type == SLICE_TYPE_P ||
          ( pict_type == SLICE_TYPE_I && rcc->last_non_b_pict_type == SLICE_TYPE_I ) ) )
    {
        if( rcc->buffer_fill/rcc->buffer_size < 0.5 )
            q /= x264_clip3f( 2.0*rcc->buffer_fill/rcc->buffer_size, 0.5, 1.0 );
    }
    /* Now a hard threshold to make sure the frame fits in VBV.
     * This one is mostly for I-frames. */
    if( rcc->buffer_size && rcc->last_satd > 0 )
    {
        double bits = predict_size( &rcc->pred[rcc->slice_type], q, rcc->last_satd );
        double qf = 1.0;
        if( bits > rcc->buffer_fill/2 )
            qf = x264_clip3f( rcc->buffer_fill/(2*bits), 0.2, 1.0 );
        q /= qf;
        bits *= qf;
        if( bits < rcc->buffer_rate/2 )
            q *= bits*2/rcc->buffer_rate;
        q = X264_MAX( q0, q );
    }

    if(lmin==lmax)
        return lmin;
    else if(rcc->b_2pass)
    {
        double min2 = log(lmin);
        double max2 = log(lmax);
        q = (log(q) - min2)/(max2-min2) - 0.5;
        q = 1.0/(1.0 + exp(-4*q));
        q = q*(max2-min2) + min2;
        return exp(q);
    }
    else
        return x264_clip3f(q, lmin, lmax);
}

// update qscale for 1 frame based on actual bits used so far
static float rate_estimate_qscale(x264_t *h, int pict_type)
{
    float q;
    x264_ratecontrol_t *rcc = h->rc;
    ratecontrol_entry_t rce;
    double lmin = rcc->lmin[pict_type];
    double lmax = rcc->lmax[pict_type];
    int64_t total_bits = 8*(h->stat.i_slice_size[SLICE_TYPE_I]
                          + h->stat.i_slice_size[SLICE_TYPE_P]
                          + h->stat.i_slice_size[SLICE_TYPE_B]);

    if( rcc->b_2pass )
    {
        rce = *rcc->rce;
        if(pict_type != rce.pict_type)
        {
            x264_log(h, X264_LOG_ERROR, "slice=%c but 2pass stats say %c\n",
                     slice_type_to_char[pict_type], slice_type_to_char[rce.pict_type]);
        }
    }

    if( pict_type == SLICE_TYPE_B )
    {
        rcc->last_satd = 0;
        if(h->fenc->b_kept_as_ref)
            q = rcc->last_qscale * sqrtf(h->param.rc.f_pb_factor);
        else
            q = rcc->last_qscale * h->param.rc.f_pb_factor;
        return x264_clip3f(q, lmin, lmax);
    }
    else
    {
        double abr_buffer = 2 * rcc->rate_tolerance * rcc->bitrate;
        if( rcc->b_2pass )
        {
            //FIXME adjust abr_buffer based on distance to the end of the video
            int64_t diff = total_bits - (int64_t)rce.expected_bits;
            q = rce.new_qscale;
            q /= x264_clip3f((double)(abr_buffer - diff) / abr_buffer, .5, 2);
            if( h->fenc->i_frame > 30 )
            {
                /* Adjust quant based on the difference between
                 * achieved and expected bitrate so far */
                double time = (double)h->fenc->i_frame / rcc->num_entries;
                double w = x264_clip3f( time*100, 0.0, 1.0 );
                q *= pow( (double)total_bits / rcc->expected_bits_sum, w );
            }
            q = x264_clip3f( q, lmin, lmax );
        }
        else /* 1pass ABR */
        {
            /* Calculate the quantizer which would have produced the desired
             * average bitrate if it had been applied to all frames so far.
             * Then modulate that quant based on the current frame's complexity
             * relative to the average complexity so far (using the 2pass RCEQ).
             * Then bias the quant up or down if total size so far was far from
             * the target.
             * Result: Depending on the value of rate_tolerance, there is a
             * tradeoff between quality and bitrate precision. But at large
             * tolerances, the bit distribution approaches that of 2pass. */

            double wanted_bits, overflow, lmin, lmax;

            rcc->last_satd = x264_rc_analyse_slice( h );
            rcc->short_term_cplxsum *= 0.5;
            rcc->short_term_cplxcount *= 0.5;
            rcc->short_term_cplxsum += rcc->last_satd;
            rcc->short_term_cplxcount ++;

            rce.p_tex_bits = rcc->last_satd;
            rce.blurred_complexity = rcc->short_term_cplxsum / rcc->short_term_cplxcount;
            rce.i_tex_bits = 0;
            rce.mv_bits = 0;
            rce.p_count = rcc->nmb;
            rce.i_count = 0;
            rce.s_count = 0;
            rce.qscale = 1;
            rce.pict_type = pict_type;
            q = get_qscale(h, &rce, rcc->wanted_bits_window / rcc->cplxr_sum, h->fenc->i_frame);

            wanted_bits = h->fenc->i_frame * rcc->bitrate / rcc->fps;
            abr_buffer *= X264_MAX( 1, sqrt(h->fenc->i_frame/25) );
            overflow = x264_clip3f( 1.0 + (total_bits - wanted_bits) / abr_buffer, .5, 2 );
            q *= overflow;

            if( pict_type == SLICE_TYPE_I
                /* should test _next_ pict type, but that isn't decided yet */
                && rcc->last_non_b_pict_type != SLICE_TYPE_I )
            {
                q = qp2qscale( rcc->accum_p_qp / rcc->accum_p_norm );
                q /= fabs( h->param.rc.f_ip_factor );
                q = clip_qscale( h, pict_type, q );
            }
            else
            {
                if( h->stat.i_slice_count[SLICE_TYPE_P] < 5 )
                {
                    float w = h->stat.i_slice_count[SLICE_TYPE_P] / 5.;
                    float q2 = qp2qscale(ABR_INIT_QP);
                    q = q*w + q2*(1-w);
                }

                /* Asymmetric clipping, because symmetric would prevent
                 * overflow control in areas of rapidly oscillating complexity */
                lmin = rcc->last_qscale_for[pict_type] / rcc->lstep;
                lmax = rcc->last_qscale_for[pict_type] * rcc->lstep;
                if( overflow > 1.1 )
                    lmax *= rcc->lstep;
                else if( overflow < 0.9 )
                    lmin /= rcc->lstep;

                q = x264_clip3f(q, lmin, lmax);
                q = clip_qscale(h, pict_type, q);
                //FIXME use get_diff_limited_q() ?
            }
        }

        rcc->last_qscale_for[pict_type] =
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
        all_const_bits += rce->misc_bits;
        rcc->i_cplx_sum[rce->pict_type] += rce->i_tex_bits * rce->qscale;
        rcc->p_cplx_sum[rce->pict_type] += rce->p_tex_bits * rce->qscale;
        rcc->mv_bits_sum[rce->pict_type] += rce->mv_bits * rce->qscale;
        rcc->frame_count[rce->pict_type] ++;
    }

    if( all_available_bits < all_const_bits)
    {
        x264_log(h, X264_LOG_ERROR, "requested bitrate is too low. estimated minimum is %d kbps\n",
                 (int)(all_const_bits * rcc->fps / (rcc->num_entries * 1000)));
        return -1;
    }

    /* Blur complexities, to reduce local fluctuation of QP.
     * We don't blur the QPs directly, because then one very simple frame
     * could drag down the QP of a nearby complex frame and give it more
     * bits than intended. */
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
            cplx_sum += weight * qscale2bits(rcj, 1);
        }
        /* weighted average of cplx of past frames */
        weight = 1.0;
        for(j=0; j<=cplxblur*2 && j<=i; j++){
            ratecontrol_entry_t *rcj = &rcc->entry[i-j];
            weight_sum += weight;
            cplx_sum += weight * qscale2bits(rcj, 1);
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

    /* Search for a factor which, when multiplied by the RCEQ values from
     * each frame, adds up to the desired total size.
     * There is no exact closed-form solution because of VBV constraints and
     * because qscale2bits is not invertible, but we can start with the simple
     * approximation of scaling the 1st pass by the ratio of bitrates.
     * The search range is probably overkill, but speed doesn't matter here. */

    expected_bits = 1;
    for(i=0; i<rcc->num_entries; i++)
        expected_bits += qscale2bits(&rcc->entry[i], get_qscale(h, &rcc->entry[i], 1.0, i));
    step_mult = all_available_bits / expected_bits;

    rate_factor = 0;
    for(step = 1E4 * step_mult; step > 1E-7 * step_mult; step *= 0.5){
        expected_bits = 0;
        rate_factor += step;

        rcc->last_non_b_pict_type = -1;
        rcc->last_accum_p_norm = 1;
        rcc->accum_p_norm = 0;
        rcc->buffer_fill = rcc->buffer_size * h->param.rc.f_vbv_buffer_init;

        /* find qscale */
        for(i=0; i<rcc->num_entries; i++){
            qscale[i] = get_qscale(h, &rcc->entry[i], rate_factor, i);
        }

        /* fixed I/B qscale relative to P */
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
            rce->new_qscale = clip_qscale(h, rce->pict_type, blurred_qscale[i]);
            assert(rce->new_qscale >= 0);
            bits = qscale2bits(rce, rce->new_qscale) + rce->misc_bits;

            rce->expected_bits = expected_bits;
            expected_bits += bits;
            update_vbv(h, bits);
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
        x264_log(h, X264_LOG_ERROR, "target: %.2f kbit/s, expected: %.2f kbit/s, avg QP: %.4f\n",
                 (float)h->param.rc.i_bitrate,
                 expected_bits * rcc->fps / (rcc->num_entries * 1000.),
                 avgq);
        if(expected_bits < all_available_bits && avgq < h->param.rc.i_qp_min + 2)
        {
            if(h->param.rc.i_qp_min > 0)
                x264_log(h, X264_LOG_ERROR, "try reducing target bitrate or reducing qp_min (currently %d)\n", h->param.rc.i_qp_min);
            else
                x264_log(h, X264_LOG_ERROR, "try reducing target bitrate\n");
        }
        else if(expected_bits > all_available_bits && avgq > h->param.rc.i_qp_max - 2)
        {
            if(h->param.rc.i_qp_max < 51)
                x264_log(h, X264_LOG_ERROR, "try increasing target bitrate or increasing qp_max (currently %d)\n", h->param.rc.i_qp_max);
            else
                x264_log(h, X264_LOG_ERROR, "try increasing target bitrate\n");
        }
        else
            x264_log(h, X264_LOG_ERROR, "internal error\n");
    }

    return 0;
}


