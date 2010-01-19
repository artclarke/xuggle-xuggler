/*****************************************************************************
 * vlc.c : vlc table
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar <fenrir@via.ecp.fr>
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

#include "common.h"

#define MKVLC( a, b ) { a, b }
const vlc_t x264_coeff0_token[5] =
{
    MKVLC( 0x1, 1 ), /* str=1 */
    MKVLC( 0x3, 2 ), /* str=11 */
    MKVLC( 0xf, 4 ), /* str=1111 */
    MKVLC( 0x3, 6 ), /* str=000011 */
    MKVLC( 0x1, 2 )  /* str=01 */
};

const vlc_t x264_coeff_token[5][16*4] =
{
    /* table 0 */
    {
        MKVLC( 0x5, 6 ), /* str=000101 */
        MKVLC( 0x1, 2 ), /* str=01 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x7, 8 ), /* str=00000111 */
        MKVLC( 0x4, 6 ), /* str=000100 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x7, 9 ), /* str=000000111 */
        MKVLC( 0x6, 8 ), /* str=00000110 */
        MKVLC( 0x5, 7 ), /* str=0000101 */
        MKVLC( 0x3, 5 ), /* str=00011 */

        MKVLC( 0x7, 10 ), /* str=0000000111 */
        MKVLC( 0x6, 9 ), /* str=000000110 */
        MKVLC( 0x5, 8 ), /* str=00000101 */
        MKVLC( 0x3, 6 ), /* str=000011 */

        MKVLC( 0x7, 11 ), /* str=00000000111 */
        MKVLC( 0x6, 10 ), /* str=0000000110 */
        MKVLC( 0x5, 9 ), /* str=000000101 */
        MKVLC( 0x4, 7 ), /* str=0000100 */

        MKVLC( 0xf, 13 ), /* str=0000000001111 */
        MKVLC( 0x6, 11 ), /* str=00000000110 */
        MKVLC( 0x5, 10 ), /* str=0000000101 */
        MKVLC( 0x4, 8 ), /* str=00000100 */

        MKVLC( 0xb, 13 ), /* str=0000000001011 */
        MKVLC( 0xe, 13 ), /* str=0000000001110 */
        MKVLC( 0x5, 11 ), /* str=00000000101 */
        MKVLC( 0x4, 9 ), /* str=000000100 */

        MKVLC( 0x8, 13 ), /* str=0000000001000 */
        MKVLC( 0xa, 13 ), /* str=0000000001010 */
        MKVLC( 0xd, 13 ), /* str=0000000001101 */
        MKVLC( 0x4, 10 ), /* str=0000000100 */

        MKVLC( 0xf, 14 ), /* str=00000000001111 */
        MKVLC( 0xe, 14 ), /* str=00000000001110 */
        MKVLC( 0x9, 13 ), /* str=0000000001001 */
        MKVLC( 0x4, 11 ), /* str=00000000100 */

        MKVLC( 0xb, 14 ), /* str=00000000001011 */
        MKVLC( 0xa, 14 ), /* str=00000000001010 */
        MKVLC( 0xd, 14 ), /* str=00000000001101 */
        MKVLC( 0xc, 13 ), /* str=0000000001100 */

        MKVLC( 0xf, 15 ), /* str=000000000001111 */
        MKVLC( 0xe, 15 ), /* str=000000000001110 */
        MKVLC( 0x9, 14 ), /* str=00000000001001 */
        MKVLC( 0xc, 14 ), /* str=00000000001100 */

        MKVLC( 0xb, 15 ), /* str=000000000001011 */
        MKVLC( 0xa, 15 ), /* str=000000000001010 */
        MKVLC( 0xd, 15 ), /* str=000000000001101 */
        MKVLC( 0x8, 14 ), /* str=00000000001000 */

        MKVLC( 0xf, 16 ), /* str=0000000000001111 */
        MKVLC( 0x1, 15 ), /* str=000000000000001 */
        MKVLC( 0x9, 15 ), /* str=000000000001001 */
        MKVLC( 0xc, 15 ), /* str=000000000001100 */

        MKVLC( 0xb, 16 ), /* str=0000000000001011 */
        MKVLC( 0xe, 16 ), /* str=0000000000001110 */
        MKVLC( 0xd, 16 ), /* str=0000000000001101 */
        MKVLC( 0x8, 15 ), /* str=000000000001000 */

        MKVLC( 0x7, 16 ), /* str=0000000000000111 */
        MKVLC( 0xa, 16 ), /* str=0000000000001010 */
        MKVLC( 0x9, 16 ), /* str=0000000000001001 */
        MKVLC( 0xc, 16 ), /* str=0000000000001100 */

        MKVLC( 0x4, 16 ), /* str=0000000000000100 */
        MKVLC( 0x6, 16 ), /* str=0000000000000110 */
        MKVLC( 0x5, 16 ), /* str=0000000000000101 */
        MKVLC( 0x8, 16 ), /* str=0000000000001000 */
    },

    /* table 1 */
    {
        MKVLC( 0xb, 6 ), /* str=001011 */
        MKVLC( 0x2, 2 ), /* str=10 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x7, 6 ), /* str=000111 */
        MKVLC( 0x7, 5 ), /* str=00111 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x7, 7 ), /* str=0000111 */
        MKVLC( 0xa, 6 ), /* str=001010 */
        MKVLC( 0x9, 6 ), /* str=001001 */
        MKVLC( 0x5, 4 ), /* str=0101 */

        MKVLC( 0x7, 8 ), /* str=00000111 */
        MKVLC( 0x6, 6 ), /* str=000110 */
        MKVLC( 0x5, 6 ), /* str=000101 */
        MKVLC( 0x4, 4 ), /* str=0100 */

        MKVLC( 0x4, 8 ), /* str=00000100 */
        MKVLC( 0x6, 7 ), /* str=0000110 */
        MKVLC( 0x5, 7 ), /* str=0000101 */
        MKVLC( 0x6, 5 ), /* str=00110 */

        MKVLC( 0x7, 9 ), /* str=000000111 */
        MKVLC( 0x6, 8 ), /* str=00000110 */
        MKVLC( 0x5, 8 ), /* str=00000101 */
        MKVLC( 0x8, 6 ), /* str=001000 */

        MKVLC( 0xf, 11 ), /* str=00000001111 */
        MKVLC( 0x6, 9 ), /* str=000000110 */
        MKVLC( 0x5, 9 ), /* str=000000101 */
        MKVLC( 0x4, 6 ), /* str=000100 */

        MKVLC( 0xb, 11 ), /* str=00000001011 */
        MKVLC( 0xe, 11 ), /* str=00000001110 */
        MKVLC( 0xd, 11 ), /* str=00000001101 */
        MKVLC( 0x4, 7 ), /* str=0000100 */

        MKVLC( 0xf, 12 ), /* str=000000001111 */
        MKVLC( 0xa, 11 ), /* str=00000001010 */
        MKVLC( 0x9, 11 ), /* str=00000001001 */
        MKVLC( 0x4, 9 ), /* str=000000100 */

        MKVLC( 0xb, 12 ), /* str=000000001011 */
        MKVLC( 0xe, 12 ), /* str=000000001110 */
        MKVLC( 0xd, 12 ), /* str=000000001101 */
        MKVLC( 0xc, 11 ), /* str=00000001100 */

        MKVLC( 0x8, 12 ), /* str=000000001000 */
        MKVLC( 0xa, 12 ), /* str=000000001010 */
        MKVLC( 0x9, 12 ), /* str=000000001001 */
        MKVLC( 0x8, 11 ), /* str=00000001000 */

        MKVLC( 0xf, 13 ), /* str=0000000001111 */
        MKVLC( 0xe, 13 ), /* str=0000000001110 */
        MKVLC( 0xd, 13 ), /* str=0000000001101 */
        MKVLC( 0xc, 12 ), /* str=000000001100 */

        MKVLC( 0xb, 13 ), /* str=0000000001011 */
        MKVLC( 0xa, 13 ), /* str=0000000001010 */
        MKVLC( 0x9, 13 ), /* str=0000000001001 */
        MKVLC( 0xc, 13 ), /* str=0000000001100 */

        MKVLC( 0x7, 13 ), /* str=0000000000111 */
        MKVLC( 0xb, 14 ), /* str=00000000001011 */
        MKVLC( 0x6, 13 ), /* str=0000000000110 */
        MKVLC( 0x8, 13 ), /* str=0000000001000 */

        MKVLC( 0x9, 14 ), /* str=00000000001001 */
        MKVLC( 0x8, 14 ), /* str=00000000001000 */
        MKVLC( 0xa, 14 ), /* str=00000000001010 */
        MKVLC( 0x1, 13 ), /* str=0000000000001 */

        MKVLC( 0x7, 14 ), /* str=00000000000111 */
        MKVLC( 0x6, 14 ), /* str=00000000000110 */
        MKVLC( 0x5, 14 ), /* str=00000000000101 */
        MKVLC( 0x4, 14 ), /* str=00000000000100 */
    },
    /* table 2 */
    {
        MKVLC( 0xf, 6 ), /* str=001111 */
        MKVLC( 0xe, 4 ), /* str=1110 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0xb, 6 ), /* str=001011 */
        MKVLC( 0xf, 5 ), /* str=01111 */
        MKVLC( 0xd, 4 ), /* str=1101 */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x8, 6 ), /* str=001000 */
        MKVLC( 0xc, 5 ), /* str=01100 */
        MKVLC( 0xe, 5 ), /* str=01110 */
        MKVLC( 0xc, 4 ), /* str=1100 */

        MKVLC( 0xf, 7 ), /* str=0001111 */
        MKVLC( 0xa, 5 ), /* str=01010 */
        MKVLC( 0xb, 5 ), /* str=01011 */
        MKVLC( 0xb, 4 ), /* str=1011 */

        MKVLC( 0xb, 7 ), /* str=0001011 */
        MKVLC( 0x8, 5 ), /* str=01000 */
        MKVLC( 0x9, 5 ), /* str=01001 */
        MKVLC( 0xa, 4 ), /* str=1010 */

        MKVLC( 0x9, 7 ), /* str=0001001 */
        MKVLC( 0xe, 6 ), /* str=001110 */
        MKVLC( 0xd, 6 ), /* str=001101 */
        MKVLC( 0x9, 4 ), /* str=1001 */

        MKVLC( 0x8, 7 ), /* str=0001000 */
        MKVLC( 0xa, 6 ), /* str=001010 */
        MKVLC( 0x9, 6 ), /* str=001001 */
        MKVLC( 0x8, 4 ), /* str=1000 */

        MKVLC( 0xf, 8 ), /* str=00001111 */
        MKVLC( 0xe, 7 ), /* str=0001110 */
        MKVLC( 0xd, 7 ), /* str=0001101 */
        MKVLC( 0xd, 5 ), /* str=01101 */

        MKVLC( 0xb, 8 ), /* str=00001011 */
        MKVLC( 0xe, 8 ), /* str=00001110 */
        MKVLC( 0xa, 7 ), /* str=0001010 */
        MKVLC( 0xc, 6 ), /* str=001100 */

        MKVLC( 0xf, 9 ), /* str=000001111 */
        MKVLC( 0xa, 8 ), /* str=00001010 */
        MKVLC( 0xd, 8 ), /* str=00001101 */
        MKVLC( 0xc, 7 ), /* str=0001100 */

        MKVLC( 0xb, 9 ), /* str=000001011 */
        MKVLC( 0xe, 9 ), /* str=000001110 */
        MKVLC( 0x9, 8 ), /* str=00001001 */
        MKVLC( 0xc, 8 ), /* str=00001100 */

        MKVLC( 0x8, 9 ), /* str=000001000 */
        MKVLC( 0xa, 9 ), /* str=000001010 */
        MKVLC( 0xd, 9 ), /* str=000001101 */
        MKVLC( 0x8, 8 ), /* str=00001000 */

        MKVLC( 0xd, 10 ), /* str=0000001101 */
        MKVLC( 0x7, 9 ), /* str=000000111 */
        MKVLC( 0x9, 9 ), /* str=000001001 */
        MKVLC( 0xc, 9 ), /* str=000001100 */

        MKVLC( 0x9, 10 ), /* str=0000001001 */
        MKVLC( 0xc, 10 ), /* str=0000001100 */
        MKVLC( 0xb, 10 ), /* str=0000001011 */
        MKVLC( 0xa, 10 ), /* str=0000001010 */

        MKVLC( 0x5, 10 ), /* str=0000000101 */
        MKVLC( 0x8, 10 ), /* str=0000001000 */
        MKVLC( 0x7, 10 ), /* str=0000000111 */
        MKVLC( 0x6, 10 ), /* str=0000000110 */

        MKVLC( 0x1, 10 ), /* str=0000000001 */
        MKVLC( 0x4, 10 ), /* str=0000000100 */
        MKVLC( 0x3, 10 ), /* str=0000000011 */
        MKVLC( 0x2, 10 ), /* str=0000000010 */
    },

    /* table 3 */
    {
        MKVLC( 0x0, 6 ), /* str=000000 */
        MKVLC( 0x1, 6 ), /* str=000001 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x4, 6 ), /* str=000100 */
        MKVLC( 0x5, 6 ), /* str=000101 */
        MKVLC( 0x6, 6 ), /* str=000110 */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x8, 6 ), /* str=001000 */
        MKVLC( 0x9, 6 ), /* str=001001 */
        MKVLC( 0xa, 6 ), /* str=001010 */
        MKVLC( 0xb, 6 ), /* str=001011 */

        MKVLC( 0xc, 6 ), /* str=001100 */
        MKVLC( 0xd, 6 ), /* str=001101 */
        MKVLC( 0xe, 6 ), /* str=001110 */
        MKVLC( 0xf, 6 ), /* str=001111 */

        MKVLC( 0x10, 6 ), /* str=010000 */
        MKVLC( 0x11, 6 ), /* str=010001 */
        MKVLC( 0x12, 6 ), /* str=010010 */
        MKVLC( 0x13, 6 ), /* str=010011 */

        MKVLC( 0x14, 6 ), /* str=010100 */
        MKVLC( 0x15, 6 ), /* str=010101 */
        MKVLC( 0x16, 6 ), /* str=010110 */
        MKVLC( 0x17, 6 ), /* str=010111 */

        MKVLC( 0x18, 6 ), /* str=011000 */
        MKVLC( 0x19, 6 ), /* str=011001 */
        MKVLC( 0x1a, 6 ), /* str=011010 */
        MKVLC( 0x1b, 6 ), /* str=011011 */

        MKVLC( 0x1c, 6 ), /* str=011100 */
        MKVLC( 0x1d, 6 ), /* str=011101 */
        MKVLC( 0x1e, 6 ), /* str=011110 */
        MKVLC( 0x1f, 6 ), /* str=011111 */

        MKVLC( 0x20, 6 ), /* str=100000 */
        MKVLC( 0x21, 6 ), /* str=100001 */
        MKVLC( 0x22, 6 ), /* str=100010 */
        MKVLC( 0x23, 6 ), /* str=100011 */

        MKVLC( 0x24, 6 ), /* str=100100 */
        MKVLC( 0x25, 6 ), /* str=100101 */
        MKVLC( 0x26, 6 ), /* str=100110 */
        MKVLC( 0x27, 6 ), /* str=100111 */

        MKVLC( 0x28, 6 ), /* str=101000 */
        MKVLC( 0x29, 6 ), /* str=101001 */
        MKVLC( 0x2a, 6 ), /* str=101010 */
        MKVLC( 0x2b, 6 ), /* str=101011 */

        MKVLC( 0x2c, 6 ), /* str=101100 */
        MKVLC( 0x2d, 6 ), /* str=101101 */
        MKVLC( 0x2e, 6 ), /* str=101110 */
        MKVLC( 0x2f, 6 ), /* str=101111 */

        MKVLC( 0x30, 6 ), /* str=110000 */
        MKVLC( 0x31, 6 ), /* str=110001 */
        MKVLC( 0x32, 6 ), /* str=110010 */
        MKVLC( 0x33, 6 ), /* str=110011 */

        MKVLC( 0x34, 6 ), /* str=110100 */
        MKVLC( 0x35, 6 ), /* str=110101 */
        MKVLC( 0x36, 6 ), /* str=110110 */
        MKVLC( 0x37, 6 ), /* str=110111 */

        MKVLC( 0x38, 6 ), /* str=111000 */
        MKVLC( 0x39, 6 ), /* str=111001 */
        MKVLC( 0x3a, 6 ), /* str=111010 */
        MKVLC( 0x3b, 6 ), /* str=111011 */

        MKVLC( 0x3c, 6 ), /* str=111100 */
        MKVLC( 0x3d, 6 ), /* str=111101 */
        MKVLC( 0x3e, 6 ), /* str=111110 */
        MKVLC( 0x3f, 6 ), /* str=111111 */
    },

    /* table 4 */
    {
        MKVLC( 0x7, 6 ), /* str=000111 */
        MKVLC( 0x1, 1 ), /* str=1 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x4, 6 ), /* str=000100 */
        MKVLC( 0x6, 6 ), /* str=000110 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x3, 6 ), /* str=000011 */
        MKVLC( 0x3, 7 ), /* str=0000011 */
        MKVLC( 0x2, 7 ), /* str=0000010 */
        MKVLC( 0x5, 6 ), /* str=000101 */

        MKVLC( 0x2, 6 ), /* str=000010 */
        MKVLC( 0x3, 8 ), /* str=00000011 */
        MKVLC( 0x2, 8 ), /* str=00000010 */
        MKVLC( 0x0, 7 ), /* str=0000000 */

        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */

        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    }
};

/* [i_total_coeff-1][i_total_zeros] */
const vlc_t x264_total_zeros[15][16] =
{
    { /* i_total 1 */
        MKVLC( 0x1, 1 ), /* str=1 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x2, 3 ), /* str=010 */
        MKVLC( 0x3, 4 ), /* str=0011 */
        MKVLC( 0x2, 4 ), /* str=0010 */
        MKVLC( 0x3, 5 ), /* str=00011 */
        MKVLC( 0x2, 5 ), /* str=00010 */
        MKVLC( 0x3, 6 ), /* str=000011 */
        MKVLC( 0x2, 6 ), /* str=000010 */
        MKVLC( 0x3, 7 ), /* str=0000011 */
        MKVLC( 0x2, 7 ), /* str=0000010 */
        MKVLC( 0x3, 8 ), /* str=00000011 */
        MKVLC( 0x2, 8 ), /* str=00000010 */
        MKVLC( 0x3, 9 ), /* str=000000011 */
        MKVLC( 0x2, 9 ), /* str=000000010 */
        MKVLC( 0x1, 9 ), /* str=000000001 */
    },
    { /* i_total 2 */
        MKVLC( 0x7, 3 ), /* str=111 */
        MKVLC( 0x6, 3 ), /* str=110 */
        MKVLC( 0x5, 3 ), /* str=101 */
        MKVLC( 0x4, 3 ), /* str=100 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x5, 4 ), /* str=0101 */
        MKVLC( 0x4, 4 ), /* str=0100 */
        MKVLC( 0x3, 4 ), /* str=0011 */
        MKVLC( 0x2, 4 ), /* str=0010 */
        MKVLC( 0x3, 5 ), /* str=00011 */
        MKVLC( 0x2, 5 ), /* str=00010 */
        MKVLC( 0x3, 6 ), /* str=000011 */
        MKVLC( 0x2, 6 ), /* str=000010 */
        MKVLC( 0x1, 6 ), /* str=000001 */
        MKVLC( 0x0, 6 ), /* str=000000 */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 3 */
        MKVLC( 0x5, 4 ), /* str=0101 */
        MKVLC( 0x7, 3 ), /* str=111 */
        MKVLC( 0x6, 3 ), /* str=110 */
        MKVLC( 0x5, 3 ), /* str=101 */
        MKVLC( 0x4, 4 ), /* str=0100 */
        MKVLC( 0x3, 4 ), /* str=0011 */
        MKVLC( 0x4, 3 ), /* str=100 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x2, 4 ), /* str=0010 */
        MKVLC( 0x3, 5 ), /* str=00011 */
        MKVLC( 0x2, 5 ), /* str=00010 */
        MKVLC( 0x1, 6 ), /* str=000001 */
        MKVLC( 0x1, 5 ), /* str=00001 */
        MKVLC( 0x0, 6 ), /* str=000000 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 4 */
        MKVLC( 0x3, 5 ), /* str=00011 */
        MKVLC( 0x7, 3 ), /* str=111 */
        MKVLC( 0x5, 4 ), /* str=0101 */
        MKVLC( 0x4, 4 ), /* str=0100 */
        MKVLC( 0x6, 3 ), /* str=110 */
        MKVLC( 0x5, 3 ), /* str=101 */
        MKVLC( 0x4, 3 ), /* str=100 */
        MKVLC( 0x3, 4 ), /* str=0011 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x2, 4 ), /* str=0010 */
        MKVLC( 0x2, 5 ), /* str=00010 */
        MKVLC( 0x1, 5 ), /* str=00001 */
        MKVLC( 0x0, 5 ), /* str=00000 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 5 */
        MKVLC( 0x5, 4 ), /* str=0101 */
        MKVLC( 0x4, 4 ), /* str=0100 */
        MKVLC( 0x3, 4 ), /* str=0011 */
        MKVLC( 0x7, 3 ), /* str=111 */
        MKVLC( 0x6, 3 ), /* str=110 */
        MKVLC( 0x5, 3 ), /* str=101 */
        MKVLC( 0x4, 3 ), /* str=100 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x2, 4 ), /* str=0010 */
        MKVLC( 0x1, 5 ), /* str=00001 */
        MKVLC( 0x1, 4 ), /* str=0001 */
        MKVLC( 0x0, 5 ), /* str=00000 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 6 */
        MKVLC( 0x1, 6 ), /* str=000001 */
        MKVLC( 0x1, 5 ), /* str=00001 */
        MKVLC( 0x7, 3 ), /* str=111 */
        MKVLC( 0x6, 3 ), /* str=110 */
        MKVLC( 0x5, 3 ), /* str=101 */
        MKVLC( 0x4, 3 ), /* str=100 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x2, 3 ), /* str=010 */
        MKVLC( 0x1, 4 ), /* str=0001 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x0, 6 ), /* str=000000 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 7 */
        MKVLC( 0x1, 6 ), /* str=000001 */
        MKVLC( 0x1, 5 ), /* str=00001 */
        MKVLC( 0x5, 3 ), /* str=101 */
        MKVLC( 0x4, 3 ), /* str=100 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x3, 2 ), /* str=11 */
        MKVLC( 0x2, 3 ), /* str=010 */
        MKVLC( 0x1, 4 ), /* str=0001 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x0, 6 ), /* str=000000 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 8 */
        MKVLC( 0x1, 6 ), /* str=000001 */
        MKVLC( 0x1, 4 ), /* str=0001 */
        MKVLC( 0x1, 5 ), /* str=00001 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x3, 2 ), /* str=11 */
        MKVLC( 0x2, 2 ), /* str=10 */
        MKVLC( 0x2, 3 ), /* str=010 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x0, 6 ), /* str=000000 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 9 */
        MKVLC( 0x1, 6 ), /* str=000001 */
        MKVLC( 0x0, 6 ), /* str=000000 */
        MKVLC( 0x1, 4 ), /* str=0001 */
        MKVLC( 0x3, 2 ), /* str=11 */
        MKVLC( 0x2, 2 ), /* str=10 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x1, 2 ), /* str=01 */
        MKVLC( 0x1, 5 ), /* str=00001 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 10 */
        MKVLC( 0x1, 5 ), /* str=00001 */
        MKVLC( 0x0, 5 ), /* str=00000 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x3, 2 ), /* str=11 */
        MKVLC( 0x2, 2 ), /* str=10 */
        MKVLC( 0x1, 2 ), /* str=01 */
        MKVLC( 0x1, 4 ), /* str=0001 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 11 */
        MKVLC( 0x0, 4 ), /* str=0000 */
        MKVLC( 0x1, 4 ), /* str=0001 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x2, 3 ), /* str=010 */
        MKVLC( 0x1, 1 ), /* str=1 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 12 */
        MKVLC( 0x0, 4 ), /* str=0000 */
        MKVLC( 0x1, 4 ), /* str=0001 */
        MKVLC( 0x1, 2 ), /* str=01 */
        MKVLC( 0x1, 1 ), /* str=1 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 13 */
        MKVLC( 0x0, 3 ), /* str=000 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x1, 1 ), /* str=1 */
        MKVLC( 0x1, 2 ), /* str=01 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 14 */
        MKVLC( 0x0, 2 ), /* str=00 */
        MKVLC( 0x1, 2 ), /* str=01 */
        MKVLC( 0x1, 1 ), /* str=1 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_total 15 */
        MKVLC( 0x0, 1 ), /* str=0 */
        MKVLC( 0x1, 1 ), /* str=1 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
};

/* [i_total_coeff-1][i_total_zeros] */
const vlc_t x264_total_zeros_dc[3][4] =
{
    {
        MKVLC( 0x01, 1 ), /* 1  */
        MKVLC( 0x01, 2 ), /* 01 */
        MKVLC( 0x01, 3 ), /* 001*/
        MKVLC( 0x00, 3 )  /* 000*/
    },
    {
        MKVLC( 0x01, 1 ), /* 1  */
        MKVLC( 0x01, 2 ), /* 01 */
        MKVLC( 0x00, 2 ), /* 00 */
        MKVLC( 0x00, 0 )  /*    */
    },
    {
        MKVLC( 0x01, 1 ), /* 1  */
        MKVLC( 0x00, 1 ), /* 0  */
        MKVLC( 0x00, 0 ), /*    */
        MKVLC( 0x00, 0 )  /*    */
    }
};

/* x264_run_before[__MIN( i_zero_left -1, 6 )][run_before] */
const vlc_t x264_run_before[7][16] =
{
    { /* i_zero_left 1 */
        MKVLC( 0x1, 1 ), /* str=1 */
        MKVLC( 0x0, 1 ), /* str=0 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_zero_left 2 */
        MKVLC( 0x1, 1 ), /* str=1 */
        MKVLC( 0x1, 2 ), /* str=01 */
        MKVLC( 0x0, 2 ), /* str=00 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_zero_left 3 */
        MKVLC( 0x3, 2 ), /* str=11 */
        MKVLC( 0x2, 2 ), /* str=10 */
        MKVLC( 0x1, 2 ), /* str=01 */
        MKVLC( 0x0, 2 ), /* str=00 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_zero_left 4 */
        MKVLC( 0x3, 2 ), /* str=11 */
        MKVLC( 0x2, 2 ), /* str=10 */
        MKVLC( 0x1, 2 ), /* str=01 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x0, 3 ), /* str=000 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_zero_left 5 */
        MKVLC( 0x3, 2 ), /* str=11 */
        MKVLC( 0x2, 2 ), /* str=10 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x2, 3 ), /* str=010 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x0, 3 ), /* str=000 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_zero_left 6 */
        MKVLC( 0x3, 2 ), /* str=11 */
        MKVLC( 0x0, 3 ), /* str=000 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x2, 3 ), /* str=010 */
        MKVLC( 0x5, 3 ), /* str=101 */
        MKVLC( 0x4, 3 ), /* str=100 */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
        MKVLC( 0x0, 0 ), /* str= */
    },
    { /* i_zero_left 7 */
        MKVLC( 0x7, 3 ), /* str=111 */
        MKVLC( 0x6, 3 ), /* str=110 */
        MKVLC( 0x5, 3 ), /* str=101 */
        MKVLC( 0x4, 3 ), /* str=100 */
        MKVLC( 0x3, 3 ), /* str=011 */
        MKVLC( 0x2, 3 ), /* str=010 */
        MKVLC( 0x1, 3 ), /* str=001 */
        MKVLC( 0x1, 4 ), /* str=0001 */
        MKVLC( 0x1, 5 ), /* str=00001 */
        MKVLC( 0x1, 6 ), /* str=000001 */
        MKVLC( 0x1, 7 ), /* str=0000001 */
        MKVLC( 0x1, 8 ), /* str=00000001 */
        MKVLC( 0x1, 9 ), /* str=000000001 */
        MKVLC( 0x1, 10 ), /* str=0000000001 */
        MKVLC( 0x1, 11 ), /* str=00000000001 */
    },
};

vlc_large_t x264_level_token[7][LEVEL_TABLE_SIZE];

void x264_init_vlc_tables()
{
    int16_t level;
    int i_suffix;
    for( i_suffix = 0; i_suffix < 7; i_suffix++ )
        for( level = -LEVEL_TABLE_SIZE/2; level < LEVEL_TABLE_SIZE/2; level++ )
        {
            int mask = level >> 15;
            int abs_level = (level^mask)-mask;
            int i_level_code = abs_level*2-mask-2;
            int i_next = i_suffix;
            vlc_large_t *vlc = &x264_level_token[i_suffix][level+LEVEL_TABLE_SIZE/2];

            if( ( i_level_code >> i_suffix ) < 14 )
            {
                vlc->i_size = (i_level_code >> i_suffix) + 1 + i_suffix;
                vlc->i_bits = (1<<i_suffix) + (i_level_code & ((1<<i_suffix)-1));
            }
            else if( i_suffix == 0 && i_level_code < 30 )
            {
                vlc->i_size = 19;
                vlc->i_bits = (1<<4) + (i_level_code - 14);
            }
            else if( i_suffix > 0 && ( i_level_code >> i_suffix ) == 14 )
            {
                vlc->i_size = 15 + i_suffix;
                vlc->i_bits = (1<<i_suffix) + (i_level_code & ((1<<i_suffix)-1));
            }
            else
            {
                i_level_code -= 15 << i_suffix;
                if( i_suffix == 0 )
                    i_level_code -= 15;
                vlc->i_size = 28;
                vlc->i_bits = (1<<12) + i_level_code;
            }
            if( i_next == 0 )
                i_next++;
            if( abs_level > (3 << (i_next-1)) && i_next < 6 )
                i_next++;
            vlc->i_next = i_next;
        }
}
