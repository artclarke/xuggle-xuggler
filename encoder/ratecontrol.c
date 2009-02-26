/***************************************************-*- coding: iso-8859-1 -*-
 * ratecontrol.c: h264 encoder library (Rate Control)
 *****************************************************************************
 * Copyright (C) 2005-2008 x264 project
 *
 * Authors: Loren Merritt <lorenm@u.washington.edu>
 *          Michael Niedermayer <michaelni@gmx.at>
 *          Gabriel Bouvigne <gabriel.bouvigne@joost.com>
 *          Jason Garrett-Glaser <darkshikari@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *****************************************************************************/

#define _ISOC99_SOURCE
#undef NDEBUG // always check asserts, the speed effect is far too small to disable them
#include <math.h>
#include <limits.h>
#include <assert.h>

#include "common/common.h"
#include "common/cpu.h"
#include "ratecontrol.h"

typedef struct
{
    int pict_type;
    int kept_as_ref;
    float qscale;
    int mv_bits;
    int tex_bits;
    int misc_bits;
    uint64_t expected_bits; /*total expected bits up to the current frame (current one excluded)*/
    double expected_vbv;
    float new_qscale;
    int new_qp;
    int i_count;
    int p_count;
    int s_count;
    float blurred_complexity;
    char direct_mode;
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
    int b_vbv;
    int b_vbv_min_rate;
    double fps;
    double bitrate;
    double rate_tolerance;
    int nmb;                    /* number of macroblocks in a frame */
    int qp_constant[5];

    /* current frame */
    ratecontrol_entry_t *rce;
    int qp;                     /* qp for current frame */
    int qpm;                    /* qp for current macroblock */
    float f_qpm;                /* qp for current macroblock: precise float for AQ */
    float qpa_rc;               /* average of macroblocks' qp before aq */
    float qpa_aq;               /* average of macroblocks' qp after aq */
    int qp_force;

    /* VBV stuff */
    double buffer_size;
    double buffer_fill_final;   /* real buffer as of the last finished frame */
    double buffer_fill;         /* planned buffer, if all in-progress frames hit their bit budget */
    double buffer_rate;         /* # of bits added to buffer_fill after each frame */
    predictor_t *pred;          /* predict frame size from satd */

    /* ABR stuff */
    int    last_satd;
    double last_rceq;
    double cplxr_sum;           /* sum of bits*qscale/rceq */
    double expected_bits_sum;   /* sum of qscale2bits after rceq, ratefactor, and overflow, only includes finished frames */
    double wanted_bits_window;  /* target bitrate * window */
    double cbr_decay;
    double short_term_cplxsum;
    double short_term_cplxcount;
    double rate_factor_constant;
    double ip_offset;
    double pb_offset;

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

    /* MBRC stuff */
    double frame_size_estimated;
    double frame_size_planned;
    predictor_t *row_pred;
    predictor_t row_preds[5];
    predictor_t *pred_b_from_p; /* predict B-frame size from P-frame satd */
    int bframes;                /* # consecutive B-frames before this P-frame */
    int bframe_bits;            /* total cost of those frames */

    int i_zones;
    x264_zone_t *zones;
    x264_zone_t *prev_zone;
};


static int parse_zones( x264_t *h );
static int init_pass2(x264_t *);
static float rate_estimate_qscale( x264_t *h );
static void update_vbv( x264_t *h, int bits );
static void update_vbv_plan( x264_t *h );
static double predict_size( predictor_t *p, double q, double var );
static void update_predictor( predictor_t *p, double q, double var, double bits );

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
    return (rce->tex_bits + .1) * pow( rce->qscale / qscale, 1.1 )
           + rce->mv_bits * pow( X264_MAX(rce->qscale, 1) / X264_MAX(qscale, 1), 0.5 )
           + rce->misc_bits;
}

// Find the total AC energy of the block in all planes.
static NOINLINE int ac_energy_mb( x264_t *h, int mb_x, int mb_y, x264_frame_t *frame )
{
    /* This function contains annoying hacks because GCC has a habit of reordering emms
     * and putting it after floating point ops.  As a result, we put the emms at the end of the
     * function and make sure that its always called before the float math.  Noinline makes
     * sure no reordering goes on. */
    unsigned int var = 0, i;
    for( i = 0; i < 3; i++ )
    {
        int w = i ? 8 : 16;
        int stride = frame->i_stride[i];
        int offset = h->mb.b_interlaced
            ? w * (mb_x + (mb_y&~1) * stride) + (mb_y&1) * stride
            : w * (mb_x + mb_y * stride);
        int pix = i ? PIXEL_8x8 : PIXEL_16x16;
        stride <<= h->mb.b_interlaced;
        var += h->pixf.var[pix]( frame->plane[i]+offset, stride );
    }
    var = X264_MAX(var,1);
    x264_emms();
    return var;
}

static const float log2_lut[128] = {
    0.00000, 0.01123, 0.02237, 0.03342, 0.04439, 0.05528, 0.06609, 0.07682,
    0.08746, 0.09803, 0.10852, 0.11894, 0.12928, 0.13955, 0.14975, 0.15987,
    0.16993, 0.17991, 0.18982, 0.19967, 0.20945, 0.21917, 0.22882, 0.23840,
    0.24793, 0.25739, 0.26679, 0.27612, 0.28540, 0.29462, 0.30378, 0.31288,
    0.32193, 0.33092, 0.33985, 0.34873, 0.35755, 0.36632, 0.37504, 0.38370,
    0.39232, 0.40088, 0.40939, 0.41785, 0.42626, 0.43463, 0.44294, 0.45121,
    0.45943, 0.46761, 0.47573, 0.48382, 0.49185, 0.49985, 0.50779, 0.51570,
    0.52356, 0.53138, 0.53916, 0.54689, 0.55459, 0.56224, 0.56986, 0.57743,
    0.58496, 0.59246, 0.59991, 0.60733, 0.61471, 0.62205, 0.62936, 0.63662,
    0.64386, 0.65105, 0.65821, 0.66534, 0.67243, 0.67948, 0.68650, 0.69349,
    0.70044, 0.70736, 0.71425, 0.72110, 0.72792, 0.73471, 0.74147, 0.74819,
    0.75489, 0.76155, 0.76818, 0.77479, 0.78136, 0.78790, 0.79442, 0.80090,
    0.80735, 0.81378, 0.82018, 0.82655, 0.83289, 0.83920, 0.84549, 0.85175,
    0.85798, 0.86419, 0.87036, 0.87652, 0.88264, 0.88874, 0.89482, 0.90087,
    0.90689, 0.91289, 0.91886, 0.92481, 0.93074, 0.93664, 0.94251, 0.94837,
    0.95420, 0.96000, 0.96578, 0.97154, 0.97728, 0.98299, 0.98868, 0.99435,
};

static const uint8_t exp2_lut[64] = {
      1,   4,   7,  10,  13,  16,  19,  22,  25,  28,  31,  34,  37,  40,  44,  47,
     50,  53,  57,  60,  64,  67,  71,  74,  78,  81,  85,  89,  93,  96, 100, 104,
    108, 112, 116, 120, 124, 128, 132, 137, 141, 145, 150, 154, 159, 163, 168, 172,
    177, 182, 186, 191, 196, 201, 206, 211, 216, 221, 226, 232, 237, 242, 248, 253,
};

static int x264_exp2fix8( float x )
{
    int i, f;
    x += 8;
    if( x <= 0 ) return 0;
    if( x >= 16 ) return 0xffff;
    i = x;
    f = (x-i)*64;
    return (exp2_lut[f]+256) << i >> 8;
}

void x264_adaptive_quant_frame( x264_t *h, x264_frame_t *frame )
{
    /* constants chosen to result in approximately the same overall bitrate as without AQ.
     * FIXME: while they're written in 5 significant digits, they're only tuned to 2. */
    float strength = h->param.rc.f_aq_strength * 1.0397;
    int mb_x, mb_y;
    for( mb_y = 0; mb_y < h->sps->i_mb_height; mb_y++ )
        for( mb_x = 0; mb_x < h->sps->i_mb_width; mb_x++ )
        {
            uint32_t energy = ac_energy_mb( h, mb_x, mb_y, frame );
            int lz = x264_clz( energy );
            float qp_adj = strength * (log2_lut[(energy<<lz>>24)&0x7f] - lz + 16.573f);
            frame->f_qp_offset[mb_x + mb_y*h->mb.i_mb_stride] = qp_adj;
            if( h->frames.b_have_lowres )
                frame->i_inv_qscale_factor[mb_x + mb_y*h->mb.i_mb_stride] = x264_exp2fix8(qp_adj*(-1.f/6.f));
        }
}


/*****************************************************************************
* x264_adaptive_quant:
 * adjust macroblock QP based on variance (AC energy) of the MB.
 * high variance  = higher QP
 * low variance = lower QP
 * This generally increases SSIM and lowers PSNR.
*****************************************************************************/
void x264_adaptive_quant( x264_t *h )
{
    x264_emms();
    h->mb.i_qp = x264_clip3( h->rc->f_qpm + h->fenc->f_qp_offset[h->mb.i_mb_xy] + .5, h->param.rc.i_qp_min, h->param.rc.i_qp_max );
    /* If the QP of this MB is within 1 of the previous MB, code the same QP as the previous MB,
     * to lower the bit cost of the qp_delta. */
    if( abs(h->mb.i_qp - h->mb.i_last_qp) == 1 )
        h->mb.i_qp = h->mb.i_last_qp;
}

int x264_ratecontrol_new( x264_t *h )
{
    x264_ratecontrol_t *rc;
    int i;

    x264_emms();

    rc = h->rc = x264_malloc( h->param.i_threads * sizeof(x264_ratecontrol_t) );
    memset( rc, 0, h->param.i_threads * sizeof(x264_ratecontrol_t) );

    rc->b_abr = h->param.rc.i_rc_method != X264_RC_CQP && !h->param.rc.b_stat_read;
    rc->b_2pass = h->param.rc.i_rc_method == X264_RC_ABR && h->param.rc.b_stat_read;

    /* FIXME: use integers */
    if(h->param.i_fps_num > 0 && h->param.i_fps_den > 0)
        rc->fps = (float) h->param.i_fps_num / h->param.i_fps_den;
    else
        rc->fps = 25.0;

    rc->bitrate = h->param.rc.i_bitrate * 1000.;
    rc->rate_tolerance = h->param.rc.f_rate_tolerance;
    rc->nmb = h->mb.i_mb_count;
    rc->last_non_b_pict_type = -1;
    rc->cbr_decay = 1.0;

    if( h->param.rc.i_rc_method == X264_RC_CRF && h->param.rc.b_stat_read )
    {
        x264_log(h, X264_LOG_ERROR, "constant rate-factor is incompatible with 2pass.\n");
        return -1;
    }
    if( h->param.rc.i_vbv_buffer_size )
    {
        if( h->param.rc.i_rc_method == X264_RC_CQP )
        {
            x264_log(h, X264_LOG_WARNING, "VBV is incompatible with constant QP, ignored.\n");
            h->param.rc.i_vbv_max_bitrate = 0;
            h->param.rc.i_vbv_buffer_size = 0;
        }
        else if( h->param.rc.i_vbv_max_bitrate == 0 )
        {
            x264_log( h, X264_LOG_DEBUG, "VBV maxrate unspecified, assuming CBR\n" );
            h->param.rc.i_vbv_max_bitrate = h->param.rc.i_bitrate;
        }
    }
    if( h->param.rc.i_vbv_max_bitrate < h->param.rc.i_bitrate &&
        h->param.rc.i_vbv_max_bitrate > 0)
        x264_log(h, X264_LOG_WARNING, "max bitrate less than average bitrate, ignored.\n");
    else if( h->param.rc.i_vbv_max_bitrate > 0 &&
             h->param.rc.i_vbv_buffer_size > 0 )
    {
        if( h->param.rc.i_vbv_buffer_size < 3 * h->param.rc.i_vbv_max_bitrate / rc->fps )
        {
            h->param.rc.i_vbv_buffer_size = 3 * h->param.rc.i_vbv_max_bitrate / rc->fps;
            x264_log( h, X264_LOG_WARNING, "VBV buffer size too small, using %d kbit\n",
                      h->param.rc.i_vbv_buffer_size );
        }
        if( h->param.rc.f_vbv_buffer_init > 1. )
            h->param.rc.f_vbv_buffer_init = x264_clip3f( h->param.rc.f_vbv_buffer_init / h->param.rc.i_vbv_buffer_size, 0, 1 );
        rc->buffer_rate = h->param.rc.i_vbv_max_bitrate * 1000. / rc->fps;
        rc->buffer_size = h->param.rc.i_vbv_buffer_size * 1000.;
        rc->buffer_fill_final = rc->buffer_size * h->param.rc.f_vbv_buffer_init;
        rc->cbr_decay = 1.0 - rc->buffer_rate / rc->buffer_size
                      * 0.5 * X264_MAX(0, 1.5 - rc->buffer_rate * rc->fps / rc->bitrate);
        rc->b_vbv = 1;
        rc->b_vbv_min_rate = !rc->b_2pass
                          && h->param.rc.i_rc_method == X264_RC_ABR
                          && h->param.rc.i_vbv_max_bitrate <= h->param.rc.i_bitrate;
    }
    else if( h->param.rc.i_vbv_max_bitrate )
    {
        x264_log(h, X264_LOG_WARNING, "VBV maxrate specified, but no bufsize.\n");
        h->param.rc.i_vbv_max_bitrate = 0;
    }
    if(rc->rate_tolerance < 0.01)
    {
        x264_log(h, X264_LOG_WARNING, "bitrate tolerance too small, using .01\n");
        rc->rate_tolerance = 0.01;
    }

    h->mb.b_variable_qp = rc->b_vbv || h->param.rc.i_aq_mode;

    if( rc->b_abr )
    {
        /* FIXME ABR_INIT_QP is actually used only in CRF */
#define ABR_INIT_QP ( h->param.rc.i_rc_method == X264_RC_CRF ? h->param.rc.f_rf_constant : 24 )
        rc->accum_p_norm = .01;
        rc->accum_p_qp = ABR_INIT_QP * rc->accum_p_norm;
        /* estimated ratio that produces a reasonable QP for the first I-frame */
        rc->cplxr_sum = .01 * pow( 7.0e5, h->param.rc.f_qcompress ) * pow( h->mb.i_mb_count, 0.5 );
        rc->wanted_bits_window = 1.0 * rc->bitrate / rc->fps;
        rc->last_non_b_pict_type = SLICE_TYPE_I;
    }

    if( h->param.rc.i_rc_method == X264_RC_CRF )
    {
        /* arbitrary rescaling to make CRF somewhat similar to QP */
        double base_cplx = h->mb.i_mb_count * (h->param.i_bframe ? 120 : 80);
        rc->rate_factor_constant = pow( base_cplx, 1 - h->param.rc.f_qcompress )
                                 / qp2qscale( h->param.rc.f_rf_constant );
    }

    rc->ip_offset = 6.0 * log(h->param.rc.f_ip_factor) / log(2.0);
    rc->pb_offset = 6.0 * log(h->param.rc.f_pb_factor) / log(2.0);
    rc->qp_constant[SLICE_TYPE_P] = h->param.rc.i_qp_constant;
    rc->qp_constant[SLICE_TYPE_I] = x264_clip3( h->param.rc.i_qp_constant - rc->ip_offset + 0.5, 0, 51 );
    rc->qp_constant[SLICE_TYPE_B] = x264_clip3( h->param.rc.i_qp_constant + rc->pb_offset + 0.5, 0, 51 );

    rc->lstep = pow( 2, h->param.rc.i_qp_step / 6.0 );
    rc->last_qscale = qp2qscale(26);
    rc->pred = x264_malloc( 5*sizeof(predictor_t) );
    rc->pred_b_from_p = x264_malloc( sizeof(predictor_t) );
    for( i = 0; i < 5; i++ )
    {
        rc->last_qscale_for[i] = qp2qscale( ABR_INIT_QP );
        rc->lmin[i] = qp2qscale( h->param.rc.i_qp_min );
        rc->lmax[i] = qp2qscale( h->param.rc.i_qp_max );
        rc->pred[i].coeff= 2.0;
        rc->pred[i].count= 1.0;
        rc->pred[i].decay= 0.5;
        rc->row_preds[i].coeff= .25;
        rc->row_preds[i].count= 1.0;
        rc->row_preds[i].decay= 0.5;
    }
    *rc->pred_b_from_p = rc->pred[0];

    if( parse_zones( h ) < 0 )
    {
        x264_log( h, X264_LOG_ERROR, "failed to parse zones\n" );
        return -1;
    }

    /* Load stat file and init 2pass algo */
    if( h->param.rc.b_stat_read )
    {
        char *p, *stats_in, *stats_buf;

        /* read 1st pass stats */
        assert( h->param.rc.psz_stat_in );
        stats_buf = stats_in = x264_slurp_file( h->param.rc.psz_stat_in );
        if( !stats_buf )
        {
            x264_log(h, X264_LOG_ERROR, "ratecontrol_init: can't open stats file\n");
            return -1;
        }

        /* check whether 1st pass options were compatible with current options */
        if( !strncmp( stats_buf, "#options:", 9 ) )
        {
            int i;
            char *opts = stats_buf;
            stats_in = strchr( stats_buf, '\n' );
            if( !stats_in )
                return -1;
            *stats_in = '\0';
            stats_in++;

            if( ( p = strstr( opts, "bframes=" ) ) && sscanf( p, "bframes=%d", &i )
                && h->param.i_bframe != i )
            {
                x264_log( h, X264_LOG_ERROR, "different number of B-frames than 1st pass (%d vs %d)\n",
                          h->param.i_bframe, i );
                return -1;
            }

            /* since B-adapt doesn't (yet) take into account B-pyramid,
             * the converse is not a problem */
            if( strstr( opts, "b_pyramid=1" ) && !h->param.b_bframe_pyramid )
                x264_log( h, X264_LOG_WARNING, "1st pass used B-pyramid, 2nd doesn't\n" );

            if( ( p = strstr( opts, "keyint=" ) ) && sscanf( p, "keyint=%d", &i )
                && h->param.i_keyint_max != i )
                x264_log( h, X264_LOG_WARNING, "different keyint than 1st pass (%d vs %d)\n",
                          h->param.i_keyint_max, i );

            if( strstr( opts, "qp=0" ) && h->param.rc.i_rc_method == X264_RC_ABR )
                x264_log( h, X264_LOG_WARNING, "1st pass was lossless, bitrate prediction will be inaccurate\n" );

            if( !strstr( opts, "direct=3" ) && h->param.analyse.i_direct_mv_pred == X264_DIRECT_PRED_AUTO )
            {
                x264_log( h, X264_LOG_WARNING, "direct=auto not used on the first pass\n" );
                h->mb.b_direct_auto_write = 1;
            }

            if( ( p = strstr( opts, "b_adapt=" ) ) && sscanf( p, "b_adapt=%d", &i ) && i >= X264_B_ADAPT_NONE && i <= X264_B_ADAPT_TRELLIS )
                h->param.i_bframe_adaptive = i;
            else if( h->param.i_bframe )
            {
                x264_log( h, X264_LOG_ERROR, "b_adapt method specified in stats file not valid\n" );
                return -1;
            }
        }

        /* find number of pics */
        p = stats_in;
        for(i=-1; p; i++)
            p = strchr(p+1, ';');
        if(i==0)
        {
            x264_log(h, X264_LOG_ERROR, "empty stats file\n");
            return -1;
        }
        rc->num_entries = i;

        if( h->param.i_frame_total < rc->num_entries && h->param.i_frame_total > 0 )
        {
            x264_log( h, X264_LOG_WARNING, "2nd pass has fewer frames than 1st pass (%d vs %d)\n",
                      h->param.i_frame_total, rc->num_entries );
        }
        if( h->param.i_frame_total > rc->num_entries )
        {
            x264_log( h, X264_LOG_ERROR, "2nd pass has more frames than 1st pass (%d vs %d)\n",
                      h->param.i_frame_total, rc->num_entries );
            return -1;
        }

        rc->entry = (ratecontrol_entry_t*) x264_malloc(rc->num_entries * sizeof(ratecontrol_entry_t));
        memset(rc->entry, 0, rc->num_entries * sizeof(ratecontrol_entry_t));

        /* init all to skipped p frames */
        for(i=0; i<rc->num_entries; i++)
        {
            ratecontrol_entry_t *rce = &rc->entry[i];
            rce->pict_type = SLICE_TYPE_P;
            rce->qscale = rce->new_qscale = qp2qscale(20);
            rce->misc_bits = rc->nmb + 10;
            rce->new_qp = 0;
        }

        /* read stats */
        p = stats_in;
        for(i=0; i < rc->num_entries; i++)
        {
            ratecontrol_entry_t *rce;
            int frame_number;
            char pict_type;
            int e;
            char *next;
            float qp;

            next= strchr(p, ';');
            if(next)
            {
                (*next)=0; //sscanf is unbelievably slow on long strings
                next++;
            }
            e = sscanf(p, " in:%d ", &frame_number);

            if(frame_number < 0 || frame_number >= rc->num_entries)
            {
                x264_log(h, X264_LOG_ERROR, "bad frame number (%d) at stats line %d\n", frame_number, i);
                return -1;
            }
            rce = &rc->entry[frame_number];
            rce->direct_mode = 0;

            e += sscanf(p, " in:%*d out:%*d type:%c q:%f tex:%d mv:%d misc:%d imb:%d pmb:%d smb:%d d:%c",
                   &pict_type, &qp, &rce->tex_bits,
                   &rce->mv_bits, &rce->misc_bits, &rce->i_count, &rce->p_count,
                   &rce->s_count, &rce->direct_mode);

            switch(pict_type)
            {
                case 'I': rce->kept_as_ref = 1;
                case 'i': rce->pict_type = SLICE_TYPE_I; break;
                case 'P': rce->pict_type = SLICE_TYPE_P; break;
                case 'B': rce->kept_as_ref = 1;
                case 'b': rce->pict_type = SLICE_TYPE_B; break;
                default:  e = -1; break;
            }
            if(e < 10)
            {
                x264_log(h, X264_LOG_ERROR, "statistics are damaged at line %d, parser out=%d\n", i, e);
                return -1;
            }
            rce->qscale = qp2qscale(qp);
            p = next;
        }

        x264_free(stats_buf);

        if(h->param.rc.i_rc_method == X264_RC_ABR)
        {
            if(init_pass2(h) < 0) return -1;
        } /* else we're using constant quant, so no need to run the bitrate allocation */
    }

    /* Open output file */
    /* If input and output files are the same, output to a temp file
     * and move it to the real name only when it's complete */
    if( h->param.rc.b_stat_write )
    {
        char *p;

        rc->psz_stat_file_tmpname = x264_malloc( strlen(h->param.rc.psz_stat_out) + 6 );
        strcpy( rc->psz_stat_file_tmpname, h->param.rc.psz_stat_out );
        strcat( rc->psz_stat_file_tmpname, ".temp" );

        rc->p_stat_file_out = fopen( rc->psz_stat_file_tmpname, "wb" );
        if( rc->p_stat_file_out == NULL )
        {
            x264_log(h, X264_LOG_ERROR, "ratecontrol_init: can't open stats file\n");
            return -1;
        }

        p = x264_param2string( &h->param, 1 );
        fprintf( rc->p_stat_file_out, "#options: %s\n", p );
        x264_free( p );
    }

    for( i=0; i<h->param.i_threads; i++ )
    {
        h->thread[i]->rc = rc+i;
        if( i )
        {
            rc[i] = rc[0];
            memcpy( &h->thread[i]->param, &h->param, sizeof( x264_param_t ) );
            h->thread[i]->mb.b_variable_qp = h->mb.b_variable_qp;
        }
    }

    return 0;
}

static int parse_zone( x264_t *h, x264_zone_t *z, char *p )
{
    int len = 0;
    char *tok, UNUSED *saveptr;
    z->param = NULL;
    z->f_bitrate_factor = 1;
    if( 3 <= sscanf(p, "%u,%u,q=%u%n", &z->i_start, &z->i_end, &z->i_qp, &len) )
        z->b_force_qp = 1;
    else if( 3 <= sscanf(p, "%u,%u,b=%f%n", &z->i_start, &z->i_end, &z->f_bitrate_factor, &len) )
        z->b_force_qp = 0;
    else if( 2 <= sscanf(p, "%u,%u%n", &z->i_start, &z->i_end, &len) )
        z->b_force_qp = 0;
    else
    {
        x264_log( h, X264_LOG_ERROR, "invalid zone: \"%s\"\n", p );
        return -1;
    }
    p += len;
    if( !*p )
        return 0;
    z->param = x264_malloc( sizeof(x264_param_t) );
    memcpy( z->param, &h->param, sizeof(x264_param_t) );
    while( (tok = strtok_r( p, ",", &saveptr )) )
    {
        char *val = strchr( tok, '=' );
        if( val )
        {
            *val = '\0';
            val++;
        }
        if( x264_param_parse( z->param, tok, val ) )
        {
            x264_log( h, X264_LOG_ERROR, "invalid zone param: %s = %s\n", tok, val );
            return -1;
        }
        p = NULL;
    }
    return 0;
}

static int parse_zones( x264_t *h )
{
    x264_ratecontrol_t *rc = h->rc;
    int i;
    if( h->param.rc.psz_zones && !h->param.rc.i_zones )
    {
        char *p, *tok, UNUSED *saveptr;
        char *psz_zones = x264_malloc( strlen(h->param.rc.psz_zones)+1 );
        strcpy( psz_zones, h->param.rc.psz_zones );
        h->param.rc.i_zones = 1;
        for( p = psz_zones; *p; p++ )
            h->param.rc.i_zones += (*p == '/');
        h->param.rc.zones = x264_malloc( h->param.rc.i_zones * sizeof(x264_zone_t) );
        p = psz_zones;
        for( i = 0; i < h->param.rc.i_zones; i++ )
        {
            tok = strtok_r( p, "/", &saveptr );
            if( !tok || parse_zone( h, &h->param.rc.zones[i], tok ) )
                return -1;
            p = NULL;
        }
        x264_free( psz_zones );
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

        rc->i_zones = h->param.rc.i_zones + 1;
        rc->zones = x264_malloc( rc->i_zones * sizeof(x264_zone_t) );
        memcpy( rc->zones+1, h->param.rc.zones, (rc->i_zones-1) * sizeof(x264_zone_t) );

        // default zone to fall back to if none of the others match
        rc->zones[0].i_start = 0;
        rc->zones[0].i_end = INT_MAX;
        rc->zones[0].b_force_qp = 0;
        rc->zones[0].f_bitrate_factor = 1;
        rc->zones[0].param = x264_malloc( sizeof(x264_param_t) );
        memcpy( rc->zones[0].param, &h->param, sizeof(x264_param_t) );
        for( i = 1; i < rc->i_zones; i++ )
        {
            if( !rc->zones[i].param )
                rc->zones[i].param = rc->zones[0].param;
        }
    }

    return 0;
}

static x264_zone_t *get_zone( x264_t *h, int frame_num )
{
    int i;
    for( i = h->rc->i_zones-1; i >= 0; i-- )
    {
        x264_zone_t *z = &h->rc->zones[i];
        if( frame_num >= z->i_start && frame_num <= z->i_end )
            return z;
    }
    return NULL;
}

void x264_ratecontrol_summary( x264_t *h )
{
    x264_ratecontrol_t *rc = h->rc;
    if( rc->b_abr && h->param.rc.i_rc_method == X264_RC_ABR && rc->cbr_decay > .9999 )
    {
        double base_cplx = h->mb.i_mb_count * (h->param.i_bframe ? 120 : 80);
        x264_log( h, X264_LOG_INFO, "final ratefactor: %.2f\n",
                  qscale2qp( pow( base_cplx, 1 - h->param.rc.f_qcompress )
                             * rc->cplxr_sum / rc->wanted_bits_window ) );
    }
}

void x264_ratecontrol_delete( x264_t *h )
{
    x264_ratecontrol_t *rc = h->rc;
    int i;

    if( rc->p_stat_file_out )
    {
        fclose( rc->p_stat_file_out );
        if( h->i_frame >= rc->num_entries )
            if( rename( rc->psz_stat_file_tmpname, h->param.rc.psz_stat_out ) != 0 )
            {
                x264_log( h, X264_LOG_ERROR, "failed to rename \"%s\" to \"%s\"\n",
                          rc->psz_stat_file_tmpname, h->param.rc.psz_stat_out );
            }
        x264_free( rc->psz_stat_file_tmpname );
    }
    x264_free( rc->pred );
    x264_free( rc->pred_b_from_p );
    x264_free( rc->entry );
    if( rc->zones )
    {
        x264_free( rc->zones[0].param );
        if( h->param.rc.psz_zones )
            for( i=1; i<rc->i_zones; i++ )
                if( rc->zones[i].param != rc->zones[0].param )
                    x264_free( rc->zones[i].param );
        x264_free( rc->zones );
    }
    x264_free( rc );
}

void x264_ratecontrol_set_estimated_size( x264_t *h, int bits )
{
    x264_pthread_mutex_lock( &h->fenc->mutex );
    h->rc->frame_size_estimated = bits;
    x264_pthread_mutex_unlock( &h->fenc->mutex );
}

int x264_ratecontrol_get_estimated_size( x264_t const *h)
{
    int size;
    x264_pthread_mutex_lock( &h->fenc->mutex );
    size = h->rc->frame_size_estimated;
    x264_pthread_mutex_unlock( &h->fenc->mutex );
    return size;
}

static void accum_p_qp_update( x264_t *h, float qp )
{
    x264_ratecontrol_t *rc = h->rc;
    rc->accum_p_qp   *= .95;
    rc->accum_p_norm *= .95;
    rc->accum_p_norm += 1;
    if( h->sh.i_type == SLICE_TYPE_I )
        rc->accum_p_qp += qp + rc->ip_offset;
    else
        rc->accum_p_qp += qp;
}

/* Before encoding a frame, choose a QP for it */
void x264_ratecontrol_start( x264_t *h, int i_force_qp )
{
    x264_ratecontrol_t *rc = h->rc;
    ratecontrol_entry_t *rce = NULL;
    x264_zone_t *zone = get_zone( h, h->fenc->i_frame );
    float q;

    x264_emms();

    if( zone && (!rc->prev_zone || zone->param != rc->prev_zone->param) )
        x264_encoder_reconfig( h, zone->param );
    rc->prev_zone = zone;

    rc->qp_force = i_force_qp;

    if( h->param.rc.b_stat_read )
    {
        int frame = h->fenc->i_frame;
        assert( frame >= 0 && frame < rc->num_entries );
        rce = h->rc->rce = &h->rc->entry[frame];

        if( h->sh.i_type == SLICE_TYPE_B
            && h->param.analyse.i_direct_mv_pred == X264_DIRECT_PRED_AUTO )
        {
            h->sh.b_direct_spatial_mv_pred = ( rce->direct_mode == 's' );
            h->mb.b_direct_auto_read = ( rce->direct_mode == 's' || rce->direct_mode == 't' );
        }
    }

    if( rc->b_vbv )
    {
        memset( h->fdec->i_row_bits, 0, h->sps->i_mb_height * sizeof(int) );
        rc->row_pred = &rc->row_preds[h->sh.i_type];
        update_vbv_plan( h );
    }

    if( h->sh.i_type != SLICE_TYPE_B )
    {
        rc->bframes = 0;
        while( h->frames.current[rc->bframes] && IS_X264_TYPE_B(h->frames.current[rc->bframes]->i_type) )
            rc->bframes++;
    }

    if( i_force_qp )
    {
        q = i_force_qp - 1;
    }
    else if( rc->b_abr )
    {
        q = qscale2qp( rate_estimate_qscale( h ) );
    }
    else if( rc->b_2pass )
    {
        rce->new_qscale = rate_estimate_qscale( h );
        q = qscale2qp( rce->new_qscale );
    }
    else /* CQP */
    {
        if( h->sh.i_type == SLICE_TYPE_B && h->fdec->b_kept_as_ref )
            q = ( rc->qp_constant[ SLICE_TYPE_B ] + rc->qp_constant[ SLICE_TYPE_P ] ) / 2;
        else
            q = rc->qp_constant[ h->sh.i_type ];

        if( zone )
        {
            if( zone->b_force_qp )
                q += zone->i_qp - rc->qp_constant[SLICE_TYPE_P];
            else
                q -= 6*log(zone->f_bitrate_factor)/log(2);
        }
    }

    rc->qpa_rc =
    rc->qpa_aq = 0;
    h->fdec->f_qp_avg_rc =
    h->fdec->f_qp_avg_aq =
    rc->qpm =
    rc->qp = x264_clip3( (int)(q + 0.5), 0, 51 );
    rc->f_qpm = q;
    if( rce )
        rce->new_qp = rc->qp;

    /* accum_p_qp needs to be here so that future frames can benefit from the
     * data before this frame is done. but this only works because threading
     * guarantees to not re-encode any frames. so the non-threaded case does
     * accum_p_qp later. */
    if( h->param.i_threads > 1 )
        accum_p_qp_update( h, rc->qp );

    if( h->sh.i_type != SLICE_TYPE_B )
        rc->last_non_b_pict_type = h->sh.i_type;
}

static double predict_row_size( x264_t *h, int y, int qp )
{
    /* average between two predictors:
     * absolute SATD, and scaled bit cost of the colocated row in the previous frame */
    x264_ratecontrol_t *rc = h->rc;
    double pred_s = predict_size( rc->row_pred, qp2qscale(qp), h->fdec->i_row_satd[y] );
    double pred_t = 0;
    if( h->sh.i_type != SLICE_TYPE_I
        && h->fref0[0]->i_type == h->fdec->i_type
        && h->fref0[0]->i_row_satd[y] > 0
        && (abs(h->fref0[0]->i_row_satd[y] - h->fdec->i_row_satd[y]) < h->fdec->i_row_satd[y]/2))
    {
        pred_t = h->fref0[0]->i_row_bits[y] * h->fdec->i_row_satd[y] / h->fref0[0]->i_row_satd[y]
                 * qp2qscale(h->fref0[0]->i_row_qp[y]) / qp2qscale(qp);
    }
    if( pred_t == 0 )
        pred_t = pred_s;

    return (pred_s + pred_t) / 2;
}

static double row_bits_so_far( x264_t *h, int y )
{
    int i;
    double bits = 0;
    for( i = 0; i <= y; i++ )
        bits += h->fdec->i_row_bits[i];
    return bits;
}

static double predict_row_size_sum( x264_t *h, int y, int qp )
{
    int i;
    double bits = row_bits_so_far(h, y);
    for( i = y+1; i < h->sps->i_mb_height; i++ )
        bits += predict_row_size( h, i, qp );
    return bits;
}


void x264_ratecontrol_mb( x264_t *h, int bits )
{
    x264_ratecontrol_t *rc = h->rc;
    const int y = h->mb.i_mb_y;

    x264_emms();

    h->fdec->i_row_bits[y] += bits;
    rc->qpa_rc += rc->f_qpm;
    rc->qpa_aq += h->mb.i_qp;

    if( h->mb.i_mb_x != h->sps->i_mb_width - 1 || !rc->b_vbv)
        return;

    h->fdec->i_row_qp[y] = rc->qpm;

    if( h->sh.i_type == SLICE_TYPE_B )
    {
        /* B-frames shouldn't use lower QP than their reference frames.
         * This code is a bit overzealous in limiting B-frame quantizers, but it helps avoid
         * underflows due to the fact that B-frames are not explicitly covered by VBV. */
        if( y < h->sps->i_mb_height-1 )
        {
            int i_estimated;
            int avg_qp = X264_MAX(h->fref0[0]->i_row_qp[y+1], h->fref1[0]->i_row_qp[y+1])
                       + rc->pb_offset * ((h->fenc->i_type == X264_TYPE_BREF) ? 0.5 : 1);
            rc->qpm = X264_MIN(X264_MAX( rc->qp, avg_qp), 51); //avg_qp could go higher than 51 due to pb_offset
            i_estimated = row_bits_so_far(h, y); //FIXME: compute full estimated size
            if (i_estimated > h->rc->frame_size_planned)
                x264_ratecontrol_set_estimated_size(h, i_estimated);
        }
    }
    else
    {
        update_predictor( rc->row_pred, qp2qscale(rc->qpm), h->fdec->i_row_satd[y], h->fdec->i_row_bits[y] );

        /* tweak quality based on difference from predicted size */
        if( y < h->sps->i_mb_height-1 && h->stat.i_slice_count[h->sh.i_type] > 0 )
        {
            int prev_row_qp = h->fdec->i_row_qp[y];
            int b0 = predict_row_size_sum( h, y, rc->qpm );
            int b1 = b0;
            int i_qp_max = X264_MIN( prev_row_qp + h->param.rc.i_qp_step, h->param.rc.i_qp_max );
            int i_qp_min = X264_MAX( prev_row_qp - h->param.rc.i_qp_step, h->param.rc.i_qp_min );
            float buffer_left_planned = rc->buffer_fill - rc->frame_size_planned;
            float rc_tol = 1;
            float headroom = 0;

            /* Don't modify the row QPs until a sufficent amount of the bits of the frame have been processed, in case a flat */
            /* area at the top of the frame was measured inaccurately. */
            if(row_bits_so_far(h,y) < 0.05 * rc->frame_size_planned)
                return;

            headroom = buffer_left_planned/rc->buffer_size;
            if(h->sh.i_type != SLICE_TYPE_I)
                headroom /= 2;
            rc_tol += headroom;

            if( !rc->b_vbv_min_rate )
                i_qp_min = X264_MAX( i_qp_min, h->sh.i_qp );

            while( rc->qpm < i_qp_max
                   && (b1 > rc->frame_size_planned * rc_tol
                    || (rc->buffer_fill - b1 < buffer_left_planned * 0.5)))
            {
                rc->qpm ++;
                b1 = predict_row_size_sum( h, y, rc->qpm );
            }

            /* avoid VBV underflow */
            while( (rc->qpm < h->param.rc.i_qp_max)
                   && (rc->buffer_fill - b1 < rc->buffer_size * 0.005))
            {
                rc->qpm ++;
                b1 = predict_row_size_sum( h, y, rc->qpm );
            }

            while( rc->qpm > i_qp_min
                   && rc->qpm > h->fdec->i_row_qp[0]
                   && ((b1 < rc->frame_size_planned * 0.8 && rc->qpm <= prev_row_qp)
                     || b1 < (rc->buffer_fill - rc->buffer_size + rc->buffer_rate) * 1.1) )
            {
                rc->qpm --;
                b1 = predict_row_size_sum( h, y, rc->qpm );
            }
            x264_ratecontrol_set_estimated_size(h, b1);
        }
    }
    /* loses the fractional part of the frame-wise qp */
    rc->f_qpm = rc->qpm;
}

int x264_ratecontrol_qp( x264_t *h )
{
    return h->rc->qpm;
}

/* In 2pass, force the same frame types as in the 1st pass */
int x264_ratecontrol_slice_type( x264_t *h, int frame_num )
{
    x264_ratecontrol_t *rc = h->rc;
    if( h->param.rc.b_stat_read )
    {
        if( frame_num >= rc->num_entries )
        {
            /* We could try to initialize everything required for ABR and
             * adaptive B-frames, but that would be complicated.
             * So just calculate the average QP used so far. */
            int i;

            h->param.rc.i_qp_constant = (h->stat.i_slice_count[SLICE_TYPE_P] == 0) ? 24
                                      : 1 + h->stat.f_slice_qp[SLICE_TYPE_P] / h->stat.i_slice_count[SLICE_TYPE_P];
            rc->qp_constant[SLICE_TYPE_P] = x264_clip3( h->param.rc.i_qp_constant, 0, 51 );
            rc->qp_constant[SLICE_TYPE_I] = x264_clip3( (int)( qscale2qp( qp2qscale( h->param.rc.i_qp_constant ) / fabs( h->param.rc.f_ip_factor )) + 0.5 ), 0, 51 );
            rc->qp_constant[SLICE_TYPE_B] = x264_clip3( (int)( qscale2qp( qp2qscale( h->param.rc.i_qp_constant ) * fabs( h->param.rc.f_pb_factor )) + 0.5 ), 0, 51 );

            x264_log(h, X264_LOG_ERROR, "2nd pass has more frames than 1st pass (%d)\n", rc->num_entries);
            x264_log(h, X264_LOG_ERROR, "continuing anyway, at constant QP=%d\n", h->param.rc.i_qp_constant);
            if( h->param.i_bframe_adaptive )
                x264_log(h, X264_LOG_ERROR, "disabling adaptive B-frames\n");

            for( i = 0; i < h->param.i_threads; i++ )
            {
                h->thread[i]->rc->b_abr = 0;
                h->thread[i]->rc->b_2pass = 0;
                h->thread[i]->param.rc.i_rc_method = X264_RC_CQP;
                h->thread[i]->param.rc.b_stat_read = 0;
                h->thread[i]->param.i_bframe_adaptive = 0;
                h->thread[i]->param.i_scenecut_threshold = 0;
                if( h->thread[i]->param.i_bframe > 1 )
                    h->thread[i]->param.i_bframe = 1;
            }
            return X264_TYPE_AUTO;
        }
        switch( rc->entry[frame_num].pict_type )
        {
            case SLICE_TYPE_I:
                return rc->entry[frame_num].kept_as_ref ? X264_TYPE_IDR : X264_TYPE_I;

            case SLICE_TYPE_B:
                return rc->entry[frame_num].kept_as_ref ? X264_TYPE_BREF : X264_TYPE_B;

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

    x264_emms();

    h->stat.frame.i_mb_count_skip = mbs[P_SKIP] + mbs[B_SKIP];
    h->stat.frame.i_mb_count_i = mbs[I_16x16] + mbs[I_8x8] + mbs[I_4x4];
    h->stat.frame.i_mb_count_p = mbs[P_L0] + mbs[P_8x8];
    for( i = B_DIRECT; i < B_8x8; i++ )
        h->stat.frame.i_mb_count_p += mbs[i];

    h->fdec->f_qp_avg_rc = rc->qpa_rc /= h->mb.i_mb_count;
    h->fdec->f_qp_avg_aq = rc->qpa_aq /= h->mb.i_mb_count;

    if( h->param.rc.b_stat_write )
    {
        char c_type = h->sh.i_type==SLICE_TYPE_I ? (h->fenc->i_poc==0 ? 'I' : 'i')
                    : h->sh.i_type==SLICE_TYPE_P ? 'P'
                    : h->fenc->b_kept_as_ref ? 'B' : 'b';
        int dir_frame = h->stat.frame.i_direct_score[1] - h->stat.frame.i_direct_score[0];
        int dir_avg = h->stat.i_direct_score[1] - h->stat.i_direct_score[0];
        char c_direct = h->mb.b_direct_auto_write ?
                        ( dir_frame>0 ? 's' : dir_frame<0 ? 't' :
                          dir_avg>0 ? 's' : dir_avg<0 ? 't' : '-' )
                        : '-';
        fprintf( rc->p_stat_file_out,
                 "in:%d out:%d type:%c q:%.2f tex:%d mv:%d misc:%d imb:%d pmb:%d smb:%d d:%c;\n",
                 h->fenc->i_frame, h->i_frame,
                 c_type, rc->qpa_rc,
                 h->stat.frame.i_tex_bits,
                 h->stat.frame.i_mv_bits,
                 h->stat.frame.i_misc_bits,
                 h->stat.frame.i_mb_count_i,
                 h->stat.frame.i_mb_count_p,
                 h->stat.frame.i_mb_count_skip,
                 c_direct);
    }

    if( rc->b_abr )
    {
        if( h->sh.i_type != SLICE_TYPE_B )
            rc->cplxr_sum += bits * qp2qscale(rc->qpa_rc) / rc->last_rceq;
        else
        {
            /* Depends on the fact that B-frame's QP is an offset from the following P-frame's.
             * Not perfectly accurate with B-refs, but good enough. */
            rc->cplxr_sum += bits * qp2qscale(rc->qpa_rc) / (rc->last_rceq * fabs(h->param.rc.f_pb_factor));
        }
        rc->cplxr_sum *= rc->cbr_decay;
        rc->wanted_bits_window += rc->bitrate / rc->fps;
        rc->wanted_bits_window *= rc->cbr_decay;

        if( h->param.i_threads == 1 )
            accum_p_qp_update( h, rc->qpa_rc );
    }

    if( rc->b_2pass )
    {
        rc->expected_bits_sum += qscale2bits( rc->rce, qp2qscale(rc->rce->new_qp) );
    }

    if( h->mb.b_variable_qp )
    {
        if( h->sh.i_type == SLICE_TYPE_B )
        {
            rc->bframe_bits += bits;
            if( !h->frames.current[0] || !IS_X264_TYPE_B(h->frames.current[0]->i_type) )
            {
                update_predictor( rc->pred_b_from_p, qp2qscale(rc->qpa_rc),
                                  h->fref1[h->i_ref1-1]->i_satd, rc->bframe_bits / rc->bframes );
                /* In some cases, such as completely blank scenes, pred_b_from_p can go nuts */
                /* Hackily cap the predictor coeff in case this happens. */
                /* FIXME FIXME FIXME */
                rc->pred_b_from_p->coeff = X264_MIN( rc->pred_b_from_p->coeff, 10. );
                rc->bframe_bits = 0;
            }
        }
    }

    update_vbv( h, bits );
}

/****************************************************************************
 * 2 pass functions
 ***************************************************************************/

/**
 * modify the bitrate curve from pass1 for one frame
 */
static double get_qscale(x264_t *h, ratecontrol_entry_t *rce, double rate_factor, int frame_num)
{
    x264_ratecontrol_t *rcc= h->rc;
    double q;
    x264_zone_t *zone = get_zone( h, frame_num );

    q = pow( rce->blurred_complexity, 1 - h->param.rc.f_qcompress );

    // avoid NaN's in the rc_eq
    if(!isfinite(q) || rce->tex_bits + rce->mv_bits == 0)
        q = rcc->last_qscale;
    else
    {
        rcc->last_rceq = q;
        q /= rate_factor;
        rcc->last_qscale = q;
    }

    if( zone )
    {
        if( zone->b_force_qp )
            q = qp2qscale(zone->i_qp);
        else
            q /= zone->f_bitrate_factor;
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
             && rce->tex_bits == 0 )
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
    if( var < 10 )
        return;
    p->count *= p->decay;
    p->coeff *= p->decay;
    p->count ++;
    p->coeff += bits*q / var;
}

// update VBV after encoding a frame
static void update_vbv( x264_t *h, int bits )
{
    x264_ratecontrol_t *rcc = h->rc;
    x264_ratecontrol_t *rct = h->thread[0]->rc;

    if( rcc->last_satd >= h->mb.i_mb_count )
        update_predictor( &rct->pred[h->sh.i_type], qp2qscale(rcc->qpa_rc), rcc->last_satd, bits );

    if( !rcc->b_vbv )
        return;

    rct->buffer_fill_final += rct->buffer_rate - bits;
    if( rct->buffer_fill_final < 0 )
        x264_log( h, X264_LOG_WARNING, "VBV underflow (%.0f bits)\n", rct->buffer_fill_final );
    rct->buffer_fill_final = x264_clip3f( rct->buffer_fill_final, 0, rct->buffer_size );
}

// provisionally update VBV according to the planned size of all frames currently in progress
static void update_vbv_plan( x264_t *h )
{
    x264_ratecontrol_t *rcc = h->rc;
    rcc->buffer_fill = h->thread[0]->rc->buffer_fill_final;
    if( h->param.i_threads > 1 )
    {
        int j = h->rc - h->thread[0]->rc;
        int i;
        for( i=1; i<h->param.i_threads; i++ )
        {
            x264_t *t = h->thread[ (j+i)%h->param.i_threads ];
            double bits = t->rc->frame_size_planned;
            if( !t->b_thread_active )
                continue;
            bits  = X264_MAX(bits, x264_ratecontrol_get_estimated_size(t));
            rcc->buffer_fill += rcc->buffer_rate - bits;
            rcc->buffer_fill = x264_clip3( rcc->buffer_fill, 0, rcc->buffer_size );
        }
    }
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
    if( rcc->b_vbv &&
        ( pict_type == SLICE_TYPE_P ||
          ( pict_type == SLICE_TYPE_I && rcc->last_non_b_pict_type == SLICE_TYPE_I ) ) )
    {
        if( rcc->buffer_fill/rcc->buffer_size < 0.5 )
            q /= x264_clip3f( 2.0*rcc->buffer_fill/rcc->buffer_size, 0.5, 1.0 );
    }

    if( rcc->b_vbv && rcc->last_satd > 0 )
    {
        /* Now a hard threshold to make sure the frame fits in VBV.
         * This one is mostly for I-frames. */
        double bits = predict_size( &rcc->pred[h->sh.i_type], q, rcc->last_satd );
        double qf = 1.0;
        if( bits > rcc->buffer_fill/2 )
            qf = x264_clip3f( rcc->buffer_fill/(2*bits), 0.2, 1.0 );
        q /= qf;
        bits *= qf;
        if( bits < rcc->buffer_rate/2 )
            q *= bits*2/rcc->buffer_rate;
        q = X264_MAX( q0, q );

        /* Check B-frame complexity, and use up any bits that would
         * overflow before the next P-frame. */
        if( h->sh.i_type == SLICE_TYPE_P )
        {
            int nb = rcc->bframes;
            double pbbits = bits;
            double bbits = predict_size( rcc->pred_b_from_p, q * h->param.rc.f_pb_factor, rcc->last_satd );
            double space;

            if( bbits > rcc->buffer_rate )
                nb = 0;
            pbbits += nb * bbits;

            space = rcc->buffer_fill + (1+nb)*rcc->buffer_rate - rcc->buffer_size;
            if( pbbits < space )
            {
                q *= X264_MAX( pbbits / space,
                               bits / (0.5 * rcc->buffer_size) );
            }
            q = X264_MAX( q0-5, q );
        }

        if( !rcc->b_vbv_min_rate )
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
static float rate_estimate_qscale( x264_t *h )
{
    float q;
    x264_ratecontrol_t *rcc = h->rc;
    ratecontrol_entry_t rce;
    int pict_type = h->sh.i_type;
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
        /* B-frames don't have independent ratecontrol, but rather get the
         * average QP of the two adjacent P-frames + an offset */

        int i0 = IS_X264_TYPE_I(h->fref0[0]->i_type);
        int i1 = IS_X264_TYPE_I(h->fref1[0]->i_type);
        int dt0 = abs(h->fenc->i_poc - h->fref0[0]->i_poc);
        int dt1 = abs(h->fenc->i_poc - h->fref1[0]->i_poc);
        float q0 = h->fref0[0]->f_qp_avg_rc;
        float q1 = h->fref1[0]->f_qp_avg_rc;

        if( h->fref0[0]->i_type == X264_TYPE_BREF )
            q0 -= rcc->pb_offset/2;
        if( h->fref1[0]->i_type == X264_TYPE_BREF )
            q1 -= rcc->pb_offset/2;

        if(i0 && i1)
            q = (q0 + q1) / 2 + rcc->ip_offset;
        else if(i0)
            q = q1;
        else if(i1)
            q = q0;
        else
            q = (q0*dt1 + q1*dt0) / (dt0 + dt1);

        if(h->fenc->b_kept_as_ref)
            q += rcc->pb_offset/2;
        else
            q += rcc->pb_offset;

        rcc->frame_size_planned = predict_size( rcc->pred_b_from_p, q, h->fref1[h->i_ref1-1]->i_satd );
        x264_ratecontrol_set_estimated_size(h, rcc->frame_size_planned);
        rcc->last_satd = 0;
        return qp2qscale(q);
    }
    else
    {
        double abr_buffer = 2 * rcc->rate_tolerance * rcc->bitrate;

        if( rcc->b_2pass )
        {
            //FIXME adjust abr_buffer based on distance to the end of the video
            int64_t diff;
            int64_t predicted_bits = total_bits;

            if( rcc->b_vbv )
            {
                if( h->param.i_threads > 1 )
                {
                    int j = h->rc - h->thread[0]->rc;
                    int i;
                    for( i=1; i<h->param.i_threads; i++ )
                    {
                        x264_t *t = h->thread[ (j+i)%h->param.i_threads ];
                        double bits = t->rc->frame_size_planned;
                        if( !t->b_thread_active )
                            continue;
                        bits  = X264_MAX(bits, x264_ratecontrol_get_estimated_size(t));
                        predicted_bits += (int64_t)bits;
                    }
                }
            }
            else
            {
                if( h->fenc->i_frame < h->param.i_threads )
                    predicted_bits += (int64_t)h->fenc->i_frame * rcc->bitrate / rcc->fps;
                else
                    predicted_bits += (int64_t)(h->param.i_threads - 1) * rcc->bitrate / rcc->fps;
            }

            diff = predicted_bits - (int64_t)rce.expected_bits;
            q = rce.new_qscale;
            q /= x264_clip3f((double)(abr_buffer - diff) / abr_buffer, .5, 2);
            if( ((h->fenc->i_frame + 1 - h->param.i_threads) >= rcc->fps) &&
                (rcc->expected_bits_sum > 0))
            {
                /* Adjust quant based on the difference between
                 * achieved and expected bitrate so far */
                double time = (double)h->fenc->i_frame / rcc->num_entries;
                double w = x264_clip3f( time*100, 0.0, 1.0 );
                q *= pow( (double)total_bits / rcc->expected_bits_sum, w );
            }
            if( rcc->b_vbv )
            {
                /* Do not overflow vbv */
                double expected_size = qscale2bits(&rce, q);
                double expected_vbv = rcc->buffer_fill + rcc->buffer_rate - expected_size;
                double expected_fullness =  rce.expected_vbv / rcc->buffer_size;
                double qmax = q*(2 - expected_fullness);
                double size_constraint = 1 + expected_fullness;
                qmax = X264_MAX(qmax, rce.new_qscale);
                if (expected_fullness < .05)
                    qmax = lmax;
                qmax = X264_MIN(qmax, lmax);
                while( ((expected_vbv < rce.expected_vbv/size_constraint) && (q < qmax)) ||
                        ((expected_vbv < 0) && (q < lmax)))
                {
                    q *= 1.05;
                    expected_size = qscale2bits(&rce, q);
                    expected_vbv = rcc->buffer_fill + rcc->buffer_rate - expected_size;
                }
                rcc->last_satd = x264_stack_align( x264_rc_analyse_slice, h );
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

            double wanted_bits, overflow=1, lmin, lmax;

            rcc->last_satd = x264_stack_align( x264_rc_analyse_slice, h );
            rcc->short_term_cplxsum *= 0.5;
            rcc->short_term_cplxcount *= 0.5;
            rcc->short_term_cplxsum += rcc->last_satd;
            rcc->short_term_cplxcount ++;

            rce.tex_bits = rcc->last_satd;
            rce.blurred_complexity = rcc->short_term_cplxsum / rcc->short_term_cplxcount;
            rce.mv_bits = 0;
            rce.p_count = rcc->nmb;
            rce.i_count = 0;
            rce.s_count = 0;
            rce.qscale = 1;
            rce.pict_type = pict_type;

            if( h->param.rc.i_rc_method == X264_RC_CRF )
            {
                q = get_qscale( h, &rce, rcc->rate_factor_constant, h->fenc->i_frame );
            }
            else
            {
                int i_frame_done = h->fenc->i_frame + 1 - h->param.i_threads;

                q = get_qscale( h, &rce, rcc->wanted_bits_window / rcc->cplxr_sum, h->fenc->i_frame );

                // FIXME is it simpler to keep track of wanted_bits in ratecontrol_end?
                wanted_bits = i_frame_done * rcc->bitrate / rcc->fps;
                if( wanted_bits > 0 )
                {
                    abr_buffer *= X264_MAX( 1, sqrt(i_frame_done/25) );
                    overflow = x264_clip3f( 1.0 + (total_bits - wanted_bits) / abr_buffer, .5, 2 );
                    q *= overflow;
                }
            }

            if( pict_type == SLICE_TYPE_I && h->param.i_keyint_max > 1
                /* should test _next_ pict type, but that isn't decided yet */
                && rcc->last_non_b_pict_type != SLICE_TYPE_I )
            {
                q = qp2qscale( rcc->accum_p_qp / rcc->accum_p_norm );
                q /= fabs( h->param.rc.f_ip_factor );
            }
            else if( h->i_frame > 0 )
            {
                /* Asymmetric clipping, because symmetric would prevent
                 * overflow control in areas of rapidly oscillating complexity */
                lmin = rcc->last_qscale_for[pict_type] / rcc->lstep;
                lmax = rcc->last_qscale_for[pict_type] * rcc->lstep;
                if( overflow > 1.1 && h->i_frame > 3 )
                    lmax *= rcc->lstep;
                else if( overflow < 0.9 )
                    lmin /= rcc->lstep;

                q = x264_clip3f(q, lmin, lmax);
            }
            else if( h->param.rc.i_rc_method == X264_RC_CRF )
            {
                q = qp2qscale( ABR_INIT_QP ) / fabs( h->param.rc.f_ip_factor );
            }

            //FIXME use get_diff_limited_q() ?
            q = clip_qscale( h, pict_type, q );
        }

        rcc->last_qscale_for[pict_type] =
        rcc->last_qscale = q;

        if( !(rcc->b_2pass && !rcc->b_vbv) && h->fenc->i_frame == 0 )
            rcc->last_qscale_for[SLICE_TYPE_P] = q;

        if( rcc->b_2pass && rcc->b_vbv)
            rcc->frame_size_planned = qscale2bits(&rce, q);
        else
            rcc->frame_size_planned = predict_size( &rcc->pred[h->sh.i_type], q, rcc->last_satd );
        x264_ratecontrol_set_estimated_size(h, rcc->frame_size_planned);
        return q;
    }
}

void x264_thread_sync_ratecontrol( x264_t *cur, x264_t *prev, x264_t *next )
{
    if( cur != prev )
    {
#define COPY(var) memcpy(&cur->rc->var, &prev->rc->var, sizeof(cur->rc->var))
        /* these vars are updated in x264_ratecontrol_start()
         * so copy them from the context that most recently started (prev)
         * to the context that's about to start (cur).
         */
        COPY(accum_p_qp);
        COPY(accum_p_norm);
        COPY(last_satd);
        COPY(last_rceq);
        COPY(last_qscale_for);
        COPY(last_non_b_pict_type);
        COPY(short_term_cplxsum);
        COPY(short_term_cplxcount);
        COPY(bframes);
        COPY(prev_zone);
#undef COPY
    }
    if( cur != next )
    {
#define COPY(var) next->rc->var = cur->rc->var
        /* these vars are updated in x264_ratecontrol_end()
         * so copy them from the context that most recently ended (cur)
         * to the context that's about to end (next)
         */
        COPY(cplxr_sum);
        COPY(expected_bits_sum);
        COPY(wanted_bits_window);
        COPY(bframe_bits);
#undef COPY
    }
    //FIXME row_preds[] (not strictly necessary, but would improve prediction)
    /* the rest of the variables are either constant or thread-local */
}

static int find_underflow( x264_t *h, double *fills, int *t0, int *t1, int over )
{
    /* find an interval ending on an overflow or underflow (depending on whether
     * we're adding or removing bits), and starting on the earliest frame that
     * can influence the buffer fill of that end frame. */
    x264_ratecontrol_t *rcc = h->rc;
    const double buffer_min = (over ? .1 : .1) * rcc->buffer_size;
    const double buffer_max = .9 * rcc->buffer_size;
    double fill = fills[*t0-1];
    double parity = over ? 1. : -1.;
    int i, start=-1, end=-1;
    for(i = *t0; i < rcc->num_entries; i++)
    {
        fill += (rcc->buffer_rate - qscale2bits(&rcc->entry[i], rcc->entry[i].new_qscale)) * parity;
        fill = x264_clip3f(fill, 0, rcc->buffer_size);
        fills[i] = fill;
        if(fill <= buffer_min || i == 0)
        {
            if(end >= 0)
                break;
            start = i;
        }
        else if(fill >= buffer_max && start >= 0)
            end = i;
    }
    *t0 = start;
    *t1 = end;
    return start>=0 && end>=0;
}

static int fix_underflow( x264_t *h, int t0, int t1, double adjustment, double qscale_min, double qscale_max)
{
    x264_ratecontrol_t *rcc = h->rc;
    double qscale_orig, qscale_new;
    int i;
    int adjusted = 0;
    if(t0 > 0)
        t0++;
    for(i = t0; i <= t1; i++)
    {
        qscale_orig = rcc->entry[i].new_qscale;
        qscale_orig = x264_clip3f(qscale_orig, qscale_min, qscale_max);
        qscale_new  = qscale_orig * adjustment;
        qscale_new  = x264_clip3f(qscale_new, qscale_min, qscale_max);
        rcc->entry[i].new_qscale = qscale_new;
        adjusted = adjusted || (qscale_new != qscale_orig);
    }
    return adjusted;
}

static double count_expected_bits( x264_t *h )
{
    x264_ratecontrol_t *rcc = h->rc;
    double expected_bits = 0;
    int i;
    for(i = 0; i < rcc->num_entries; i++)
    {
        ratecontrol_entry_t *rce = &rcc->entry[i];
        rce->expected_bits = expected_bits;
        expected_bits += qscale2bits(rce, rce->new_qscale);
    }
    return expected_bits;
}

static void vbv_pass2( x264_t *h )
{
    /* for each interval of buffer_full .. underflow, uniformly increase the qp of all
     * frames in the interval until either buffer is full at some intermediate frame or the
     * last frame in the interval no longer underflows.  Recompute intervals and repeat.
     * Then do the converse to put bits back into overflow areas until target size is met */

    x264_ratecontrol_t *rcc = h->rc;
    double *fills = x264_malloc((rcc->num_entries+1)*sizeof(double));
    double all_available_bits = h->param.rc.i_bitrate * 1000. * rcc->num_entries / rcc->fps;
    double expected_bits = 0;
    double adjustment;
    double prev_bits = 0;
    int i, t0, t1;
    double qscale_min = qp2qscale(h->param.rc.i_qp_min);
    double qscale_max = qp2qscale(h->param.rc.i_qp_max);
    int iterations = 0;
    int adj_min, adj_max;

    fills++;

    /* adjust overall stream size */
    do
    {
        iterations++;
        prev_bits = expected_bits;

        if(expected_bits != 0)
        {   /* not first iteration */
            adjustment = X264_MAX(X264_MIN(expected_bits / all_available_bits, 0.999), 0.9);
            fills[-1] = rcc->buffer_size * h->param.rc.f_vbv_buffer_init;
            t0 = 0;
            /* fix overflows */
            adj_min = 1;
            while(adj_min && find_underflow(h, fills, &t0, &t1, 1))
            {
                adj_min = fix_underflow(h, t0, t1, adjustment, qscale_min, qscale_max);
                t0 = t1;
            }
        }

        fills[-1] = rcc->buffer_size * (1. - h->param.rc.f_vbv_buffer_init);
        t0 = 0;
        /* fix underflows -- should be done after overflow, as we'd better undersize target than underflowing VBV */
        adj_max = 1;
        while(adj_max && find_underflow(h, fills, &t0, &t1, 0))
            adj_max = fix_underflow(h, t0, t1, 1.001, qscale_min, qscale_max);

        expected_bits = count_expected_bits(h);
    } while((expected_bits < .995*all_available_bits) && ((int)(expected_bits+.5) > (int)(prev_bits+.5)) );

    if (!adj_max)
        x264_log( h, X264_LOG_WARNING, "vbv-maxrate issue, qpmax or vbv-maxrate too low\n");

    /* store expected vbv filling values for tracking when encoding */
    for(i = 0; i < rcc->num_entries; i++)
        rcc->entry[i].expected_vbv = rcc->buffer_size - fills[i];

    x264_free(fills-1);
}

static int init_pass2( x264_t *h )
{
    x264_ratecontrol_t *rcc = h->rc;
    uint64_t all_const_bits = 0;
    uint64_t all_available_bits = (uint64_t)(h->param.rc.i_bitrate * 1000. * rcc->num_entries / rcc->fps);
    double rate_factor, step, step_mult;
    double qblur = h->param.rc.f_qblur;
    double cplxblur = h->param.rc.f_complexity_blur;
    const int filter_size = (int)(qblur*4) | 1;
    double expected_bits;
    double *qscale, *blurred_qscale;
    int i;

    /* find total/average complexity & const_bits */
    for(i=0; i<rcc->num_entries; i++)
    {
        ratecontrol_entry_t *rce = &rcc->entry[i];
        all_const_bits += rce->misc_bits;
    }

    if( all_available_bits < all_const_bits)
    {
        x264_log(h, X264_LOG_ERROR, "requested bitrate is too low. estimated minimum is %d kbps\n",
                 (int)(all_const_bits * rcc->fps / (rcc->num_entries * 1000.)));
        return -1;
    }

    /* Blur complexities, to reduce local fluctuation of QP.
     * We don't blur the QPs directly, because then one very simple frame
     * could drag down the QP of a nearby complex frame and give it more
     * bits than intended. */
    for(i=0; i<rcc->num_entries; i++)
    {
        ratecontrol_entry_t *rce = &rcc->entry[i];
        double weight_sum = 0;
        double cplx_sum = 0;
        double weight = 1.0;
        double gaussian_weight;
        int j;
        /* weighted average of cplx of future frames */
        for(j=1; j<cplxblur*2 && j<rcc->num_entries-i; j++)
        {
            ratecontrol_entry_t *rcj = &rcc->entry[i+j];
            weight *= 1 - pow( (float)rcj->i_count / rcc->nmb, 2 );
            if(weight < .0001)
                break;
            gaussian_weight = weight * exp(-j*j/200.0);
            weight_sum += gaussian_weight;
            cplx_sum += gaussian_weight * (qscale2bits(rcj, 1) - rcj->misc_bits);
        }
        /* weighted average of cplx of past frames */
        weight = 1.0;
        for(j=0; j<=cplxblur*2 && j<=i; j++)
        {
            ratecontrol_entry_t *rcj = &rcc->entry[i-j];
            gaussian_weight = weight * exp(-j*j/200.0);
            weight_sum += gaussian_weight;
            cplx_sum += gaussian_weight * (qscale2bits(rcj, 1) - rcj->misc_bits);
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
    for(step = 1E4 * step_mult; step > 1E-7 * step_mult; step *= 0.5)
    {
        expected_bits = 0;
        rate_factor += step;

        rcc->last_non_b_pict_type = -1;
        rcc->last_accum_p_norm = 1;
        rcc->accum_p_norm = 0;

        /* find qscale */
        for(i=0; i<rcc->num_entries; i++)
        {
            qscale[i] = get_qscale(h, &rcc->entry[i], rate_factor, i);
        }

        /* fixed I/B qscale relative to P */
        for(i=rcc->num_entries-1; i>=0; i--)
        {
            qscale[i] = get_diff_limited_q(h, &rcc->entry[i], qscale[i]);
            assert(qscale[i] >= 0);
        }

        /* smooth curve */
        if(filter_size > 1)
        {
            assert(filter_size%2==1);
            for(i=0; i<rcc->num_entries; i++)
            {
                ratecontrol_entry_t *rce = &rcc->entry[i];
                int j;
                double q=0.0, sum=0.0;

                for(j=0; j<filter_size; j++)
                {
                    int index = i+j-filter_size/2;
                    double d = index-i;
                    double coeff = qblur==0 ? 1.0 : exp(-d*d/(qblur*qblur));
                    if(index < 0 || index >= rcc->num_entries)
                        continue;
                    if(rce->pict_type != rcc->entry[index].pict_type)
                        continue;
                    q += qscale[index] * coeff;
                    sum += coeff;
                }
                blurred_qscale[i] = q/sum;
            }
        }

        /* find expected bits */
        for(i=0; i<rcc->num_entries; i++)
        {
            ratecontrol_entry_t *rce = &rcc->entry[i];
            rce->new_qscale = clip_qscale(h, rce->pict_type, blurred_qscale[i]);
            assert(rce->new_qscale >= 0);
            expected_bits += qscale2bits(rce, rce->new_qscale);
        }

        if(expected_bits > all_available_bits) rate_factor -= step;
    }

    x264_free(qscale);
    if(filter_size > 1)
        x264_free(blurred_qscale);

    if(rcc->b_vbv)
        vbv_pass2(h);
    expected_bits = count_expected_bits(h);

    if(fabs(expected_bits/all_available_bits - 1.0) > 0.01)
    {
        double avgq = 0;
        for(i=0; i<rcc->num_entries; i++)
            avgq += rcc->entry[i].new_qscale;
        avgq = qscale2qp(avgq / rcc->num_entries);

        if ((expected_bits > all_available_bits) || (!rcc->b_vbv))
            x264_log(h, X264_LOG_WARNING, "Error: 2pass curve failed to converge\n");
        x264_log(h, X264_LOG_WARNING, "target: %.2f kbit/s, expected: %.2f kbit/s, avg QP: %.4f\n",
                 (float)h->param.rc.i_bitrate,
                 expected_bits * rcc->fps / (rcc->num_entries * 1000.),
                 avgq);
        if(expected_bits < all_available_bits && avgq < h->param.rc.i_qp_min + 2)
        {
            if(h->param.rc.i_qp_min > 0)
                x264_log(h, X264_LOG_WARNING, "try reducing target bitrate or reducing qp_min (currently %d)\n", h->param.rc.i_qp_min);
            else
                x264_log(h, X264_LOG_WARNING, "try reducing target bitrate\n");
        }
        else if(expected_bits > all_available_bits && avgq > h->param.rc.i_qp_max - 2)
        {
            if(h->param.rc.i_qp_max < 51)
                x264_log(h, X264_LOG_WARNING, "try increasing target bitrate or increasing qp_max (currently %d)\n", h->param.rc.i_qp_max);
            else
                x264_log(h, X264_LOG_WARNING, "try increasing target bitrate\n");
        }
        else if(!(rcc->b_2pass && rcc->b_vbv))
            x264_log(h, X264_LOG_WARNING, "internal error\n");
    }

    return 0;
}


