/*****************************************************************************
 * config.c: vfw x264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: config.c,v 1.1 2004/06/03 19:27:09 fenrir Exp $
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
static const reg_int_t reg_int_table[] =
{
    /* Main dialog */
    { "bitrate",        &reg.bitrate,           800 },
    { "quantizer",      &reg.i_qp,              26 },
    { "encoding_type",  &reg.i_encoding_type,   1 },
    { "passbitrate",    &reg.i_2passbitrate,    800 },
    { "pass_number",    &reg.i_pass,            1 },
    { "fast1pass",      &reg.b_fast1pass,       1 },
    { "updatestats",    &reg.b_updatestats,     1 },
    { "threads",        &reg.i_threads,         1 },

    /* Advance dialog */
    { "cabac",          &reg.b_cabac,           1 },
    { "loop_filter",    &reg.b_filter,          1 },
    { "keyint_max",     &reg.i_keyint_max,      250 },
    { "keyint_min",     &reg.i_keyint_min,      25 },
    { "scenecut",       &reg.i_scenecut_threshold, 40 },
    { "qp_min",         &reg.i_qp_min,         10 },
    { "qp_max",         &reg.i_qp_max,         51 },
    { "qp_step",        &reg.i_qp_step,         4 },
    { "refmax",         &reg.i_refmax,          1 },
    { "bmax",           &reg.i_bframe,          2 },
    { "direct_pred",    &reg.i_direct_mv_pred,  1 },
    { "b_refs",         &reg.b_b_refs,          0 },
    { "b_bias",         &reg.i_bframe_bias,     0 },
    { "b_adapt",        &reg.b_bframe_adaptive, 1 },
    { "b_wpred",        &reg.b_b_wpred,         1 },
    { "inloop_a",       &reg.i_inloop_a,        0 },
    { "inloop_b",       &reg.i_inloop_b,        0 },
    { "key_boost",      &reg.i_key_boost,       40 },
    { "b_red",          &reg.i_b_red,           30 },
    { "curve_comp",     &reg.i_curve_comp,      60 },
    { "sar_width",      &reg.i_sar_width,       1 },
    { "sar_height",     &reg.i_sar_height,      1 },

    { "log_level",      &reg.i_log_level,       1 },

    /* analysis */
    { "i4x4",           &reg.b_i4x4,            1 },
    { "i8x8",           &reg.b_i8x8,            1 },
    { "dct8x8",         &reg.b_dct8x8,          0 },
    { "psub16x16",      &reg.b_psub16x16,       1 },
    { "psub8x8",        &reg.b_psub8x8,         1 },
    { "bsub16x16",      &reg.b_bsub16x16,       1 },
    { "me_method",      &reg.i_me_method,       1 },
    { "me_range",       &reg.i_me_range,       16 },
    { "chroma_me",      &reg.b_chroma_me,       1 },
    { "subpel",         &reg.i_subpel_refine,   4 }

};

static const reg_str_t reg_str_table[] =
{
    { "fourcc",         reg.fcc,         "H264",                5 },
    { "statsfile",      reg.stats,       ".\\x264.stats",       MAX_PATH-4 } // -4 because we add pass number
};

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

/* config_reg_defaults: */
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

/* Enables or Disables Window Elements based on Selection
 */
static void main_enable_item( HWND hDlg, CONFIG * config )
{
    switch( config->i_encoding_type )
    {
    case 0 : /* 1 Pass, Bitrate Based */
        EnableWindow( GetDlgItem( hDlg, IDC_BITRATEEDIT ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_BITRATESLIDER ), TRUE );

        EnableWindow( GetDlgItem( hDlg, IDC_QUANTEDIT ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_QUANTSLIDER ), FALSE );

        EnableWindow( GetDlgItem( hDlg, IDC_2PASS1 ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASS2 ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASSBITRATE ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASSBITRATE_S ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_FAST1PASS ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_UPDATESTATS ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_STATSFILE ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_STATSFILE_BROWSE ), FALSE );
        break;

    case 1 : /* 1 Pass, Quantizer Based */
        EnableWindow( GetDlgItem( hDlg, IDC_BITRATEEDIT ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_BITRATESLIDER ), FALSE );

        EnableWindow( GetDlgItem( hDlg, IDC_QUANTEDIT ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_QUANTSLIDER ), TRUE );

        EnableWindow( GetDlgItem( hDlg, IDC_2PASS1 ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASS2 ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASSBITRATE ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASSBITRATE_S ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_FAST1PASS ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_UPDATESTATS ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_STATSFILE ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_STATSFILE_BROWSE ), FALSE );
        break;
    
    case 2 : /* 2 Pass */
        EnableWindow( GetDlgItem( hDlg, IDC_BITRATEEDIT ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_BITRATESLIDER ), FALSE );

        EnableWindow( GetDlgItem( hDlg, IDC_QUANTEDIT ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_QUANTSLIDER ), FALSE );

        EnableWindow( GetDlgItem( hDlg, IDC_2PASS1 ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASS2 ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASSBITRATE ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASSBITRATE_S ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_FAST1PASS ), config->i_pass == 1);
        EnableWindow( GetDlgItem( hDlg, IDC_UPDATESTATS ), config->i_pass > 1 );
        EnableWindow( GetDlgItem( hDlg, IDC_STATSFILE ), config->i_pass == 1 );
        EnableWindow( GetDlgItem( hDlg, IDC_STATSFILE_BROWSE ), config->i_pass == 1 );
        break;
    }

    SendDlgItemMessage( hDlg, IDC_BITRATESLIDER, TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG( 0, BITRATE_MAX ) );
    SendDlgItemMessage( hDlg, IDC_QUANTSLIDER, TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG( 0, QUANT_MAX ) );
    SendDlgItemMessage( hDlg, IDC_2PASSBITRATE_S, TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG( 0, BITRATE_MAX ) );

}

/* Updates the window from config */
static void main_update_dlg( HWND hDlg, CONFIG * config )
{
    SetDlgItemInt( hDlg, IDC_BITRATEEDIT, config->bitrate, FALSE );
    SetDlgItemInt( hDlg, IDC_QUANTEDIT, config->i_qp, FALSE );
    SetDlgItemInt( hDlg, IDC_2PASSBITRATE, config->i_2passbitrate, FALSE );
    SetDlgItemText( hDlg, IDC_STATSFILE, config->stats );
    SetDlgItemInt( hDlg, IDC_THREADEDIT, config->i_threads, FALSE );

    switch( config->i_encoding_type )
    {
    case 0 : /* 1 Pass, Bitrate Based */
        CheckRadioButton( hDlg,
                          IDC_RADIOBITRATE, IDC_RADIOTWOPASS, IDC_RADIOBITRATE);
        break;
    case 1 : /* 1 Pass, Quantizer Based */
        CheckRadioButton( hDlg,
                          IDC_RADIOBITRATE, IDC_RADIOTWOPASS, IDC_RADIOQUANT);
            break;
    case 2 : /* 2 Pass */
        CheckRadioButton( hDlg,
                          IDC_RADIOBITRATE, IDC_RADIOTWOPASS, IDC_RADIOTWOPASS);

        if (config->i_pass == 1)
            CheckRadioButton(hDlg,
                             IDC_2PASS1, IDC_2PASS2, IDC_2PASS1);
        else 
            CheckRadioButton(hDlg,
                             IDC_2PASS1, IDC_2PASS2, IDC_2PASS2);
        EnableWindow( GetDlgItem( hDlg, IDC_2PASSBITRATE ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASSBITRATE_S ), TRUE );

        break;
    }

    SendDlgItemMessage( hDlg, IDC_BITRATESLIDER, TBM_SETPOS, TRUE,
                        config->bitrate );
    SendDlgItemMessage( hDlg, IDC_QUANTSLIDER, TBM_SETPOS, TRUE,
                        config->i_qp );
    SendDlgItemMessage( hDlg, IDC_2PASSBITRATE_S, TBM_SETPOS, TRUE,
                        config->i_2passbitrate );
    CheckDlgButton( hDlg, IDC_FAST1PASS, config->b_fast1pass ? BST_CHECKED : BST_UNCHECKED );
    CheckDlgButton( hDlg, IDC_UPDATESTATS, config->b_updatestats ? BST_CHECKED : BST_UNCHECKED );

}


/* Main config dialog */
BOOL CALLBACK callback_main( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CONFIG* config = (CONFIG*)GetWindowLong(hDlg, GWL_USERDATA);

    switch( uMsg )
    {
    case WM_INITDIALOG :
        SetWindowLong( hDlg, GWL_USERDATA, lParam );
        config = (CONFIG*)lParam;

        main_enable_item( hDlg, config );
        main_update_dlg( hDlg, config );

        break;

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
            case IDC_ADVANCED :
                DialogBoxParam( g_hInst, MAKEINTRESOURCE(IDD_ADVANCED),
                                (HWND)lParam, callback_advanced,
                                (LPARAM)config );
                break;
            case IDC_DEBUG :
                DialogBoxParam( g_hInst, MAKEINTRESOURCE(IDD_DEBUG),
                                (HWND)lParam, callback_debug,
                                (LPARAM)config );
                break;
            case IDC_DEFAULTS :
                if( MessageBox( hDlg, X264_DEF_TEXT, X264_NAME, MB_YESNO ) == IDYES )
                {
                    config_reg_defaults( config );
                    main_enable_item( hDlg, config );
                    main_update_dlg( hDlg, config );
                }
                break;
            case IDC_RADIOBITRATE :
                config->i_encoding_type = 0; /* 1 Pass, Bitrate Mode=0 */
                main_enable_item( hDlg, config );
                main_update_dlg( hDlg, config );
                break;
            case IDC_RADIOQUANT :
                config->i_encoding_type = 1; /* 1 Pass, Quantizer Mode=1 */
                main_enable_item( hDlg, config );
                main_update_dlg( hDlg, config );
                break;
            case IDC_RADIOTWOPASS :
                config->i_encoding_type = 2; /* 2 Pass Mode=2 */
                main_enable_item( hDlg,  config );
                main_update_dlg( hDlg, config );
                break;
            case IDC_2PASS1 :
                config->i_pass = 1; /* 1st pass */
                main_enable_item( hDlg,  config );
                main_update_dlg( hDlg, config );
                break;
            case IDC_2PASS2 :
                config->i_pass = 2; /* 2nd pass */
                main_enable_item( hDlg,  config );
                main_update_dlg( hDlg, config );
                break;
            case IDC_FAST1PASS :
                config->b_fast1pass = ( IsDlgButtonChecked( hDlg, IDC_FAST1PASS ) == BST_CHECKED );
                break;
            case IDC_UPDATESTATS :
                config->b_updatestats = ( IsDlgButtonChecked( hDlg, IDC_UPDATESTATS ) == BST_CHECKED );
                break;
            case IDC_STATSFILE_BROWSE :
                {
                OPENFILENAME ofn;
                char tmp[MAX_PATH];

                GetDlgItemText( hDlg, IDC_STATSFILE, tmp, MAX_PATH );

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
                    SetDlgItemText( hDlg, IDC_STATSFILE, tmp );
                }
                break;
            }
            break;
        case EN_CHANGE :
            switch( LOWORD( wParam ) )
            {
            case IDC_THREADEDIT :
                config->i_threads = GetDlgItemInt( hDlg, IDC_THREADEDIT, FALSE, FALSE );
                if (config->i_threads < 1)
                {
                    config->i_threads = 1;
                    SetDlgItemInt( hDlg, IDC_THREADEDIT, config->i_threads, FALSE );
                }
                else if (config->i_threads > 4)
                {
                    config->i_threads = 4;
                    SetDlgItemInt( hDlg, IDC_THREADEDIT, config->i_threads, FALSE );
                }                        
                break;
            case IDC_BITRATEEDIT :
                config->bitrate = GetDlgItemInt( hDlg, IDC_BITRATEEDIT, FALSE, FALSE );
                SendDlgItemMessage( hDlg, IDC_BITRATESLIDER, TBM_SETPOS, TRUE, config->bitrate );
                break;
            case IDC_QUANTEDIT :
                config->i_qp = GetDlgItemInt( hDlg, IDC_QUANTEDIT, FALSE, FALSE );
                SendDlgItemMessage( hDlg, IDC_QUANTSLIDER, TBM_SETPOS, TRUE, config->i_qp );
                break;
            case IDC_2PASSBITRATE :
                config->i_2passbitrate = GetDlgItemInt( hDlg, IDC_2PASSBITRATE, FALSE, FALSE );
                SendDlgItemMessage( hDlg, IDC_2PASSBITRATE_S, TBM_SETPOS, TRUE, config->i_2passbitrate );
                break;
            case IDC_STATSFILE :
                if( GetDlgItemText( hDlg, IDC_STATSFILE, config->stats, MAX_PATH ) == 0 )
                    lstrcpy( config->stats, ".\\x264.stats" );
                break;
            }
            break;
        default:
            break;
        }
        break;

        case WM_HSCROLL :
            if( (HWND) lParam == GetDlgItem( hDlg, IDC_BITRATESLIDER ) )
            {
                config->bitrate = SendDlgItemMessage( hDlg, IDC_BITRATESLIDER, TBM_GETPOS, 0, 0 );
                SetDlgItemInt( hDlg, IDC_BITRATEEDIT, config->bitrate, FALSE );

            }
            else if( (HWND) lParam == GetDlgItem( hDlg, IDC_QUANTSLIDER ) )
            {
                config->i_qp = SendDlgItemMessage( hDlg, IDC_QUANTSLIDER, TBM_GETPOS, 0, 0 );
                SetDlgItemInt( hDlg, IDC_QUANTEDIT, config->i_qp, FALSE );
            }
            else if( (HWND) lParam == GetDlgItem( hDlg, IDC_2PASSBITRATE_S ) )
            {
                config->i_2passbitrate = SendDlgItemMessage( hDlg, IDC_2PASSBITRATE_S, TBM_GETPOS, 0, 0 );
                SetDlgItemInt( hDlg, IDC_2PASSBITRATE, config->i_2passbitrate, FALSE );
            }
            break;

    default :
        return 0;
    }

    return 1;
}

/* About dialog */
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
static void debug_update_dlg( HWND hDlg, CONFIG * config )
{
    char fourcc[5];

    SendDlgItemMessage(hDlg, IDC_LOG, CB_ADDSTRING, 0, (LPARAM)"None");
    SendDlgItemMessage(hDlg, IDC_LOG, CB_ADDSTRING, 0, (LPARAM)"Error");
    SendDlgItemMessage(hDlg, IDC_LOG, CB_ADDSTRING, 0, (LPARAM)"Warning");
    SendDlgItemMessage(hDlg, IDC_LOG, CB_ADDSTRING, 0, (LPARAM)"Info");
    SendDlgItemMessage(hDlg, IDC_LOG, CB_ADDSTRING, 0, (LPARAM)"Debug");
    SendDlgItemMessage(hDlg, IDC_LOG, CB_SETCURSEL, (config->i_log_level), 0);

    memcpy( fourcc, config->fcc, 4 );
    fourcc[4] = '\0';
    SetDlgItemText( hDlg, IDC_FOURCC, fourcc );

}
BOOL CALLBACK callback_debug( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CONFIG* config = (CONFIG*)GetWindowLong(hDlg, GWL_USERDATA);

    switch( uMsg )
    {
    case WM_INITDIALOG :
        SetWindowLong( hDlg, GWL_USERDATA, lParam );
        config = (CONFIG*)lParam;
        debug_update_dlg( hDlg, config );
        break;

    case WM_COMMAND:
        switch ( HIWORD( wParam ) )
        {
        case BN_CLICKED :
            switch( LOWORD( wParam ) )
            {
            case IDOK :
                EndDialog( hDlg, LOWORD( wParam ) );
                break;
            case IDCANCEL :
                EndDialog( hDlg, LOWORD( wParam ) );
                break;
            }
        case EN_CHANGE :
            switch( LOWORD( wParam ) )
            {
            case IDC_FOURCC :
                GetDlgItemText( hDlg, IDC_FOURCC, config->fcc, 5 );
                break;
            }
            break;
            case LBN_SELCHANGE :
                switch ( LOWORD( wParam ) ) {
                case IDC_LOG:
                    config->i_log_level = SendDlgItemMessage(hDlg, IDC_LOG, CB_GETCURSEL, 0, 0);
                    break;
                }
            break;
        }
        break;

    default :
        return 0;
    }

    return 1;
}

static void set_dlgitem_int(HWND hDlg, UINT item, int value)
{
    char buf[8];
    sprintf(buf, "%i", value);
    SetDlgItemText(hDlg, item, buf);
}

static void adv_update_dlg( HWND hDlg, CONFIG * config )
{
    SendDlgItemMessage(hDlg, IDC_ME_METHOD, CB_ADDSTRING, 0, (LPARAM)"Diamond Search");
    SendDlgItemMessage(hDlg, IDC_ME_METHOD, CB_ADDSTRING, 0, (LPARAM)"Hexagonal Search");
    SendDlgItemMessage(hDlg, IDC_ME_METHOD, CB_ADDSTRING, 0, (LPARAM)"Uneven Multi-Hexagon");
    SendDlgItemMessage(hDlg, IDC_ME_METHOD, CB_ADDSTRING, 0, (LPARAM)"Exhaustive Search");

    SetDlgItemInt( hDlg, IDC_MERANGE, config->i_me_range, FALSE );
    CheckDlgButton( hDlg,IDC_CHROMAME,
                    config->b_chroma_me ? BST_CHECKED: BST_UNCHECKED );

    SendDlgItemMessage(hDlg, IDC_DIRECTPRED, CB_ADDSTRING, 0, (LPARAM)"Spatial");
    SendDlgItemMessage(hDlg, IDC_DIRECTPRED, CB_ADDSTRING, 0, (LPARAM)"Temporal");

    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"1 (Fastest)");
    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"2");
    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"3");
    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"4");
    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"5 (High Quality)");
    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"6 (RDO - Slowest)");

    CheckDlgButton( hDlg,IDC_CABAC,
                    config->b_cabac ? BST_CHECKED : BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_LOOPFILTER,
                    config->b_filter ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_WBPRED,
                    config->b_b_wpred ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_BADAPT,
                    config->b_bframe_adaptive ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_BREFS,
                    config->b_b_refs ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_P16X16,
                    config->b_psub16x16 ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_P8X8,
                    config->b_psub8x8 ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_B16X16,
                    config->b_bsub16x16 ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_I4X4,
                    config->b_i4x4 ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_I8X8,
                    config->b_i8x8 ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_DCT8X8,
                    config->b_dct8x8 ? BST_CHECKED: BST_UNCHECKED );

    SetDlgItemInt( hDlg, IDC_KEYINTMIN, config->i_keyint_min, FALSE );
    SetDlgItemInt( hDlg, IDC_KEYINTMAX, config->i_keyint_max, FALSE );
    SetDlgItemInt( hDlg, IDC_SCENECUT, config->i_scenecut_threshold, TRUE );
    SetDlgItemInt( hDlg, IDC_REFFRAMES, config->i_refmax, FALSE );
    SetDlgItemInt( hDlg, IDC_BFRAME, config->i_bframe, FALSE );
    SetDlgItemInt( hDlg, IDC_QPMIN, config->i_qp_min, FALSE );
    SetDlgItemInt( hDlg, IDC_QPMAX, config->i_qp_max, FALSE );
    SetDlgItemInt( hDlg, IDC_QPSTEP, config->i_qp_step, FALSE );

    SetDlgItemInt( hDlg, IDC_BBIAS, config->i_bframe_bias, TRUE );
    SendDlgItemMessage( hDlg, IDC_BBIASSLIDER, TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG( -100, 100 ) );
    SendDlgItemMessage( hDlg, IDC_BBIASSLIDER, TBM_SETPOS, TRUE,
                        config->i_bframe_bias );

    SetDlgItemInt( hDlg, IDC_SAR_W, config->i_sar_width,  FALSE );
    SetDlgItemInt( hDlg, IDC_SAR_H, config->i_sar_height, FALSE );

    SetDlgItemInt( hDlg, IDC_IPRATIO, config->i_key_boost, FALSE );
    SetDlgItemInt( hDlg, IDC_PBRATIO, config->i_b_red, FALSE );
    SetDlgItemInt( hDlg, IDC_CURVECOMP, config->i_curve_comp, FALSE );

    SendDlgItemMessage(hDlg, IDC_DIRECTPRED, CB_SETCURSEL, (config->i_direct_mv_pred), 0);
    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_SETCURSEL, (config->i_subpel_refine), 0);
    SendDlgItemMessage(hDlg, IDC_ME_METHOD, CB_SETCURSEL, (config->i_me_method), 0);

    SendDlgItemMessage( hDlg, IDC_INLOOP_A, TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG( -6, 6 ) );
    SendDlgItemMessage( hDlg, IDC_INLOOP_A, TBM_SETPOS, TRUE,
                        config->i_inloop_a );
    set_dlgitem_int( hDlg, IDC_LOOPA_TXT, config->i_inloop_a);
    SendDlgItemMessage( hDlg, IDC_INLOOP_B, TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG( -6, 6 ) );
    SendDlgItemMessage( hDlg, IDC_INLOOP_B, TBM_SETPOS, TRUE,
                        config->i_inloop_b );
    set_dlgitem_int( hDlg, IDC_LOOPB_TXT, config->i_inloop_b);

    EnableWindow( GetDlgItem( hDlg, IDC_P8X8 ), config->b_psub16x16 );
    EnableWindow( GetDlgItem( hDlg, IDC_BREFS ), config->i_bframe > 1 );
    EnableWindow( GetDlgItem( hDlg, IDC_WBPRED ), config->i_bframe > 1 );
    EnableWindow( GetDlgItem( hDlg, IDC_BADAPT ), config->i_bframe > 0 );
    EnableWindow( GetDlgItem( hDlg, IDC_BBIAS ), config->i_bframe > 0 );
    EnableWindow( GetDlgItem( hDlg, IDC_BBIASSLIDER ), config->i_bframe > 0 );
    EnableWindow( GetDlgItem( hDlg, IDC_DIRECTPRED ), config->i_bframe > 0 );
    EnableWindow( GetDlgItem( hDlg, IDC_I8X8 ), config->b_dct8x8 );
    EnableWindow( GetDlgItem( hDlg, IDC_MERANGE ), config->i_me_method > 1 );
    EnableWindow( GetDlgItem( hDlg, IDC_INLOOP_A ), config->b_filter );
    EnableWindow( GetDlgItem( hDlg, IDC_INLOOP_B ), config->b_filter );
    EnableWindow( GetDlgItem( hDlg, IDC_CHROMAME ), config->i_subpel_refine >= 4 );
}

/* advanced configuration dialog process */
BOOL CALLBACK callback_advanced( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CONFIG* config = (CONFIG*)GetWindowLong(hDlg, GWL_USERDATA);

    switch( uMsg )
    {
    case WM_INITDIALOG :
        SetWindowLong( hDlg, GWL_USERDATA, lParam );
        config = (CONFIG*)lParam;

        adv_update_dlg( hDlg, config );
        break;

    case WM_COMMAND:
        switch ( HIWORD( wParam ) )
        {
        case BN_CLICKED :
            switch( LOWORD( wParam ) )
            {
            case IDOK :
                EndDialog( hDlg, LOWORD( wParam ) );
                break;
            case IDC_CABAC :
                config->b_cabac = ( IsDlgButtonChecked( hDlg, IDC_CABAC ) == BST_CHECKED );
                break;
            case IDC_LOOPFILTER :
                config->b_filter = ( IsDlgButtonChecked( hDlg, IDC_LOOPFILTER ) == BST_CHECKED );
                EnableWindow( GetDlgItem( hDlg, IDC_INLOOP_A ), config->b_filter );
                EnableWindow( GetDlgItem( hDlg, IDC_INLOOP_B ), config->b_filter );
                break;
            case IDC_BREFS :
                config->b_b_refs = ( IsDlgButtonChecked( hDlg, IDC_BREFS ) == BST_CHECKED );
                break;
            case IDC_WBPRED :
                config->b_b_wpred = ( IsDlgButtonChecked( hDlg, IDC_WBPRED ) == BST_CHECKED );
                break;
            case IDC_BADAPT :
                config->b_bframe_adaptive = ( IsDlgButtonChecked( hDlg, IDC_BADAPT ) == BST_CHECKED );
                break;
            case IDC_P16X16 :
                config->b_psub16x16 = ( IsDlgButtonChecked( hDlg, IDC_P16X16 ) == BST_CHECKED );
                EnableWindow( GetDlgItem( hDlg, IDC_P8X8 ), config->b_psub16x16 );
                break;
            case IDC_P8X8 :
                config->b_psub8x8 = ( IsDlgButtonChecked( hDlg, IDC_P8X8 ) == BST_CHECKED );
                break;
            case IDC_B16X16 :
                config->b_bsub16x16 = ( IsDlgButtonChecked( hDlg, IDC_B16X16 ) == BST_CHECKED );
                break;
            case IDC_I4X4 :
                config->b_i4x4 = ( IsDlgButtonChecked( hDlg, IDC_I4X4 ) == BST_CHECKED );
                break;
            case IDC_I8X8 :
                config->b_i8x8 = ( IsDlgButtonChecked( hDlg, IDC_I8X8 ) == BST_CHECKED );
                break;
            case IDC_DCT8X8 :
                config->b_dct8x8 = ( IsDlgButtonChecked( hDlg, IDC_DCT8X8 ) == BST_CHECKED );
                EnableWindow( GetDlgItem( hDlg, IDC_I8X8 ), config->b_dct8x8 );
                break;
            case IDC_CHROMAME :
                config->b_chroma_me = ( IsDlgButtonChecked( hDlg, IDC_CHROMAME ) == BST_CHECKED );
                break;
            }
            break;
        case EN_KILLFOCUS :
        	  switch( LOWORD( wParam ) )
        	  {
            case IDC_MERANGE :
                config->i_me_range = GetDlgItemInt( hDlg, IDC_MERANGE, FALSE, FALSE );
                if( config->i_me_range < 4 )
                {
                    config->i_me_range = 4;
                    SetDlgItemInt( hDlg, IDC_MERANGE, config->i_me_range, FALSE );
                }
                break;
            }
            break;
        case EN_CHANGE :
            switch( LOWORD( wParam ) )
            {
            case IDC_KEYINTMIN :
                config->i_keyint_min = GetDlgItemInt( hDlg, IDC_KEYINTMIN, FALSE, FALSE );
                break;
            case IDC_KEYINTMAX :
                config->i_keyint_max = GetDlgItemInt( hDlg, IDC_KEYINTMAX, FALSE, FALSE );
                break;
            case IDC_SCENECUT :
                config->i_scenecut_threshold = GetDlgItemInt( hDlg, IDC_SCENECUT, FALSE, TRUE );
                if( config->i_scenecut_threshold > 100 )
                {
                    config->i_scenecut_threshold = 100;
                    SetDlgItemInt( hDlg, IDC_SCENECUT, config->i_scenecut_threshold, TRUE );
                } else if ( config->i_scenecut_threshold < -1 )
                {
                    config->i_scenecut_threshold = -1;
                    SetDlgItemInt( hDlg, IDC_SCENECUT, config->i_scenecut_threshold, TRUE );
                }
                break;
            case IDC_QPMIN :
                config->i_qp_min = GetDlgItemInt( hDlg, IDC_QPMIN, FALSE, FALSE );
                if( config->i_qp_min > 51 )
                {
                    config->i_qp_min = 51;
                    SetDlgItemInt( hDlg, IDC_QPMIN, config->i_qp_min, FALSE );
                } else if ( config->i_qp_min < 1 )
                {
                    config->i_qp_min = 1;
                    SetDlgItemInt( hDlg, IDC_QPMIN, config->i_qp_min, FALSE );
                }
                break;
            case IDC_QPMAX :
                config->i_qp_max = GetDlgItemInt( hDlg, IDC_QPMAX, FALSE, FALSE );
                if( config->i_qp_max > 51 )
                {
                    config->i_qp_max = 51;
                    SetDlgItemInt( hDlg, IDC_QPMAX, config->i_qp_max, FALSE );
                } else if ( config->i_qp_max < 1 )
                {
                    config->i_qp_max = 1;
                    SetDlgItemInt( hDlg, IDC_QPMAX, config->i_qp_max, FALSE );
                }
                break;
            case IDC_QPSTEP :
                config->i_qp_step = GetDlgItemInt( hDlg, IDC_QPSTEP, FALSE, FALSE );
                if( config->i_qp_step > 50 )
                {
                    config->i_qp_step = 50;
                    SetDlgItemInt( hDlg, IDC_QPSTEP, config->i_qp_step, FALSE );
                } else if ( config->i_qp_step < 1 )
                {
                    config->i_qp_step = 1;
                    SetDlgItemInt( hDlg, IDC_QPSTEP, config->i_qp_step, FALSE );
                }
                break;
            case IDC_SAR_W :
                config->i_sar_width = GetDlgItemInt( hDlg, IDC_SAR_W, FALSE, FALSE );
                break;
            case IDC_SAR_H :
                config->i_sar_height = GetDlgItemInt( hDlg, IDC_SAR_H, FALSE, FALSE );
                break;
            case IDC_REFFRAMES :
                config->i_refmax = GetDlgItemInt( hDlg, IDC_REFFRAMES, FALSE, FALSE );
                if( config->i_refmax > 16 )
                {
                    config->i_refmax = 16;
                    SetDlgItemInt( hDlg, IDC_REFFRAMES, config->i_refmax, FALSE );
                }
                break;
            case IDC_MERANGE :
                config->i_me_range = GetDlgItemInt( hDlg, IDC_MERANGE, FALSE, FALSE );
                if( config->i_me_range > 64 )
                {
                    config->i_me_range = 64;
                    SetDlgItemInt( hDlg, IDC_MERANGE, config->i_me_range, FALSE );
                }
                break;

            case IDC_BFRAME :
                config->i_bframe = GetDlgItemInt( hDlg, IDC_BFRAME, FALSE, FALSE );
                if( config->i_bframe > 5 )
                {
                    config->i_bframe = 5;
                    SetDlgItemInt( hDlg, IDC_BFRAME, config->i_bframe, FALSE );
                }
                EnableWindow( GetDlgItem( hDlg, IDC_BREFS ), config->i_bframe > 1 );
                EnableWindow( GetDlgItem( hDlg, IDC_WBPRED ), config->i_bframe > 1 );
                EnableWindow( GetDlgItem( hDlg, IDC_DIRECTPRED ), config->i_bframe > 0 );
                EnableWindow( GetDlgItem( hDlg, IDC_BADAPT ), config->i_bframe > 0 );
                EnableWindow( GetDlgItem( hDlg, IDC_BBIAS ), config->i_bframe > 0 );
                EnableWindow( GetDlgItem( hDlg, IDC_BBIASSLIDER ), config->i_bframe > 0 );
                break;
            case IDC_BBIAS :
                config->i_bframe_bias = GetDlgItemInt( hDlg, IDC_BBIAS, FALSE, TRUE );
                SendDlgItemMessage(hDlg, IDC_BBIASSLIDER, TBM_SETPOS, 1,
                config->i_bframe_bias);
                if( config->i_bframe_bias > 100 )
                {
                    config->i_bframe_bias = 100;
                    SetDlgItemInt( hDlg, IDC_BBIAS, config->i_bframe_bias, TRUE );
                } else if ( config->i_bframe_bias < -100 )
                {
                    config->i_bframe_bias = -100;
                    SetDlgItemInt( hDlg, IDC_BBIAS, config->i_bframe_bias, TRUE );
                }
                break;
            case IDC_IPRATIO :
                config->i_key_boost = GetDlgItemInt( hDlg, IDC_IPRATIO, FALSE, FALSE );
                if (config->i_key_boost < 0)
                {
                    config->i_key_boost = 0;
                    SetDlgItemInt( hDlg, IDC_IPRATIO, config->i_key_boost, FALSE );
                }
                else if (config->i_key_boost > 70)
                {
                    config->i_key_boost = 70;
                    SetDlgItemInt( hDlg, IDC_IPRATIO, config->i_key_boost, FALSE );
                }                        
                break;
            case IDC_PBRATIO :
                config->i_b_red = GetDlgItemInt( hDlg, IDC_PBRATIO, FALSE, FALSE );
                if (config->i_b_red < 0)
                {
                    config->i_b_red = 0;
                    SetDlgItemInt( hDlg, IDC_PBRATIO, config->i_b_red, FALSE );
                }
                else if (config->i_b_red > 60)
                {
                    config->i_b_red = 60;
                    SetDlgItemInt( hDlg, IDC_PBRATIO, config->i_b_red, FALSE );
                }                        
                break;
            case IDC_CURVECOMP:
                config->i_curve_comp = GetDlgItemInt( hDlg, IDC_CURVECOMP, FALSE, FALSE );
                if( config->i_curve_comp < 0 )
                {
                    config->i_curve_comp = 0;
                    SetDlgItemInt( hDlg, IDC_CURVECOMP, config->i_curve_comp, FALSE );
                }
                else if( config->i_curve_comp > 100 )
                {
                    config->i_curve_comp = 100;
                    SetDlgItemInt( hDlg, IDC_CURVECOMP, config->i_curve_comp, FALSE );
                }                        
                break;
            }
            break;
            case LBN_SELCHANGE :
                switch ( LOWORD( wParam ) ) {
                case IDC_DIRECTPRED:
                    config->i_direct_mv_pred = SendDlgItemMessage(hDlg, IDC_DIRECTPRED, CB_GETCURSEL, 0, 0);
                    break;
                case IDC_SUBPEL:
                    config->i_subpel_refine = SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_GETCURSEL, 0, 0);
                    EnableWindow( GetDlgItem( hDlg, IDC_CHROMAME ), config->i_subpel_refine >= 4 );
                    break;
                case IDC_ME_METHOD:
                    config->i_me_method = SendDlgItemMessage(hDlg, IDC_ME_METHOD, CB_GETCURSEL, 0, 0);
                    EnableWindow( GetDlgItem( hDlg, IDC_MERANGE ), config->i_me_method > 1 );
                    break;
                }
            break;
        }
        break;
        case WM_HSCROLL : 
        if( (HWND) lParam == GetDlgItem( hDlg, IDC_INLOOP_A ) ) {
                config->i_inloop_a = SendDlgItemMessage( hDlg, IDC_INLOOP_A, TBM_GETPOS, 0, 0 );
                set_dlgitem_int( hDlg, IDC_LOOPA_TXT, config->i_inloop_a);
        }
        if( (HWND) lParam == GetDlgItem( hDlg, IDC_INLOOP_B ) ) {
                config->i_inloop_b = SendDlgItemMessage( hDlg, IDC_INLOOP_B, TBM_GETPOS, 0, 0 );
                set_dlgitem_int( hDlg, IDC_LOOPB_TXT, config->i_inloop_b);
        }
        if( (HWND) lParam == GetDlgItem( hDlg, IDC_BBIASSLIDER ) ) {
                config->i_bframe_bias = SendDlgItemMessage( hDlg, IDC_BBIASSLIDER, TBM_GETPOS, 0, 0 );
                set_dlgitem_int( hDlg, IDC_BBIAS, config->i_bframe_bias);
        }
        break;
        case WM_CLOSE:
            EndDialog( hDlg, LOWORD( wParam ) );
            break;
    default :
        return 0;
    }
    return 1;
}

/* error console dialog process */
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
