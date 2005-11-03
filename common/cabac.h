/*****************************************************************************
 * cabac.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: cabac.h,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#ifndef _CABAC_H
#define _CABAC_H 1

typedef struct
{
    /* context */
    /* #436-459 are for interlacing, so are omitted for now */
    uint8_t state[436];

    /* state */
    int i_low;
    int i_range;

    /* bit stream */
    int i_bits_outstanding;
    int f8_bits_encoded; // only if using x264_cabac_size_decision()
    bs_t *s;

} x264_cabac_t;

/* encoder/decoder: init the contexts given i_slice_type, the quantif and the model */
void x264_cabac_context_init( x264_cabac_t *cb, int i_slice_type, int i_qp, int i_model );

/* decoder only: */
void x264_cabac_decode_init    ( x264_cabac_t *cb, bs_t *s );
int  x264_cabac_decode_decision( x264_cabac_t *cb, int i_ctx_idx );
int  x264_cabac_decode_bypass  ( x264_cabac_t *cb );
int  x264_cabac_decode_terminal( x264_cabac_t *cb );

/* encoder only: */
void x264_cabac_encode_init ( x264_cabac_t *cb, bs_t *s );
void x264_cabac_encode_decision( x264_cabac_t *cb, int i_ctx_idx, int b );
void x264_cabac_encode_bypass( x264_cabac_t *cb, int b );
void x264_cabac_encode_terminal( x264_cabac_t *cb, int b );
void x264_cabac_encode_flush( x264_cabac_t *cb );
/* don't write the bitstream, just calculate cost: */
void x264_cabac_size_decision( x264_cabac_t *cb, int i_ctx, int b );
int  x264_cabac_size_decision2( uint8_t *state, int b );
int  x264_cabac_size_decision_noup( uint8_t *state, int b );

static inline int x264_cabac_pos( x264_cabac_t *cb )
{
    return bs_pos( cb->s ) + cb->i_bits_outstanding;
}

#endif
