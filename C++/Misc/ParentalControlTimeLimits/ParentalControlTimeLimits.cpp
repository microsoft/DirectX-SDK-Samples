//--------------------------------------------------------------------------------------
// File: ParentalControlTimeLimits.cpp
//
// The Time Limits feature in Windows Parental Controls enables parents to manage
// their childrens' time on the computer.  A child with time restricitons will be locked
// out of their account during the hours specified by the parent.
//
// As a game developer, you should anticipate user time restrictions and adjust the
// user experience accordingly.  For instance, you may wish to provide a user with a
// UI cue that an account lock out is imminent then provide them with the option to save
// their progress.  You could also warn the user that they may not have adequate time to
// complete a game mission if the average time of the mission is determined to be greater
// than the time remaining.
//
// This sample demonstrates a small API (TimeLimitAPI.h) that you can leverage to obtain
// parental control time limit information for the current user.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "resource.h"
#include "TimeLimitAPI.h"

//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------
const int                   HoursPerDay = 24;
const int                   DaysPerWeek = 7;
const D3DCOLOR              ClrBkgnd = D3DCOLOR_XRGB( 0, 63, 63 );
const D3DCOLOR              ClrGridLine = D3DCOLOR_XRGB( 158, 158, 158 );
const D3DCOLOR              ClrGridCell = D3DCOLOR_XRGB( 187, 0, 0 );
const D3DCOLOR              ClrGridBkgnd = D3DCOLOR_XRGB( 255, 255, 255 );
const D3DCOLOR              ClrGridHours = D3DCOLOR_XRGB( 255, 255, 255 );
const D3DCOLOR              ClrGridDays = D3DCOLOR_XRGB( 0, 0, 0 );
const D3DCOLOR              ClrText = D3DCOLOR_XRGB( 190, 190, 190 );
const D3DCOLOR              ClrTime = D3DCOLOR_XRGB( 255, 242, 0 );

// GUI Event Constants
const int                   IDC_TOGGLEFULLSCREEN = 1;

//--------------------------------------------------------------------------------------
// Function Prototypes
//--------------------------------------------------------------------------------------
bool CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat,
                                  D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                 void* pUserContext );
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                void* pUserContext );
void CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnLostDevice( void* pUserContext );
void CALLBACK OnDestroyDevice( void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext );
void InitApp();

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
ID3DXFont*                  g_pMiscTextFont = NULL;
ID3DXFont*                  g_pDayOfWeekFont = NULL;
ID3DXFont*                  g_pHourFont = NULL;
ID3DXLine*                  g_pGridLine = NULL;
ID3DXSprite*                g_pSprite = NULL;
CD3DSettingsDlg             g_settingsDlg;
CDXUTDialog                 g_HUD;
CDXUTDialogResourceManager  g_dialogResourceManager;
POINT                       g_gridPos;
SIZE                        g_gridSize;
UINT                        g_gridCellWidth;
SIZE                        g_bbSize;

//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR, int )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Set the callback functions
    DXUTSetCallbackD3D9DeviceAcceptable( IsDeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnCreateDevice );
    DXUTSetCallbackD3D9FrameRender( OnFrameRender );
    DXUTSetCallbackD3D9DeviceReset( OnResetDevice );
    DXUTSetCallbackD3D9DeviceLost( OnLostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnDestroyDevice );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    // Initialize DXUT and create the desired Win32 window and Direct3D device for the application
    DXUTInit(); // Parse the command line and show msgboxes
    DXUTSetHotkeyHandling();    // hotkey handling
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen

    InitApp();

    DXUTCreateWindow( L"ParentalControlTimeLimits" );
    DXUTCreateDevice( true, 800, 600 );

    // Start the render loop
    DXUTMainLoop();

    return DXUTGetExitCode();
}

void InitApp()
{
    g_settingsDlg.Init( &g_dialogResourceManager );
    g_HUD.Init( &g_dialogResourceManager );

    g_HUD.SetFont( 1, L"Arial", 14, FW_BOLD );
    CDXUTElement* pElement = g_HUD.GetDefaultElement( DXUT_CONTROL_STATIC, 0 );
    if( pElement )
    {
        pElement->iFont = 1;
        pElement->dwTextFormat = DT_LEFT | DT_BOTTOM;
    }

    // Initialize dialogs
    int iY = 10;
    g_HUD.SetCallback( OnGUIEvent );
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 160, iY, 125, 22 );


}

//--------------------------------------------------------------------------------------
// Rejects any devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat,
                                  D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // Typically want to skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                                         AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
                                         D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// Before a device is created, modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
    // then switch to SWVP.
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    D3DCAPS9 caps;
    HRESULT hr;
    V( pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &caps ) );

    if( ( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) == 0 ||
        caps.VertexShaderVersion < D3DVS_VERSION( 1, 1 ) )
    {
        pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3DPOOL_MANAGED resources here 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                 void* pUserContext )
{
    HRESULT hr;
    V_RETURN( g_dialogResourceManager.OnD3D9CreateDevice( pd3dDevice ) );
    V_RETURN( g_settingsDlg.OnD3D9CreateDevice( pd3dDevice ) );

    V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pSprite ) );
    V_RETURN( D3DXCreateLine( pd3dDevice, &g_pGridLine ) );
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3DPOOL_DEFAULT resources here 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice,
                                const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;
    V_RETURN( g_dialogResourceManager.OnD3D9ResetDevice() );
    V_RETURN( g_settingsDlg.OnD3D9ResetDevice() );

    g_pSprite->OnResetDevice();
    g_pGridLine->OnResetDevice();

    const D3DCOLOR backColor = D3DCOLOR_ARGB( 0, 150, 150, 150 );
    const D3DCOLOR bottomColor = D3DCOLOR_ARGB( 0, 100, 100, 100 );
    g_HUD.SetBackgroundColors( bottomColor, bottomColor, backColor, backColor );
    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 305, 0 );
    g_HUD.SetSize( 305, pBackBufferSurfaceDesc->Height );

    g_bbSize.cx = pBackBufferSurfaceDesc->Width;
    g_bbSize.cy = pBackBufferSurfaceDesc->Height;

    // Adjust the dimensions of the restricted hour grid so it is always in
    // proportion no matter what the dimension of the back buffer is set to.
    if( g_bbSize.cx >= g_bbSize.cy )
    {
        // Since the back buffer width is greater than its height, we make the
        // restricted hour grid width the same dimension as the back buffer height.
        g_gridSize.cx = g_bbSize.cy;
    }
    else
    {
        // Since the back buffer height is greater than width, we set the
        // restricted hour grid width to 9/10 the size of the back buffer
        // width.
        g_gridSize.cx = g_bbSize.cx - ( int )( g_bbSize.cx * 0.1f );
    }

    // calculate size of each cell in the grid
    g_gridCellWidth = g_gridSize.cx / HoursPerDay;
    // recalc the grid width so it is evenly divisible by g_gridCellWidth
    g_gridSize.cx = g_gridCellWidth * HoursPerDay;
    // calculate the grid height
    g_gridSize.cy = g_gridCellWidth * DaysPerWeek;
    // set the grid's on-screen position
    g_gridPos.x = g_bbSize.cx / 2 - g_gridSize.cx / 2;
    g_gridPos.y = g_bbSize.cy / 2 - g_gridSize.cy / 3;

    // g_pHourFont
    V( D3DXCreateFont( pd3dDevice, g_gridCellWidth - 8, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Courier New",
                       &g_pHourFont ) );

    // g_pDayOfWeekFont
    V( D3DXCreateFont( pd3dDevice, g_gridCellWidth, 0, FW_BOLD, 1, TRUE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Courier New",
                       &g_pDayOfWeekFont ) );

    // g_pMiscTextFont
    V_RETURN( D3DXCreateFont( pd3dDevice, g_gridCellWidth - 5, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                              L"Arial", &g_pMiscTextFont ) );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Render the scene 
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    // If the settings dialog is being shown, then render
    // it instead of rendering the app's scene
    if( g_settingsDlg.IsActive() )
    {
        g_settingsDlg.OnRender( fElapsedTime );
        return;
    }

    HRESULT hr;

    // Clear the render target and the zbuffer
    V( pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, ClrBkgnd, 1.0f, 0 ) );

    // Obtain the top level surface
    IDirect3DSurface9* pSurface = NULL;
    V( pd3dDevice->GetRenderTarget( 0, &pSurface ) );

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
        //
        // Fill the restricted hours grid background
        //
        RECT drawRect;
        drawRect.left = g_gridPos.x;
        drawRect.top = g_gridPos.y;
        drawRect.right = drawRect.left + g_gridSize.cx - 1;
        drawRect.bottom = drawRect.top + g_gridSize.cy - 1;
        V( pd3dDevice->ColorFill( pSurface, &drawRect, ClrGridBkgnd ) );

        //
        // Paint the grid cells that correspond to restricted hours
        //
        // NOTE: You should not call the TimeLimitAPI functions on each update as they
        // are sloooooow.  We only do it here for demonstration.
        drawRect.left = 0;
        drawRect.right = g_bbSize.cx - 1;
        drawRect.top = 0;
        drawRect.bottom = ( g_bbSize.cy / 2 ) - ( g_gridCellWidth * 3 );

        CDXUTTextHelper miscTxt( g_pMiscTextFont, g_pSprite, g_gridCellWidth );
        miscTxt.Begin();
        miscTxt.SetForegroundColor( ClrTime );

        if( TimeLimitEnabled() )
        {
            //
            // Draw account lock out time
            //
            TimeRemaining remaining;
            if( SUCCEEDED( TimeLimitGetTimeRemaining( &remaining ) ) )
            {
                V( miscTxt.DrawFormattedTextLine( drawRect,
                                                  DT_CENTER | DT_VCENTER,
                                                  L"Time until account lock-out: %dd %dh %dm %ds",
                                                  remaining.days,
                                                  remaining.hours,
                                                  remaining.minutes,
                                                  remaining.seconds ) );
            }

            //
            // Fill in each grid cell that corresponds to a restriced hour
            //
            HourlyRestrictions restrictions;
            if( SUCCEEDED( TimeLimitGetHourlyRestrictions( &restrictions ) ) )
            {
                for( int day = 0; day < DaysPerWeek; ++day )
                {
                    for( int hour = 0; hour < HoursPerDay; ++hour )
                    {
                        unsigned int hourBitField = 1 << hour;
                        // A restricted hour is represented by an unset bit in the hour bit field
                        if( 0 == ( restrictions.days[day] & hourBitField ) )
                        {
                            // fill in the appropriate cell
                            RECT cell;
                            cell.left = g_gridPos.x + g_gridCellWidth * hour;
                            cell.right = cell.left + g_gridCellWidth;
                            cell.top = g_gridPos.y + g_gridCellWidth * day;
                            cell.bottom = cell.top + g_gridCellWidth;
                            V( pd3dDevice->ColorFill( pSurface, &cell, ClrGridCell ) );
                        }
                    }
                }
            }
        }
        else
        {
            V( miscTxt.DrawTextLine( drawRect,
                                     DT_CENTER | DT_VCENTER,
                                     L"User does not have time restrictions." ) );
        }
        miscTxt.End();

        // 
        // Draw the restricted hours grid
        //
        V( g_pGridLine->Begin() );

        // vertical lines
        D3DXVECTOR2 gridLine[2];                        // 2 Verts per line segment
        gridLine[0].y = ( float )( g_gridPos.y - 5 );    // - 5 allows for the vertical tick mark 
        gridLine[1].y = ( float )( g_gridPos.y + g_gridSize.cy - 1 );
        for( int i = 0; i < HoursPerDay; ++i )
        {
            // draw the left border of the column
            gridLine[0].x = ( float )( g_gridPos.x + i * g_gridCellWidth );
            gridLine[1].x = gridLine[0].x;
            V( g_pGridLine->Draw( gridLine, 2, ClrGridLine ) );
            // draw the right border of the column
            gridLine[0].x += g_gridCellWidth - 1;
            gridLine[1].x = gridLine[0].x;
            V( g_pGridLine->Draw( gridLine, 2, ClrGridLine ) );
        }

        // horizontal lines
        gridLine[0].x = ( float )g_gridPos.x;
        gridLine[1].x = ( float )( g_gridPos.x + g_gridSize.cx - 1 );
        for( int i = 0; i < DaysPerWeek; ++i )
        {
            // draw the top border of the row
            gridLine[0].y = ( float )( g_gridPos.y + i * g_gridCellWidth );
            gridLine[1].y = gridLine[0].y;
            V( g_pGridLine->Draw( gridLine, 2, ClrGridLine ) );
            // draw the bottom border of the row
            gridLine[0].y += g_gridCellWidth - 1;
            gridLine[1].y = gridLine[0].y;
            V( g_pGridLine->Draw( gridLine, 2, ClrGridLine ) );
        }

        V( g_pGridLine->End() );

        //
        // Draw the hour text above the grid
        //
        const WCHAR wstrTime[][3] =
        {
            L"12", L"1", L"2", L"3", L"4",
            L"5", L"6", L"7", L"8", L"9",
            L"10", L"11", L"12", L"1", L"2",
            L"3", L"4", L"5", L"6", L"7",
            L"8", L"9", L"10", L"11", L"12"
        };

        CDXUTTextHelper hourTxt( g_pHourFont, g_pSprite, g_gridCellWidth );
        hourTxt.Begin();
        hourTxt.SetForegroundColor( ClrGridHours );

        drawRect.top = g_gridPos.y - g_gridCellWidth - 5;
        drawRect.bottom = drawRect.top + g_gridCellWidth;
        for( int i = 0; i < sizeof( wstrTime ) / sizeof( wstrTime[0] ); ++i )
        {
            drawRect.left = g_gridPos.x + ( i * g_gridCellWidth ) - ( g_gridCellWidth / 2 );
            drawRect.right = drawRect.left + g_gridCellWidth;
            V( hourTxt.DrawTextLine( drawRect, DT_CENTER | DT_BOTTOM, wstrTime[i] ) );
        }
        hourTxt.End();

        //
        // Draw the day of the week text inside each grid row
        //
        const WCHAR wstrDays[DaysPerWeek][10] =
        {
            L"Sunday",    L"Monday",
            L"Tuesday",   L"Wednesday",
            L"Thursday",  L"Friday",
            L"Saturday"
        };

        CDXUTTextHelper dowTxt( g_pDayOfWeekFont, g_pSprite, g_gridCellWidth );
        dowTxt.Begin();
        dowTxt.SetForegroundColor( ClrGridDays );

        drawRect.left = g_gridPos.x;
        drawRect.right = drawRect.left + g_gridSize.cx - 1;
        for( int i = 0; i < DaysPerWeek; ++i )
        {
            drawRect.top = g_gridPos.y + i * g_gridCellWidth;
            drawRect.bottom = drawRect.top + g_gridCellWidth;
            V( dowTxt.DrawTextLine( drawRect, DT_CENTER | DT_VCENTER, wstrDays[i] ) );
        }
        dowTxt.End();

        //
        // Draw the midnight/noon differentiation markers above the hour text
        //
        miscTxt.Begin();
        miscTxt.SetForegroundColor( ClrText );
        miscTxt.SetInsertionPos( g_gridPos.x, g_gridPos.y - 2 * g_gridCellWidth );
        miscTxt.DrawTextLine( L"| Midnight (AM)" );
        miscTxt.SetInsertionPos( g_gridPos.x + g_gridSize.cx / 2, g_gridPos.y - 2 * g_gridCellWidth );
        miscTxt.DrawTextLine( L"| Noon (PM)" );

        //
        // Draw the legend
        //
        drawRect.left = g_gridPos.x;
        drawRect.right = drawRect.left + g_gridCellWidth;
        drawRect.top = g_gridPos.y + g_gridSize.cy + g_gridCellWidth;
        drawRect.bottom = drawRect.top + g_gridCellWidth;
        V( pd3dDevice->ColorFill( pSurface, &drawRect, ClrGridCell ) );

        OffsetRect( &drawRect, g_gridCellWidth, 0 );
        drawRect.right += g_gridSize.cx;
        V( miscTxt.DrawTextLine( drawRect, DT_LEFT | DT_VCENTER, L" - Blocked" ) );

        drawRect.left = g_gridPos.x;
        drawRect.right = drawRect.left + g_gridCellWidth;
        drawRect.top += g_gridCellWidth + 5;
        drawRect.bottom = drawRect.top + g_gridCellWidth;
        V( pd3dDevice->ColorFill( pSurface, &drawRect, ClrGridBkgnd ) );

        OffsetRect( &drawRect, g_gridCellWidth, 0 );
        drawRect.right += g_gridSize.cx;
        V( miscTxt.DrawTextLine( drawRect, DT_LEFT | DT_VCENTER, L" - Allowed" ) );

        miscTxt.End();

        V( g_HUD.OnRender( fElapsedTime ) );
        V( pd3dDevice->EndScene() );
    }

    SAFE_RELEASE( pSurface );
}


//--------------------------------------------------------------------------------------
// Handle messages to the application 
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext )
{
    // Always allow dialg resource manager calls to handle global messages
    // so GUI state is updated correctly
    *pbNoFurtherProcessing = g_dialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    if( g_settingsDlg.IsActive() )
    {
        g_settingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    return 0;
}

//--------------------------------------------------------------------------------------
// Handle the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
    }
}

//--------------------------------------------------------------------------------------
// Release resources created in the OnResetDevice callback here 
//--------------------------------------------------------------------------------------
void CALLBACK OnLostDevice( void* pUserContext )
{
    g_dialogResourceManager.OnD3D9LostDevice();
    g_settingsDlg.OnD3D9LostDevice();

    g_pSprite->OnLostDevice();
    g_pGridLine->OnLostDevice();

    SAFE_RELEASE( g_pMiscTextFont );
    SAFE_RELEASE( g_pHourFont );
    SAFE_RELEASE( g_pDayOfWeekFont );
}


//--------------------------------------------------------------------------------------
// Release resources created in the OnCreateDevice callback here
//--------------------------------------------------------------------------------------
void CALLBACK OnDestroyDevice( void* pUserContext )
{
    g_dialogResourceManager.OnD3D9DestroyDevice();
    g_settingsDlg.OnD3D9DestroyDevice();

    SAFE_RELEASE( g_pSprite );
    SAFE_RELEASE( g_pGridLine );
}
