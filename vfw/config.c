/*****************************************************************************
 * config.c: vfw x264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: config.c,v 1.1 2004/06/03 19:27:09 fenrir Exp $
 *
 * Authors: Justin Clay
 *          Laurent Aimar <fenrir@via.ecp.fr>
 *          Antony Boucher <proximodo@free.fr>
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

/**************************************************************************
 *
 *  History:
 *
 *  2004.05.14  CBR encode mode support
 *
 **************************************************************************/

#include "x264vfw.h"
#include <stdio.h>  /* sprintf */
#include <commctrl.h>

#ifdef _MSC_VER
#define X264_VERSION ""
#else
#include "config.h"
#endif

/* Registry */
#define X264_REG_KEY    HKEY_CURRENT_USER
#define X264_REG_PARENT "Software\\GNU"
#define X264_REG_CHILD  "x264"
#define X264_REG_CLASS  "config"

/* window controls */
#define BITRATE_MAX        5000
#define QUANT_MAX        51

/* description */
#define X264_NAME        "x264"
#define X264_DEF_TEXT    "Are you sure you want to load default values?"

/* Registery handling */
typedef struct
{
    char *reg_value;
    int  *config_int;
    int  default_int;
} reg_int_t;

typedef struct
{
    char *reg_value;
    char *config_str;
    char *default_str;
    int max_len;  /* maximum string length, including the terminating NULL char */
} reg_str_t;

CONFIG reg;
HWND hTooltip;
HWND hTabs[8];
static const reg_int_t reg_int_table[] =
{
    /* Main dialog */
    { "bitrate",        &reg.bitrate,         800 },
    { "quantizer",      &reg.i_qp,             26 },
    { "encoding_type",  &reg.i_encoding_type,   1 },
    { "passbitrate",    &reg.i_2passbitrate,  800 },
    { "pass_number",    &reg.i_pass,            1 },
    { "fast1pass",      &reg.b_fast1pass,       1 },
    { "updatestats",    &reg.b_updatestats,     1 },

    /* Rate Control */
    { "key_boost",      &reg.i_key_boost,      40 },
    { "b_red",          &reg.i_b_red,          30 },
    { "curve_comp",     &reg.i_curve_comp,     60 },
    { "qp_min",         &reg.i_qp_min,         10 },
    { "qp_max",         &reg.i_qp_max,         51 },
    { "qp_step",        &reg.i_qp_step,         4 },
    { "scenecut",       &reg.i_scenecut_threshold, 40 },
    { "keyint_min",     &reg.i_keyint_min,     25 },
    { "keyint_max",     &reg.i_keyint_max,    250 },

    /* MBs&Frames */
    { "dct8x8",         &reg.b_dct8x8,          1 },
    { "psub16x16",      &reg.b_psub16x16,       1 },
    { "bsub16x16",      &reg.b_bsub16x16,       1 },
    { "psub8x8",        &reg.b_psub8x8,         0 },
    { "i8x8",           &reg.b_i8x8,            1 },
    { "i4x4",           &reg.b_i4x4,            1 },
    { "bmax",           &reg.i_bframe,          2 },
    { "b_bias",         &reg.i_bframe_bias,     0 },
    { "b_refs",         &reg.b_b_refs,          0 },
    { "b_adapt",        &reg.b_bframe_adaptive, 1 },
    { "b_bidir_me",     &reg.b_bidir_me,        0 },
    { "b_wpred",        &reg.b_b_wpred,         1 },
    { "direct_pred",    &reg.i_direct_mv_pred,  1 },

    /* analysis */
    { "subpel",         &reg.i_subpel_refine,   4 },
    { "me_method",      &reg.i_me_method,       1 },
    { "me_range",       &reg.i_me_range,       16 },
    { "chroma_me",      &reg.b_chroma_me,       1 },
    { "refmax",         &reg.i_refmax,          1 },
    { "mixedref",       &reg.b_mixedref,        0 },
    { "sar_width",      &reg.i_sar_width,       1 },
    { "sar_height",     &reg.i_sar_height,      1 },
    { "threads",        &reg.i_threads,         1 },
    { "cabac",          &reg.b_cabac,           1 },
    { "trellis",        &reg.i_trellis,         1 },
    { "noise_reduction",&reg.i_noise_reduction, 0 },
    { "loop_filter",    &reg.b_filter,          1 },
    { "inloop_a",       &reg.i_inloop_a,        0 },
    { "inloop_b",       &reg.i_inloop_b,        0 },
    { "log_level",      &reg.i_log_level,       1 }

};

static const reg_str_t reg_str_table[] =
{
    { "fourcc",         reg.fcc,         "H264",                5 },
    { "statsfile",      reg.stats,       ".\\x264.stats",       MAX_PATH-4 } // -4 because we add pass number
};

static void set_dlgitem_int(HWND hDlg, UINT item, int value)
{
    char buf[8];
    sprintf(buf, "%i", value);
    SetDlgItemText(hDlg, item, buf);
}


/* Registry access */

void config_reg_load( CONFIG *config )
{
    HKEY    hKey;
    DWORD   i_size;
    int     i;

    RegOpenKeyEx( X264_REG_KEY, X264_REG_PARENT "\\" X264_REG_CHILD,
                  0, KEY_READ, &hKey );

    /* Read all integers */
    for( i = 0; i < sizeof( reg_int_table )/sizeof( reg_int_t); i++ )
    {
        i_size = sizeof( int );
        if( RegQueryValueEx( hKey, reg_int_table[i].reg_value, 0, 0,
                             (LPBYTE)reg_int_table[i].config_int,
                             &i_size ) != ERROR_SUCCESS )
            *reg_int_table[i].config_int = reg_int_table[i].default_int;
    }

    /* Read strings */
    for( i = 0; i < sizeof( reg_str_table )/sizeof( reg_str_t); i++ )
    {
        i_size = reg_str_table[i].max_len;
        if( RegQueryValueEx( hKey, reg_str_table[i].reg_value, 0, 0,
                             (LPBYTE)reg_str_table[i].config_str,
                             &i_size ) != ERROR_SUCCESS )
            lstrcpy( reg_str_table[i].config_str,
                     reg_str_table[i].default_str );
    }

    RegCloseKey( hKey );

    memcpy( config, &reg, sizeof( CONFIG ) );
}

void config_reg_save( CONFIG *config )
{
    HKEY    hKey;
    DWORD   i_size;
    int     i;

    if( RegCreateKeyEx( X264_REG_KEY,
                        X264_REG_PARENT "\\" X264_REG_CHILD,
                        0,
                        X264_REG_CLASS,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        0, &hKey, &i_size ) != ERROR_SUCCESS )
        return;

    memcpy( &reg, config, sizeof( CONFIG ) );

    /* Save all integers */
    for( i = 0; i < sizeof( reg_int_table )/sizeof( reg_int_t); i++ )
    {
        RegSetValueEx( hKey, reg_int_table[i].reg_value, 0, REG_DWORD,
                       (LPBYTE)reg_int_table[i].config_int, sizeof( int ) );
    }

    /* Save strings */
    for( i = 0; i < sizeof( reg_str_table )/sizeof( reg_str_t); i++ )
    {
        RegSetValueEx( hKey, reg_str_table[i].reg_value, 0, REG_SZ,
                       (LPBYTE)reg_str_table[i].config_str,
                       lstrlen(reg_str_table[i].config_str)+1 );
    }

    RegCloseKey( hKey );
}

void config_reg_defaults( CONFIG *config )
{
    HKEY hKey;

    if(RegOpenKeyEx( X264_REG_KEY, X264_REG_PARENT, 0, KEY_ALL_ACCESS, &hKey )) {
        return;
    }
    if( RegDeleteKey( hKey, X264_REG_CHILD ) ) {
        return;
    }
    RegCloseKey( hKey );

    /* Just in case */
    memset( config, 0, sizeof( CONFIG ) );

    config_reg_load( config );
    config_reg_save( config );
}


/* assigns tooltips */

BOOL CALLBACK enum_tooltips(HWND hWnd, LPARAM lParam)
{
    char help[500];

	/* The tooltip for a control is named the same as the control itself */
    if (LoadString(g_hInst, GetDlgCtrlID(hWnd), help, 500))
    {
        TOOLINFO ti;

        ti.cbSize = sizeof(TOOLINFO);
        ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
        ti.hwnd = GetParent(hWnd);
        ti.uId  = (LPARAM)hWnd;
        ti.lpszText = help;

        SendMessage(hTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
    }

    return TRUE;
}


/* Main window */

BOOL CALLBACK callback_main( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    TCITEM tie;
    CONFIG* config = (CONFIG*)GetWindowLong(hDlg, GWL_USERDATA);

    switch( uMsg )
    {
    case WM_INITDIALOG :
        {
            RECT rect;
            HWND hTabCtrl = GetDlgItem( hDlg, IDC_TAB1 );
            SetWindowLong( hDlg, GWL_USERDATA, lParam );
            config = (CONFIG*)lParam;

            // insert tabs in tab control
            tie.mask = TCIF_TEXT;
            tie.iImage = -1;
            tie.pszText = "Bitrate";         TabCtrl_InsertItem(hTabCtrl, 0, &tie);
            tie.pszText = "Rate Control";    TabCtrl_InsertItem(hTabCtrl, 1, &tie);
            tie.pszText = "MBs&&Frames";     TabCtrl_InsertItem(hTabCtrl, 2, &tie);
            tie.pszText = "More...";         TabCtrl_InsertItem(hTabCtrl, 3, &tie);
            hTabs[0] = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_TAB_BITRATE),     hDlg, (DLGPROC)callback_tabs, lParam);
            hTabs[1] = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_TAB_RATECONTROL), hDlg, (DLGPROC)callback_tabs, lParam);
            hTabs[2] = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_TAB_IPFRAMES),    hDlg, (DLGPROC)callback_tabs, lParam);
            hTabs[3] = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_TAB_MISC),        hDlg, (DLGPROC)callback_tabs, lParam);
            GetClientRect(hDlg, &rect);
            TabCtrl_AdjustRect(hTabCtrl, FALSE, &rect);
            MoveWindow(hTabs[0], rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top-40, TRUE);
            MoveWindow(hTabs[1], rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top-40, TRUE);
            MoveWindow(hTabs[2], rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top-40, TRUE);
            MoveWindow(hTabs[3], rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top-40, TRUE);

            if ((hTooltip = CreateWindow(TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                NULL, NULL, g_hInst, NULL)))
            {
                SetWindowPos(hTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
                SendMessage(hTooltip, TTM_SETMAXTIPWIDTH, 0, 400);
                EnumChildWindows(hDlg, enum_tooltips, 0);
            }

            tabs_enable_items( hDlg, config );
            tabs_update_items( hDlg, config );
            ShowWindow( hTabs[0], SW_SHOW );
            BringWindowToTop( hTabs[0] );
            UpdateWindow( hDlg );
            break;
        }

    case WM_NOTIFY:
        {
            NMHDR FAR *tem = (NMHDR FAR *)lParam;
            if (tem->code == TCN_SELCHANGING)
            {
                HWND hTabCtrl = GetDlgItem( hDlg, IDC_TAB1 );
                int num = TabCtrl_GetCurSel(hTabCtrl);
                ShowWindow( hTabs[num], SW_HIDE );
                UpdateWindow( hDlg );
            }
            else if (tem->code == TCN_SELCHANGE)
            {
                HWND hTabCtrl = GetDlgItem( hDlg, IDC_TAB1 );
                int num = TabCtrl_GetCurSel(hTabCtrl);
                ShowWindow( hTabs[num], SW_SHOW );
                BringWindowToTop( hTabs[num] );
                UpdateWindow( hDlg );
            }
            break;
        }

    case WM_COMMAND:
        switch ( HIWORD( wParam ) )
        {
        case BN_CLICKED :
            switch( LOWORD( wParam ) )
            {
            case IDOK :
                config->b_save = TRUE;
                EndDialog( hDlg, LOWORD(wParam) );
                break;

            case IDCANCEL :
                config->b_save = FALSE;
                EndDialog( hDlg, LOWORD(wParam) );
                break;

            case IDC_DEFAULTS :
                if( MessageBox( hDlg, X264_DEF_TEXT, X264_NAME, MB_YESNO ) == IDYES )
                {
                    config_reg_defaults( config );
                    tabs_enable_items( hDlg, config );
                    tabs_update_items( hDlg, config );
                }
                break;
            }
        }

    default :
        return 0;
    }

    return 1;
}


/* Tabs */

void tabs_enable_items( HWND hDlg, CONFIG * config )
{
    char szTmp[1024];
    sprintf( szTmp, "Core %d%s, build %s %s", X264_BUILD, X264_VERSION, __DATE__, __TIME__ );
    SetDlgItemText( hTabs[0], IDC_BUILDREV,  szTmp );

    switch( config->i_encoding_type )
    {
    case 0 : /* 1 Pass, Bitrate Based */
        SetDlgItemText( hTabs[0], IDC_BITRATELABEL, "Average Bitrate" );
        SetDlgItemText( hTabs[0], IDC_BITRATELOW, "0" );
        sprintf(szTmp, "%d", BITRATE_MAX);
        SetDlgItemText( hTabs[0], IDC_BITRATEHIGH, szTmp );
        SendDlgItemMessage( hTabs[0], IDC_BITRATESLIDER, TBM_SETRANGE, TRUE,
                            (LPARAM) MAKELONG( 0, BITRATE_MAX ) );
        EnableWindow( GetDlgItem( hTabs[0], IDC_UPDATESTATS ), FALSE );
        EnableWindow( GetDlgItem( hTabs[0], IDC_STATSFILE ), FALSE );
        EnableWindow( GetDlgItem( hTabs[0], IDC_STATSFILE_BROWSE ), FALSE );
        EnableWindow( GetDlgItem( hTabs[1], IDC_CURVECOMP ), TRUE );
        EnableWindow( GetDlgItem( hTabs[1], IDC_QPMIN ), TRUE );
        EnableWindow( GetDlgItem( hTabs[1], IDC_QPMAX ), TRUE );
        EnableWindow( GetDlgItem( hTabs[1], IDC_QPSTEP ), TRUE );
        break;

    case 1 : /* 1 Pass, Quantizer Based */
        SetDlgItemText( hTabs[0], IDC_BITRATELABEL, "Quantizer" );
        SetDlgItemText( hTabs[0], IDC_BITRATELOW, "0 (High Quality)" );
        sprintf(szTmp, "(Low Quality) %d", QUANT_MAX);
        SetDlgItemText( hTabs[0], IDC_BITRATEHIGH, szTmp );
        SendDlgItemMessage( hTabs[0], IDC_BITRATESLIDER, TBM_SETRANGE, TRUE,
                            (LPARAM) MAKELONG( 0, QUANT_MAX ) );
        EnableWindow( GetDlgItem( hTabs[0], IDC_UPDATESTATS ), FALSE );
        EnableWindow( GetDlgItem( hTabs[0], IDC_STATSFILE ), FALSE );
        EnableWindow( GetDlgItem( hTabs[0], IDC_STATSFILE_BROWSE ), FALSE );
        EnableWindow( GetDlgItem( hTabs[1], IDC_CURVECOMP ), FALSE );
        EnableWindow( GetDlgItem( hTabs[1], IDC_QPMIN ), FALSE );
        EnableWindow( GetDlgItem( hTabs[1], IDC_QPMAX ), FALSE );
        EnableWindow( GetDlgItem( hTabs[1], IDC_QPSTEP ), FALSE );
        break;

    case 2 : /* 2 Pass */
        SetDlgItemText( hTabs[0], IDC_BITRATELABEL, "Target Bitrate" );
        SetDlgItemText( hTabs[0], IDC_BITRATELOW, "0" );
        sprintf(szTmp, "%d", BITRATE_MAX);
        SetDlgItemText( hTabs[0], IDC_BITRATEHIGH, szTmp );
        SendDlgItemMessage( hTabs[0], IDC_BITRATESLIDER, TBM_SETRANGE, TRUE,
                            (LPARAM) MAKELONG( 0, BITRATE_MAX ) );
        EnableWindow( GetDlgItem( hTabs[0], IDC_UPDATESTATS ), TRUE );
        EnableWindow( GetDlgItem( hTabs[0], IDC_STATSFILE ), TRUE );
        EnableWindow( GetDlgItem( hTabs[0], IDC_STATSFILE_BROWSE ), TRUE );
        EnableWindow( GetDlgItem( hTabs[1], IDC_CURVECOMP ), TRUE );
        EnableWindow( GetDlgItem( hTabs[1], IDC_QPMIN ), TRUE );
        EnableWindow( GetDlgItem( hTabs[1], IDC_QPMAX ), TRUE );
        EnableWindow( GetDlgItem( hTabs[1], IDC_QPSTEP ), TRUE );
        break;
    }

    EnableWindow( GetDlgItem( hTabs[2], IDC_DIRECTPRED  ), config->i_bframe > 0 );
    EnableWindow( GetDlgItem( hTabs[3], IDC_INLOOP_A    ), config->b_filter );
    EnableWindow( GetDlgItem( hTabs[3], IDC_INLOOP_B    ), config->b_filter );
    EnableWindow( GetDlgItem( hTabs[2], IDC_P4X4        ), config->b_psub16x16 );
    EnableWindow( GetDlgItem( hTabs[2], IDC_I8X8        ), config->b_dct8x8 );
    EnableWindow( GetDlgItem( hTabs[2], IDC_B8X8        ), config->i_bframe > 0 );
    EnableWindow( GetDlgItem( hTabs[2], IDC_BREFS       ), config->i_bframe > 1 );
    EnableWindow( GetDlgItem( hTabs[2], IDC_WBPRED      ), config->i_bframe > 1 );
    EnableWindow( GetDlgItem( hTabs[2], IDC_BADAPT      ), config->i_bframe > 0 );
    EnableWindow( GetDlgItem( hTabs[2], IDC_BIDIR_ME    ), config->i_bframe > 0 );
    EnableWindow( GetDlgItem( hTabs[2], IDC_BBIAS       ), config->i_bframe > 0 && config->b_bframe_adaptive );
    EnableWindow( GetDlgItem( hTabs[2], IDC_BBIASSLIDER ), config->i_bframe > 0 && config->b_bframe_adaptive );
    EnableWindow( GetDlgItem( hTabs[1], IDC_PBRATIO     ), config->i_bframe > 0 );
    EnableWindow( GetDlgItem( hTabs[3], IDC_MERANGE     ), config->i_me_method > 1 );
    EnableWindow( GetDlgItem( hTabs[3], IDC_CHROMAME    ), config->i_subpel_refine >= 4 );
    EnableWindow( GetDlgItem( hTabs[3], IDC_TRELLIS     ), config->b_cabac );
    EnableWindow( GetDlgItem( hTabs[3], IDC_MIXEDREF    ), config->i_refmax > 1 );
}

void tabs_update_items( HWND hDlg, CONFIG * config )
{
    char fourcc[5];

    /* update bitrate tab */
    if (SendMessage( GetDlgItem(hTabs[0],IDC_BITRATEMODE), CB_GETCOUNT, 0, 0 ) == 0)
    {
        SendDlgItemMessage(hTabs[0], IDC_BITRATEMODE, CB_ADDSTRING, 0, (LPARAM)"Single Pass - Bitrate");
        SendDlgItemMessage(hTabs[0], IDC_BITRATEMODE, CB_ADDSTRING, 0, (LPARAM)"Single Pass - Quantizer");
        SendDlgItemMessage(hTabs[0], IDC_BITRATEMODE, CB_ADDSTRING, 0, (LPARAM)"Multipass - First Pass");
        SendDlgItemMessage(hTabs[0], IDC_BITRATEMODE, CB_ADDSTRING, 0, (LPARAM)"Multipass - First Pass (fast)");
        SendDlgItemMessage(hTabs[0], IDC_BITRATEMODE, CB_ADDSTRING, 0, (LPARAM)"Multipass - Nth Pass");
    }
    switch( config->i_encoding_type )
    {
    case 0 : /* 1 Pass, Bitrate Based */
        SendDlgItemMessage(hTabs[0], IDC_BITRATEMODE, CB_SETCURSEL, 0, 0);
        SetDlgItemInt( hTabs[0], IDC_BITRATEEDIT, config->bitrate, FALSE );
        SendDlgItemMessage(hTabs[0], IDC_BITRATESLIDER, TBM_SETPOS, TRUE,
                           config->bitrate );
        break;
    case 1 : /* 1 Pass, Quantizer Based */
        SendDlgItemMessage(hTabs[0], IDC_BITRATEMODE, CB_SETCURSEL, 1, 0);
        SetDlgItemInt( hTabs[0], IDC_BITRATEEDIT, config->i_qp, FALSE );
        SendDlgItemMessage(hTabs[0], IDC_BITRATESLIDER, TBM_SETPOS, TRUE,
                           config->i_qp );
        break;
    case 2 : /* 2 Pass */
        if (config->i_pass >= 2) {
            SendDlgItemMessage(hTabs[0], IDC_BITRATEMODE, CB_SETCURSEL, 4, 0);
        } else if (config->b_fast1pass) {
            SendDlgItemMessage(hTabs[0], IDC_BITRATEMODE, CB_SETCURSEL, 3, 0);
        } else {
            SendDlgItemMessage(hTabs[0], IDC_BITRATEMODE, CB_SETCURSEL, 2, 0);
        }
        SetDlgItemInt( hTabs[0], IDC_BITRATEEDIT, config->i_2passbitrate, FALSE );
        SendDlgItemMessage(hTabs[0], IDC_BITRATESLIDER, TBM_SETPOS, TRUE,
                           config->i_2passbitrate );
        break;
    }

    CheckDlgButton( hTabs[0], IDC_UPDATESTATS, config->b_updatestats ? BST_CHECKED : BST_UNCHECKED );
    SetDlgItemText( hTabs[0], IDC_STATSFILE, config->stats );

    /* update rate control tab */
    if (SendMessage( GetDlgItem(hTabs[2],IDC_DIRECTPRED), CB_GETCOUNT, 0, 0 ) == 0)
    {
        SendDlgItemMessage(hTabs[2], IDC_DIRECTPRED, CB_ADDSTRING, 0, (LPARAM)"Spatial");
        SendDlgItemMessage(hTabs[2], IDC_DIRECTPRED, CB_ADDSTRING, 0, (LPARAM)"Temporal");
    }
    SetDlgItemInt( hTabs[1], IDC_QPMIN, config->i_qp_min, FALSE );
    SetDlgItemInt( hTabs[1], IDC_QPMAX, config->i_qp_max, FALSE );
    SetDlgItemInt( hTabs[1], IDC_QPSTEP, config->i_qp_step, FALSE );
    SetDlgItemInt( hTabs[1], IDC_IPRATIO, config->i_key_boost, FALSE );
    SetDlgItemInt( hTabs[1], IDC_PBRATIO, config->i_b_red, FALSE );
    SetDlgItemInt( hTabs[1], IDC_CURVECOMP, config->i_curve_comp, FALSE );

    /* update debug tab */
    if (SendMessage( GetDlgItem(hTabs[3],IDC_LOG), CB_GETCOUNT, 0, 0 ) == 0)
    {
        SendDlgItemMessage(hTabs[3], IDC_LOG, CB_ADDSTRING, 0, (LPARAM)"None");
        SendDlgItemMessage(hTabs[3], IDC_LOG, CB_ADDSTRING, 0, (LPARAM)"Error");
        SendDlgItemMessage(hTabs[3], IDC_LOG, CB_ADDSTRING, 0, (LPARAM)"Warning");
        SendDlgItemMessage(hTabs[3], IDC_LOG, CB_ADDSTRING, 0, (LPARAM)"Info");
        SendDlgItemMessage(hTabs[3], IDC_LOG, CB_ADDSTRING, 0, (LPARAM)"Debug");
    }
    SendDlgItemMessage(hTabs[3], IDC_LOG, CB_SETCURSEL, (config->i_log_level), 0);

    memcpy( fourcc, config->fcc, 4 );
    fourcc[4] = '\0';
    SetDlgItemText( hTabs[3], IDC_FOURCC, fourcc );

    /* update misc. tab */
    SetDlgItemInt( hTabs[3], IDC_THREADEDIT, config->i_threads, FALSE );
    SetDlgItemInt( hTabs[3], IDC_NR, config->i_noise_reduction, FALSE );
    CheckDlgButton( hTabs[3],IDC_CABAC,
                    config->b_cabac ? BST_CHECKED : BST_UNCHECKED );
    CheckDlgButton( hTabs[3],IDC_TRELLIS,
                    config->i_trellis ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hTabs[3],IDC_LOOPFILTER,
                    config->b_filter ? BST_CHECKED: BST_UNCHECKED );

    SetDlgItemInt( hTabs[3], IDC_SAR_W, config->i_sar_width,  FALSE );
    SetDlgItemInt( hTabs[3], IDC_SAR_H, config->i_sar_height, FALSE );

    SendDlgItemMessage( hTabs[3], IDC_INLOOP_A, TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG( -6, 6 ) );
    SendDlgItemMessage( hTabs[3], IDC_INLOOP_A, TBM_SETPOS, TRUE,
                        config->i_inloop_a );
    set_dlgitem_int( hTabs[3], IDC_LOOPA_TXT, config->i_inloop_a);
    SendDlgItemMessage( hTabs[3], IDC_INLOOP_B, TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG( -6, 6 ) );
    SendDlgItemMessage( hTabs[3], IDC_INLOOP_B, TBM_SETPOS, TRUE,
                        config->i_inloop_b );
    set_dlgitem_int( hTabs[3], IDC_LOOPB_TXT, config->i_inloop_b);

    /* update i/p-frames tab */
    CheckDlgButton( hTabs[2],IDC_P8X8,
                    config->b_psub16x16 ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hTabs[2],IDC_P4X4,
                    config->b_psub8x8 ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hTabs[2],IDC_I4X4,
                    config->b_i4x4 ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hTabs[2],IDC_I8X8,
                    config->b_i8x8 ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hTabs[2],IDC_DCT8X8,
                    config->b_dct8x8 ? BST_CHECKED: BST_UNCHECKED );

    /* update b-frames tab */
    CheckDlgButton( hTabs[2],IDC_WBPRED,
                    config->b_b_wpred ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hTabs[2],IDC_BADAPT,
                    config->b_bframe_adaptive ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hTabs[2],IDC_BIDIR_ME,
                    config->b_bidir_me ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hTabs[2],IDC_BREFS,
                    config->b_b_refs ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hTabs[2],IDC_B8X8,
                    config->b_bsub16x16 ? BST_CHECKED: BST_UNCHECKED );
    SetDlgItemInt( hTabs[2], IDC_BFRAME, config->i_bframe, FALSE );
    SetDlgItemInt( hTabs[2], IDC_BBIAS, config->i_bframe_bias, TRUE );
    SendDlgItemMessage( hTabs[2], IDC_BBIASSLIDER, TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG( -100, 100 ) );
    SendDlgItemMessage( hTabs[2], IDC_BBIASSLIDER, TBM_SETPOS, TRUE,
                        config->i_bframe_bias );
    SendDlgItemMessage(hTabs[2], IDC_DIRECTPRED, CB_SETCURSEL, (config->i_direct_mv_pred), 0);

    /* update scene-cuts tab */
    SetDlgItemInt( hTabs[1], IDC_KEYINTMIN, config->i_keyint_min, FALSE );
    SetDlgItemInt( hTabs[1], IDC_KEYINTMAX, config->i_keyint_max, FALSE );
    SetDlgItemInt( hTabs[1], IDC_SCENECUT, config->i_scenecut_threshold, TRUE );

    /* update motion estimation tab */
    if (SendMessage( GetDlgItem(hTabs[3],IDC_ME_METHOD), CB_GETCOUNT, 0, 0 ) == 0)
    {
        SendDlgItemMessage(hTabs[3], IDC_ME_METHOD, CB_ADDSTRING, 0, (LPARAM)"Diamond Search");
        SendDlgItemMessage(hTabs[3], IDC_ME_METHOD, CB_ADDSTRING, 0, (LPARAM)"Hexagonal Search");
        SendDlgItemMessage(hTabs[3], IDC_ME_METHOD, CB_ADDSTRING, 0, (LPARAM)"Uneven Multi-Hexagon");
        SendDlgItemMessage(hTabs[3], IDC_ME_METHOD, CB_ADDSTRING, 0, (LPARAM)"Exhaustive Search");
        SendDlgItemMessage(hTabs[3], IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"1 (Fastest)");
        SendDlgItemMessage(hTabs[3], IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"2");
        SendDlgItemMessage(hTabs[3], IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"3");
        SendDlgItemMessage(hTabs[3], IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"4");
        SendDlgItemMessage(hTabs[3], IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"5 (High Quality)");
        SendDlgItemMessage(hTabs[3], IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"6 (RDO)");
        SendDlgItemMessage(hTabs[3], IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"6b (RDO on B-frames)");
    }

    SendDlgItemMessage(hTabs[3], IDC_ME_METHOD, CB_SETCURSEL, (config->i_me_method), 0);
    SendDlgItemMessage(hTabs[3], IDC_SUBPEL, CB_SETCURSEL, (config->i_subpel_refine), 0);
    SetDlgItemInt( hTabs[3], IDC_MERANGE, config->i_me_range, FALSE );
    CheckDlgButton( hTabs[3],IDC_CHROMAME,
                    config->b_chroma_me ? BST_CHECKED: BST_UNCHECKED );
    SetDlgItemInt( hTabs[3], IDC_REFFRAMES, config->i_refmax, FALSE );
    CheckDlgButton( hTabs[3],IDC_MIXEDREF,
                    config->b_mixedref ? BST_CHECKED: BST_UNCHECKED );
}

BOOL CALLBACK callback_tabs( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CONFIG* config = (CONFIG*)GetWindowLong(hDlg, GWL_USERDATA);
    switch( uMsg )
    {
    case WM_INITDIALOG :

        SetWindowLong( hDlg, GWL_USERDATA, lParam );
        config = (CONFIG*)lParam;
        break;

    case WM_COMMAND:

        switch ( HIWORD( wParam ) )
        {
        case BN_CLICKED :

            switch( LOWORD( wParam ) )
            {
            case IDC_CABAC :
                config->b_cabac = ( IsDlgButtonChecked( hTabs[3], IDC_CABAC ) == BST_CHECKED );
                break;
            case IDC_TRELLIS :
                config->i_trellis = ( IsDlgButtonChecked( hTabs[3], IDC_TRELLIS ) == BST_CHECKED );
                break;
            case IDC_LOOPFILTER :
                config->b_filter = ( IsDlgButtonChecked( hTabs[3], IDC_LOOPFILTER ) == BST_CHECKED );
                break;
            case IDC_BREFS :
                config->b_b_refs = ( IsDlgButtonChecked( hTabs[2], IDC_BREFS ) == BST_CHECKED );
                break;
            case IDC_WBPRED :
                config->b_b_wpred = ( IsDlgButtonChecked( hTabs[2], IDC_WBPRED ) == BST_CHECKED );
                break;
            case IDC_BADAPT :
                config->b_bframe_adaptive = ( IsDlgButtonChecked( hTabs[2], IDC_BADAPT ) == BST_CHECKED );
                break;
            case IDC_BIDIR_ME :
                config->b_bidir_me = ( IsDlgButtonChecked( hTabs[2], IDC_BIDIR_ME ) == BST_CHECKED );
                break;
            case IDC_P8X8 :
                config->b_psub16x16 = ( IsDlgButtonChecked( hTabs[2], IDC_P8X8 ) == BST_CHECKED );
                break;
            case IDC_P4X4 :
                config->b_psub8x8 = ( IsDlgButtonChecked( hTabs[2], IDC_P4X4 ) == BST_CHECKED );
                break;
            case IDC_B8X8 :
                config->b_bsub16x16 = ( IsDlgButtonChecked( hTabs[2], IDC_B8X8 ) == BST_CHECKED );
                break;
            case IDC_I4X4 :
                config->b_i4x4 = ( IsDlgButtonChecked( hTabs[2], IDC_I4X4 ) == BST_CHECKED );
                break;
            case IDC_I8X8 :
                config->b_i8x8 = ( IsDlgButtonChecked( hTabs[2], IDC_I8X8 ) == BST_CHECKED );
                break;
            case IDC_DCT8X8 :
                config->b_dct8x8 = ( IsDlgButtonChecked( hTabs[2], IDC_DCT8X8 ) == BST_CHECKED );
                break;
            case IDC_MIXEDREF :
                config->b_mixedref = ( IsDlgButtonChecked( hTabs[3], IDC_MIXEDREF ) == BST_CHECKED );
                break;
            case IDC_CHROMAME :
                config->b_chroma_me = ( IsDlgButtonChecked( hTabs[3], IDC_CHROMAME ) == BST_CHECKED );
                break;
            case IDC_UPDATESTATS :
                config->b_updatestats = ( IsDlgButtonChecked( hTabs[0], IDC_UPDATESTATS ) == BST_CHECKED );
                break;
            case IDC_STATSFILE_BROWSE :
                {
                OPENFILENAME ofn;
                char tmp[MAX_PATH];

                GetDlgItemText( hTabs[0], IDC_STATSFILE, tmp, MAX_PATH );

                memset( &ofn, 0, sizeof( OPENFILENAME ) );
                ofn.lStructSize = sizeof( OPENFILENAME );

                ofn.hwndOwner = hDlg;
                ofn.lpstrFilter = "Statsfile (*.stats)\0*.stats\0All files (*.*)\0*.*\0\0";
                ofn.lpstrFile = tmp;
                ofn.nMaxFile = MAX_PATH-4;
                ofn.Flags = OFN_PATHMUSTEXIST;

                if( config->i_pass == 1 )
                    ofn.Flags |= OFN_OVERWRITEPROMPT;
                else ofn.Flags |= OFN_FILEMUSTEXIST;

                if( ( config->i_pass == 1 && GetSaveFileName( &ofn ) ) ||
                    ( config->i_pass > 1 && GetOpenFileName( &ofn ) ) )
                    SetDlgItemText( hTabs[0], IDC_STATSFILE, tmp );
                }
                break;
            }
            break;

        case EN_CHANGE :

            switch( LOWORD( wParam ) )
            {
            case IDC_FOURCC :
                GetDlgItemText( hTabs[3], IDC_FOURCC, config->fcc, 5 );
                break;
            case IDC_NR :
                config->i_noise_reduction = GetDlgItemInt( hTabs[3], IDC_NR, FALSE, FALSE );
            case IDC_THREADEDIT :
                config->i_threads = GetDlgItemInt( hTabs[3], IDC_THREADEDIT, FALSE, FALSE );
                if (config->i_threads < 1)
                {
                    config->i_threads = 1;
                    SetDlgItemInt( hTabs[3], IDC_THREADEDIT, config->i_threads, FALSE );
                }
                else if (config->i_threads > 4)
                {
                    config->i_threads = 4;
                    SetDlgItemInt( hTabs[3], IDC_THREADEDIT, config->i_threads, FALSE );
                }
                break;
            case IDC_BITRATEEDIT :
                switch (config->i_encoding_type)
                {
                case 0:
                    config->bitrate = GetDlgItemInt( hTabs[0], IDC_BITRATEEDIT, FALSE, FALSE );
                    SendDlgItemMessage( hTabs[0], IDC_BITRATESLIDER, TBM_SETPOS, TRUE, config->bitrate );
                    break;
                case 1:
                    config->i_qp = GetDlgItemInt( hTabs[0], IDC_BITRATEEDIT, FALSE, FALSE );
                    SendDlgItemMessage( hTabs[0], IDC_BITRATESLIDER, TBM_SETPOS, TRUE, config->i_qp );
                    break;
                case 2:
                    config->i_2passbitrate = GetDlgItemInt( hTabs[0], IDC_BITRATEEDIT, FALSE, FALSE );
                    SendDlgItemMessage( hTabs[0], IDC_BITRATESLIDER, TBM_SETPOS, TRUE, config->i_2passbitrate );
                    break;
                }
                break;
            case IDC_STATSFILE :
                if( GetDlgItemText( hTabs[0], IDC_STATSFILE, config->stats, MAX_PATH ) == 0 )
                    lstrcpy( config->stats, ".\\x264.stats" );
                break;
            case IDC_KEYINTMIN :
                config->i_keyint_min = GetDlgItemInt( hTabs[1], IDC_KEYINTMIN, FALSE, FALSE );
                break;
            case IDC_KEYINTMAX :
                config->i_keyint_max = GetDlgItemInt( hTabs[1], IDC_KEYINTMAX, FALSE, FALSE );
                break;
            case IDC_SCENECUT :
                config->i_scenecut_threshold = GetDlgItemInt( hTabs[1], IDC_SCENECUT, FALSE, TRUE );
                if( config->i_scenecut_threshold > 100 )
                {
                    config->i_scenecut_threshold = 100;
                    SetDlgItemInt( hTabs[1], IDC_SCENECUT, config->i_scenecut_threshold, TRUE );
                } else if ( config->i_scenecut_threshold < -1 )
                {
                    config->i_scenecut_threshold = -1;
                    SetDlgItemInt( hTabs[1], IDC_SCENECUT, config->i_scenecut_threshold, TRUE );
                }
                break;
            case IDC_QPMIN :
                config->i_qp_min = GetDlgItemInt( hTabs[1], IDC_QPMIN, FALSE, FALSE );
                if( config->i_qp_min > 51 )
                {
                    config->i_qp_min = 51;
                    SetDlgItemInt( hTabs[1], IDC_QPMIN, config->i_qp_min, FALSE );
                } else if ( config->i_qp_min < 1 )
                {
                    config->i_qp_min = 1;
                    SetDlgItemInt( hTabs[1], IDC_QPMIN, config->i_qp_min, FALSE );
                }
                break;
            case IDC_QPMAX :
                config->i_qp_max = GetDlgItemInt( hTabs[1], IDC_QPMAX, FALSE, FALSE );
                if( config->i_qp_max > 51 )
                {
                    config->i_qp_max = 51;
                    SetDlgItemInt( hTabs[1], IDC_QPMAX, config->i_qp_max, FALSE );
                } else if ( config->i_qp_max < 1 )
                {
                    config->i_qp_max = 1;
                    SetDlgItemInt( hTabs[1], IDC_QPMAX, config->i_qp_max, FALSE );
                }
                break;
            case IDC_QPSTEP :
                config->i_qp_step = GetDlgItemInt( hTabs[1], IDC_QPSTEP, FALSE, FALSE );
                if( config->i_qp_step > 50 )
                {
                    config->i_qp_step = 50;
                    SetDlgItemInt( hTabs[1], IDC_QPSTEP, config->i_qp_step, FALSE );
                } else if ( config->i_qp_step < 1 )
                {
                    config->i_qp_step = 1;
                    SetDlgItemInt( hTabs[1], IDC_QPSTEP, config->i_qp_step, FALSE );
                }
                break;
            case IDC_SAR_W :
                config->i_sar_width = GetDlgItemInt( hTabs[3], IDC_SAR_W, FALSE, FALSE );
                break;
            case IDC_SAR_H :
                config->i_sar_height = GetDlgItemInt( hTabs[3], IDC_SAR_H, FALSE, FALSE );
                break;
            case IDC_REFFRAMES :
                config->i_refmax = GetDlgItemInt( hTabs[3], IDC_REFFRAMES, FALSE, FALSE );
                if( config->i_refmax > 16 )
                {
                    config->i_refmax = 16;
                    SetDlgItemInt( hTabs[3], IDC_REFFRAMES, config->i_refmax, FALSE );
                }
                break;
            case IDC_MERANGE :
                config->i_me_range = GetDlgItemInt( hTabs[3], IDC_MERANGE, FALSE, FALSE );
                if( config->i_me_range > 64 )
                {
                    config->i_me_range = 64;
                    SetDlgItemInt( hTabs[3], IDC_MERANGE, config->i_me_range, FALSE );
                }
                break;

            case IDC_BFRAME :
                config->i_bframe = GetDlgItemInt( hTabs[2], IDC_BFRAME, FALSE, FALSE );
                if( config->i_bframe > 5 )
                {
                    config->i_bframe = 5;
                    SetDlgItemInt( hTabs[2], IDC_BFRAME, config->i_bframe, FALSE );
                }
                break;
            case IDC_BBIAS :
                config->i_bframe_bias = GetDlgItemInt( hTabs[2], IDC_BBIAS, FALSE, TRUE );
                SendDlgItemMessage(hTabs[2], IDC_BBIASSLIDER, TBM_SETPOS, 1,
                config->i_bframe_bias);
                if( config->i_bframe_bias > 100 )
                {
                    config->i_bframe_bias = 100;
                    SetDlgItemInt( hTabs[2], IDC_BBIAS, config->i_bframe_bias, TRUE );
                } else if ( config->i_bframe_bias < -100 )
                {
                    config->i_bframe_bias = -100;
                    SetDlgItemInt( hTabs[2], IDC_BBIAS, config->i_bframe_bias, TRUE );
                }
                break;
            case IDC_IPRATIO :
                config->i_key_boost = GetDlgItemInt( hTabs[1], IDC_IPRATIO, FALSE, FALSE );
                if (config->i_key_boost < 0)
                {
                    config->i_key_boost = 0;
                    SetDlgItemInt( hTabs[1], IDC_IPRATIO, config->i_key_boost, FALSE );
                }
                else if (config->i_key_boost > 70)
                {
                    config->i_key_boost = 70;
                    SetDlgItemInt( hTabs[1], IDC_IPRATIO, config->i_key_boost, FALSE );
                }
                break;
            case IDC_PBRATIO :
                config->i_b_red = GetDlgItemInt( hTabs[1], IDC_PBRATIO, FALSE, FALSE );
                if (config->i_b_red < 0)
                {
                    config->i_b_red = 0;
                    SetDlgItemInt( hTabs[1], IDC_PBRATIO, config->i_b_red, FALSE );
                }
                else if (config->i_b_red > 60)
                {
                    config->i_b_red = 60;
                    SetDlgItemInt( hTabs[1], IDC_PBRATIO, config->i_b_red, FALSE );
                }
                break;
            case IDC_CURVECOMP:
                config->i_curve_comp = GetDlgItemInt( hTabs[1], IDC_CURVECOMP, FALSE, FALSE );
                if( config->i_curve_comp < 0 )
                {
                    config->i_curve_comp = 0;
                    SetDlgItemInt( hTabs[1], IDC_CURVECOMP, config->i_curve_comp, FALSE );
                }
                else if( config->i_curve_comp > 100 )
                {
                    config->i_curve_comp = 100;
                    SetDlgItemInt( hTabs[1], IDC_CURVECOMP, config->i_curve_comp, FALSE );
                }
                break;
            }
            break;

        case LBN_SELCHANGE :

            switch ( LOWORD( wParam ) )
            {
            case IDC_DIRECTPRED:
                config->i_direct_mv_pred = SendDlgItemMessage(hTabs[2], IDC_DIRECTPRED, CB_GETCURSEL, 0, 0);
                break;
            case IDC_SUBPEL:
                config->i_subpel_refine = SendDlgItemMessage(hTabs[3], IDC_SUBPEL, CB_GETCURSEL, 0, 0);
                break;
            case IDC_ME_METHOD:
                config->i_me_method = SendDlgItemMessage(hTabs[3], IDC_ME_METHOD, CB_GETCURSEL, 0, 0);
                break;
            case IDC_LOG:
                config->i_log_level = SendDlgItemMessage(hTabs[3], IDC_LOG, CB_GETCURSEL, 0, 0);
                break;
            case IDC_BITRATEMODE:
                switch(SendDlgItemMessage(hTabs[0], IDC_BITRATEMODE, CB_GETCURSEL, 0, 0))
                {
                case 0:
                    config->i_encoding_type = 0;
                    break;
                case 1:
                    config->i_encoding_type = 1;
                    break;
                case 2:
                    config->i_encoding_type = 2;
                    config->i_pass = 1;
                    config->b_fast1pass = FALSE;
                    break;
                case 3:
                    config->i_encoding_type = 2;
                    config->i_pass = 1;
                    config->b_fast1pass = TRUE;
                    break;
                case 4:
                    config->i_encoding_type = 2;
                    config->i_pass = 2;
                    break;
                }
                tabs_update_items( hDlg, config );
                break;
            }
            break;

        case EN_KILLFOCUS :

            switch( LOWORD( wParam ) )
            {
            case IDC_MERANGE :
                config->i_me_range = GetDlgItemInt( hTabs[3], IDC_MERANGE, FALSE, FALSE );
                if( config->i_me_range < 4 )
                {
                    config->i_me_range = 4;
                    SetDlgItemInt( hTabs[3], IDC_MERANGE, config->i_me_range, FALSE );
                }
                break;
            }
            break;
        }
        break;

    case WM_HSCROLL :

        if( (HWND) lParam == GetDlgItem( hTabs[0], IDC_BITRATESLIDER ) )
        {
            switch (config->i_encoding_type)
            {
            case 0:
                config->bitrate = SendDlgItemMessage( hTabs[0], IDC_BITRATESLIDER, TBM_GETPOS, 0, 0 );
                SetDlgItemInt( hTabs[0], IDC_BITRATEEDIT, config->bitrate, FALSE );
                break;
            case 1:
                config->i_qp = SendDlgItemMessage( hTabs[0], IDC_BITRATESLIDER, TBM_GETPOS, 0, 0 );
                SetDlgItemInt( hTabs[0], IDC_BITRATEEDIT, config->i_qp, FALSE );
                break;
            case 2:
                config->i_2passbitrate = SendDlgItemMessage( hTabs[0], IDC_BITRATESLIDER, TBM_GETPOS, 0, 0 );
                SetDlgItemInt( hTabs[0], IDC_BITRATEEDIT, config->i_2passbitrate, FALSE );
                break;
            }
            break;
        }
        else if( (HWND) lParam == GetDlgItem( hTabs[3], IDC_INLOOP_A ) )
        {
            config->i_inloop_a = SendDlgItemMessage( hTabs[3], IDC_INLOOP_A, TBM_GETPOS, 0, 0 );
            set_dlgitem_int( hTabs[3], IDC_LOOPA_TXT, config->i_inloop_a);
        }
        else if( (HWND) lParam == GetDlgItem( hTabs[3], IDC_INLOOP_B ) )
        {
            config->i_inloop_b = SendDlgItemMessage( hTabs[3], IDC_INLOOP_B, TBM_GETPOS, 0, 0 );
            set_dlgitem_int( hTabs[3], IDC_LOOPB_TXT, config->i_inloop_b);
        }
        else if( (HWND) lParam == GetDlgItem( hTabs[2], IDC_BBIASSLIDER ) )
        {
            config->i_bframe_bias = SendDlgItemMessage( hTabs[2], IDC_BBIASSLIDER, TBM_GETPOS, 0, 0 );
            set_dlgitem_int( hTabs[2], IDC_BBIAS, config->i_bframe_bias);
        }
        break;

    default :
        return 0;
    }

    tabs_enable_items( hDlg, config );
    return 1;
}


/* About box */

BOOL CALLBACK callback_about( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
    case WM_INITDIALOG :
    {
        char temp[1024];
        sprintf( temp, "Core %d%s, build %s %s", X264_BUILD, X264_VERSION, __DATE__, __TIME__ );
        SetDlgItemText( hDlg, IDC_BUILD,  temp );
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_HOMEPAGE && HIWORD(wParam) == STN_CLICKED)
            ShellExecute( hDlg, "open", X264_WEBSITE, NULL, NULL, SW_SHOWNORMAL );
        else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            EndDialog( hDlg, LOWORD(wParam) );
        break;

    default :
        return 0;
    }

    return 1;
}


/* Error console */

BOOL CALLBACK callback_err_console( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
    case WM_INITDIALOG :
        break;
    case WM_DESTROY :
        break;
    case WM_COMMAND :
        if( HIWORD( wParam ) == BN_CLICKED ) {
            switch( LOWORD( wParam ) ) {
            case IDOK :
                DestroyWindow( hWnd );
                break;
            case IDC_COPYCLIP :
                if( OpenClipboard( hWnd ) )
                    {
                        int i;
                        int num_lines = SendDlgItemMessage( hWnd, IDC_CONSOLE,
                                        LB_GETCOUNT, 0, 0 );
                        int text_size;
                        char *buffer;
                        HGLOBAL clipbuffer;

                        if( num_lines <= 0 )
                            break;

                        /* calculate text size */
                        for( i = 0, text_size = 0; i < num_lines; i++ )
                            text_size += SendDlgItemMessage( hWnd, IDC_CONSOLE,
                                   LB_GETTEXTLEN, ( WPARAM )i, 0 );

                        /* CR-LF for each line + terminating NULL */
                        text_size += 2 * num_lines + 1;

                        EmptyClipboard( );
                        clipbuffer = GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE,
                                     text_size );
                        buffer = (char *)GlobalLock( clipbuffer );

                        /* concatenate lines of text in the global buffer */
                        for( i = 0; i < num_lines; i++ )
                        {
                            char msg_buf[1024];

                            SendDlgItemMessage( hWnd, IDC_CONSOLE, LB_GETTEXT,
                                              ( WPARAM )i, ( LPARAM )msg_buf );
                            strcat( msg_buf, "\r\n" );
                            memcpy( buffer, msg_buf, strlen( msg_buf ) );
                            buffer += strlen( msg_buf );
                        }
                        *buffer = 0; /* null-terminate the buffer */

                        GlobalUnlock( clipbuffer );
                        SetClipboardData( CF_TEXT, clipbuffer );
                        CloseClipboard( );
                    }
                break;
            default :
                return 0;
            }
            break;
        }
        break;

    default :
        return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

    return 0;
}


