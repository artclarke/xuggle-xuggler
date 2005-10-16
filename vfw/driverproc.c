/*****************************************************************************
 * drvproc.c: vfw x264 wrapper
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: driverproc.c,v 1.1 2004/06/03 19:27:09 fenrir Exp $
 *
 * Authors: Justin Clay
 *          Laurent Aimar <fenrir@via.ecp.fr>
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

#include "x264vfw.h"

/* Global dll instance */
HINSTANCE g_hInst;


/* Calling back point for our DLL so we can keep track of the window in g_hInst */
BOOL WINAPI DllMain( HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
{
    g_hInst = (HINSTANCE) hModule;
    return TRUE;
}

/* This little puppy handles the calls which vfw programs send out to the codec */
LRESULT WINAPI DriverProc( DWORD dwDriverId, HDRVR hDriver, UINT uMsg, LPARAM lParam1, LPARAM lParam2 )
{
    CODEC *codec = (CODEC *)dwDriverId;

    switch( uMsg )
    {
        case DRV_LOAD:
        case DRV_FREE:
            return DRV_OK;

        case DRV_OPEN:
        {
            ICOPEN *icopen = (ICOPEN *)lParam2;

            if( icopen != NULL && icopen->fccType != ICTYPE_VIDEO )
                return DRV_CANCEL;

            if( ( codec = malloc( sizeof( CODEC ) ) ) == NULL )
            {
                if( icopen != NULL )
                    icopen->dwError = ICERR_MEMORY;
                return 0;
            }

            memset( codec, 0, sizeof( CODEC ) );
            config_reg_load( &codec->config );
            codec->h = NULL;

            if( icopen != NULL )
                icopen->dwError = ICERR_OK;
            return (LRESULT)codec;
        }

        case DRV_CLOSE:
            /* From xvid: compress_end/decompress_end don't always get called */
            compress_end(codec);
            free( codec );
            return DRV_OK;

        case DRV_DISABLE:
        case DRV_ENABLE:
            return DRV_OK;

        case DRV_INSTALL:
        case DRV_REMOVE:
            return DRV_OK;

        case DRV_QUERYCONFIGURE:
        case DRV_CONFIGURE:
            return DRV_CANCEL;

        /* info */
        case ICM_GETINFO:
        {
            ICINFO *icinfo = (ICINFO *)lParam1;

            /* return a description */
            icinfo->fccType      = ICTYPE_VIDEO;
            icinfo->fccHandler   = FOURCC_X264;
            icinfo->dwFlags      = VIDCF_COMPRESSFRAMES | VIDCF_FASTTEMPORALC;

            icinfo->dwVersion    = 0;
            icinfo->dwVersionICM = ICVERSION;

            wcscpy( icinfo->szName, X264_NAME_L);
            wcscpy( icinfo->szDescription, X264_DESC_L);

            return lParam2; /* size of struct */
        }

        case ICM_ABOUT:
            if( lParam1 != -1 )
            {
                DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_ABOUT), (HWND)lParam1, callback_about, 0 );
            }
            return ICERR_OK;

        case ICM_CONFIGURE:
            if( lParam1 != -1 )
            {
                CONFIG temp;

                codec->config.b_save = FALSE;
                memcpy( &temp, &codec->config, sizeof(CONFIG) );

                DialogBoxParam( g_hInst, MAKEINTRESOURCE(IDD_MAINCONFIG), (HWND)lParam1, callback_main, (LPARAM)&temp );

                if( temp.b_save )
                {
                    memcpy( &codec->config, &temp, sizeof(CONFIG) );
                    config_reg_save( &codec->config );
                }
            }
            return ICERR_OK;

        case ICM_GETSTATE:
            if( (void*)lParam1 == NULL )
            {
                return sizeof( CONFIG );
            }
            memcpy( (void*)lParam1, &codec->config, sizeof( CONFIG ) );
            return ICERR_OK;

        case ICM_SETSTATE:
            if( (void*)lParam1 == NULL )
            {
                config_reg_load( &codec->config );
                return 0;
            }
            memcpy( &codec->config, (void*)lParam1, sizeof( CONFIG ) );
            return 0;

        /* not sure the difference, private/public data? */
        case ICM_GET:
        case ICM_SET:
            return ICERR_OK;


        /* older-stype config */
        case ICM_GETDEFAULTQUALITY:
        case ICM_GETQUALITY:
        case ICM_SETQUALITY:
        case ICM_GETBUFFERSWANTED:
        case ICM_GETDEFAULTKEYFRAMERATE:
            return ICERR_UNSUPPORTED;


        /* compressor */
        case ICM_COMPRESS_QUERY:
            return compress_query(codec, (BITMAPINFO *)lParam1, (BITMAPINFO *)lParam2);

        case ICM_COMPRESS_GET_FORMAT:
            return compress_get_format(codec, (BITMAPINFO *)lParam1, (BITMAPINFO *)lParam2);

        case ICM_COMPRESS_GET_SIZE:
            return compress_get_size(codec, (BITMAPINFO *)lParam1, (BITMAPINFO *)lParam2);

        case ICM_COMPRESS_FRAMES_INFO:
            return compress_frames_info(codec, (ICCOMPRESSFRAMES *)lParam1);

        case ICM_COMPRESS_BEGIN:
            return compress_begin(codec, (BITMAPINFO *)lParam1, (BITMAPINFO *)lParam2);

        case ICM_COMPRESS_END:
            return compress_end(codec);

        case ICM_COMPRESS:
            return compress(codec, (ICCOMPRESS *)lParam1);

        /* decompressor : not implemented */
        case ICM_DECOMPRESS_QUERY:
        case ICM_DECOMPRESS_GET_FORMAT:
        case ICM_DECOMPRESS_BEGIN:
        case ICM_DECOMPRESS_END:
        case ICM_DECOMPRESS:
        case ICM_DECOMPRESS_GET_PALETTE:
        case ICM_DECOMPRESS_SET_PALETTE:
        case ICM_DECOMPRESSEX_QUERY:
        case ICM_DECOMPRESSEX_BEGIN:
        case ICM_DECOMPRESSEX_END:
        case ICM_DECOMPRESSEX:
            return ICERR_UNSUPPORTED;

        default:
        if (uMsg < DRV_USER)
            return DefDriverProc(dwDriverId, hDriver, uMsg, lParam1, lParam2);
        else 
            return ICERR_UNSUPPORTED;
    }
}

void WINAPI Configure(HWND hwnd, HINSTANCE hinst, LPTSTR lpCmdLine, int nCmdShow)
{
    DWORD dwDriverId;

    dwDriverId = DriverProc(0, 0, DRV_OPEN, 0, 0);
    if (dwDriverId != (DWORD)NULL)
    {
        DriverProc(dwDriverId, 0, ICM_CONFIGURE, (LPARAM)GetDesktopWindow(), 0);
        DriverProc(dwDriverId, 0, DRV_CLOSE, 0, 0);
    }
}
