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
#define X264_DEF_TEXT    "Are you sure you want to load default values"

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

    /* Advance dialog */
    { "cabac",          &reg.b_cabac,           1 },
    { "loop_filter",    &reg.b_filter,          1 },
    { "keyint_max",     &reg.i_keyint_min,      25 },
    { "keyint_min",     &reg.i_keyint_max,      250},
    { "refmax",         &reg.i_refmax,          1 },
    { "bmax",           &reg.i_bframe,          0 },
    {"direct_pred",     &reg.i_direct_mv_pred,  2 },
    {"inloop_a",        &reg.i_inloop_a,        0 },
    {"inloop_b",        &reg.i_inloop_b,        0 },

    /* analysis */
    {"i4x4",            &reg.b_i4x4,            1 },
    {"psub16x16",       &reg.b_psub16x16,       1 },
    {"psub8x8",         &reg.b_psub8x8,         1 },
    {"bsub16x16",       &reg.b_bsub16x16,       1 },
    {"subpel",          &reg.i_subpel_refine,   4 }

};

static const reg_str_t reg_str_table[] =
{
    { "fourcc",         reg.fcc,                "h264" }
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
        i_size = 5;   /* fourcc + 1 FIXME ugly */
        if( RegQueryValueEx( hKey, reg_str_table[i].reg_value, 0, 0,
                             (LPBYTE)reg_str_table[i].config_str,
                             &i_size ) != ERROR_SUCCESS )
            memcpy( reg_str_table[i].config_str,
                    reg_str_table[i].default_str, 5 );
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
                        5 );    /* FIXME */
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
        break;
    
    case 2 : /* 2 Pass */
        EnableWindow( GetDlgItem( hDlg, IDC_BITRATEEDIT ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_BITRATESLIDER ), FALSE );

        EnableWindow( GetDlgItem( hDlg, IDC_QUANTEDIT ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_QUANTSLIDER ), FALSE );

        EnableWindow( GetDlgItem( hDlg, IDC_2PASS1 ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASS2 ), TRUE );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASSBITRATE ), config->i_pass > 1 );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASSBITRATE_S ), config->i_pass > 1 );

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
        EnableWindow( GetDlgItem( hDlg, IDC_2PASSBITRATE ), config->i_pass > 1 );
        EnableWindow( GetDlgItem( hDlg, IDC_2PASSBITRATE_S ), config->i_pass > 1 );

        break;
    }

    SendDlgItemMessage( hDlg, IDC_BITRATESLIDER, TBM_SETPOS, TRUE,
                        config->bitrate );
    SendDlgItemMessage( hDlg, IDC_QUANTSLIDER, TBM_SETPOS, TRUE,
                        config->i_qp );
    SendDlgItemMessage( hDlg, IDC_2PASSBITRATE_S, TBM_SETPOS, TRUE,
                        config->i_2passbitrate );

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
            }
            break;
        case EN_CHANGE :
            switch( LOWORD( wParam ) )
            {
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
        sprintf( temp, "Core %d, build %s %s", X264_BUILD, __DATE__, __TIME__ );
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

static void set_dlgitem_int(HWND hDlg, UINT item, int value)
{
    char buf[8];
    sprintf(buf, "%i", value);
    SetDlgItemText(hDlg, item, buf);
}

static void adv_update_dlg( HWND hDlg, CONFIG * config )
{
    char fourcc[5];

    SendDlgItemMessage(hDlg, IDC_DIRECTPRED, CB_ADDSTRING, 0, (LPARAM)"None");
    SendDlgItemMessage(hDlg, IDC_DIRECTPRED, CB_ADDSTRING, 0, (LPARAM)"Spatial");
    SendDlgItemMessage(hDlg, IDC_DIRECTPRED, CB_ADDSTRING, 0, (LPARAM)"Temporal");

    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"1 (Fastest)");
    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"2");
    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"3");
    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"4");
    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_ADDSTRING, 0, (LPARAM)"5 (Max Quality)");

    CheckDlgButton( hDlg,IDC_CABAC,
                    config->b_cabac ? BST_CHECKED : BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_LOOPFILTER,
                    config->b_filter ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_P16X16,
                    config->b_psub16x16 ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_P8X8,
                    config->b_psub8x8 ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_B16X16,
                    config->b_bsub16x16 ? BST_CHECKED: BST_UNCHECKED );
    CheckDlgButton( hDlg,IDC_I4X4,
                    config->b_i4x4 ? BST_CHECKED: BST_UNCHECKED );

    SetDlgItemInt( hDlg, IDC_IDRFRAMES, config->i_keyint_min, FALSE );
    SetDlgItemInt( hDlg, IDC_IFRAMES, config->i_keyint_max, FALSE );
    SetDlgItemInt( hDlg, IDC_KEYFRAME, config->i_refmax, FALSE );
    SetDlgItemInt( hDlg, IDC_BFRAME, config->i_bframe, FALSE );

    SendDlgItemMessage(hDlg, IDC_DIRECTPRED, CB_SETCURSEL, (config->i_direct_mv_pred), 0);
    SendDlgItemMessage(hDlg, IDC_SUBPEL, CB_SETCURSEL, (config->i_subpel_refine), 0);

    SendDlgItemMessage( hDlg, IDC_INLOOP_A, TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG( -6, 6 ) );
    SendDlgItemMessage( hDlg, IDC_INLOOP_B, TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG( -6, 6 ) );

    SendDlgItemMessage( hDlg, IDC_INLOOP_A, TBM_SETPOS, TRUE,
                        config->i_inloop_a );
    SendDlgItemMessage( hDlg, IDC_INLOOP_B, TBM_SETPOS, TRUE,
                        config->i_inloop_b );
    set_dlgitem_int( hDlg, IDC_LOOPA_TXT, config->i_inloop_a);
    set_dlgitem_int( hDlg, IDC_LOOPB_TXT, config->i_inloop_b);

    EnableWindow( GetDlgItem( hDlg, IDC_P8X8 ), config->b_psub16x16 );


    memcpy( fourcc, config->fcc, 4 );
    fourcc[4] = '\0';

    SetDlgItemText( hDlg, IDC_FOURCC, fourcc );
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
            }
            break;
        case EN_CHANGE :
            switch( LOWORD( wParam ) )
            {
            case IDC_IDRFRAMES :
                config->i_keyint_min = GetDlgItemInt( hDlg, IDC_IDRFRAMES, FALSE, FALSE );
                break;
            case IDC_IFRAMES :
                config->i_keyint_max = GetDlgItemInt( hDlg, IDC_IFRAMES, FALSE, FALSE );
                break;
            case IDC_KEYFRAME :
                config->i_refmax = GetDlgItemInt( hDlg, IDC_KEYFRAME, FALSE, FALSE );
                break;
            case IDC_FOURCC :
                GetDlgItemText( hDlg, IDC_FOURCC, config->fcc, 5 );
                break;
            case IDC_BFRAME :
                config->i_bframe = GetDlgItemInt( hDlg, IDC_BFRAME, FALSE, FALSE );
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
                    break;
                }
            break;
        }
        break;
        case WM_HSCROLL : 
        if( (HWND) lParam == GetDlgItem( hDlg, IDC_INLOOP_A ) ) {
                config->i_inloop_a = SendDlgItemMessage( hDlg, IDC_INLOOP_A, TBM_GETPOS, 0, 0 );
                set_dlgitem_int( hDlg, IDC_LOOPA_TXT, config->i_inloop_a);

        } else if ( (HWND) lParam == GetDlgItem( hDlg, IDC_INLOOP_B ) ) {
                config->i_inloop_b = SendDlgItemMessage( hDlg, IDC_INLOOP_B, TBM_GETPOS, 0, 0 );
                set_dlgitem_int( hDlg, IDC_LOOPB_TXT, config->i_inloop_b);
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

