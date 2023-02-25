//-------------------------------------------------------------------------------------
//  ExportConsoleDialog.cpp
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "ExportConsoleDialog.h"
#include "ExportResources.h"

namespace ATG
{
	extern const CHAR* g_strTitle;
	extern HWND g_hParentWindow;
	extern HINSTANCE g_hInstance;

	ExportConsoleDialog::ExportConsoleDialog()
	{
		m_strTemplate = "ExportConsole";
	}

	LRESULT ExportConsoleDialog::OnInitDialog( HWND hwndFocusCtrl )
	{
		UNREFERENCED_PARAMETER( hwndFocusCtrl );
		m_hRichTextBox = GetDlgItem( m_hwnd, IDC_RICHTEXT );
		m_hProgressBar = GetDlgItem( m_hwnd, IDC_EXPORTPROGRESS );
		m_hProgressText = GetDlgItem( m_hwnd, IDC_EXPORTTEXT );
		m_hOKButton = GetDlgItem( m_hwnd, IDOK );
		SetWindowText( m_hwnd, g_strTitle );
		SetWindowText( m_hProgressText, g_strTitle );

		/*
		// Change the font type of the output window to courier
		CHARFORMAT cf;
		cf.cbSize = sizeof(CHARFORMAT);
		SendMessage( m_hRichTextBox, EM_GETCHARFORMAT, 0, (LPARAM)&cf );
		cf.dwMask &= ~CFM_COLOR;
		strcpy_s( cf.szFaceName, "courier" );
		SendMessage( m_hRichTextBox, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf );
		*/
		SendMessage( m_hwnd, WM_SIZE, 0, 0 );
		SetTimer( m_hwnd, 1, 0, NULL );

		return FALSE;
	}

	LRESULT ExportConsoleDialog::OnCommand( WORD wNotifyCode, WORD idCtrl, HWND hwndCtrl )
	{
		UNREFERENCED_PARAMETER( hwndCtrl );
		switch (idCtrl)
		{
		case IDCANCEL: // the little x in the upper right?
		case IDOK:
		case IDCLOSE:
			switch (wNotifyCode)
			{
			case BN_CLICKED:
				Hide();
				return TRUE;
			}
			break;
		}
		return FALSE;
	}

	LRESULT ExportConsoleDialog::OnMessage( UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		UNREFERENCED_PARAMETER( wParam );
		UNREFERENCED_PARAMETER( lParam );

		switch( uMsg )
		{
		case WM_TIMER:
			Hide();
			KillTimer( m_hwnd, wParam );
			return FALSE;
		case WM_SIZE:
			{
				GridLayout Layout( 3, 2 );
				Layout.SetRowSpec( 0, 1.0f );
				Layout.SetRowSpec( 1, 30 );
				Layout.SetRowSpec( 2, 30 );
				Layout.SetColumnSpec( 0, 1 );
				Layout.SetColumnSpec( 1, 100 );

				const DWORD dwOuterBorder = 4;
				const DWORD dwInnerBorder = 4;
				Layout.SetClientRect( m_hwnd, dwOuterBorder );
				Layout.PlaceWindow( m_hRichTextBox, 0, 0, 1, 2, dwInnerBorder );
				Layout.PlaceWindow( m_hProgressText, 1, 0, 1, 2, dwInnerBorder );
				Layout.PlaceWindow( m_hProgressBar, 2, 0, 1, 1, dwInnerBorder );
				Layout.PlaceWindow( m_hOKButton, 2, 1, 1, 1, dwInnerBorder );
				return FALSE;
			}
		}
		return FALSE;
	}

	VOID ExportConsoleDialog::ConsolePrint( COLORREF rgb, const CHAR* strText )
	{
		CHARRANGE cr = { -1, -2 };

		if( rgb != CLR_INVALID )
		{
			// Set whatever colors, etc. they want
			CHARFORMAT cf = {0};
			cf.cbSize = sizeof(cf);
			cf.dwMask = CFM_COLOR;
			cf.dwEffects = 0;
			cf.crTextColor = rgb;
			SendDlgItemMessage( m_hwnd, IDC_RICHTEXT, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf );
		}

		// Move the selection to the end
		SendDlgItemMessage( m_hwnd, IDC_RICHTEXT, EM_EXSETSEL, 0, (LPARAM)&cr );

		// Add the text and scroll it into view
		SendDlgItemMessage( m_hwnd, IDC_RICHTEXT, EM_REPLACESEL, 0, (LPARAM)strText );
		SendDlgItemMessage( m_hwnd, IDC_RICHTEXT, EM_SCROLLCARET, 0, 0L );
	}

	VOID ExportConsoleDialog::ConsoleNewline()
	{
		ConsolePrint( 0, "\n" );
		SendMessage( m_hRichTextBox, EM_LINESCROLL, 0, 1 );
	}

	VOID ExportConsoleDialog::ConsoleClear()
	{
		SETTEXTEX stx;
		stx.codepage = CP_ACP;
		stx.flags = ST_DEFAULT;
		SendDlgItemMessage( m_hwnd, IDC_RICHTEXT, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)"" );
	}

	VOID ExportConsoleDialog::LogCommand( DWORD dwCommand, VOID* pData )
	{
		UNREFERENCED_PARAMETER( pData );

		switch( dwCommand )
		{
		case ExportLog::ELC_CLEAR:
			ConsoleClear();
			Show();
			break;
		case ExportLog::ELC_STARTEXPORT:
		case ExportLog::ELC_ENDEXPORT:
			Show();
			break;
		}
	}

	VOID ExportConsoleDialog::LogMessage( const CHAR* strMessage )
	{
		ConsolePrint( RGB(0, 0, 0), strMessage );
		ConsoleNewline();
	}

	VOID ExportConsoleDialog::LogWarning( const CHAR* strMessage )
	{
		ConsolePrint( RGB(255, 128, 0), strMessage );
		ConsoleNewline();
	}

	VOID ExportConsoleDialog::LogError( const CHAR* strMessage )
	{
		ConsolePrint( RGB(255, 0, 0), strMessage );
		ConsoleNewline();
	}

	VOID ExportConsoleDialog::Initialize( const CHAR *strTitle )
	{
		UNREFERENCED_PARAMETER( strTitle );
		m_fCurrentTaskMin = 0;
		m_fCurrentTaskSize = 0;

		SendDlgItemMessage( m_hwnd, IDC_EXPORTPROGRESS, PBM_SETRANGE32, 0, 1000 );
		SendDlgItemMessage( m_hwnd, IDC_EXPORTPROGRESS, PBM_SETPOS, 0, 0 );
		SetCaption( "" );
	}

	VOID ExportConsoleDialog::Terminate()
	{
		SetCaption( "Export complete." );
		SendDlgItemMessage( m_hwnd, IDC_EXPORTPROGRESS, PBM_SETPOS, 1000, 0 );
	}

	VOID ExportConsoleDialog::SetCaption( const CHAR* strCaption )
	{
		SetWindowText( m_hProgressText, strCaption );
	}

	VOID ExportConsoleDialog::StartNewTask( const CHAR* strCaption, FLOAT fTaskPercentOfWhole )
	{
		SetCaption( strCaption );
		m_fCurrentTaskMin += m_fCurrentTaskSize;
		m_fCurrentTaskSize = fTaskPercentOfWhole;
		SetProgress( 0 );
	}

	VOID ExportConsoleDialog::SetProgress( FLOAT fTaskRelativeProgress )
	{
		if( fTaskRelativeProgress > 1 )
			fTaskRelativeProgress = 1;
		else if( fTaskRelativeProgress < 0 )
			fTaskRelativeProgress = 0;
		FLOAT fAbsoluteProgress = ( fTaskRelativeProgress * m_fCurrentTaskSize ) + m_fCurrentTaskMin;
		INT iProgress = (INT)( fAbsoluteProgress * 1000.0f );
		SendDlgItemMessage( m_hwnd, IDC_EXPORTPROGRESS, PBM_SETPOS, iProgress, 0 );
	}

}