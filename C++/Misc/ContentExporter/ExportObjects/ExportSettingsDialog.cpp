//-------------------------------------------------------------------------------------
//  ExportSettingsDialog.cpp
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "ExportSettingsDialog.h"
#include "ExportResources.h"

#define IDC_DYNAMICCONTROL   4000
#define IDC_DYNAMICDROPLIST   4001

namespace ATG
{
	extern const CHAR* g_strTitle;
	extern HWND g_hParentWindow;
	extern HINSTANCE g_hInstance;

	ExportSettingsDialog::ExportSettingsDialog()
	{
		m_strTemplate = "ExportSettings";
		m_pCurrentCategory = NULL;
        m_DialogState = DS_HIDDEN_UNKNOWN;
        m_bControlDataUpdate = FALSE;
	}

    VOID ExportSettingsDialog::Show()
    {
        ExportDialogBase::Show();
        m_DialogState = DS_VISIBLE;
        TreeView_SelectItem( m_hCategoryTree, NULL );
        TreeView_SelectItem( m_hCategoryTree, TreeView_GetRoot( m_hCategoryTree ) );
    }

	LRESULT CALLBACK ExportSettingsDialog::ScrollPaneWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		switch( message )
		{
		case WM_CREATE:
		case WM_DESTROY:
		case WM_ENABLE:
		case WM_GETDLGCODE:
		case WM_GETFONT:
		case WM_GETTEXT:
		case WM_GETTEXTLENGTH:
		case WM_SETFONT:
		case WM_SETTEXT:
		case WM_PAINT:
		case WM_ERASEBKGND:
			{
				DWORD dwID = (DWORD)GetWindowLong( hWnd, GWL_ID );
				if( dwID != IDC_DYNAMICCONTROL )
				{
					ExportSettingsDialog* pDlg = (ExportSettingsDialog*)GetWindowLong( hWnd, GWL_USERDATA );
					return CallWindowProc( pDlg->m_pStaticWndProc, hWnd, message, wParam, lParam );
				}
				return DefWindowProc( hWnd, message, wParam, lParam );
			}
		case WM_COMMAND:
		case WM_VSCROLL:
		case WM_NOTIFY:
		case WM_HSCROLL:
			return DlgProc( hWnd, message, wParam, lParam );
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
		}
	}


	LRESULT ExportSettingsDialog::OnInitDialog( HWND hwndFocusCtrl )
	{
		UNREFERENCED_PARAMETER( hwndFocusCtrl );
		m_hCategoryTree = GetDlgItem( m_hwnd, IDC_CATEGORIES );
		m_hOKButton = GetDlgItem( m_hwnd, IDOK );
		m_hCancelButton = GetDlgItem( m_hwnd, IDCANCEL );

		m_hScrollingPane = CreateWindowEx( 
			WS_EX_CLIENTEDGE | WS_EX_CONTROLPARENT,                          
			"Static",                // window class 
			NULL,    // text for window title bar 
			WS_CHILD |        // window styles 
			WS_CLIPCHILDREN |
			WS_VSCROLL |
			WS_VISIBLE |
			SS_NOTIFY, 
			300,
			0,
			100,
			100,
			m_hwnd,             
			(HMENU) NULL,        // window class menu 
			g_hInstance,               // instance owning this window 
			NULL        // pointer not needed 
			); 

		SetWindowLong( m_hScrollingPane, GWL_USERDATA, (LONG)(LPARAM)this );
		m_pStaticWndProc = (WNDPROC)SetWindowLong( m_hScrollingPane, GWL_WNDPROC, (LONG)ScrollPaneWndProc );

		SetWindowText( m_hwnd, g_strTitle );

		PopulateCategories();
		TreeView_SelectItem( m_hCategoryTree, TreeView_GetRoot( m_hCategoryTree ) );
		//PopulateControls();

		SendMessage( m_hwnd, WM_SIZE, 0, 0 );
		SetTimer( m_hwnd, 1, 0, NULL );

		return FALSE;
	}


	BOOL CALLBACK DestroyWindowCallback( HWND hWnd, LPARAM lParam )
	{
		UNREFERENCED_PARAMETER( lParam );
		DestroyWindow( hWnd );
		return TRUE;
	}


	HWND CreateStaticLabel( HWND hParent, const CHAR* strText, HFONT hFont, DWORD dwX, DWORD dwY, DWORD dwWidth )
	{
		HWND hLabel = CreateWindow( "Static", 
			strText, 
			WS_CHILD | WS_VISIBLE, 
			dwX, dwY,
			dwWidth, 25,
			hParent,
			NULL, g_hInstance, NULL );

		SendMessage( hLabel, WM_SETFONT, (WPARAM)hFont, TRUE );

		return hLabel;
	}


	HWND CreateCheckbox( HWND hParent, DWORD dwX, DWORD dwY, ExportSettingsEntry* pData )
	{
		HWND hCheckbox = CreateWindow( "Button",
			"",
			WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
			dwX, dwY, 25, 25,
			hParent, 
			NULL, g_hInstance, NULL );
		SetWindowLong( hCheckbox, GWL_USERDATA, (LONG)pData );
		SetWindowLong( hCheckbox, GWL_ID, IDC_DYNAMICCONTROL );
		return hCheckbox;
	}


	HWND CreateEditBox( HWND hParent, DWORD dwX, DWORD dwY, DWORD dwWidth, ExportSettingsEntry* pData )
	{
		HWND hEditBox = CreateWindowEx( WS_EX_CLIENTEDGE,
			"Edit",
			"",
			WS_CHILD | WS_VISIBLE,
			dwX, dwY, dwWidth, 25,
			hParent,
			NULL, g_hInstance, NULL );
		SetWindowLong( hEditBox, GWL_USERDATA, (LONG)pData );
		SetWindowLong( hEditBox, GWL_ID, IDC_DYNAMICCONTROL );
		return hEditBox;
	}


	HWND CreateTrackBar( HWND hParent, DWORD dwX, DWORD dwY, DWORD dwWidth, ExportSettingsEntry* pData )
	{
		HWND hTrackBar = CreateWindowEx( 0, TRACKBAR_CLASS, NULL, 
			TBS_AUTOTICKS | TBS_HORZ | TBS_TOOLTIPS | WS_VISIBLE | WS_CHILD, 
			dwX, dwY, dwWidth, 30,
			hParent,
			NULL, g_hInstance, NULL );
		SetWindowLong( hTrackBar, GWL_USERDATA, (LONG)pData );
		SetWindowLong( hTrackBar, GWL_ID, IDC_DYNAMICCONTROL );
		SendMessage( hTrackBar, TBM_SETRANGE, TRUE, MAKELONG( pData->m_MinValue.m_iValue, pData->m_MaxValue.m_iValue ) );
		SendMessage( hTrackBar, TBM_SETTICFREQ, 10, 0 );
		return hTrackBar;
	}


	HWND CreateTrackBarFloat( HWND hParent, DWORD dwX, DWORD dwY, DWORD dwWidth, ExportSettingsEntry* pData )
	{
		HWND hTrackBar = CreateWindowEx( 0, TRACKBAR_CLASS, NULL, 
			TBS_AUTOTICKS | TBS_HORZ | WS_VISIBLE | WS_CHILD, 
			dwX, dwY, dwWidth, 30,
			hParent,
			NULL, g_hInstance, NULL );
		SetWindowLong( hTrackBar, GWL_USERDATA, (LONG)pData );
		SetWindowLong( hTrackBar, GWL_ID, IDC_DYNAMICCONTROL );
		SendMessage( hTrackBar, TBM_SETRANGE, TRUE, MAKELONG( 0, 1000 ) );
		SendMessage( hTrackBar, TBM_SETTICFREQ, 100, 0 );
		return hTrackBar;
	}


	HWND CreateDropList( HWND hParent, DWORD dwX, DWORD dwY, DWORD dwWidth, ExportSettingsEntry* pData )
	{
		assert( pData->m_Type == ExportSettingsEntry::CT_ENUM );
		HWND hDropList = CreateWindowEx( 0, WC_COMBOBOXEX, "",
			CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE,
			dwX, dwY, dwWidth, 300,
			hParent, NULL, g_hInstance, NULL );
		SetWindowLong( hDropList, GWL_USERDATA, (LONG)pData );
		SetWindowLong( hDropList, GWL_ID, IDC_DYNAMICDROPLIST );

		for( DWORD i = 0; i < pData->m_dwEnumValueCount; ++i )
		{
			const ExportEnumValue* pEnumValue = &pData->m_pEnumValues[i];
			COMBOBOXEXITEMA CBItem = {0};
			CBItem.mask = CBEIF_TEXT;
			CBItem.pszText = (LPSTR)pEnumValue->strLabel;
			CBItem.iItem = (INT)i;
			SendMessageA( hDropList, CBEM_INSERTITEM, 0, (LPARAM)&CBItem );
		}
		return hDropList;
	}


	VOID ExportSettingsDialog::PopulateControls()
	{
		HFONT DlgFont = (HFONT)SendMessage( m_hOKButton, WM_GETFONT, 0, 0 );

		EnumChildWindows( m_hScrollingPane, DestroyWindowCallback, NULL );

		if( m_pCurrentCategory == NULL )
			return;

		RECT ClientRect;
		GetClientRect( m_hScrollingPane, &ClientRect );
		const DWORD dwClientWidth = ClientRect.right - ClientRect.left;
		const DWORD dwClientMargin = 8;

		DWORD dwYPos = dwClientMargin;
		const DWORD dwXPosLabel = dwClientMargin;
		const DWORD dwXPosControl = dwClientWidth / 3;
		const DWORD dwLabelWidth = ( dwXPosControl - dwXPosLabel - 5 );
		ExportSettingsEntry* pSetting = m_pCurrentCategory->m_pFirstChild;

		const DWORD dwControlWidth = dwClientWidth - dwXPosControl;

		while( pSetting != NULL )
		{
			HWND hwndControl = NULL;
			DWORD dwControlHeight = 30;
			CreateStaticLabel( m_hScrollingPane, pSetting->m_DisplayName, DlgFont, dwXPosLabel, dwYPos, dwLabelWidth );
			switch( pSetting->m_Type )
			{
			case ExportSettingsEntry::CT_CHECKBOX:
				hwndControl = CreateCheckbox( m_hScrollingPane, dwXPosControl, dwYPos, pSetting );
				SynchronizeControlUI( hwndControl );
				break;
			case ExportSettingsEntry::CT_STRING:
				hwndControl = CreateEditBox( m_hScrollingPane, dwXPosControl, dwYPos, dwControlWidth, pSetting );
				SendMessage( hwndControl, WM_SETFONT, (WPARAM)DlgFont, TRUE );
				SynchronizeControlUI( hwndControl );
				break;
			case ExportSettingsEntry::CT_BOUNDEDINTSLIDER:
				hwndControl = CreateTrackBar( m_hScrollingPane, dwXPosControl, dwYPos, dwControlWidth, pSetting );
				SynchronizeControlUI( hwndControl );
				dwControlHeight = 40;
				break;
			case ExportSettingsEntry::CT_BOUNDEDFLOATSLIDER:
				hwndControl = CreateTrackBarFloat( m_hScrollingPane, dwXPosControl, dwYPos, dwControlWidth, pSetting );
				SynchronizeControlUI( hwndControl );
				dwControlHeight = 40;
				break;
			case ExportSettingsEntry::CT_ENUM:
				hwndControl = CreateDropList( m_hScrollingPane, dwXPosControl, dwYPos, dwControlWidth, pSetting );
				SendMessage( hwndControl, WM_SETFONT, (WPARAM)DlgFont, TRUE );
				SynchronizeControlUI( hwndControl );
				break;
			}
			dwYPos += dwControlHeight;
			pSetting = pSetting->m_pSibling;
		}

		m_dwControlsHeight = dwYPos;
		UpdateVScroll();
		InvalidateRect( m_hScrollingPane, NULL, TRUE );
	}


	VOID ExportSettingsDialog::PopulateCategories( const ExportSettingsEntry* pEntry, VOID* hParentItem )
	{
		if( pEntry == NULL )
		{
			TreeView_DeleteAllItems( m_hCategoryTree );
			DWORD dwRootCount = g_SettingsManager.GetRootCategoryCount();
			for( DWORD i = 0; i < dwRootCount; ++i )
			{
				PopulateCategories( g_SettingsManager.GetRootCategory( i ), NULL );
			}
			return;
		}

		if( pEntry->m_Type != ExportSettingsEntry::CT_CATEGORY )
			return;

		TVINSERTSTRUCT TreeItem;
		ZeroMemory( &TreeItem, sizeof( TVINSERTSTRUCT ) );
		CHAR strText[MAX_PATH];
		strcpy_s( strText, pEntry->m_DisplayName.SafeString() );
		TreeItem.item.pszText = strText;
		TreeItem.item.mask = TVIF_TEXT | TVIF_PARAM;
		TreeItem.item.lParam = (LPARAM)pEntry;
		TreeItem.hParent = (HTREEITEM)hParentItem;
		TreeItem.hInsertAfter = TVI_LAST;

		HTREEITEM hCurrentItem = TreeView_InsertItem( m_hCategoryTree, &TreeItem );

		if( pEntry->m_pFirstChild != NULL )
		{
			PopulateCategories( pEntry->m_pFirstChild, hCurrentItem );
		}
		if( pEntry->m_pSibling != NULL )
		{
			PopulateCategories( pEntry->m_pSibling, hParentItem );
		}
	}


	INT GetNormalizedIntValue( ExportSettingsEntry* pEntry )
	{
		FLOAT fValue = pEntry->GetValueFloat();
		FLOAT fNorm = ( fValue - pEntry->m_MinValue.m_fValue ) / ( pEntry->m_MaxValue.m_fValue - pEntry->m_MinValue.m_fValue );
		return (INT)( fNorm * 1000.0f );
	}


	FLOAT GetFloatFromNormalizedInt( ExportSettingsEntry* pEntry, INT iValue )
	{
		FLOAT fNorm = (FLOAT)iValue / 1000.0f;
		FLOAT fValue = ( fNorm * ( pEntry->m_MaxValue.m_fValue - pEntry->m_MinValue.m_fValue ) ) + pEntry->m_MinValue.m_fValue;
		return fValue;
	}


	VOID ExportSettingsDialog::SynchronizeControlUI( HWND hwndControl )
	{
		if( hwndControl == NULL )
			return;
		ExportSettingsEntry* pEntry = (ExportSettingsEntry*)GetWindowLong( hwndControl, GWL_USERDATA );
		if( pEntry == NULL )
			return;
        m_bControlDataUpdate = TRUE;
		switch( pEntry->m_Type )
		{
		case ExportSettingsEntry::CT_CHECKBOX:
			{
				BOOL bValue = pEntry->GetValueBool();
				SendMessage( hwndControl, BM_SETCHECK, bValue ? BST_CHECKED : BST_UNCHECKED, 0 );
				break;
			}
		case ExportSettingsEntry::CT_STRING:
			{
				SetWindowText( hwndControl, pEntry->GetValueString() );
				break;
			}
		case ExportSettingsEntry::CT_BOUNDEDINTSLIDER:
			{
				INT iValue = pEntry->GetValueInt();
				SendMessage( hwndControl, TBM_SETPOS, TRUE, iValue );
				break;
			}
		case ExportSettingsEntry::CT_BOUNDEDFLOATSLIDER:
			{
				INT iValue = GetNormalizedIntValue( pEntry );
				SendMessage( hwndControl, TBM_SETPOS, TRUE, iValue );
				break;
			}
		case ExportSettingsEntry::CT_ENUM:
			{
				DWORD dwSelectionIndex = 0;
				INT iCurrentValue = pEntry->GetValueInt();
				for( DWORD i = 0; i < pEntry->m_dwEnumValueCount; ++i )
				{
					const ExportEnumValue* pEnumValue = &pEntry->m_pEnumValues[i];
					if( pEnumValue->iValue == iCurrentValue )
						dwSelectionIndex = i;
				}
				ComboBox_SetCurSel( hwndControl, dwSelectionIndex );
				break;
			}
		}
        m_bControlDataUpdate = FALSE;
	}


	VOID ExportSettingsDialog::UpdateVScroll()
	{
		RECT PageRect;
		GetWindowRect( m_hScrollingPane, &PageRect );
		SCROLLINFO si;
		si.cbSize = sizeof( SCROLLINFO );
		si.nMin = 0;
		si.nMax = m_dwControlsHeight;
		si.nPage = PageRect.bottom - PageRect.top;
		si.nPos = 0;
		si.nTrackPos = 0;
		si.fMask = SIF_ALL;
		SetScrollInfo( m_hScrollingPane, SB_VERT, &si, TRUE );
	}

	LRESULT ExportSettingsDialog::OnCommand( WORD wNotifyCode, WORD idCtrl, HWND hwndCtrl )
	{
		if( idCtrl == 0 )
		{
			idCtrl = (WORD)GetWindowLong( hwndCtrl, GWL_ID );
		}
		switch (idCtrl)
		{
		case IDCANCEL: // the little x in the upper right?
		case IDOK:
		case IDCLOSE:
			switch (wNotifyCode)
			{
			case BN_CLICKED:
				Hide();
                if( idCtrl == IDOK )
                    m_DialogState = DS_HIDDEN_OK;
                else
                    m_DialogState = DS_HIDDEN_CANCELED;
				return 0;
			}
			break;
		case IDC_DYNAMICCONTROL:
			{
                if( m_bControlDataUpdate )
                    return TRUE;
				ExportSettingsEntry* pEntry = (ExportSettingsEntry*)GetWindowLong( hwndCtrl, GWL_USERDATA );
				if( wNotifyCode == BN_CLICKED && pEntry->m_Type == ExportSettingsEntry::CT_CHECKBOX )
				{
					pEntry->SetValue( !pEntry->GetValueBool() );
					SynchronizeControlUI( hwndCtrl );
				}
				if( wNotifyCode == EN_CHANGE && pEntry->m_Type == ExportSettingsEntry::CT_STRING )
				{
                    CHAR strText[256];
                    GetWindowText( hwndCtrl, strText, 256 );
                    pEntry->SetValue( strText );
				}
				return TRUE;
			}
		case IDC_DYNAMICDROPLIST:
			{
                if( m_bControlDataUpdate )
                    return TRUE;
				ExportSettingsEntry* pEntry = (ExportSettingsEntry*)GetWindowLong( hwndCtrl, GWL_USERDATA );
				assert( pEntry->m_Type == ExportSettingsEntry::CT_ENUM );
				if( wNotifyCode == CBN_SELCHANGE )
				{
					INT iEnumIndex = ComboBox_GetCurSel( hwndCtrl );
					if( iEnumIndex < 0 )
						iEnumIndex = 0;
					if( iEnumIndex >= (INT)pEntry->m_dwEnumValueCount )
						iEnumIndex = (INT)pEntry->m_dwEnumValueCount - 1;
					pEntry->SetValue( pEntry->m_pEnumValues[iEnumIndex].iValue );
				}
				return TRUE;
			}
		}
		return FALSE;
	}

	LRESULT ExportSettingsDialog::OnMessage( UINT uMsg, WPARAM wParam, LPARAM lParam )
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
				const DWORD dwBorderSize = 4;

				GridLayout RootLayout( 2, 1 );
				RootLayout.SetRowSpec( 0, 1.0f );
				RootLayout.SetRowSpec( 1, 30 );
				RootLayout.SetClientRect( m_hwnd, dwBorderSize );

				const FLOAT fSplitterWidth = 5.0f;
				GridLayout PanesLayout( 1, 3 );
				PanesLayout.SetColumnSpec( 0, 200 );
				PanesLayout.SetColumnSpec( 1, fSplitterWidth );
				PanesLayout.SetColumnSpec( 2, 1.0f );
				RootLayout.PlaceLayout( PanesLayout, 0, 0, 1, 1, dwBorderSize );

				const FLOAT fButtonWidth = 100.0f;
				GridLayout ButtonsLayout( 1, 3 );
				ButtonsLayout.SetColumnSpec( 0, 1.0f );
				ButtonsLayout.SetColumnSpec( 1, fButtonWidth );
				ButtonsLayout.SetColumnSpec( 2, fButtonWidth );
				RootLayout.PlaceLayout( ButtonsLayout, 1, 0, 1, 1, 0 );

				PanesLayout.PlaceWindow( m_hCategoryTree, 0, 0, 1, 1, 0 );
				PanesLayout.PlaceWindow( m_hScrollingPane, 0, 2, 1, 1, 0 );

				ButtonsLayout.PlaceWindow( m_hOKButton, 0, 1, 1, 1, dwBorderSize );
				ButtonsLayout.PlaceWindow( m_hCancelButton, 0, 2, 1, 1, dwBorderSize );

				PopulateControls();

				return FALSE;
			}
		case WM_VSCROLL:
			{
				RECT PageRect;
				GetWindowRect( m_hScrollingPane, &PageRect );
				SCROLLINFO si;
				// Get all the vertial scroll bar information
				si.cbSize = sizeof (si);
				si.fMask  = SIF_ALL;
				GetScrollInfo( m_hScrollingPane, SB_VERT, &si );
				si.nPage = PageRect.bottom - PageRect.top;
				// Save the position for comparison later on
				INT iCurrentScrollPos = si.nPos;
				switch (LOWORD (wParam))
				{
					// user clicked the HOME keyboard key
				case SB_TOP:
					si.nPos = si.nMin;
					break;

					// user clicked the END keyboard key
				case SB_BOTTOM:
					si.nPos = si.nMax;
					break;

					// user clicked the top arrow
				case SB_LINEUP:
					si.nPos -= 10;
					break;

					// user clicked the bottom arrow
				case SB_LINEDOWN:
					si.nPos += 10;
					break;

					// user clicked the scroll bar shaft above the scroll box
				case SB_PAGEUP:
					si.nPos -= si.nPage;
					break;

					// user clicked the scroll bar shaft below the scroll box
				case SB_PAGEDOWN:
					si.nPos += si.nPage;
					break;

					// user dragged the scroll box
				case SB_THUMBTRACK:
					si.nPos = si.nTrackPos;
					break;

				default:
					break; 
				}
				// Set the position and then retrieve it.  Due to adjustments
				//   by Windows it may not be the same as the value set.
				si.fMask = SIF_POS;
				SetScrollInfo (m_hScrollingPane, SB_VERT, &si, TRUE);
				GetScrollInfo (m_hScrollingPane, SB_VERT, &si);
				// If the position has changed, scroll window and update it
				if( si.nPos != iCurrentScrollPos )
				{                    
					ScrollWindow(m_hScrollingPane, 0, ( iCurrentScrollPos - si.nPos ), NULL, NULL);
					UpdateWindow (m_hScrollingPane);
				}
				return FALSE;
			}
		case WM_HSCROLL:
			{
                if( m_bControlDataUpdate )
                    return FALSE;
				HWND hwndControl = (HWND)lParam;
				if( GetWindowLong( hwndControl, GWL_ID ) != IDC_DYNAMICCONTROL )
					return 1;
				ExportSettingsEntry* pEntry = (ExportSettingsEntry*)GetWindowLong( hwndControl, GWL_USERDATA );
				if( pEntry == NULL )
					return FALSE;
				INT iPos = (INT)SendMessage( hwndControl, TBM_GETPOS, 0, 0 );
				if( pEntry->m_Type == ExportSettingsEntry::CT_BOUNDEDINTSLIDER )
					pEntry->SetValue( iPos );
				else
					pEntry->SetValue( GetFloatFromNormalizedInt( pEntry, iPos ) );
				return FALSE;
			}
		}
		return FALSE;
	}

	LRESULT ExportSettingsDialog::OnNotify( INT idCtrl, LPNMHDR pnmh )
	{
		switch( idCtrl )
		{
		case IDC_CATEGORIES:
			switch( pnmh->code )
			{
			case TVN_SELCHANGED:
				{
					NM_TREEVIEW* pNMTV = (NM_TREEVIEW*)pnmh;
					ExportSettingsEntry* pEntry = (ExportSettingsEntry*)pNMTV->itemNew.lParam;
                    if( pEntry == NULL )
                        return TRUE;
					assert( pEntry != NULL );
					assert( pEntry->m_Type == ExportSettingsEntry::CT_CATEGORY );
					m_pCurrentCategory = pEntry;
					PopulateControls();
					return TRUE;
				}
			}
			return TRUE;
		}
		return FALSE;
	}
}