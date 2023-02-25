//-------------------------------------------------------------------------------------
//  ExportDialogs.cpp
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"

#include "ExportDialogs.h"
#include "ExportConsoleDialog.h"
#include "ExportSettingsDialog.h"

namespace ATG
{
    const CHAR* g_strTitle = NULL;
    HWND g_hParentWindow = NULL;
    HINSTANCE g_hInstance = NULL;
    HMODULE g_hRichEdit = NULL;

    ExportConsoleDialog g_ConsoleDlg;
    ExportSettingsDialog g_SettingsDlg;

    UINT WINAPI ExportConsoleDialog::ThreadEntry( VOID* pData )
    {
        ExportConsoleDialog* pDlg = (ExportConsoleDialog*)pData;
        pDlg->DoModal( g_hInstance, g_hParentWindow );
        return 0;
    }

    UINT WINAPI ExportSettingsDialog::ThreadEntry( VOID* pData )
    {
        ExportSettingsDialog* pDlg = (ExportSettingsDialog*)pData;
		pDlg->DoModal( g_hInstance, g_hParentWindow );
        return 0;
    }

    VOID InitializeExportDialogs( const CHAR* strTitle, HWND hParentWindow, HINSTANCE hInst )
    {
        g_strTitle = strTitle;
        g_hInstance = hInst;
        g_hParentWindow = hParentWindow;

        // Pull in common and rich edit controls
		INITCOMMONCONTROLSEX ICEX;
		ICEX.dwSize = sizeof( INITCOMMONCONTROLSEX );
		ICEX.dwICC = ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_USEREX_CLASSES;
		InitCommonControlsEx( &ICEX );
		InitCommonControls();
        HMODULE g_hRichEdit = LoadLibrary( TEXT( "Riched32.dll" ) );
        assert( g_hRichEdit != NULL );
        UNREFERENCED_PARAMETER(g_hRichEdit);

        ExportLog::AddListener( &g_ConsoleDlg );
        g_pProgress = &g_ConsoleDlg;

        const DWORD dwStackSize = 8192;

        _beginthreadex( NULL, dwStackSize, ExportConsoleDialog::ThreadEntry, &g_ConsoleDlg, 0, NULL );
        _beginthreadex( NULL, dwStackSize, ExportSettingsDialog::ThreadEntry, &g_SettingsDlg, 0, NULL );
    }

    VOID TerminateExportDialogs()
    {
        FreeLibrary( g_hRichEdit );
    }

    VOID ShowConsoleDialog()
    {
        g_ConsoleDlg.Show();
    }

    BOOL ShowSettingsDialog( BOOL bModal )
    {
        g_SettingsDlg.Show();
        if( bModal )
        {
            while( g_SettingsDlg.GetDialogState() == ExportSettingsDialog::DS_VISIBLE ) { Sleep(0); }
            if( g_SettingsDlg.GetDialogState() != ExportSettingsDialog::DS_HIDDEN_OK )
                return FALSE;
        }
        return TRUE;
    }

    VOID ExportDialogBase::Show()
    {
        ShowWindow( m_hwnd, SW_SHOWNORMAL );
        SetWindowPos( m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
        UpdateWindow( m_hwnd );
    }

    VOID ExportDialogBase::Hide()
    {
        ShowWindow( m_hwnd, SW_HIDE );
    }
}