/***************************************************-*- coding: iso-8859-1 -*-
 * ratecontrol.c: h264 encoder library (Rate Control)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: ratecontrol.c,v 1.1 2004/06/03 19:27:08 fenrir Exp $
 *
 * Authors: Måns Rullgård <mru@mru.ath.cx>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "../core/common.h"
#include "../core/cpu.h"
#include "ratecontrol.h"

#define DEBUG_RC 0

struct x264_ratecontrol_t
{
    /* constants */
    float fps;
    int gop_size;
    int bitrate;
    int nmb;                    /* number of MBs */
    int buffer_size;
    int rcbufrate;
    int init_qp;

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
    int qpa;                    /* average qp for last frame */
    int qps;
    int qp_avg_p;               /* average QP for P frames */
    int qp_last_p;
    int fbits;                  /* bits allocated for current frame */
    int ufbits;                 /* bits used for current frame */
    int nzcoeffs;               /* # of 0-quantized coefficients */
    int ncoeffs;                /* total # of coefficients */
    int overhead;
};

int x264_ratecontrol_new( x264_t *h )
{
    x264_ratecontrol_t *rc = x264_malloc( sizeof( x264_ratecontrol_t ) );
    float bpp;

    memset(rc, 0, sizeof(*rc));

    rc->fps = h->param.f_fps > 0.1 ? h->param.f_fps : 25.0f;
    rc->gop_size = h->param.i_iframe;
    rc->bitrate = h->param.i_bitrate * 1000;
    rc->nmb = ((h->param.i_width + 15) / 16) * ((h->param.i_height + 15) / 16);

    rc->qp = h->param.i_qp_constant;
    rc->qpa = rc->qp;
    rc->qpm = rc->qp;

    rc->buffer_size = h->param.i_rc_buffer_size;
    if(rc->buffer_size <= 0)
        rc->buffer_size = rc->bitrate / 2;
    rc->buffer_fullness = h->param.i_rc_init_buffer;
    rc->rcbufrate = rc->bitrate / rc->fps;

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

#if DEBUG_RC
    fprintf(stderr, "%f fps, %i bps, bufsize %i\n",
         rc->fps, rc->bitrate, rc->buffer_size);
#endif

    h->rc = rc;

    return 0;
}

void x264_ratecontrol_delete( x264_t *h )
{
    x264_ratecontrol_t *rc = h->rc;
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

    if(!h->param.b_cbr)
        return;

    x264_cpu_restore( h->param.cpu );

    rc->slice_type = i_slice_type;

    switch(i_slice_type){
    case SLICE_TYPE_I:
        gbuf = rc->buffer_fullness + (rc->gop_size-1) * rc->rcbufrate;
        rc->bits_gop = gbuf - rc->buffer_size / 2;

        if(!rc->mb && rc->pframes){
            int qp = (float) rc->qp_avg_p / rc->pframes + 0.5;
#if 0 /* JM does this without explaining why */
            int gdq = (float) rc->gop_size / 15 + 0.5;
            if(gdq > 2)
                gdq = 2;
            qp -= gdq;
            if(qp > rc->qp_last_p - 2)
                qp--;
#endif
            qp = x264_clip3(qp, rc->gop_qp - 4, rc->gop_qp + 4);
            qp =
                x264_clip3(qp, h->param.i_qp_min, h->param.i_qp_max);
            rc->gop_qp = qp;
        } else if(rc->frames > 4){
            rc->gop_qp = rc->init_qp;
        }

        kp = h->param.f_ip_factor * h->param.f_pb_factor;

#if DEBUG_RC
        fprintf(stderr, "gbuf=%i bits_gop=%i frames=%i gop_qp=%i\n",
                gbuf, rc->bits_gop, rc->frames, rc->gop_qp);
#endif

        rc->bits_last_gop = 0;
        rc->frames = 0;
        rc->pframes = 0;
        rc->qp_avg_p = 0;
        break;

    case SLICE_TYPE_P:
        kp = h->param.f_pb_factor;
        break;

    case SLICE_TYPE_B:
        kp = 1.0;
        break;

    default:
        fprintf(stderr, "x264: ratecontrol: unknown slice type %i\n",
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
        (h->param.f_ip_factor * h->param.f_pb_factor * iframes +
         h->param.f_pb_factor * pframes + bframes);

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
        dqp = h->param.i_rc_sens * exp2f((float) rc->qpa / 6) *
            (zn - rc->nzcoeffs) / rc->nzcoeffs;
        dqp = x264_clip3(dqp, -h->param.i_qp_step, h->param.i_qp_step);
        rc->qp = rc->qpa + dqp;
    }

    if(rc->fbits > 0.8 * maxbits)
        rc->qp += 1;
    else if(rc->fbits > 0.9 * maxbits)
        rc->qp += 2;
    else if(rc->fbits < 1.2 * minbits)
        rc->qp -= 1;
    else if(rc->fbits < 1.1 * minbits)
        rc->qp -= 2;

    rc->qp = x264_clip3(rc->qp, h->param.i_qp_min, h->param.i_qp_max);
    rc->qpm = rc->qp;

#if DEBUG_RC > 1
    fprintf(stderr, "fbits=%i, qp=%i, z=%i, min=%i, max=%i\n",
         rc->fbits, rc->qpm, zn, minbits, maxbits);
#endif

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

    if( !h->param.b_cbr )
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
    dqp = (float) 2*h->param.i_rc_sens * exp2f((float) rc->qps / rc->mb / 6) *
        (zn - enz) / enz;
    rc->qpm = x264_clip3(rc->qpm + dqp, rc->qp - 3, rc->qp + 3);
    if(rbits <= 0)
        rc->qpm++;
    rc->qpm = x264_clip3(rc->qpm, h->param.i_qp_min, h->param.i_qp_max);
}

int  x264_ratecontrol_qp( x264_t *h )
{
    return h->rc->qpm;
}

void x264_ratecontrol_end( x264_t *h, int bits )
{
    x264_ratecontrol_t *rc = h->rc;

    if(!h->param.b_cbr)
        return;

    rc->buffer_fullness += rc->rcbufrate - bits;
    if(rc->buffer_fullness < 0){
        fprintf(stderr, "x264: buffer underflow %i\n", rc->buffer_fullness);
        rc->buffer_fullness = 0;
    }

    rc->qpa = rc->qps / rc->mb;
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

#if DEBUG_RC > 1
    fprintf(stderr, " bits=%i, qp=%i, z=%i, zr=%6.3f, buf=%i\n",
         bits, rc->qpa, rc->nzcoeffs,
         (float) rc->nzcoeffs / rc->ncoeffs, rc->buffer_fullness);
#endif

    rc->bits_last_gop += bits;
    rc->frames++;
    rc->mb = 0;
}

/*
  Local Variables:
  indent-tabs-mode: nil
  End:
*/
