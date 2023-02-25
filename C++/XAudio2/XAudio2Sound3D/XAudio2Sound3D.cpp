//--------------------------------------------------------------------------------------
// File: XAudio2Sound3D.cpp
//
// XNA Developer Connection
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "resource.h"
#include "audio.h"

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager      g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg                 g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*                g_pTxtHelper = NULL;
CDXUTDialog                     g_HUD;                  // dialog for standard controls
CDXUTDialog                     g_SampleUI;             // dialog for sample specific controls

// Direct3D 9 resources
ID3DXFont*                      g_pFont9 = NULL;
ID3DXSprite*                    g_pSprite9 = NULL;
ID3DXEffect*                    g_pEffect9 = NULL;
IDirect3DVertexDeclaration9*    g_pVertexDecl = NULL;
LPDIRECT3DVERTEXBUFFER9         g_pvbFloor = NULL;
LPDIRECT3DVERTEXBUFFER9         g_pvbSource = NULL;
LPDIRECT3DVERTEXBUFFER9         g_pvbListener = NULL;
LPDIRECT3DVERTEXBUFFER9         g_pvbListenerCone = NULL;
LPDIRECT3DVERTEXBUFFER9         g_pvbInnerRadius = NULL;
LPDIRECT3DVERTEXBUFFER9         g_pvbGrid = NULL;

struct D3DVERTEX
{
    D3DXVECTOR3 p;
    D3DCOLOR c;
};


const LPWSTR g_SOUND_NAMES[] =
{
    L"Heli.wav",
    L"MusicMono.wav",
};

enum CONTROL_MODE
{
    CONTROL_SOURCE=0,
    CONTROL_LISTENER
}                               g_eControlMode = CONTROL_SOURCE;

// Must match order of g_PRESET_PARAMS
const LPWSTR g_PRESET_NAMES[ NUM_PRESETS ] =
{
    L"Forest",
    L"Default",
    L"Generic",
    L"Padded cell",
    L"Room",
    L"Bathroom",
    L"Living room",
    L"Stone room",
    L"Auditorium",
    L"Concert hall",
    L"Cave",
    L"Arena",
    L"Hangar",
    L"Carpeted hallway",
    L"Hallway",
    L"Stone Corridor",
    L"Alley",
    L"City",
    L"Mountains",
    L"Quarry",
    L"Plain",
    L"Parking lot",
    L"Sewer pipe",
    L"Underwater",
    L"Small room",
    L"Medium room",
    L"Large room",
    L"Medium hall",
    L"Large hall",
    L"Plate",
};

#define FLAG_MOVE_UP        0x1
#define FLAG_MOVE_LEFT      0x2
#define FLAG_MOVE_RIGHT     0x4
#define FLAG_MOVE_DOWN      0x8

int                             g_moveFlags = 0;

const float                     MOTION_SCALE = 10.0f;

//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------

// UI control IDs
#define IDC_STATIC              -1
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3
#define IDC_SOUND               4
#define IDC_CONTROL_MODE        5
#define IDC_PRESET              6
#define IDC_UP                  7
#define IDC_LEFT                8
#define IDC_RIGHT               9
#define IDC_DOWN                10
#define IDC_LISTENERCONE        11
#define IDC_INNERRADIUS         12

// Constants for colors
static const DWORD              SOURCE_COLOR = 0xffea1b1b;
static const DWORD              LISTENER_COLOR = 0xff2b2bff;
static const DWORD              FLOOR_COLOR = 0xff101010;
static const DWORD              GRID_COLOR = 0xff00a000;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );

bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
                                      bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext );
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                    void* pUserContext );
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnD3D9LostDevice( void* pUserContext );
void CALLBACK OnD3D9DestroyDevice( void* pUserContext );

void InitApp();
void RenderText();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D10)
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
    DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );
    DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );

    InitApp();

    HRESULT hr = InitAudio();
    if( FAILED( hr ) )
    {
        OutputDebugString( L"InitAudio() failed.  Disabling audio support\n" );
    }

    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"XAudio2Sound3D" );
    DXUTCreateDevice( true, 640, 480 );

    hr = PrepareAudio( g_SOUND_NAMES[0] );
    if( FAILED( hr ) )
    {
        OutputDebugString( L"PrepareAudio() failed\n" );
    }

    DXUTMainLoop(); // Enter into the DXUT render loop

    CleanupAudio();

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2 );

    g_SampleUI.SetCallback( OnGUIEvent );

    //
    // Sound control
    //

    CDXUTComboBox* pComboBox = NULL;
    g_SampleUI.AddStatic( IDC_STATIC, L"S(o)und", 10, 0, 105, 25 );
    g_SampleUI.AddComboBox( IDC_SOUND, 10, 25, 140, 24, 'O', false, &pComboBox );
    if( pComboBox )
        pComboBox->SetDropHeight( 50 );

    for( int i = 0; i < sizeof( g_SOUND_NAMES ) / sizeof( WCHAR* ); i++ )
    {
        pComboBox->AddItem( g_SOUND_NAMES[i], IntToPtr( i ) );
    }

    //
    // Control mode
    //

    g_SampleUI.AddStatic( IDC_STATIC, L"(C)ontrol mode", 10, 45, 105, 25 );
    g_SampleUI.AddComboBox( IDC_CONTROL_MODE, 10, 70, 140, 24, 'C', false, &pComboBox );
    if( pComboBox )
        pComboBox->SetDropHeight( 30 );

    pComboBox->AddItem( L"Source", IntToPtr( CONTROL_SOURCE ) );
    pComboBox->AddItem( L"Listener", IntToPtr( CONTROL_LISTENER ) );

    //
    // I3DL2 reverb preset control
    //

    g_SampleUI.AddStatic( IDC_STATIC, L"(R)everb", 10, 90, 105, 25 );
    g_SampleUI.AddComboBox( IDC_PRESET, 10, 115, 140, 24, 'R', false, &pComboBox );
    if( pComboBox )
        pComboBox->SetDropHeight( 50 );

    for( int i = 0; i < sizeof( g_PRESET_NAMES ) / sizeof( WCHAR* ); i++ )
    {
        pComboBox->AddItem( g_PRESET_NAMES[i], IntToPtr( i ) );
    }

    //
    // Movement buttons
    //

    iY = 160;
    g_SampleUI.AddButton( IDC_UP, L"(W)", 40, iY, 70, 24 );
    g_SampleUI.AddButton( IDC_LEFT, L"(A)", 5, iY += 30, 70, 24 );
    g_SampleUI.AddButton( IDC_RIGHT, L"(D)", 75, iY, 70, 24 );
    g_SampleUI.AddButton( IDC_DOWN, L"(S)", 40, iY += 30, 70, 24 );

    //
    // Listener cone and inner radius buttons
    //
    g_SampleUI.AddButton( IDC_LISTENERCONE, L"Toggle Listener Cone", 10, iY += 50, 125, 22);
    g_SampleUI.AddButton( IDC_INNERRADIUS, L"Toggle Inner Radius", 10, iY += 24, 125, 22);
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();

    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawFormattedTextLine( L"Source: %.1f, %.1f, %.1f",
                                         g_audioState.emitter.Position.x, g_audioState.emitter.Position.y,
                                         g_audioState.emitter.Position.z );

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 0.0f, 0.0f, 0.5f, 1.0f ) );
    g_pTxtHelper->DrawFormattedTextLine( L"Listener: %.1f, %.1f, %.1f",
                                         g_audioState.listener.Position.x, g_audioState.listener.Position.y,
                                         g_audioState.listener.Position.z );

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( L"Coefficients:" );

    // Interpretation of channels depends on channel mask
    switch( g_audioState.dwChannelMask )
    {
        case SPEAKER_MONO:
            g_pTxtHelper->DrawFormattedTextLine( L" C: %.3f", g_audioState.dspSettings.pMatrixCoefficients[0] );
            break;

        case SPEAKER_STEREO:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.dspSettings.pMatrixCoefficients[1] );
            break;

        case SPEAKER_2POINT1:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" LFE: %.3f", g_audioState.dspSettings.pMatrixCoefficients[2] );
            break;

        case SPEAKER_SURROUND:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" C: %.3f", g_audioState.dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" B: %.3f", g_audioState.dspSettings.pMatrixCoefficients[3] );
            break;

        case SPEAKER_QUAD:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" Lb: %.3f", g_audioState.dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rb: %.3f", g_audioState.dspSettings.pMatrixCoefficients[3] );
            break;

        case SPEAKER_4POINT1:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" LFE: %.3f", g_audioState.dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" Lb: %.3f", g_audioState.dspSettings.pMatrixCoefficients[3] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rb: %.3f", g_audioState.dspSettings.pMatrixCoefficients[4] );
            break;

        case SPEAKER_5POINT1:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" C: %.3f", g_audioState.dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" LFE: %.3f", g_audioState.dspSettings.pMatrixCoefficients[3] );
            g_pTxtHelper->DrawFormattedTextLine( L" Lb: %.3f", g_audioState.dspSettings.pMatrixCoefficients[4] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rb: %.3f", g_audioState.dspSettings.pMatrixCoefficients[5] );
            break;

        case SPEAKER_7POINT1:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" C: %.3f", g_audioState.dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" LFE: %.3f", g_audioState.dspSettings.pMatrixCoefficients[3] );
            g_pTxtHelper->DrawFormattedTextLine( L" Lb: %.3f", g_audioState.dspSettings.pMatrixCoefficients[4] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rb: %.3f", g_audioState.dspSettings.pMatrixCoefficients[5] );
            g_pTxtHelper->DrawFormattedTextLine( L" Lfc: %.3f", g_audioState.dspSettings.pMatrixCoefficients[6] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rfc: %.3f", g_audioState.dspSettings.pMatrixCoefficients[7] );
            break;

        case SPEAKER_5POINT1_SURROUND:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" C: %.3f", g_audioState.dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" LFE: %.3f", g_audioState.dspSettings.pMatrixCoefficients[3] );
            g_pTxtHelper->DrawFormattedTextLine( L" Ls: %.3f", g_audioState.dspSettings.pMatrixCoefficients[4] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rs: %.3f", g_audioState.dspSettings.pMatrixCoefficients[5] );
            break;

        case SPEAKER_7POINT1_SURROUND:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" C: %.3f", g_audioState.dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" LFE: %.3f", g_audioState.dspSettings.pMatrixCoefficients[3] );
            g_pTxtHelper->DrawFormattedTextLine( L" Lb: %.3f", g_audioState.dspSettings.pMatrixCoefficients[4] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rb: %.3f", g_audioState.dspSettings.pMatrixCoefficients[5] );
            g_pTxtHelper->DrawFormattedTextLine( L" Ls: %.3f", g_audioState.dspSettings.pMatrixCoefficients[6] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rs: %.3f", g_audioState.dspSettings.pMatrixCoefficients[7] );
            break;

        default:
            // Generic channel output for non-standard channel masks
            g_pTxtHelper->DrawFormattedTextLine( L" [0]: %.3f", g_audioState.dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" [1]: %.3f", g_audioState.dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" [2]: %.3f", g_audioState.dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" [3]: %.3f", g_audioState.dspSettings.pMatrixCoefficients[3] );
            g_pTxtHelper->DrawFormattedTextLine( L" [4]: %.3f", g_audioState.dspSettings.pMatrixCoefficients[4] );
            g_pTxtHelper->DrawFormattedTextLine( L" [5]: %.3f", g_audioState.dspSettings.pMatrixCoefficients[5] );
            g_pTxtHelper->DrawFormattedTextLine( L" [6]: %.3f", g_audioState.dspSettings.pMatrixCoefficients[6] );
            g_pTxtHelper->DrawFormattedTextLine( L" [7]: %.3f", g_audioState.dspSettings.pMatrixCoefficients[7] );
            break;
    }

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 0.5f, 0.5f, 0.5f, 1.0f ) );
    g_pTxtHelper->DrawFormattedTextLine( L"Distance: %.3f", g_audioState.dspSettings.EmitterToListenerDistance );

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    g_pTxtHelper->DrawFormattedTextLine( L"Doppler factor: %.3f", g_audioState.dspSettings.DopplerFactor );

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 0.5f, 0.5f, 0.5f, 1.0f ) );
    g_pTxtHelper->DrawFormattedTextLine( L"LPF Direct: %.3f", g_audioState.dspSettings.LPFDirectCoefficient );
    g_pTxtHelper->DrawFormattedTextLine( L"LPF Reverb: %.3f", g_audioState.dspSettings.LPFReverbCoefficient );
    g_pTxtHelper->DrawFormattedTextLine( L"Reverb: %.3f", g_audioState.dspSettings.ReverbLevel );

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Rejects any D3D9 devices that aren't acceptable to the app by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat,
                                      D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // Skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                                         AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
                                         D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    // No fallback defined by this app, so reject any device that
    // doesn't support at least ps2.0
    if( pCaps->PixelShaderVersion < D3DPS_VERSION( 2, 0 ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    if( pDeviceSettings->ver == DXUT_D3D9_DEVICE )
    {
        IDirect3D9* pD3D = DXUTGetD3D9Object();
        D3DCAPS9 Caps;
        pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &Caps );

        // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW
        // then switch to SWVP.
        if( ( Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) == 0 ||
            Caps.VertexShaderVersion < D3DVS_VERSION( 1, 1 ) )
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
    }

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF ) ||
            ( DXUT_D3D10_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE ) )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that will live through a device reset (D3DPOOL_MANAGED)
// and aren't tied to the back buffer size
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D9CreateDevice( pd3dDevice ) );
    V_RETURN( g_SettingsDlg.OnD3D9CreateDevice( pd3dDevice ) );

    V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                              L"Arial", &g_pFont9 ) );

    // Read the D3DX effect file
    WCHAR str[MAX_PATH];
    DWORD dwShaderFlags = D3DXFX_NOT_CLONEABLE;
#ifdef DEBUG_VS
        dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
    #endif
#ifdef DEBUG_PS
        dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
    #endif
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"XAudio2Sound3D.fx" ) );
    V_RETURN( D3DXCreateEffectFromFile( pd3dDevice, str, NULL, NULL, dwShaderFlags,
                                        NULL, &g_pEffect9, NULL ) );

    //
    // Create render elements
    //

    // Define the vertex elements.
    D3DVERTEXELEMENT9 VertexElements[ 3 ] =
    {
        { 0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
        D3DDECL_END()
    };

    // Create a vertex declaration from the element descriptions.
    V_RETURN( pd3dDevice->CreateVertexDeclaration( VertexElements, &g_pVertexDecl ) );

    // Create our vertex buffers
    V_RETURN( pd3dDevice->CreateVertexBuffer( 4 * sizeof( D3DVERTEX ), 0, 0, D3DPOOL_MANAGED, &g_pvbFloor, NULL ) );
    V_RETURN( pd3dDevice->CreateVertexBuffer( 4 * sizeof( D3DVERTEX ), 0, 0, D3DPOOL_MANAGED, &g_pvbSource, NULL ) );
    V_RETURN( pd3dDevice->CreateVertexBuffer( 3 * sizeof( D3DVERTEX ), 0, 0, D3DPOOL_MANAGED, &g_pvbListener, NULL ) );
    V_RETURN( pd3dDevice->CreateVertexBuffer( 7 * sizeof( D3DVERTEX ), 0, 0, D3DPOOL_MANAGED, &g_pvbListenerCone, NULL ) );
    V_RETURN( pd3dDevice->CreateVertexBuffer( 9 * sizeof( D3DVERTEX ), 0, 0, D3DPOOL_MANAGED, &g_pvbInnerRadius, NULL ) );

    const UINT lcount = 2 * ( ( ZMAX - ZMIN + 1 ) + ( XMAX - XMIN + 1 ) );
    V_RETURN( pd3dDevice->CreateVertexBuffer( 2 * sizeof( D3DVERTEX ) * lcount, 0, 0, D3DPOOL_MANAGED, &g_pvbGrid,
                                              NULL ) );

    D3DVERTEX* pVertices;

    // Fill the VB for the listener, listener cone, and inner radius
    V_RETURN( g_pvbListener->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
    pVertices[0].p = D3DXVECTOR3( -0.5f, -1.f, 0 );
    pVertices[0].c = LISTENER_COLOR;
    pVertices[1].p = D3DXVECTOR3( 0, 1.f, 0 );
    pVertices[1].c = LISTENER_COLOR;
    pVertices[2].p = D3DXVECTOR3( 0.5f, -1.f, 0 );
    pVertices[2].c = LISTENER_COLOR;
    g_pvbListener->Unlock();
    V_RETURN( g_pvbListenerCone->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
    pVertices[0].p = D3DXVECTOR3( -1.04f, -3.86f, 0 );
    pVertices[0].c = LISTENER_COLOR;
    pVertices[1].p = D3DXVECTOR3( 0, 0, 0 );
    pVertices[1].c = LISTENER_COLOR;
    pVertices[2].p = D3DXVECTOR3( -3.86f, 1.04f, 0 );
    pVertices[2].c = LISTENER_COLOR;
    pVertices[3].p = D3DXVECTOR3( 0, 0, 0 );
    pVertices[3].c = LISTENER_COLOR;
    pVertices[4].p = D3DXVECTOR3( 3.86f, 1.04f, 0 );
    pVertices[4].c = LISTENER_COLOR;
    pVertices[5].p = D3DXVECTOR3( 0, 0, 0 );
    pVertices[5].c = LISTENER_COLOR;
    pVertices[6].p = D3DXVECTOR3( 1.04f, -3.86f, 0 );
    pVertices[6].c = LISTENER_COLOR;
    g_pvbListenerCone->Unlock();
    V_RETURN( g_pvbInnerRadius->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
    pVertices[0].p = D3DXVECTOR3( 0.0f, -2.0f, 0 );
    pVertices[0].c = LISTENER_COLOR;
    pVertices[1].p = D3DXVECTOR3( 1.4f, -1.4f, 0 );
    pVertices[1].c = LISTENER_COLOR;
    pVertices[2].p = D3DXVECTOR3( 2.0f, 0.0f, 0 );
    pVertices[2].c = LISTENER_COLOR;
    pVertices[3].p = D3DXVECTOR3( 1.4f, 1.4f, 0 );
    pVertices[3].c = LISTENER_COLOR;
    pVertices[4].p = D3DXVECTOR3( 0.0f, 2.0f, 0 );
    pVertices[4].c = LISTENER_COLOR;
    pVertices[5].p = D3DXVECTOR3( -1.4f, 1.4f, 0 );
    pVertices[5].c = LISTENER_COLOR;
    pVertices[6].p = D3DXVECTOR3( -2.0f, 0.0f, 0 );
    pVertices[6].c = LISTENER_COLOR;
    pVertices[7].p = D3DXVECTOR3( -1.4f, -1.4f, 0 );
    pVertices[7].c = LISTENER_COLOR;
    pVertices[8].p = D3DXVECTOR3( 0.0f, -2.0f, 0 );
    pVertices[8].c = LISTENER_COLOR;
    g_pvbInnerRadius->Unlock();

    // Fill the VB for the source
    V_RETURN( g_pvbSource->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
    pVertices[0].p = D3DXVECTOR3( -0.5f, -0.5f, 0 );
    pVertices[0].c = SOURCE_COLOR;
    pVertices[1].p = D3DXVECTOR3( -0.5f, 0.5f, 0 );
    pVertices[1].c = SOURCE_COLOR;
    pVertices[2].p = D3DXVECTOR3( 0.5f, -0.5f, 0 );
    pVertices[2].c = SOURCE_COLOR;
    pVertices[3].p = D3DXVECTOR3( 0.5f, 0.5f, 0 );
    pVertices[3].c = SOURCE_COLOR;
    g_pvbSource->Unlock();

    // Fill the VB for the floor
    V_RETURN( g_pvbFloor->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
    pVertices[0].p = D3DXVECTOR3( ( FLOAT )XMIN, ( FLOAT )ZMIN, 0 );
    pVertices[0].c = FLOOR_COLOR;
    pVertices[1].p = D3DXVECTOR3( ( FLOAT )XMIN, ( FLOAT )ZMAX, 0 );
    pVertices[1].c = FLOOR_COLOR;
    pVertices[2].p = D3DXVECTOR3( ( FLOAT )XMAX, ( FLOAT )ZMIN, 0 );
    pVertices[2].c = FLOOR_COLOR;
    pVertices[3].p = D3DXVECTOR3( ( FLOAT )XMAX, ( FLOAT )ZMAX, 0 );
    pVertices[3].c = FLOOR_COLOR;
    g_pvbFloor->Unlock();

    // Fill the VB for the grid
    INT i, j;
    V_RETURN( g_pvbGrid->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
    for( i = ZMIN, j = 0; i <= ZMAX; ++i, ++j )
    {
        pVertices[ j * 2 + 0 ].p = D3DXVECTOR3( ( FLOAT )XMIN, ( FLOAT )i, 0 );
        pVertices[ j * 2 + 0 ].c = GRID_COLOR;
        pVertices[ j * 2 + 1 ].p = D3DXVECTOR3( ( FLOAT )XMAX, ( FLOAT )i, 0 );
        pVertices[ j * 2 + 1 ].c = GRID_COLOR;
    }
    for( i = XMIN; i <= XMAX; ++i, ++j )
    {
        pVertices[ j * 2 + 0 ].p = D3DXVECTOR3( ( FLOAT )i, ( FLOAT )ZMIN, 0 );
        pVertices[ j * 2 + 0 ].c = GRID_COLOR;
        pVertices[ j * 2 + 1 ].p = D3DXVECTOR3( ( FLOAT )i, ( FLOAT )ZMAX, 0 );
        pVertices[ j * 2 + 1 ].c = GRID_COLOR;
    }
    g_pvbGrid->Unlock();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT)
// or that are tied to the back buffer size
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice,
                                    const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D9ResetDevice() );
    V_RETURN( g_SettingsDlg.OnD3D9ResetDevice() );

    if( g_pFont9 ) V_RETURN( g_pFont9->OnResetDevice() );
    if( g_pEffect9 ) V_RETURN( g_pEffect9->OnResetDevice() );

    V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pSprite9 ) );
    g_pTxtHelper = new CDXUTTextHelper( g_pFont9, g_pSprite9, NULL, NULL, 15 );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 350 );
    g_SampleUI.SetSize( 170, 300 );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    if( fElapsedTime > 0 )
    {
        D3DXVECTOR3* vec =
            ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.vEmitterPos;

        if( g_moveFlags & FLAG_MOVE_UP )
        {
            vec->z += fElapsedTime * MOTION_SCALE;
            vec->z = min( float( ZMAX ), vec->z );
        }

        if( g_moveFlags & FLAG_MOVE_LEFT )
        {
            vec->x -= fElapsedTime * MOTION_SCALE;
            vec->x = max( float( XMIN ), vec->x );
        }

        if( g_moveFlags & FLAG_MOVE_RIGHT )
        {
            vec->x += fElapsedTime * MOTION_SCALE;
            vec->x = min( float( XMAX ), vec->x );
        }

        if( g_moveFlags & FLAG_MOVE_DOWN )
        {
            vec->z -= fElapsedTime * MOTION_SCALE;
            vec->z = max( float( ZMIN ), vec->z );
        }
    }

    UpdateAudio( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Clear the render target and the zbuffer
    V( pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 45, 50, 170 ), 1.0f, 0 ) );

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
        V( g_pEffect9->SetTechnique( "RenderScene" ) );

        D3DXMATRIXA16 mScale;
        D3DXMatrixScaling( &mScale, 1.f / ( XMAX - XMIN ), 1.f / ( ZMAX - ZMIN ), 1 );

        DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"World" ); // These events are to help PIX identify what the code is doing

        UINT iPass, cPasses;
        V( g_pEffect9->Begin( &cPasses, 0 ) );

        V( pd3dDevice->SetVertexDeclaration( g_pVertexDecl ) );

        for( iPass = 0; iPass < cPasses; ++iPass )
        {
            V( g_pEffect9->BeginPass( iPass ) );

            V( g_pEffect9->SetMatrix( "g_mTransform", &mScale ) );
            V( g_pEffect9->CommitChanges() );

            // Draw the floor
            V( pd3dDevice->SetStreamSource( 0, g_pvbFloor, 0, sizeof( D3DVERTEX ) ) );
            V( pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 ) );

            // Draw the grid
            V( pd3dDevice->SetStreamSource( 0, g_pvbGrid, 0, sizeof( D3DVERTEX ) ) );

            const UINT lcount = ( ( ZMAX - ZMIN + 1 ) + ( XMAX - XMIN + 1 ) );
            V( pd3dDevice->DrawPrimitive( D3DPT_LINELIST, 0, lcount ) );

            // Draw the listener
            {
                D3DXMATRIXA16 mTrans;
                D3DXMatrixTranslation( &mTrans, g_audioState.vListenerPos.x, g_audioState.vListenerPos.z, 0 );

                D3DXMATRIXA16 mRot;
                D3DXMatrixRotationZ( &mRot, -g_audioState.fListenerAngle );

                D3DXMATRIXA16 mat = mRot * mTrans * mScale;
                V( g_pEffect9->SetMatrix( "g_mTransform", &mat ) );
                V( g_pEffect9->CommitChanges() );

                pd3dDevice->SetStreamSource( 0, g_pvbListener, 0, sizeof( D3DVERTEX ) );
                pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 1 );
                if (g_audioState.fUseListenerCone)
                {
                    pd3dDevice->SetStreamSource( 0, g_pvbListenerCone, 0, sizeof( D3DVERTEX ) );
                    pd3dDevice->DrawPrimitive( D3DPT_LINESTRIP, 0, 6 );
                }
                if (g_audioState.fUseInnerRadius)
                {
                    pd3dDevice->SetStreamSource( 0, g_pvbInnerRadius, 0, sizeof( D3DVERTEX ) );
                    pd3dDevice->DrawPrimitive( D3DPT_LINESTRIP, 0, 8 );
                }
            }

            // Draw the source
            {
                D3DXMATRIXA16 mTrans;
                D3DXMatrixTranslation( &mTrans, g_audioState.vEmitterPos.x, g_audioState.vEmitterPos.z, 0 );

                D3DXMATRIXA16 mat = mTrans * mScale;
                V( g_pEffect9->SetMatrix( "g_mTransform", &mat ) );
                V( g_pEffect9->CommitChanges() );

                pd3dDevice->SetStreamSource( 0, g_pvbSource, 0, sizeof( D3DVERTEX ) );
                pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );
            }

            V( g_pEffect9->EndPass() );
        }

        V( g_pEffect9->End() );

        DXUT_EndPerfEvent();

        DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" ); // These events are to help PIX identify what the code is doing
        RenderText();
        V( g_HUD.OnRender( fElapsedTime ) );
        V( g_SampleUI.OnRender( fElapsedTime ) );
        DXUT_EndPerfEvent();

        V( pd3dDevice->EndScene() );
    }
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // We use a simple sound focus model of not hearing the sound if the application is full-screen and minimized
    if( uMsg == WM_ACTIVATEAPP )
    {
        if( wParam == TRUE && !DXUTIsActive() ) // Handle only if previously not active
        {
            if( !DXUTIsWindowed() )
                PauseAudio( true );
        }
        else if( wParam == FALSE && DXUTIsActive() ) // Handle only if previously active
        {
            if( !DXUTIsWindowed() )
                PauseAudio( false );
        }
    }

    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    switch( nChar )
    {
        case 'W':
        case 'w':
            if( bKeyDown )
                g_moveFlags |= FLAG_MOVE_UP;
            else
                g_moveFlags &= ~FLAG_MOVE_UP;
            break;

        case 'A':
        case 'a':
            if( bKeyDown )
                g_moveFlags |= FLAG_MOVE_LEFT;
            else
                g_moveFlags &= ~FLAG_MOVE_LEFT;
            break;

        case 'D':
        case 'd':
            if( bKeyDown )
                g_moveFlags |= FLAG_MOVE_RIGHT;
            else
                g_moveFlags &= ~FLAG_MOVE_RIGHT;
            break;

        case 'S':
        case 's':
            if( bKeyDown )
                g_moveFlags |= FLAG_MOVE_DOWN;
            else
                g_moveFlags &= ~FLAG_MOVE_DOWN;
            break;
    }

}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    CDXUTComboBox* pComboBox = NULL;

    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;

        case IDC_SOUND:
            pComboBox = ( CDXUTComboBox* )pControl;
            PrepareAudio( g_SOUND_NAMES[ PtrToInt( pComboBox->GetSelectedData() ) ] );
            break;

        case IDC_CONTROL_MODE:
            pComboBox = ( CDXUTComboBox* )pControl;
            g_eControlMode = ( CONTROL_MODE )( int )PtrToInt( pComboBox->GetSelectedData() );
            break;

        case IDC_PRESET:
            pComboBox = ( CDXUTComboBox* )pControl;
            SetReverb( ( int )PtrToInt( pComboBox->GetSelectedData() ) );
            break;

        case IDC_UP:
        {
            D3DXVECTOR3* vec =
                ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.vEmitterPos;
            vec->z += 0.5f;
            vec->z = min( float( ZMAX ), vec->z );
        }
            break;

        case IDC_LEFT:
        {
            D3DXVECTOR3* vec =
                ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.vEmitterPos;
            vec->x -= 0.5f;
            vec->x = max( float( XMIN ), vec->x );
        }
            break;

        case IDC_RIGHT:
        {
            D3DXVECTOR3* vec =
                ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.vEmitterPos;
            vec->x += 0.5f;
            vec->x = min( float( XMAX ), vec->x );
        }
            break;

        case IDC_DOWN:
        {
            D3DXVECTOR3* vec =
                ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.vEmitterPos;
            vec->z -= 0.5f;
            vec->z = max( float( ZMIN ), vec->z );
        }
            break;
        case IDC_LISTENERCONE:
        {
            g_audioState.fUseListenerCone = !g_audioState.fUseListenerCone;
        }
            break;
        case IDC_INNERRADIUS:
        {
            g_audioState.fUseInnerRadius = !g_audioState.fUseInnerRadius;
        }
            break;
    }
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9LostDevice( void* pUserContext )
{
    g_moveFlags = 0;

    g_DialogResourceManager.OnD3D9LostDevice();
    g_SettingsDlg.OnD3D9LostDevice();
    if( g_pFont9 ) g_pFont9->OnLostDevice();
    if( g_pEffect9 ) g_pEffect9->OnLostDevice();
    SAFE_RELEASE( g_pSprite9 );
    SAFE_DELETE( g_pTxtHelper );
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D9DestroyDevice();
    g_SettingsDlg.OnD3D9DestroyDevice();
    SAFE_RELEASE( g_pEffect9 );
    SAFE_RELEASE( g_pFont9 );
    SAFE_RELEASE( g_pVertexDecl );
    SAFE_RELEASE( g_pvbFloor );
    SAFE_RELEASE( g_pvbSource );
    SAFE_RELEASE( g_pvbListener );
    SAFE_RELEASE( g_pvbListenerCone );
    SAFE_RELEASE( g_pvbInnerRadius );
    SAFE_RELEASE( g_pvbGrid );
}


