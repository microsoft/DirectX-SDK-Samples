//-------------------------------------------------------------------------------------
//  ExportSettingsDialog.h
//
//  Implements a GUI for modifying the settings variables.
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once
#include "ExportDialogs.h"

namespace ATG
{
	class ExportSettingsDialog : public ExportDialogBase
	{
	public:
        enum DialogState
        {
            DS_HIDDEN_UNKNOWN,
            DS_HIDDEN_CANCELED,
            DS_HIDDEN_OK,
            DS_VISIBLE
        };
		ExportSettingsDialog();

		virtual LRESULT OnCommand( WORD wNotifyCode, WORD idCtrl, HWND hwndCtrl ); 
		virtual LRESULT OnInitDialog( HWND hwndFocusCtrl );
		virtual LRESULT OnMessage( UINT uMsg, WPARAM wParam, LPARAM lParam );
		virtual LRESULT OnNotify( INT idCtrl, LPNMHDR pnmh );

		static UINT WINAPI ThreadEntry( VOID* pData );
		static LRESULT CALLBACK ScrollPaneWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

        virtual VOID Show();
        DialogState GetDialogState() const { return m_DialogState; }

	protected:
		VOID PopulateCategories( const ExportSettingsEntry* pEntry = NULL, VOID* hParentItem = NULL );
		VOID PopulateControls();
		VOID UpdateVScroll();
		VOID SynchronizeControlUI( HWND hwndControl );

	public:
		WNDPROC     m_pStaticWndProc;

	protected:
		HWND    m_hCategoryTree;
		HWND    m_hOKButton;
		HWND    m_hCancelButton;

		HWND    m_hScrollingPane;

		DWORD   m_dwControlsHeight;
		ExportSettingsEntry*    m_pCurrentCategory;
        DialogState   m_DialogState;
        BOOL    m_bControlDataUpdate;
	};
}
