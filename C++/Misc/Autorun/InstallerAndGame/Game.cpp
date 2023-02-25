//-----------------------------------------------------------------------------
// File: Game.cpp
//
// The faux game for the Autorun sample.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "Common.h"
#include "ProcessInfo.h"
#include "resource.h"


//-----------------------------------------------------------------------------
// Forward declarations.
//-----------------------------------------------------------------------------
INT_PTR CALLBACK DialogProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );


//-----------------------------------------------------------------------------
// Program entry point.
//-----------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE, LPWSTR, int )
{
    DialogBox( hInstance,
               MAKEINTRESOURCE( IDD_DIALOG ),
               NULL,
               DialogProc );

    return 0;
}


//-----------------------------------------------------------------------------
// Message handler for the dialog.
//-----------------------------------------------------------------------------
INT_PTR CALLBACK DialogProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM /* lParam */ )
{
    switch( Msg )
    {
        case WM_INITDIALOG:
        {
            ProcessInfo *processInfo = ProcessInfo::GetProcessInfo();

            // Update UI.
            SetWindowText( hDlg, L"Game" );
            SetDlgItemText( hDlg, IDC_DESCRIPTION,
                            L"This is the faux game for the Autorun Sample.\r\n\r\n"
                            L"If the Autorun application is working, this application "
                            L"should have started without a preceding UAC prompt.  "
                            L"Confirm below that this application does not have elevated "
                            L"privileges and is running under the account of the user who "
                            L"started the Autorun application." );

            WCHAR szBuffer[512];
            if( processInfo )
            {
                swprintf_s( szBuffer, L"This process is %s and is running as %s.",
                            ( processInfo->Elevated() ? L"elevated" : L"not elevated" ),
                            processInfo->UserName() );
                
                delete processInfo;
            }
            else
            {
                swprintf_s( szBuffer, L"Unable to determine run level and user account." );
            }
            SetDlgItemText( hDlg, IDC_EDIT, szBuffer );
        }
        return TRUE;
        
        case WM_COMMAND:
        {
            if( wParam == IDOK || wParam == IDCANCEL )
            {
                EndDialog( hDlg, ( INT_PTR )wParam );
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
