//--------------------------------------------------------------------------------------
// File: InstallOnDemand.cpp
//
// Starting point for new Direct3D applications
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTsettingsdlg.h"
#include "DXUTcamera.h"
#include "SDKmesh.h"
#include "SDKmisc.h"
#include "resource.h"
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
// The Microsoft Platform SDK or Microsoft Windows SDK is required to compile this sample
#define _WIN32_MSI 300
#include "msiutil.h"
#include <msi.h>


//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 


#define LEVEL_COUNT 5


// Product ID
#define MSI_PRODUCTCODE L"{6CA2FDF1-6123-4EA2-931D-013A1FC43F9C}"

// Component IDs
#define CORECOMP        L"{DB30BD9C-DF36-4DD3-BFFE-507412E22ED2}"
#define LEVEL1COMP      L"{6E01B3C1-69E2-41F0-89EC-7EFDD29B36EE}"
#define LEVEL2COMP      L"{39147F4B-4199-4061-99FA-B5371AC580FC}"
#define LEVEL3COMP      L"{B032A754-9B7A-46A9-BE74-3A923113F617}"
#define LEVEL4COMP      L"{BDE10787-DBF5-424C-B38E-87AEB995CF5D}"
#define LEVEL5COMP      L"{57490C96-331E-4D74-B4DC-71C8004BCF1B}"
#define L1SKYBOXCOMP    L"{5413AED0-DCD7-4141-816B-DC9256F343A8}"
#define L2SKYBOXCOMP    L"{EC604FE7-02D0-48C4-A90D-B7BB479D1B39}"
#define L3SKYBOXCOMP    L"{ABBE7D9E-CAFD-4F4C-A489-66FA6C948767}"
#define L4SKYBOXCOMP    L"{8AEAD45E-8F35-4B8A-9C8C-C31E75092C32}"
#define L5SKYBOXCOMP    L"{B5C18BA9-FD63-4D7E-905D-98AF12BD9192}"
#define SPEEDERCOMP     L"{5FCB88FF-5CE1-48F4-B191-D20B262876AD}"
#define MISCCOMP        L"{C21CCB9D-25DC-4C24-A751-A75B0BE5CE0A}"
#define UICOMP          L"{2ACAE66E-ACC8-48CB-B8B5-EFFAAEE85C3E}"

const struct LEVELCOMPID
{
    WCHAR   szComp[39];
    WCHAR   szSkyboxComp[39];
} g_LevelCompId[LEVEL_COUNT] =
{
    { LEVEL1COMP, L1SKYBOXCOMP },
    { LEVEL2COMP, L2SKYBOXCOMP },
    { LEVEL3COMP, L3SKYBOXCOMP },
    { LEVEL4COMP, L4SKYBOXCOMP },
    { LEVEL5COMP, L5SKYBOXCOMP },
};


const WCHAR g_wszLevelFeatureName[LEVEL_COUNT][MAX_FEATURE_CHARS] =
{
    L"Level1",
    L"Level2",
    L"Level3",
    L"Level4",
    L"Level5",
};


struct QUADVERT
{
    float x, y, z, rhw;
    float u, v;
};


D3DVERTEXELEMENT9 g_aQuadDecl[] =
{
    { 0, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
    { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },
    D3DDECL_END()
};


struct SKYBOX_VERTEX
{
    D3DXVECTOR4 pos;
    D3DXVECTOR3 tex;
};


D3DVERTEXELEMENT9 g_aSkyboxDecl[] =
{
    { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    D3DDECL_END()
};


enum LOADSTATE
{
    AppState_Normal,        // The app is rendering the scene.
    AppState_Intro,         // The app is playing intro sequence.
    AppState_Menu,          // The app is displaying the main menu.
    AppState_NewGameMenu,   // The app is displaying the new game screen.
    AppState_OptionMenu,    // The app is displaying the option menu.
    AppState_Load,          // The app is displaying the loading screen.
    AppState_FullInstall,   // The app is performing full product install.
    AppState_Stats
};       // The app is displaying the end-of-level statistics.


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
ID3DXFont*                      g_pFont = NULL;         // Font for drawing text
ID3DXFont*                      g_pBigFont = NULL;      // Font for drawing text at loading screen
ID3DXSprite*                    g_pTextSprite = NULL;   // Sprite for batching draw text calls
ID3DXEffect*                    g_pEffect = NULL;       // D3DX effect interface
IDirect3DVertexDeclaration9*    g_pQuadDecl = NULL;// Vertex decl for quad that represents progress bars.
CFirstPersonCamera              g_Camera;               // A model viewing camera
bool                            g_bShowHelp = true;     // If true, it renders the UI control text
CDXUTDialogResourceManager      g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg                 g_SettingsDlg;          // Device settings dialog
CDXUTDialog                     g_HUD;                  // dialog for standard controls
CDXUTDialog                     g_SampleUI;             // dialog for sample specific controls
CDXUTDialog                     g_MainMenu;             // Dialog containing the main game menu
CDXUTDialog                     g_NewGameMenu;          // Dialog containing the new game screen
CDXUTDialog                     g_OptionMenu;           // Dialog containing the option menu
CDXUTDialog                     g_StatResult;           // Statistics of the completed level
IDirect3DTexture9*              g_pLoadTex = NULL;      // Texture for loading screen
CDXUTXFileMesh                  g_Speeder;              // Mesh for the speeder
CDXUTXFileMesh                  g_IntroTerrain;         // Mesh for the intro terrain
CDXUTXFileMesh                  g_LevelMesh;            // Mesh for level terrain
D3DXMATRIXA16                   g_mLevelWorld;          // World matrix for level terrain
IDirect3DTexture9*              g_pLevelTex = NULL;     // Level terrain texture
IDirect3DCubeTexture9*          g_pLevelEnvTex = NULL;  // Level environment map texture
IDirect3DVertexBuffer9*         g_pSkyboxVB = NULL;     // Vertex buffer for rendering skybox
IDirect3DVertexDeclaration9*    g_pSkyboxDecl = NULL; // Vertex decl for skybox
int                             g_nCurrentLevel = -1;  // Current level. -1 = no level, 0 = level 1, 1 = level 2, so on.
LOADSTATE                       g_AppState = AppState_Intro;
bool                            g_bLevelLoaded = false;
D3DXMATRIXA16                   g_mIntroWorldMatrix;    // World matrix for intro sequence (speeder)
DWORD                           g_dwLoadLevel;          // Level being install and to load when install is done.
HANDLE                          g_hEvent = NULL;        // Event for installation notification
CMsiUtil                        g_Msi;                  // CMsiUtil object to handle installation tasks


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN        1
#define IDC_TOGGLEREF               2
#define IDC_CHANGEDEVICE            3
#define IDC_LOADLEVEL1              4
#define IDC_LOADLEVEL2              5
#define IDC_LOADLEVEL3              6
#define IDC_LOADLEVEL4              7
#define IDC_LOADLEVEL5              8
// Main menu
#define IDC_NEWGAME                 9
#define IDC_OPTION                  10
#define IDC_QUITGAME                11
#define IDC_BACKTOGAME              12
// Option menu
#define IDC_OPTION_FULLINSTALL      13
#define IDC_OPTION_PREVIOUS         14
// New game menu
#define IDC_NEW_EDITNAMETEXT        15
#define IDC_NEW_EDITNAME            16
#define IDC_NEW_RADIOSEX1           17
#define IDC_NEW_RADIOSEX2           18
#define IDC_NEW_STR                 19
#define IDC_NEW_STA                 20
#define IDC_NEW_DEX                 21
#define IDC_NEW_AGI                 22
#define IDC_NEW_INT                 23
#define IDC_NEW_WIS                 24
#define IDC_NEW_REROLL              25
#define IDC_NEW_ABORT               26
#define IDC_NEW_ACCEPT              27
#define IDC_STAT_YOURKILL           28
#define IDC_STAT_YOURKILL_VALUE     29
#define IDC_STAT_TEAMKILL           30
#define IDC_STAT_TEAMKILL_VALUE     31
#define IDC_STAT_ENEMYKILL          32
#define IDC_STAT_ENEMYKILL_VALUE    33
#define IDC_STAT_YOURRAZED          34
#define IDC_STAT_YOURRAZED_VALUE    35
#define IDC_STAT_TEAMRAZED          36
#define IDC_STAT_TEAMRAZED_VALUE    37
#define IDC_STAT_ENEMYRAZED         38
#define IDC_STAT_ENEMYRAZED_VALUE   39
#define IDC_STAT_OK                 40


//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed,
                                  void* pUserContext );
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                 void* pUserContext );
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void CALLBACK OnLostDevice( void* pUserContext );
void CALLBACK OnDestroyDevice( void* pUserContext );

void InitApp();
HRESULT LoadMesh( IDirect3DDevice9* pd3dDevice, WCHAR* strFileName, ID3DXMesh** ppMesh );
void RenderText();


INSTALLSTATE GetFilePathFromMsi( LPCWSTR szProduct, LPCWSTR szComponent, LPCWSTR szFileName, LPWSTR str, UINT cSize )
{
    WCHAR* pcSlash;
    DWORD dwSize = cSize;
    INSTALLSTATE State = MsiGetComponentPath( szProduct, szComponent, str, &dwSize );
    str[MAX_PATH - 1] = L'\0';

    // If szFileName is not NULL, replace the component key name with it.
    if( NULL != szFileName )
    {
        pcSlash = wcsrchr( str, L'\\' );
        if( pcSlash )
        {
            *pcSlash = L'\0';
            StringCchCat( str, cSize, L"\\" );
        }
        else
        {
            pcSlash = str;
            *pcSlash = L'\0';
        }
        StringCchCat( str, cSize, szFileName );
    }

    return State;
}

HRESULT LoadLevelResources( IDirect3DDevice9* pd3dDevice )
{
    HRESULT hr;
    WCHAR str[MAX_PATH];

    GetFilePathFromMsi( MSI_PRODUCTCODE, g_LevelCompId[g_nCurrentLevel].szComp, NULL, str, MAX_PATH );
    V_RETURN( D3DXCreateTextureFromFile( pd3dDevice, str, &g_pLevelTex ) );
    GetFilePathFromMsi( MSI_PRODUCTCODE, g_LevelCompId[g_nCurrentLevel].szSkyboxComp, NULL, str, MAX_PATH );
    V_RETURN( D3DXCreateCubeTextureFromFile( pd3dDevice, str, &g_pLevelEnvTex ) );

    return S_OK;
}


// bLoad controls whether to load the resources. If bLoad is true,
// LoadLevel will attempt to install the feature then load the resources.
// If bLoad is false, LoadLevel will attempt to install the feature only.
HRESULT LoadLevel( IDirect3DDevice9* pd3dDevice, bool bLoad )
{
    HRESULT hr;

    // Disable helper loading buttons.  It doesn't make sense to request
    // another level while a requested level is loading.
    g_SampleUI.SetControlEnabled( IDC_LOADLEVEL1, false );
    g_SampleUI.SetControlEnabled( IDC_LOADLEVEL2, false );
    g_SampleUI.SetControlEnabled( IDC_LOADLEVEL3, false );
    g_SampleUI.SetControlEnabled( IDC_LOADLEVEL4, false );
    g_SampleUI.SetControlEnabled( IDC_LOADLEVEL5, false );

    // Call UseFeature() to request the resources for the feature corresponding to this level.
    // This feature may not have been installed.  UseFeature() queues the request and invokes
    // MSI to install in this case.
    bool bInstalled;
    V_RETURN( g_Msi.UseFeatureSet( g_wszLevelFeatureName[g_dwLoadLevel], g_hEvent, bInstalled ) );

    // If the feature is installed, then we can load.
    if( bInstalled && bLoad )
    {
        g_AppState = AppState_Load;
        DXUTPause( true, false );
        DXUTRender3DEnvironment();

        // Release current resources before we create new ones.
        SAFE_RELEASE( g_pLevelTex );
        SAFE_RELEASE( g_pLevelEnvTex );

        g_nCurrentLevel = g_dwLoadLevel;  // Update current level number

        // Load resources for the new level.
        LoadLevelResources( pd3dDevice );

        Sleep( 500 );   // Pause 0.5 second so that the user has enough time to notice
        // the loading screen.

        g_AppState = AppState_Normal;

        // If the intro is not playing, we want to re-enable the level buttons.
        if( AppState_Intro != g_AppState )
        {
            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL1, true );
            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL2, true );
            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL3, true );
            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL4, true );
            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL5, true );
        }

        // Reset the camera's view parameters
        D3DXVECTOR3 vecEye( 0.0f, 10.0f, -5.0f );
        D3DXVECTOR3 vecAt ( 0.0f, 8.0f, -0.0f );
        g_Camera.SetViewParams( &vecEye, &vecAt );

        DXUTPause( false, false );
    }

    return hr;
}


void PerformFullInstall()
{
    // Call UseProduct() to request for the entire product.
    g_AppState = AppState_FullInstall;
    g_Msi.UseProduct( g_hEvent );
}


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR, int )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Check if this product is installed.  The sample must be installed by Windows
    // Installer first before it can be run, so exit if this is not the case.
    INSTALLSTATE ProductState = MsiQueryProductState( MSI_PRODUCTCODE );
    if( INSTALLSTATE_UNKNOWN == ProductState )
    {
        MessageBox( NULL, L"The sample must be installed by Windows Installer first before "
                    L"it can run.  Please use the Install Project option in the Sample "
                    L"Browser to copy the sample files to a folder, then launch "
                    L"InstallOnDemand.msi in that folder.",
                    L"Sample Error", MB_OK | MB_ICONSTOP );
        return 0;
    }

    // Set the callback functions. These functions allow DXUT to notify
    // the application about device changes, user input, and windows messages.  The 
    // callbacks are optional so you need only set callbacks for events you're interested 
    // in. However, if you don't handle the device reset/lost callbacks then the sample 
    // framework won't be able to reset your device since the application must first 
    // release all device resources before resetting.  Likewise, if you don't handle the 
    // device created/destroyed callbacks then DXUT won't be able to 
    // recreate your device resources.
    DXUTSetCallbackD3D9DeviceAcceptable( IsDeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnCreateDevice );
    DXUTSetCallbackD3D9FrameRender( OnFrameRender );
    DXUTSetCallbackD3D9DeviceReset( OnResetDevice );
    DXUTSetCallbackD3D9DeviceLost( OnLostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnDestroyDevice );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( KeyboardProc );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    // Show the cursor and clip it when in full screen
    DXUTSetCursorSettings( true, true );

    InitApp();

    // Initialize the event object and g_Msi object
    g_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    HRESULT hr = g_Msi.Initialize( MSI_PRODUCTCODE );
    if( !g_hEvent || FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"InstallOnDemand is unable to create resources necessary for its execution. The program will shut down.",
                    L"Critical Error", MB_OK );
        goto Cleanup;
    }
    g_Msi.EnablePassiveInstall( true );  // Before we enter the real gameplay, it's ok to
    // do background install.
    g_Msi.EnablePassiveInstall( true );  // This extra call is to offset the decrement caused by
    // the very first WM_ACTIVATEAPP message that the
    // application receives.

    // Initialize DXUT and create the desired Win32 window and Direct3D 
    // device for the application. Calling each of these functions is optional, but they
    // allow you to set several options which control the behavior of the framework.
    DXUTInit(); // Parse the command line, handle the default hotkeys, and show msgboxes
    DXUTSetHotkeyHandling();    // handle the default hotkeys
    DXUTCreateWindow( L"InstallOnDemand" );
    DXUTCreateDevice( true, 640, 480 );

    // Pass control to DXUT for handling the message pump and 
    // dispatching render calls. DXUT will call your FrameMove 
    // and FrameRender callback when there is idle time between handling window messages.
    DXUTMainLoop();

    // Perform any application-level cleanup here. Direct3D device resources are released within the
    // appropriate callback functions and therefore don't require any cleanup code here.

Cleanup:
    if( g_hEvent )
        CloseHandle( g_hEvent );

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    // Initialize dialogs
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );
    g_MainMenu.Init( &g_DialogResourceManager );
    g_NewGameMenu.Init( &g_DialogResourceManager );
    g_OptionMenu.Init( &g_DialogResourceManager );
    g_StatResult.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2 );

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 10;
    g_SampleUI.AddButton( IDC_LOADLEVEL1, L"Play Level 1", 35, iY += 24, 125, 22 );
    g_SampleUI.AddButton( IDC_LOADLEVEL2, L"Play Level 2", 35, iY += 24, 125, 22 );
    g_SampleUI.AddButton( IDC_LOADLEVEL3, L"Play Level 3", 35, iY += 24, 125, 22 );
    g_SampleUI.AddButton( IDC_LOADLEVEL4, L"Play Level 4", 35, iY += 24, 125, 22 );
    g_SampleUI.AddButton( IDC_LOADLEVEL5, L"Play Level 5", 35, iY += 24, 125, 22 );
    g_SampleUI.SetControlEnabled( IDC_LOADLEVEL1, false );
    g_SampleUI.SetControlEnabled( IDC_LOADLEVEL2, false );
    g_SampleUI.SetControlEnabled( IDC_LOADLEVEL3, false );
    g_SampleUI.SetControlEnabled( IDC_LOADLEVEL4, false );
    g_SampleUI.SetControlEnabled( IDC_LOADLEVEL5, false );

    // Initialize the main menu, initially minimized.
    g_MainMenu.SetFont( 0, L"Arial", 24, FW_NORMAL );
    g_MainMenu.SetVisible( false );
    g_MainMenu.SetCallback( OnGUIEvent ); iY = 0;
    g_MainMenu.AddButton( IDC_NEWGAME, L"New Game", 0, iY, 200, 40 );
    g_MainMenu.AddButton( IDC_BACKTOGAME, L"Return to Game", 0, iY, 200, 40 );
    g_MainMenu.AddButton( IDC_OPTION, L"Option", 0, iY += 50, 200, 40 );
    g_MainMenu.AddButton( IDC_QUITGAME, L"Quit Game", 0, iY += 50, 200, 40, VK_ESCAPE );
    // New Game and Back to Game overlap each other. As main menu, return to game is
    // hidden.  As in-game menu, New Game is hidden.
    // Initially, Back to Game button is invisible.
    g_MainMenu.GetControl( IDC_BACKTOGAME )->SetVisible( false );

    // Initialize the option menu, initially minimized.
    g_OptionMenu.SetFont( 0, L"Arial", 24, FW_NORMAL );
    g_OptionMenu.SetVisible( false );
    g_OptionMenu.SetCallback( OnGUIEvent ); iY = 0;
    g_OptionMenu.AddButton( IDC_OPTION_FULLINSTALL, L"Perform Full Install", 0, iY, 200, 40 );
    g_OptionMenu.AddButton( IDC_OPTION_PREVIOUS, L"Previous Menu", 0, iY += 50, 200, 40, VK_ESCAPE );

    // Initialize the new game menu, initially minimized.
    g_NewGameMenu.SetFont( 0, L"Arial", 24, FW_NORMAL );
    g_NewGameMenu.SetVisible( false );
    g_NewGameMenu.SetCallback( OnGUIEvent ); iY = 0;
    g_NewGameMenu.AddStatic( IDC_NEW_EDITNAMETEXT, L"Type in new character name:", 0, iY, 300, 20 );
    g_NewGameMenu.AddEditBox( IDC_NEW_EDITNAME, L"", 0, iY += 25, 300, 40 );
    g_NewGameMenu.AddRadioButton( IDC_NEW_RADIOSEX1, 0, L"Male", 50, iY += 45, 100, 20 );
    g_NewGameMenu.AddRadioButton( IDC_NEW_RADIOSEX2, 0, L"Female", 150, iY, 100, 20 );
    g_NewGameMenu.AddStatic( IDC_NEW_STR, L"STR", 50, iY += 30, 200, 20 );
    g_NewGameMenu.AddStatic( IDC_NEW_STA, L"STA", 50, iY += 30, 200, 20 );
    g_NewGameMenu.AddStatic( IDC_NEW_DEX, L"DEX", 50, iY += 30, 200, 20 );
    g_NewGameMenu.AddStatic( IDC_NEW_AGI, L"AGI", 50, iY += 30, 200, 20 );
    g_NewGameMenu.AddStatic( IDC_NEW_INT, L"INT", 50, iY += 30, 200, 20 );
    g_NewGameMenu.AddStatic( IDC_NEW_WIS, L"WIS", 50, iY += 30, 200, 20 );
    g_NewGameMenu.AddButton( IDC_NEW_REROLL, L"Reroll Stats", 50, iY += 40, 200, 40 );
    g_NewGameMenu.AddButton( IDC_NEW_ABORT, L"Abort", 50, iY += 50, 100, 40, VK_ESCAPE );
    g_NewGameMenu.AddButton( IDC_NEW_ACCEPT, L"Accept", 150, iY, 100, 40, VK_RETURN );

    // Initialize the stats result dialog, initially minimized.
    g_StatResult.SetFont( 0, L"Arial", 24, FW_NORMAL );
    g_StatResult.SetVisible( false );
    g_StatResult.SetCallback( OnGUIEvent ); iY = 0;
    g_StatResult.AddStatic( IDC_STAT_YOURKILL, L"Your kill", 0, iY, 300, 24 );
    g_StatResult.GetControl( IDC_STAT_YOURKILL )->GetElement( 0 )->dwTextFormat = DT_LEFT | DT_VCENTER | DT_WORDBREAK;
    g_StatResult.AddStatic( IDC_STAT_YOURKILL_VALUE, L"0", 0, iY, 300, 24 );
    g_StatResult.GetControl( IDC_STAT_YOURKILL_VALUE )->GetElement( 0 )->dwTextFormat = DT_RIGHT | DT_VCENTER |
        DT_WORDBREAK;
    g_StatResult.AddStatic( IDC_STAT_TEAMKILL, L"Allies' kill", 0, iY += 30, 300, 24 );
    g_StatResult.GetControl( IDC_STAT_TEAMKILL )->GetElement( 0 )->dwTextFormat = DT_LEFT | DT_VCENTER | DT_WORDBREAK;
    g_StatResult.AddStatic( IDC_STAT_TEAMKILL_VALUE, L"0", 0, iY, 300, 24 );
    g_StatResult.GetControl( IDC_STAT_TEAMKILL_VALUE )->GetElement( 0 )->dwTextFormat = DT_RIGHT | DT_VCENTER |
        DT_WORDBREAK;
    g_StatResult.AddStatic( IDC_STAT_ENEMYKILL, L"Enemy's kill", 0, iY += 30, 300, 24 );
    g_StatResult.GetControl( IDC_STAT_ENEMYKILL )->GetElement( 0 )->dwTextFormat = DT_LEFT | DT_VCENTER | DT_WORDBREAK;
    g_StatResult.AddStatic( IDC_STAT_ENEMYKILL_VALUE, L"0", 0, iY, 300, 24 );
    g_StatResult.GetControl( IDC_STAT_ENEMYKILL_VALUE )->GetElement( 0 )->dwTextFormat = DT_RIGHT | DT_VCENTER |
        DT_WORDBREAK;
    g_StatResult.AddStatic( IDC_STAT_YOURRAZED, L"Structures Razed by you", 0, iY += 46, 300, 24 );
    g_StatResult.GetControl( IDC_STAT_YOURRAZED )->GetElement( 0 )->dwTextFormat = DT_LEFT | DT_VCENTER | DT_WORDBREAK;
    g_StatResult.AddStatic( IDC_STAT_YOURRAZED_VALUE, L"0", 0, iY, 300, 24 );
    g_StatResult.GetControl( IDC_STAT_YOURRAZED_VALUE )->GetElement( 0 )->dwTextFormat = DT_RIGHT | DT_VCENTER |
        DT_WORDBREAK;
    g_StatResult.AddStatic( IDC_STAT_TEAMRAZED, L"Structures Razed by allies", 0, iY += 30, 300, 24 );
    g_StatResult.GetControl( IDC_STAT_TEAMRAZED )->GetElement( 0 )->dwTextFormat = DT_LEFT | DT_VCENTER | DT_WORDBREAK;
    g_StatResult.AddStatic( IDC_STAT_TEAMRAZED_VALUE, L"0", 0, iY, 300, 24 );
    g_StatResult.GetControl( IDC_STAT_TEAMRAZED_VALUE )->GetElement( 0 )->dwTextFormat = DT_RIGHT | DT_VCENTER |
        DT_WORDBREAK;
    g_StatResult.AddStatic( IDC_STAT_ENEMYRAZED, L"Structures Razed by enemy", 0, iY += 30, 300, 24 );
    g_StatResult.GetControl( IDC_STAT_ENEMYRAZED )->GetElement( 0 )->dwTextFormat = DT_LEFT | DT_VCENTER |
        DT_WORDBREAK;
    g_StatResult.AddStatic( IDC_STAT_ENEMYRAZED_VALUE, L"0", 0, iY, 300, 24 );
    g_StatResult.GetControl( IDC_STAT_ENEMYRAZED_VALUE )->GetElement( 0 )->dwTextFormat = DT_RIGHT | DT_VCENTER |
        DT_WORDBREAK;
    g_StatResult.AddButton( IDC_STAT_OK, L"OK", 100, iY += 80, 100, 30, VK_RETURN );
    g_StatResult.GetControl( IDC_STAT_YOURKILL )->GetElement( 0 )->FontColor.States[DXUT_STATE_NORMAL] =
        g_StatResult.GetControl( IDC_STAT_YOURKILL_VALUE )->GetElement( 0 )->FontColor.States[DXUT_STATE_NORMAL] =
        g_StatResult.GetControl( IDC_STAT_TEAMKILL )->GetElement( 0 )->FontColor.States[DXUT_STATE_NORMAL] =
        g_StatResult.GetControl( IDC_STAT_TEAMKILL_VALUE )->GetElement( 0 )->FontColor.States[DXUT_STATE_NORMAL] =
        g_StatResult.GetControl( IDC_STAT_ENEMYKILL )->GetElement( 0 )->FontColor.States[DXUT_STATE_NORMAL] =
        g_StatResult.GetControl( IDC_STAT_ENEMYKILL_VALUE )->GetElement( 0 )->FontColor.States[DXUT_STATE_NORMAL] =
        g_StatResult.GetControl( IDC_STAT_YOURRAZED )->GetElement( 0 )->FontColor.States[DXUT_STATE_NORMAL] =
        g_StatResult.GetControl( IDC_STAT_YOURRAZED_VALUE )->GetElement( 0 )->FontColor.States[DXUT_STATE_NORMAL] =
        g_StatResult.GetControl( IDC_STAT_TEAMRAZED )->GetElement( 0 )->FontColor.States[DXUT_STATE_NORMAL] =
        g_StatResult.GetControl( IDC_STAT_TEAMRAZED_VALUE )->GetElement( 0 )->FontColor.States[DXUT_STATE_NORMAL] =
        g_StatResult.GetControl( IDC_STAT_ENEMYRAZED )->GetElement( 0 )->FontColor.States[DXUT_STATE_NORMAL] =
        g_StatResult.GetControl( IDC_STAT_ENEMYRAZED_VALUE )->GetElement( 0 )->FontColor.States[DXUT_STATE_NORMAL] =
        D3DCOLOR_ARGB( 255, 96, 128, 255 );

    // Setup thec camera
    g_Camera.SetScalers( 0.01f, 15.0f );
    D3DXVECTOR3 vecEye( 0.0f, 1.0f, -5.0f );
    D3DXVECTOR3 vecAt ( 0.0f, 1.0f, -0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );

    D3DXMatrixScaling( &g_mLevelWorld, 200.0f, 200.0f, 200.0f );

    // Initialize the intro world matrix for speeder
    D3DXMATRIXA16 m;
    D3DXMatrixScaling( &g_mIntroWorldMatrix, 0.005f, 0.005f, 0.005f );
    D3DXMatrixTranslation( &m, 10.0f, 0.0f, 0.0f );
    g_mIntroWorldMatrix *= m;
}


//--------------------------------------------------------------------------------------
// Called during device initialization, this code checks the device for some 
// minimum set of capabilities, and rejects those that don't pass by returning false.
//--------------------------------------------------------------------------------------
bool CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat,
                                  D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // Skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                                         AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
                                         D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    // Must support pixel shader 2.0
    if( pCaps->PixelShaderVersion < D3DPS_VERSION( 2, 0 ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// DXUT will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise IDirect3D9::CreateDevice() will fail.  
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    D3DCAPS9 caps;
    HRESULT hr;
    V( pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &caps ) );

    // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
    // then switch to SWVP.
    if( ( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) == 0 ||
        caps.VertexShaderVersion < D3DVS_VERSION( 1, 1 ) )
    {
        pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }

    // Debugging vertex shaders requires either REF or software vertex processing 
    // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
    if( pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF )
    {
        pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
        pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
        pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }
#endif
#ifdef DEBUG_PS
    pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }

    return true;
}


// Compute a matrix that scales Mesh to a specified size and centers around origin
void ComputeMeshScaling( ID3DXMesh* pMesh, D3DXMATRIX* pmScalingCenter, float fTargetRadius )
{
    LPVOID pVerts = NULL;
    D3DXMatrixIdentity( pmScalingCenter );
    if( SUCCEEDED( pMesh->LockVertexBuffer( 0, &pVerts ) ) )
    {
        D3DXVECTOR3 vCtr;
        D3DXVECTOR3 vMin, vMax;
        if( SUCCEEDED( D3DXComputeBoundingBox( ( const D3DXVECTOR3* )pVerts,
                                               pMesh->GetNumVertices(),
                                               pMesh->GetNumBytesPerVertex(),
                                               &vMin, &vMax ) ) )
        {
            D3DXMatrixTranslation( pmScalingCenter, -vMin.x - ( vMax.x - vMin.x ) * 0.5f, -vMin.y -
                                   ( vMax.y - vMin.y ) * 0.5f, -vMin.z - ( vMax.z - vMin.z ) * 0.5f );
            D3DXMATRIXA16 m;
            D3DXMatrixScaling( &m, fTargetRadius / ( vMax.y - vMin.y ),
                               fTargetRadius / ( vMax.y - vMin.y ),
                               fTargetRadius / ( vMax.y - vMin.y ) );
            D3DXMatrixMultiply( pmScalingCenter, pmScalingCenter, &m );
        }
        pMesh->UnlockVertexBuffer();
    }
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// created, which will happen during application initialization and windowed/full screen 
// toggles. This is the best location to create D3DPOOL_MANAGED resources since these 
// resources need to be reloaded whenever the device is destroyed. Resources created  
// here should be released in the OnDestroyDevice callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                 void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D9CreateDevice( pd3dDevice ) );
    V_RETURN( g_SettingsDlg.OnD3D9CreateDevice( pd3dDevice ) );

    // Initialize the fonts
    V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                              L"Arial", &g_pFont ) );
    V_RETURN( D3DXCreateFont( pd3dDevice, 48, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                              L"Arial", &g_pBigFont ) );

    // Define DEBUG_VS and/or DEBUG_PS to debug vertex and/or pixel shaders with the 
    // shader debugger. Debugging vertex shaders requires either REF or software vertex 
    // processing, and debugging pixel shaders requires REF.  The 
    // D3DXSHADER_FORCE_*_SOFTWARE_NOOPT flag improves the debug experience in the 
    // shader debugger.  It enables source level debugging, prevents instruction 
    // reordering, prevents dead code elimination, and forces the compiler to compile 
    // against the next higher available software target, which ensures that the 
    // unoptimized shaders do not exceed the shader model limitations.  Setting these 
    // flags will cause slower rendering since the shaders will be unoptimized and 
    // forced into software.  See the DirectX documentation for more information about 
    // using the shader debugger.
    DWORD dwShaderFlags = 0;
#ifdef DEBUG_VS
        dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
    #endif
#ifdef DEBUG_PS
        dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
    #endif

    // Read the D3DX effect file
    INSTALLSTATE State;
    WCHAR str[MAX_PATH];
    State = GetFilePathFromMsi( MSI_PRODUCTCODE, CORECOMP, L"InstallOnDemand.fx", str, MAX_PATH );

    // If this fails, there should be debug output as to 
    // they the .fx file failed to compile
    V_RETURN( D3DXCreateEffectFromFile( pd3dDevice, str, NULL, NULL, dwShaderFlags,
                                        NULL, &g_pEffect, NULL ) );

    // Create the vertex declaration for the progress bar quad
    V_RETURN( pd3dDevice->CreateVertexDeclaration( g_aQuadDecl, &g_pQuadDecl ) );

    // Create the vertex declaration for the skybox
    V_RETURN( pd3dDevice->CreateVertexDeclaration( g_aSkyboxDecl, &g_pSkyboxDecl ) );

    // Create the level mesh
    State = GetFilePathFromMsi( MSI_PRODUCTCODE, CORECOMP, L"Level.x", str, MAX_PATH );
    V_RETURN( g_LevelMesh.Create( pd3dDevice, str ) );
    V_RETURN( g_LevelMesh.SetFVF( pd3dDevice, g_LevelMesh.GetMesh()->GetFVF() | D3DFVF_NORMAL ) );

    // Create the hover speeder mesh
    State = GetFilePathFromMsi( MSI_PRODUCTCODE, SPEEDERCOMP, L"LandShark.x", str, MAX_PATH );
    V_RETURN( g_Speeder.Create( pd3dDevice, str ) );
    V_RETURN( g_Speeder.SetFVF( pd3dDevice, g_LevelMesh.GetMesh()->GetFVF() | D3DFVF_NORMAL ) );

    // Create the intro terrain mesh
    State = GetFilePathFromMsi( MSI_PRODUCTCODE, MISCCOMP, L"seafloor.x", str, MAX_PATH );
    V_RETURN( g_IntroTerrain.Create( pd3dDevice, str ) );
    V_RETURN( g_IntroTerrain.SetFVF( pd3dDevice, g_LevelMesh.GetMesh()->GetFVF() | D3DFVF_NORMAL ) );

    // Load the level loading screen texture
    State = GetFilePathFromMsi( MSI_PRODUCTCODE, CORECOMP, L"Loading.bmp", str, MAX_PATH );
    V_RETURN( D3DXCreateTextureFromFile( pd3dDevice, str, &g_pLoadTex ) );

    // If the player was in a level, the level resources must be
    // reloaded on device reset.
    if( -1 != g_nCurrentLevel )
        LoadLevelResources( pd3dDevice );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// reset, which will happen after a lost device scenario. This is the best location to 
// create D3DPOOL_DEFAULT resources since these resources need to be reloaded whenever 
// the device is lost. Resources created here should be released in the OnLostDevice 
// callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice,
                                const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D9ResetDevice() );
    V_RETURN( g_SettingsDlg.OnD3D9ResetDevice() );

    if( g_pFont )
        V_RETURN( g_pFont->OnResetDevice() );
    if( g_pBigFont )
        V_RETURN( g_pBigFont->OnResetDevice() );
    if( g_pEffect )
        V_RETURN( g_pEffect->OnResetDevice() );

    // Create a sprite to help batch calls when drawing many lines of text
    V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pTextSprite ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 0.1f, 1000.0f );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 350 );
    g_SampleUI.SetSize( 170, 300 );
    g_MainMenu.SetLocation( pBackBufferSurfaceDesc->Width / 2 - 100, pBackBufferSurfaceDesc->Height / 2 - 100 );
    g_MainMenu.SetSize( 200, 200 );
    g_OptionMenu.SetLocation( pBackBufferSurfaceDesc->Width / 2 - 100, pBackBufferSurfaceDesc->Height / 2 - 100 );
    g_OptionMenu.SetSize( 200, 200 );
    g_NewGameMenu.SetLocation( pBackBufferSurfaceDesc->Width / 2 - 150, pBackBufferSurfaceDesc->Height / 2 - 200 );
    g_NewGameMenu.SetSize( 300, 400 );
    g_StatResult.SetLocation( pBackBufferSurfaceDesc->Width / 2 - 150, pBackBufferSurfaceDesc->Height / 2 - 100 );
    g_StatResult.SetSize( 300, 300 );

    V_RETURN( g_LevelMesh.RestoreDeviceObjects( pd3dDevice ) );
    V_RETURN( g_Speeder.RestoreDeviceObjects( pd3dDevice ) );
    V_RETURN( g_IntroTerrain.RestoreDeviceObjects( pd3dDevice ) );

    // Create a quad vertex buffer for skybox rendering
    V_RETURN( pd3dDevice->CreateVertexBuffer( 4 * sizeof( SKYBOX_VERTEX ),
                                              D3DUSAGE_WRITEONLY, 0,
                                              D3DPOOL_DEFAULT, &g_pSkyboxVB, NULL ) );

    // Fill the vertex buffer
    SKYBOX_VERTEX* pVertex = NULL;
    V_RETURN( g_pSkyboxVB->Lock( 0, 0, ( void** )&pVertex, 0 ) );

    // Map texels to pixels 
    float fHighW = -1.0f - ( 1.0f / ( float )pBackBufferSurfaceDesc->Width );
    float fHighH = -1.0f - ( 1.0f / ( float )pBackBufferSurfaceDesc->Height );
    float fLowW = 1.0f - ( 1.0f / ( float )pBackBufferSurfaceDesc->Width );
    float fLowH = 1.0f - ( 1.0f / ( float )pBackBufferSurfaceDesc->Height );
    pVertex[0].pos = D3DXVECTOR4( fLowW, fLowH, 1.0f, 1.0f );
    pVertex[1].pos = D3DXVECTOR4( fLowW, fHighH, 1.0f, 1.0f );
    pVertex[2].pos = D3DXVECTOR4( fHighW, fLowH, 1.0f, 1.0f );
    pVertex[3].pos = D3DXVECTOR4( fHighW, fHighH, 1.0f, 1.0f );
    g_pSkyboxVB->Unlock();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Renders a progress bar for the installation progress
void RenderProgressBar( IDirect3DDevice9* pd3dDevice, RECT& rc, int nProgCurrent, int nProgMax )
{
    HRESULT hr;

    QUADVERT quad[4] =
    {
        { -0.5f + rc.left,  -0.5f + rc.top,    0.5f, 1.0f, 0.0f, 0.0f },
        { -0.5f + rc.right, -0.5f + rc.top,    0.5f, 1.0f, 1.0f, 0.0f },
        { -0.5f + rc.left,  -0.5f + rc.bottom, 0.5f, 1.0f, 0.0f, 1.0f },
        { -0.5f + rc.right, -0.5f + rc.bottom, 0.5f, 1.0f, 1.0f, 1.0f }
    };

    // Draw the progress bar background so that it looks different from the rest of the
    // loading screen.
    g_pEffect->SetTechnique( "RenderProgressBarBackground" );
    pd3dDevice->SetVertexDeclaration( g_pQuadDecl );
    UINT cPasses;
    if( SUCCEEDED( g_pEffect->Begin( &cPasses, 0 ) ) )
    {
        for( UINT p = 0; p < cPasses; ++p )
        {
            V( g_pEffect->BeginPass( p ) );
            V( pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, quad, sizeof( QUADVERT ) ) );
            V( g_pEffect->EndPass() );
        }
        V( g_pEffect->End() );
    }

    // Compute how long the bar should be, then draw the progress bar.
    int nCappedProgress = min( nProgCurrent, nProgMax );
    int length = nProgMax ? int( ( quad[1].x - quad[0].x ) * nCappedProgress / nProgMax ) : 0;

    quad[1].x = quad[0].x + length;
    quad[3].x = quad[2].x + length;

    g_pEffect->SetTechnique( "RenderProgressBar" );
    if( SUCCEEDED( g_pEffect->Begin( &cPasses, 0 ) ) )
    {
        for( UINT p = 0; p < cPasses; ++p )
        {
            V( g_pEffect->BeginPass( p ) );
            V( pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, quad, sizeof( QUADVERT ) ) );
            V( g_pEffect->EndPass() );
        }
        V( g_pEffect->End() );
    }
}


void RenderLoadingScreenBackground( IDirect3DDevice9* pd3dDevice )
{
    HRESULT hr;

    // If loading a level, just blit the loading screen.
    const D3DSURFACE_DESC* pdsd = DXUTGetD3D9BackBufferSurfaceDesc();
    QUADVERT quad[4] =
    {
        { -0.5f,               -0.5f,                0.5f, 1.0f, 0.0f, 0.0f },
        { -0.5f + pdsd->Width, -0.5f,                0.5f, 1.0f, 1.0f, 0.0f },
        { -0.5f,               -0.5f + pdsd->Height, 0.5f, 1.0f, 0.0f, 1.0f },
        { -0.5f + pdsd->Width, -0.5f + pdsd->Height, 0.5f, 1.0f, 1.0f, 1.0f }
    };
    g_pEffect->SetTechnique( "RenderQuad" );
    g_pEffect->SetTexture( "g_txScene", g_pLoadTex );
    pd3dDevice->SetVertexDeclaration( g_pQuadDecl );
    UINT cPasses;
    if( SUCCEEDED( g_pEffect->Begin( &cPasses, 0 ) ) )
    {
        for( UINT p = 0; p < cPasses; ++p )
        {
            V( g_pEffect->BeginPass( p ) );
            V( pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, quad, sizeof( QUADVERT ) ) );
            V( g_pEffect->EndPass() );
        }
        V( g_pEffect->End() );
    }
}


void RenderInfoText( IDirect3DDevice9* pd3dDevice, bool bSpriteInit )
{
#define INPROGTEXT L"The worker thread is installing necessary components of feature\n" \
                   L"\"%s\". The main thread is free to render anything here,\n" \
                   L"not just static scene, using resources already loaded."

#define DONETEXT L"The request feature has been completely installed."

    if( g_bShowHelp )
    {
        // Print some text to let the user know that anything could be
        // rendered while installation is in progress, not just static
        // scene.
        CDXUTTextHelper txtHelperSmall( g_pFont, g_pTextSprite, 15 );
        if( !bSpriteInit )
            txtHelperSmall.Begin();
        WCHAR szFeature[MAX_FEATURE_CHARS+1];
        g_Msi.GetCurrentFeatureName( szFeature, MAX_FEATURE_CHARS + 1 );
        // Draw twice to create shadow effect, for demo purposes only.
        txtHelperSmall.SetForegroundColor( D3DXCOLOR( 0.0f, 0.0f, 0.0f, 1.0f ) );
        txtHelperSmall.SetInsertionPos( 11, 36 );
        txtHelperSmall.DrawFormattedTextLine( g_Msi.IsInstallInProgress() ? INPROGTEXT : DONETEXT, szFeature );
        txtHelperSmall.SetForegroundColor( D3DXCOLOR( 0.0f, 0.75f, 0.5f, 1.0f ) );
        txtHelperSmall.SetInsertionPos( 10, 35 );
        txtHelperSmall.DrawFormattedTextLine( g_Msi.IsInstallInProgress() ? INPROGTEXT : DONETEXT, szFeature );

        // If we are in the intro, print some text to inform the user this.
        if( AppState_Intro == g_AppState )
        {
            const D3DSURFACE_DESC* pdsd = DXUTGetD3D9BackBufferSurfaceDesc();
            txtHelperSmall.SetForegroundColor( D3DXCOLOR( 0.5f, 1.0f, 1.0f, 1.0f ) );
            txtHelperSmall.SetInsertionPos( pdsd->Width / 2 - 80, 100 );
            txtHelperSmall.DrawTextLine( L"Playing Intro. Press Esc to End." );
        }

        if( !bSpriteInit )
            txtHelperSmall.End();
    }
}


//--------------------------------------------------------------------------------------
// Renders a loading screen
void RenderLoadingScreen( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime )
{
    RenderLoadingScreenBackground( pd3dDevice );

    const D3DSURFACE_DESC* pdsd = DXUTGetD3D9BackBufferSurfaceDesc();
    if( g_Msi.IsInstallInProgress() )
    {
        RECT rc =
        {
            pdsd->Width / 2 - 150, pdsd->Height - 90,
            pdsd->Width / 2 + 150, pdsd->Height - 40
        };
        RenderProgressBar( pd3dDevice, rc, g_Msi.GetProgress(), g_Msi.GetProgressMax() );

        // If background installation is progress, render a progress bar if 
        // show help is turned on.
        if( g_bShowHelp )
        {
            const D3DSURFACE_DESC* pdsd = DXUTGetD3D9BackBufferSurfaceDesc();
            RECT rc =
            {
                pdsd->Width / 2 - 50, pdsd->Height - 25,
                pdsd->Width / 2 + 50, pdsd->Height - 5
            };
            RenderProgressBar( pd3dDevice, rc, g_Msi.GetProgress(), g_Msi.GetProgressMax() );
        }
    }

    // Put some text up to let the player know that the level is installing/loading.
    CDXUTTextHelper txtHelper( g_pBigFont, g_pTextSprite, 15 );

    // Output statistics
    txtHelper.Begin();
    txtHelper.SetForegroundColor( D3DXCOLOR( 0.0f, 1.0f, 0.0f, 1.0f ) );

    if( g_Msi.IsInstallInProgress() )
    {
        // Print some installation status.
        RenderInfoText( pd3dDevice, true );

        if( AppState_FullInstall == g_AppState )
        {
            txtHelper.SetInsertionPos( pdsd->Width / 2 - 280, pdsd->Height - 150 );
            txtHelper.DrawTextLine( L"PERFORMING FULL INSTALL" );
        }
        else
        {
            txtHelper.SetInsertionPos( pdsd->Width / 2 - 210, pdsd->Height - 150 );
            txtHelper.DrawFormattedTextLine( L"INSTALLING LEVEL %d", g_dwLoadLevel + 1 );
        }
    }
    else
    {
        txtHelper.SetInsertionPos( pdsd->Width / 2 - 170, pdsd->Height - 150 );
        txtHelper.DrawFormattedTextLine( L"LOADING LEVEL %d", g_dwLoadLevel + 1 );
    }

    txtHelper.End();
}


//--------------------------------------------------------------------------------------
// The purpose of this method is to render the intro movie sequence when the
// app first loads up.
void RenderIntro( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime )
{
    HRESULT hr;
    D3DXMATRIXA16 m, mRotate;
    D3DXMATRIXA16 mProj;
    D3DXMATRIXA16 mView;
    static float fRotate;

    fRotate -= D3DX_PI * 0.3f * fElapsedTime;
    D3DXMatrixRotationY( &mRotate, fRotate );
    m = g_mIntroWorldMatrix * mRotate;

    // Set up the camera to look at the speeder
    D3DXVECTOR3 vecEye( 0.0f, 1.0f, -15.0f );
    D3DXVECTOR3 vecAt ( m._41, m._42, m._43 );
    g_Camera.SetViewParams( &vecEye, &vecAt );

    mProj = *g_Camera.GetProjMatrix();
    mView = *g_Camera.GetViewMatrix();

    V( g_pEffect->SetTechnique( "RenderScene" ) );
    V( g_pEffect->SetMatrix( "g_mWorld", &m ) );
    m *= mView;
    V( g_pEffect->SetMatrix( "g_mWorldView", &m ) );
    m *= mProj;
    V( g_pEffect->SetMatrix( "g_mWorldViewProjection", &m ) );
    V( pd3dDevice->SetFVF( g_Speeder.GetMesh()->GetFVF() ) );
    g_Speeder.Render( g_pEffect, "g_txScene", "g_vMaterial", 0, 0, 0, 0, true, false );

    // Render the ground
    D3DXMatrixTranslation( &m, 0.0f, -5.0f, 0.0f );
    V( g_pEffect->SetMatrix( "g_mWorld", &m ) );
    m *= mView;
    V( g_pEffect->SetMatrix( "g_mWorldView", &m ) );
    m *= mProj;
    V( g_pEffect->SetMatrix( "g_mWorldViewProjection", &m ) );
    V( pd3dDevice->SetFVF( g_IntroTerrain.GetMesh()->GetFVF() ) );
    g_IntroTerrain.Render( g_pEffect, "g_txScene", "g_vMaterial", 0, 0, 0, 0, true, false );

    // Display some helper information if enabled.
    if( g_bShowHelp )
    {
        // If background installation is progress, render a progress bar if 
        // show help is turned on.
        if( ( g_Msi.IsInstallInProgress() ) )
        {
            const D3DSURFACE_DESC* pdsd = DXUTGetD3D9BackBufferSurfaceDesc();
            RECT rc =
            {
                pdsd->Width / 2 - 50, pdsd->Height - 25,
                pdsd->Width / 2 + 50, pdsd->Height - 5
            };
            RenderProgressBar( pd3dDevice, rc, g_Msi.GetProgress(), g_Msi.GetProgressMax() );
        }
    }
}


//--------------------------------------------------------------------------------------
// This callback function will be called at the end of every frame to perform all the 
// rendering calls for the scene, and it will also be called if the window needs to be 
// repainted. After this function has returned, DXUT will call 
// IDirect3DDevice9::Present to display the contents of the next buffer in the swap chain
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;
    D3DXMATRIXA16 mWorld;
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;
    D3DXMATRIXA16 mWorldView;
    D3DXMATRIXA16 mWorldViewProjection;

    // If the settings dialog is being shown, then
    // render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Load the current level
    if( !g_bLevelLoaded )
    {
        g_bLevelLoaded = true;
        g_dwLoadLevel = 0;
        LoadLevel( pd3dDevice, false );
    }

    // Clear the render target and the zbuffer 
    V( pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 66, 75, 121 ), 1.0f, 0 ) );

    // If installing, check to see if it is complete.
    DWORD dwWait = WaitForSingleObject( g_hEvent, 0 );
    if( WAIT_OBJECT_0 == dwWait )
    {
        // The installation is complete.
        if( AppState_FullInstall == g_AppState )
        {
            // If we were performing a full install, switch back
            // to option menu.
            g_AppState = AppState_OptionMenu;
            g_OptionMenu.SetVisible( true );
        }
        else // If at normal state, resources will need to be loaded now.
        if( AppState_Normal == g_AppState || AppState_Load == g_AppState )
        {
            // Call LoadLevel again to load the level resources.
            LoadLevel( pd3dDevice, true );
        }
    }

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
        if( AppState_Load == g_AppState || AppState_FullInstall == g_AppState )
        {
            RenderLoadingScreen( pd3dDevice, fTime, fElapsedTime );
        }
        else if( AppState_Intro == g_AppState )
        {
            // Playing intro sequence
            RenderIntro( pd3dDevice, fTime, fElapsedTime );
        }
        else if( AppState_Stats == g_AppState )
        {
            RenderLoadingScreenBackground( pd3dDevice );
        }
        else if( AppState_Normal == g_AppState )
        {
            if( -1 != g_nCurrentLevel )
            {
                // Render the skybox
                D3DXMATRIXA16 mInvWorldViewProj;
                D3DXMATRIXA16 mVP = *g_Camera.GetViewMatrix();
                mVP._41 = mVP._42 = mVP._43 = 0.0f;
                mVP *= *g_Camera.GetProjMatrix();
                D3DXMatrixInverse( &mInvWorldViewProj, NULL, &mVP );
                V( g_pEffect->SetMatrix( "g_mInvWorldViewProjection", &mInvWorldViewProj ) );

                V( g_pEffect->SetTechnique( "RenderSkybox" ) );
                V( g_pEffect->SetTexture( "g_txSkybox", g_pLevelEnvTex ) );

                pd3dDevice->SetStreamSource( 0, g_pSkyboxVB, 0, sizeof( SKYBOX_VERTEX ) );
                pd3dDevice->SetVertexDeclaration( g_pSkyboxDecl );

                UINT cPasses;
                V( g_pEffect->Begin( &cPasses, 0 ) );
                for( UINT p = 0; p < cPasses; ++p )
                {
                    V( g_pEffect->BeginPass( p ) );
                    V( pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 ) );
                    V( g_pEffect->EndPass() );
                }
                V( g_pEffect->End() );

                // Get the projection & view matrix from the camera class
                mProj = *g_Camera.GetProjMatrix();
                mView = *g_Camera.GetViewMatrix();

                // Update the effect's variables.  Instead of using strings, it would 
                // be more efficient to cache a handle to the parameter by calling 
                // ID3DXEffect::GetParameterByName
                mWorldView = g_mLevelWorld * mView;
                mWorldViewProjection = g_mLevelWorld * mView * mProj;
                V( g_pEffect->SetMatrix( "g_mWorldViewProjection", &mWorldViewProjection ) );
                V( g_pEffect->SetMatrix( "g_mWorldView", &mWorldView ) );
                V( g_pEffect->SetMatrix( "g_mWorld", &g_mLevelWorld ) );

                g_pEffect->SetTechnique( "RenderTerrain" );
                g_pEffect->SetTexture( "g_txScene", g_pLevelTex );
                pd3dDevice->SetFVF( g_LevelMesh.GetMesh()->GetFVF() );
                g_LevelMesh.Render( g_pEffect, 0, 0, 0, 0, 0, 0, true, false );
            }
            else
            {
                // No level is loaded yet but the user has completed char creation.
                // Render loading screen.
                RenderLoadingScreen( pd3dDevice, fTime, fElapsedTime );
            }
        }

        if( g_bShowHelp )
        {
            const D3DSURFACE_DESC* pdsd = DXUTGetD3D9BackBufferSurfaceDesc();

            // If installation is progress, render a progress bar.
            if( g_Msi.IsInstallInProgress() )
            {
                RECT rc =
                {
                    pdsd->Width / 2 - 50, pdsd->Height - 25,
                    pdsd->Width / 2 + 50, pdsd->Height - 5
                };
                RenderProgressBar( pd3dDevice, rc, g_Msi.GetProgress(), g_Msi.GetProgressMax() );
            }

            // Render the progress bar for passive install.
            RECT rc =
            {
                pdsd->Width - 150, pdsd->Height - 25,
                pdsd->Width - 20, pdsd->Height - 5
            };
            RenderProgressBar( pd3dDevice, rc, g_Msi.GetPassiveProgress(), g_Msi.GetPassiveProgressMax() );
        }

        // These events are to help PIX identify what the code is doing
        DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );

        RenderText();
        // Print installation status
        if( AppState_Normal != g_AppState || g_nCurrentLevel == -1 )
            RenderInfoText( pd3dDevice, false );
        // Hide helper information if show help is turned off.
        if( g_bShowHelp )
        {
            V( g_HUD.OnRender( fElapsedTime ) );
            V( g_SampleUI.OnRender( fElapsedTime ) );
        }
        V( g_MainMenu.OnRender( fElapsedTime ) );
        V( g_OptionMenu.OnRender( fElapsedTime ) );
        V( g_NewGameMenu.OnRender( fElapsedTime ) );
        V( g_StatResult.OnRender( fElapsedTime ) );
        DXUT_EndPerfEvent();

        V( pd3dDevice->EndScene() );
    }
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    // The helper object simply helps keep track of text position, and color
    // and then it calls pFont->DrawText( m_pSprite, strMsg, -1, &rc, DT_NOCLIP, m_clr );
    // If NULL is passed in as the sprite object, then it will work however the 
    // pFont->DrawText() will not be batched together.  Batching calls will improves performance.
    const D3DSURFACE_DESC* pd3dsdBackBuffer = DXUTGetD3D9BackBufferSurfaceDesc();
    CDXUTTextHelper txtHelper( g_pFont, g_pTextSprite, 15 );

    // Output statistics
    txtHelper.Begin();
    txtHelper.SetInsertionPos( 5, 5 );
    txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    txtHelper.DrawTextLine( DXUTGetFrameStats() );
    txtHelper.DrawTextLine( DXUTGetDeviceStats() );

    // Draw help
    if( g_bShowHelp )
    {
        // Do not display control information unless we are in game itself.
        if( AppState_Normal == g_AppState && -1 != g_nCurrentLevel )
        {
            txtHelper.SetInsertionPos( 10, pd3dsdBackBuffer->Height - 15 * 7 );
            txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
            txtHelper.DrawTextLine( L"Controls:" );

            txtHelper.SetInsertionPos( 40, pd3dsdBackBuffer->Height - 15 * 6 );
            txtHelper.DrawTextLine( L"Rotate: Left drag\n"
                                    L"Move: W/S/A/D\n"
                                    L"Menu: ESC" );
        }

        txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
        txtHelper.SetInsertionPos( pd3dsdBackBuffer->Width - 145, pd3dsdBackBuffer->Height - 33 );
        if( g_Msi.GetCompletedPassiveFeatureCount() < g_Msi.GetRequestedPassiveFeatureCount() )
            txtHelper.DrawFormattedTextLine( L"Passive:\nFeatures %d / %d",
                                             g_Msi.GetCompletedPassiveFeatureCount(),
                                             g_Msi.GetRequestedPassiveFeatureCount() );
        else
            txtHelper.DrawTextLine( L"Passive:\nAll installed" );
    }

    txtHelper.SetForegroundColor( D3DXCOLOR( 0.75f, 0.0f, 1.0f, 1.0f ) );
    if( g_bShowHelp )
    {
        txtHelper.SetInsertionPos( 1, pd3dsdBackBuffer->Height - 36 );
        txtHelper.DrawTextLine(
            L"Press F1 to hide control and helper information,\nwhich simulates the end-user experience." );
    }
    else
    {
        txtHelper.SetInsertionPos( 1, pd3dsdBackBuffer->Height - 21 );
        txtHelper.DrawTextLine( L"Press F1 to display control and helper information." );
    }

    txtHelper.End();
}


//--------------------------------------------------------------------------------------
// Before handling window messages, DXUT passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then DXUT will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Always allow dialog resource manager calls to handle global messages
    // so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first

    // g_HUD and g_SampleUI are hidden when g_bShowHelp is false (off), so do not
    // let them process messages either.
    if( g_bShowHelp )
    {
        *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
        if( *pbNoFurtherProcessing )
            return 0;
        *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
        if( *pbNoFurtherProcessing )
            return 0;
    }
    *pbNoFurtherProcessing = g_MainMenu.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_OptionMenu.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_NewGameMenu.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_StatResult.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    switch( uMsg )
    {
        case WM_ACTIVATEAPP:
            DXUTTRACE( L"WM_ACTIVATEAPP: %u\n", wParam );
            if( wParam )
                g_Msi.EnablePassiveInstall( false );
            else // Workaround the problem that we can get multiple deactivate message
            // when in fullscreen mode.
            if( GetCurrentThreadId() != ( DWORD )lParam )
                g_Msi.EnablePassiveInstall( true );
            break;

        case WM_KEYDOWN:
        {
            switch( wParam )
            {
                case VK_ESCAPE:
                {
                    // If the user presses any key while the intro is playing, stop
                    // the intro and go right into the game.
                    if( AppState_Menu != g_AppState )
                    {
                        // If the user is not at the main menu, trap
                        // Escape so that the sample doesn't just close
                        // when the key is hit.
                        *pbNoFurtherProcessing = true;

                        // If the intro was playing, prepare the transition
                        // to menu mode.
                        if( AppState_Intro == g_AppState )
                        {
                            g_MainMenu.SetVisible( true );
                            g_AppState = AppState_Menu;
                        }
                        else if( AppState_Normal == g_AppState )
                        {
                            // Stepping away from gameplay.
                            // Enable passive install.
                            g_Msi.EnablePassiveInstall( true );

                            g_MainMenu.SetVisible( true );
                            g_AppState = AppState_Menu;

                            // Disable level buttons when in menu.
                            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL1, false );
                            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL2, false );
                            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL3, false );
                            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL4, false );
                            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL5, false );
                        }
                    }
                    break;
                }
            }
        }
    }

    return 0;
}


//--------------------------------------------------------------------------------------
// As a convenience, DXUT inspects the incoming windows messages for
// keystroke messages and decodes the message parameters to pass relevant keyboard
// messages to the application.  The framework does not remove the underlying keystroke 
// messages, which are still passed to the application's MsgProc callback.
//--------------------------------------------------------------------------------------
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
            case VK_F1:
                g_bShowHelp = !g_bShowHelp; break;
        }
    }
}


void UpdateStatText( CDXUTStatic* pControl, LPCWSTR wszStatName )
{
    assert( pControl );

    int nValue = ( rand() + 15 ) * 20 / RAND_MAX + 10;

    WCHAR wsz[256];

    StringCchPrintf( wsz, 256, L"%s          %d", wszStatName, nValue );
    pControl->SetText( wsz );
}


void Reroll()
{
    UpdateStatText( g_NewGameMenu.GetStatic( IDC_NEW_STR ), L"STR" );
    UpdateStatText( g_NewGameMenu.GetStatic( IDC_NEW_STA ), L"STA" );
    UpdateStatText( g_NewGameMenu.GetStatic( IDC_NEW_DEX ), L"DEX" );
    UpdateStatText( g_NewGameMenu.GetStatic( IDC_NEW_AGI ), L"AGI" );
    UpdateStatText( g_NewGameMenu.GetStatic( IDC_NEW_INT ), L"INT" );
    UpdateStatText( g_NewGameMenu.GetStatic( IDC_NEW_WIS ), L"WIS" );
}


void UpdateLevelStatistics()
{
    DWORD nValue;
    WCHAR wsz[50];
    nValue = ( rand() % 10 + 1 ) * ( rand() % 10 + 1 );
    StringCchPrintf( wsz, 50, L"%u", nValue );
    g_StatResult.GetStatic( IDC_STAT_YOURKILL_VALUE )->SetText( wsz );
    nValue = ( rand() % 10 + 1 ) * ( rand() % 10 + 1 );
    StringCchPrintf( wsz, 50, L"%u", nValue );
    g_StatResult.GetStatic( IDC_STAT_TEAMKILL_VALUE )->SetText( wsz );
    nValue = ( rand() % 10 + 1 ) * ( rand() % 10 + 1 );
    StringCchPrintf( wsz, 50, L"%u", nValue );
    g_StatResult.GetStatic( IDC_STAT_ENEMYKILL_VALUE )->SetText( wsz );
    nValue = ( rand() % 10 + 1 ) * ( rand() % 10 + 1 );
    StringCchPrintf( wsz, 50, L"%u", nValue );
    g_StatResult.GetStatic( IDC_STAT_YOURRAZED_VALUE )->SetText( wsz );
    nValue = ( rand() % 10 + 1 ) * ( rand() % 10 + 1 );
    StringCchPrintf( wsz, 50, L"%u", nValue );
    g_StatResult.GetStatic( IDC_STAT_TEAMRAZED_VALUE )->SetText( wsz );
    nValue = ( rand() % 10 + 1 ) * ( rand() % 10 + 1 );
    StringCchPrintf( wsz, 50, L"%u", nValue );
    g_StatResult.GetStatic( IDC_STAT_ENEMYRAZED_VALUE )->SetText( wsz );
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;
        case IDC_LOADLEVEL1:
            if( g_nCurrentLevel != 0 )
            {
                g_dwLoadLevel = 0;
                g_StatResult.SetVisible( true );
                g_AppState = AppState_Stats;
                UpdateLevelStatistics();
                LoadLevel( DXUTGetD3D9Device(), false );

                // Stat screen is not perf critical. Enable
                // passive install.
                g_Msi.EnablePassiveInstall( true );
            }
            break;
        case IDC_LOADLEVEL2:
            if( g_nCurrentLevel != 1 )
            {
                g_dwLoadLevel = 1;
                g_StatResult.SetVisible( true );
                g_AppState = AppState_Stats;
                UpdateLevelStatistics();
                LoadLevel( DXUTGetD3D9Device(), false );

                // Stat screen is not perf critical. Enable
                // passive install.
                g_Msi.EnablePassiveInstall( true );
            }
            break;
        case IDC_LOADLEVEL3:
            if( g_nCurrentLevel != 2 )
            {
                g_dwLoadLevel = 2;
                g_StatResult.SetVisible( true );
                g_AppState = AppState_Stats;
                UpdateLevelStatistics();
                LoadLevel( DXUTGetD3D9Device(), false );

                // Stat screen is not perf critical. Enable
                // passive install.
                g_Msi.EnablePassiveInstall( true );
            }
            break;
        case IDC_LOADLEVEL4:
            if( g_nCurrentLevel != 3 )
            {
                g_dwLoadLevel = 3;
                g_StatResult.SetVisible( true );
                g_AppState = AppState_Stats;
                UpdateLevelStatistics();
                LoadLevel( DXUTGetD3D9Device(), false );

                // Stat screen is not perf critical. Enable
                // passive install.
                g_Msi.EnablePassiveInstall( true );
            }
            break;
        case IDC_LOADLEVEL5:
            if( g_nCurrentLevel != 4 )
            {
                g_dwLoadLevel = 4;
                g_StatResult.SetVisible( true );
                g_AppState = AppState_Stats;
                UpdateLevelStatistics();
                LoadLevel( DXUTGetD3D9Device(), false );

                // Stat screen is not perf critical. Enable
                // passive install.
                g_Msi.EnablePassiveInstall( true );
            }
            break;

            //
            // Main menu items
            //
        case IDC_NEWGAME:
            g_MainMenu.SetVisible( false );
            g_NewGameMenu.SetVisible( true );
            g_AppState = AppState_NewGameMenu;

            // Empty the name edit box
            g_NewGameMenu.GetEditBox( IDC_NEW_EDITNAME )->SetText( L"" );

            // Initially disable the accept button
            g_NewGameMenu.GetButton( IDC_NEW_ACCEPT )->SetEnabled( false );

            Reroll();
            break;

        case IDC_BACKTOGAME:
            // About to play the game again.
            // Disable passive install.
            g_Msi.EnablePassiveInstall( false );

            g_MainMenu.SetVisible( false );
            g_AppState = AppState_Normal;

            // Enable level buttons.
            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL1, true );
            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL2, true );
            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL3, true );
            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL4, true );
            g_SampleUI.SetControlEnabled( IDC_LOADLEVEL5, true );
            break;

        case IDC_OPTION:
            g_MainMenu.SetVisible( false );
            g_OptionMenu.SetVisible( true );
            g_AppState = AppState_OptionMenu;
            break;

        case IDC_QUITGAME:
            SendMessage( DXUTGetHWND(), WM_CLOSE, 0, 0 );
            break;

            //
            // Option menu items
            //
        case IDC_OPTION_FULLINSTALL:
            g_OptionMenu.SetVisible( false );
            PerformFullInstall();
            break;

        case IDC_OPTION_PREVIOUS:
            g_OptionMenu.SetVisible( false );
            g_MainMenu.SetVisible( true );
            g_AppState = AppState_Menu;
            break;

            //
            // New game menu items
            //
        case IDC_NEW_REROLL:
            Reroll();
            break;

        case IDC_NEW_ABORT:
            g_NewGameMenu.SetVisible( false );
            g_MainMenu.SetVisible( true );
            g_AppState = AppState_Menu;
            break;

        case IDC_NEW_ACCEPT:
            // A new game is starting.  Make some changes to the
            // main game menu.
            g_MainMenu.GetControl( IDC_NEWGAME )->SetVisible( false );
            g_MainMenu.GetControl( IDC_BACKTOGAME )->SetVisible( true );
            // Assign Escape hotkey to Return to Game
            g_MainMenu.GetControl( IDC_BACKTOGAME )->SetHotkey( VK_ESCAPE );
            g_MainMenu.GetControl( IDC_QUITGAME )->SetHotkey( 0 );

            // Game is about to start. Disable passive installs in
            // the background.
            g_Msi.EnablePassiveInstall( false );

            g_NewGameMenu.SetVisible( false );
            g_AppState = AppState_Normal;
            LoadLevel( DXUTGetD3D9Device(), true );
            break;

        case IDC_NEW_EDITNAME:
            if( ( ( CDXUTEditBox* )pControl )->GetTextLength() > 0 )
            {
                if( g_NewGameMenu.GetRadioButton( IDC_NEW_RADIOSEX1 )->GetChecked() ||
                    g_NewGameMenu.GetRadioButton( IDC_NEW_RADIOSEX2 )->GetChecked() )
                {
                    g_NewGameMenu.GetButton( IDC_NEW_ACCEPT )->SetEnabled( true );
                }
            }
            else
                g_NewGameMenu.GetButton( IDC_NEW_ACCEPT )->SetEnabled( false );
            break;

        case IDC_NEW_RADIOSEX1:
        case IDC_NEW_RADIOSEX2:
            if( g_NewGameMenu.GetEditBox( IDC_NEW_EDITNAME )->GetTextLength() > 0 )
                g_NewGameMenu.GetButton( IDC_NEW_ACCEPT )->SetEnabled( true );
            break;

            //
            // Stats Screen
            //
        case IDC_STAT_OK:
            // Game is about to start. Disable passive installs in
            // the background.
            g_Msi.EnablePassiveInstall( false );

            g_StatResult.SetVisible( false );
            g_AppState = AppState_Load;
            LoadLevel( DXUTGetD3D9Device(), true );
            break;

    }
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// entered a lost state and before IDirect3DDevice9::Reset is called. Resources created
// in the OnResetDevice callback should be released here, which generally includes all 
// D3DPOOL_DEFAULT resources. See the "Lost Devices" section of the documentation for 
// information about lost devices.
//--------------------------------------------------------------------------------------
void CALLBACK OnLostDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D9LostDevice();
    g_SettingsDlg.OnD3D9LostDevice();
    if( g_pFont )
        g_pFont->OnLostDevice();
    if( g_pBigFont )
        g_pBigFont->OnLostDevice();
    if( g_pEffect )
        g_pEffect->OnLostDevice();
    SAFE_RELEASE( g_pTextSprite );
    SAFE_RELEASE( g_pSkyboxVB );
    g_LevelMesh.InvalidateDeviceObjects();
    g_Speeder.InvalidateDeviceObjects();
    g_IntroTerrain.InvalidateDeviceObjects();
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles. Resources created in the OnCreateDevice callback 
// should be released here, which generally includes all D3DPOOL_MANAGED resources. 
//--------------------------------------------------------------------------------------
void CALLBACK OnDestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D9DestroyDevice();
    g_SettingsDlg.OnD3D9DestroyDevice();
    SAFE_RELEASE( g_pEffect );
    SAFE_RELEASE( g_pFont );
    SAFE_RELEASE( g_pBigFont );
    SAFE_RELEASE( g_pQuadDecl );
    SAFE_RELEASE( g_pSkyboxDecl );

    g_LevelMesh.Destroy();
    g_Speeder.Destroy();
    g_IntroTerrain.Destroy();

    // Release the level terrain textures
    SAFE_RELEASE( g_pLoadTex );
    SAFE_RELEASE( g_pLevelTex );
    SAFE_RELEASE( g_pLevelEnvTex );
}
