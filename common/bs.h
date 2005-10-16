/*****************************************************************************
 * bs.h :
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: bs.h,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#ifdef _BS_H
#warning FIXME Multiple inclusion of bs.h
#else
#define _BS_H

typedef struct bs_s
{
    uint8_t *p_start;
    uint8_t *p;
    uint8_t *p_end;

    int     i_left;    /* i_count number of available bits */
    int     i_bits_encoded; /* RD only */
} bs_t;

static inline void bs_init( bs_t *s, void *p_data, int i_data )
{
    s->p_start = p_data;
    s->p       = p_data;
    s->p_end   = s->p + i_data;
    s->i_left  = 8;
}
static inline int bs_pos( bs_t *s )
{
    return( 8 * ( s->p - s->p_start ) + 8 - s->i_left );
}
static inline int bs_eof( bs_t *s )
{
    return( s->p >= s->p_end ? 1: 0 );
}
static inline uint32_t bs_read( bs_t *s, int i_count )
{
     static uint32_t i_mask[33] ={0x00,
                                  0x01,      0x03,      0x07,      0x0f,
                                  0x1f,      0x3f,      0x7f,      0xff,
                                  0x1ff,     0x3ff,     0x7ff,     0xfff,
                                  0x1fff,    0x3fff,    0x7fff,    0xffff,
                                  0x1ffff,   0x3ffff,   0x7ffff,   0xfffff,
                                  0x1fffff,  0x3fffff,  0x7fffff,  0xffffff,
                                  0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
                                  0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff};
    int      i_shr;
    uint32_t i_result = 0;

    while( i_count > 0 )
    {
        if( s->p >= s->p_end )
        {
            break;
        }

        if( ( i_shr = s->i_left - i_count ) >= 0 )
        {
            /* more in the buffer than requested */
            i_result |= ( *s->p >> i_shr )&i_mask[i_count];
            s->i_left -= i_count;
            if( s->i_left == 0 )
            {
                s->p++;
                s->i_left = 8;
            }
            return( i_result );
        }
        else
        {
            /* less in the buffer than requested */
           i_result |= (*s->p&i_mask[s->i_left]) << -i_shr;
           i_count  -= s->i_left;
           s->p++;
           s->i_left = 8;
        }
    }

    return( i_result );
}

static inline uint32_t bs_read1( bs_t *s )
{

    if( s->p < s->p_end )
    {
        unsigned int i_result;

        s->i_left--;
        i_result = ( *s->p >> s->i_left )&0x01;
        if( s->i_left == 0 )
        {
            s->p++;
            s->i_left = 8;
        }
        return i_result;
    }

    return 0;
}
static inline uint32_t bs_show( bs_t *s, int i_count )
{
    if( s->p < s->p_end && i_count > 0 )
    {
        uint32_t i_cache = ((s->p[0] << 24)+(s->p[1] << 16)+(s->p[2] << 8)+s->p[3]) << (8-s->i_left);
        return( i_cache >> ( 32 - i_count) );
    }
    return 0;
}

/* TODO optimize */
static inline void bs_skip( bs_t *s, int i_count )
{
    s->i_left -= i_count;

    while( s->i_left <= 0 )
    {
        s->p++;
        s->i_left += 8;
    }
}


static inline int bs_read_ue( bs_t *s )
{
    int i = 0;

    while( bs_read1( s ) == 0 && s->p < s->p_end && i < 32 )
    {
        i++;
    }
    return( ( 1 << i) - 1 + bs_read( s, i ) );
}
static inline int bs_read_se( bs_t *s )
{
    int val = bs_read_ue( s );

    return val&0x01 ? (val+1)/2 : -(val/2);
}

static inline int bs_read_te( bs_t *s, int x )
{
    if( x == 1 )
    {
        return 1 - bs_read1( s );
    }
    else if( x > 1 )
    {
        return bs_read_ue( s );
    }
    return 0;
}

static inline void bs_write( bs_t *s, int i_count, uint32_t i_bits )
{
    if( s->p >= s->p_end - 4 )
        return;
    while( i_count > 0 )
    {
        if( i_count < 32 )
            i_bits &= (1<<i_count)-1;
        if( i_count < s->i_left )
        {
            *s->p = (*s->p << i_count) | i_bits;
            s->i_left -= i_count;
            break;
        }
        else
        {
            *s->p = (*s->p << s->i_left) | (i_bits >> (i_count - s->i_left));
            i_count -= s->i_left;
            s->p++;
            s->i_left = 8;
        }
    }
}

static inline void bs_write1( bs_t *s, uint32_t i_bit )
{
    if( s->p < s->p_end )
    {
        *s->p <<= 1;
        *s->p |= i_bit;
        s->i_left--;
        if( s->i_left == 0 )
        {
            s->p++;
            s->i_left = 8;
        }
    }
}

static inline void bs_align_0( bs_t *s )
{
    if( s->i_left != 8 )
    {
        *s->p <<= s->i_left;
        s->i_left = 8;
        s->p++;
    }
}
static inline void bs_align_1( bs_t *s )
{
    if( s->i_left != 8 )
    {
        *s->p <<= s->i_left;
        *s->p |= (1 << s->i_left) - 1;
        s->i_left = 8;
        s->p++;
    }
}
static inline void bs_align( bs_t *s )
{
    bs_align_0( s );
}



/* golomb functions */

static inline void bs_write_ue( bs_t *s, unsigned int val )
{
    int i_size = 0;
    static const int i_size0_255[256] =
    {
        1,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
    };

    if( val == 0 )
    {
        bs_write1( s, 1 );
    }
    else
    {
        unsigned int tmp = ++val;

        if( tmp >= 0x00010000 )
        {
            i_size += 16;
            tmp >>= 16;
        }
        if( tmp >= 0x100 )
        {
            i_size += 8;
            tmp >>= 8;
        }
        i_size += i_size0_255[tmp];

        bs_write( s, 2 * i_size - 1, val );
    }
}

static inline void bs_write_se( bs_t *s, int val )
{
    bs_write_ue( s, val <= 0 ? -val * 2 : val * 2 - 1);
}

static inline void bs_write_te( bs_t *s, int x, int val )
{
    if( x == 1 )
    {
        bs_write1( s, 1&~val );
    }
    else if( x > 1 )
    {
        bs_write_ue( s, val );
    }
}

static inline void bs_rbsp_trailing( bs_t *s )
{
    bs_write1( s, 1 );
    if( s->i_left != 8 )
    {
        bs_write( s, s->i_left, 0x00 );
    }
}

static inline int bs_size_ue( unsigned int val )
{
    static const int i_size0_254[255] =
    {
        1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
        11,11,11,11,11,11,11,11,11,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
        13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
        13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
        13,13,13,13,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
        15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
        15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
        15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
        15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
        15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
    };

    if( val < 255 )
    {
        return i_size0_254[val];
    }
    else
    {
        int i_size = 0;

        val++;

        if( val >= 0x10000 )
        {
            i_size += 32;
            val = (val >> 16) - 1;
        }
        if( val >= 0x100 )
        {
            i_size += 16;
            val = (val >> 8) - 1;
        }
        return i_size0_254[val] + i_size;
    }
}

static inline int bs_size_se( int val )
{
    return bs_size_ue( val <= 0 ? -val * 2 : val * 2 - 1);
}

static inline int bs_size_te( int x, int val )
{
    if( x == 1 )
    {
        return 1;
    }
    else if( x > 1 )
    {
        return bs_size_ue( val );
    }
    return 0;
}



#endif
