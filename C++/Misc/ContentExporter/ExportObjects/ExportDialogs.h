//-------------------------------------------------------------------------------------
//  ExportDialogs.h
//
//  Entry points for various GUI dialogs.
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once
#include "ExportDialogUtils.h"

namespace ATG
{
    VOID InitializeExportDialogs( const CHAR* strTitle, HWND hParentWindow, HINSTANCE hInst );
    VOID TerminateExportDialogs();

    VOID ShowConsoleDialog();
    BOOL ShowSettingsDialog( BOOL bModal );

    class ExportDialogBase : public ThinDialog
    {
    public:
        virtual VOID Show();
        virtual VOID Hide();
    };
}