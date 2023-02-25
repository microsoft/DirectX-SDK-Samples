//--------------------------------------------------------------------------------------
// File: FluidCS11.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Smoothed Particle Hydrodynamics Algorithm Based Upon:
// Particle-Based Fluid Simulation for Interactive Applications
// Matthias Müller
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Optimized Grid Algorithm Based Upon:
// Broad-Phase Collision Detection with CUDA
// Scott Le Grand
//--------------------------------------------------------------------------------------

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "resource.h"
#include "WaitDlg.h"
#include <xnamath.h>

struct Particle
{
    XMFLOAT2 vPosition;
    XMFLOAT2 vVelocity;
};

struct ParticleDensity
{
    FLOAT fDensity;
};

struct ParticleForces
{
    XMFLOAT2 vAcceleration;
};

struct UINT2
{
    UINT x;
    UINT y;
};

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

// Compute Shader Constants
// Grid cell key size for sorting, 8-bits for x and y
const UINT NUM_GRID_INDICES = 65536;

// Numthreads size for the simulation
const UINT SIMULATION_BLOCK_SIZE = 256;

// Numthreads size for the sort
const UINT BITONIC_BLOCK_SIZE = 512;
const UINT TRANSPOSE_BLOCK_SIZE = 16;

// For this sample, only use power-of-2 numbers >= 8K and <= 64K
// The algorithm can be extended to support any number of particles
// But to keep the sample simple, we do not implement boundary conditions to handle it
const UINT NUM_PARTICLES_8K = 8 * 1024;
const UINT NUM_PARTICLES_16K = 16 * 1024;
const UINT NUM_PARTICLES_32K = 32 * 1024;
const UINT NUM_PARTICLES_64K = 64 * 1024;
UINT g_iNumParticles = NUM_PARTICLES_16K;

// Particle Properties
// These will control how the fluid behaves
FLOAT g_fInitialParticleSpacing = 0.0045f;
FLOAT g_fSmoothlen = 0.012f;
FLOAT g_fPressureStiffness = 200.0f;
FLOAT g_fRestDensity = 1000.0f;
FLOAT g_fParticleMass = 0.0002f;
FLOAT g_fViscosity = 0.1f;
FLOAT g_fMaxAllowableTimeStep = 0.005f;
FLOAT g_fParticleRenderSize = 0.003f;

// Gravity Directions
const XMFLOAT2A GRAVITY_DOWN(0, -0.5f);
const XMFLOAT2A GRAVITY_UP(0, 0.5f);
const XMFLOAT2A GRAVITY_LEFT(-0.5f, 0);
const XMFLOAT2A GRAVITY_RIGHT(0.5f, 0);
XMFLOAT2A g_vGravity = GRAVITY_DOWN;

// Map Size
// These values should not be larger than 256 * fSmoothlen
// Since the map must be divided up into fSmoothlen sized grid cells
// And the grid cell is used as a 16-bit sort key, 8-bits for x and y
FLOAT g_fMapHeight = 1.2f;
FLOAT g_fMapWidth = (4.0f / 3.0f) * g_fMapHeight;

// Map Wall Collision Planes
FLOAT g_fWallStiffness = 3000.0f;
XMFLOAT3A g_vPlanes[4] = {
    XMFLOAT3A(1, 0, 0),
    XMFLOAT3A(0, 1, 0),
    XMFLOAT3A(-1, 0, g_fMapWidth),
    XMFLOAT3A(0, -1, g_fMapHeight)
};

// Simulation Algorithm
enum eSimulationMode
{
    SIM_MODE_SIMPLE,
    SIM_MODE_SHARED,
    SIM_MODE_GRID
};

eSimulationMode g_eSimMode = SIM_MODE_GRID;

//--------------------------------------------------------------------------------------
// Direct3D11 Global variables
//--------------------------------------------------------------------------------------
ID3D11ShaderResourceView* const     g_pNullSRV = NULL;       // Helper to Clear SRVs
ID3D11UnorderedAccessView* const    g_pNullUAV = NULL;       // Helper to Clear UAVs
ID3D11Buffer* const                 g_pNullBuffer = NULL;    // Helper to Clear Buffers
UINT                                g_iNullUINT = 0;         // Helper to Clear Buffers

CDXUTDialogResourceManager          g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg                     g_D3DSettingsDlg;        // Device settings dialog
CDXUTDialog                         g_HUD;                   // manages the 3D   
CDXUTDialog                         g_SampleUI;              // dialog for sample specific controls

// Resources
CDXUTTextHelper*                    g_pTxtHelper = NULL;

// Shaders
ID3D11VertexShader*                 g_pParticleVS = NULL;
ID3D11GeometryShader*               g_pParticleGS = NULL;
ID3D11PixelShader*                  g_pParticlePS = NULL;

ID3D11ComputeShader*                g_pBuildGridCS = NULL;
ID3D11ComputeShader*                g_pClearGridIndicesCS = NULL;
ID3D11ComputeShader*                g_pBuildGridIndicesCS = NULL;
ID3D11ComputeShader*                g_pRearrangeParticlesCS = NULL;
ID3D11ComputeShader*                g_pDensity_SimpleCS = NULL;
ID3D11ComputeShader*                g_pForce_SimpleCS = NULL;
ID3D11ComputeShader*                g_pDensity_SharedCS = NULL;
ID3D11ComputeShader*                g_pForce_SharedCS = NULL;
ID3D11ComputeShader*                g_pDensity_GridCS = NULL;
ID3D11ComputeShader*                g_pForce_GridCS = NULL;
ID3D11ComputeShader*                g_pIntegrateCS = NULL;

ID3D11ComputeShader*                g_pSortBitonic = NULL;
ID3D11ComputeShader*                g_pSortTranspose = NULL;

// Structured Buffers
ID3D11Buffer*                       g_pParticles = NULL;
ID3D11ShaderResourceView*           g_pParticlesSRV = NULL;
ID3D11UnorderedAccessView*          g_pParticlesUAV = NULL;

ID3D11Buffer*                       g_pSortedParticles = NULL;
ID3D11ShaderResourceView*           g_pSortedParticlesSRV = NULL;
ID3D11UnorderedAccessView*          g_pSortedParticlesUAV = NULL;

ID3D11Buffer*                       g_pParticleDensity = NULL;
ID3D11ShaderResourceView*           g_pParticleDensitySRV = NULL;
ID3D11UnorderedAccessView*          g_pParticleDensityUAV = NULL;

ID3D11Buffer*                       g_pParticleForces = NULL;
ID3D11ShaderResourceView*           g_pParticleForcesSRV = NULL;
ID3D11UnorderedAccessView*          g_pParticleForcesUAV = NULL;

ID3D11Buffer*                       g_pGrid = NULL;
ID3D11ShaderResourceView*           g_pGridSRV = NULL;
ID3D11UnorderedAccessView*          g_pGridUAV = NULL;

ID3D11Buffer*                       g_pGridPingPong = NULL;
ID3D11ShaderResourceView*           g_pGridPingPongSRV = NULL;
ID3D11UnorderedAccessView*          g_pGridPingPongUAV = NULL;

ID3D11Buffer*                       g_pGridIndices = NULL;
ID3D11ShaderResourceView*           g_pGridIndicesSRV = NULL;
ID3D11UnorderedAccessView*          g_pGridIndicesUAV = NULL;

// Constant Buffer Layout
#pragma warning(push)
#pragma warning(disable:4324) // structure was padded due to __declspec(align())
_DECLSPEC_ALIGN_16_ struct CBSimulationConstants
{
    UINT iNumParticles;
    FLOAT fTimeStep;
    FLOAT fSmoothlen;
    FLOAT fPressureStiffness;
    FLOAT fRestDensity;
    FLOAT fDensityCoef;
    FLOAT fGradPressureCoef;
    FLOAT fLapViscosityCoef;
    FLOAT fWallStiffness;
    
    XMFLOAT2A vGravity;
    XMFLOAT4A vGridDim;

    XMFLOAT3A vPlanes[4];
};

_DECLSPEC_ALIGN_16_ struct CBRenderConstants
{
    XMFLOAT4X4 mViewProjection;
    FLOAT fParticleSize;
};

_DECLSPEC_ALIGN_16_ struct SortCB
{
    UINT iLevel;
    UINT iLevelMask;
    UINT iWidth;
    UINT iHeight;
};
#pragma warning(pop)

// Constant Buffers
ID3D11Buffer*                       g_pcbSimulationConstants = NULL;
ID3D11Buffer*                       g_pcbRenderConstants = NULL;
ID3D11Buffer*                       g_pSortCB = NULL;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN      1
#define IDC_TOGGLEREF             3
#define IDC_CHANGEDEVICE          4

#define IDC_RESETSIM              5
#define IDC_NUMPARTICLES          6
#define IDC_GRAVITY               7
#define IDC_SIMMODE               8
#define IDC_SIMSIMPLE             9
#define IDC_SIMSHARED             10
#define IDC_SIMGRID               11

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext );

HRESULT CreateSimulationBuffers( ID3D11Device* pd3dDevice );
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

    // DXUT will create and use the best device (either D3D10 or D3D11) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    InitApp();
    DXUTInit( true, true ); // Parse the command line, show msgboxes on error, and an extra cmd line param to force REF for now
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"FluidCS11" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 1024, 768 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    // Initialize dialogs
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 20;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 22, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 22, VK_F2 );

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 0;

    g_SampleUI.AddButton( IDC_RESETSIM, L"Reset Particles", 0, iY += 26, 170, 22 );
    
    g_SampleUI.AddComboBox( IDC_NUMPARTICLES, 0, iY += 26, 170, 22 );
    g_SampleUI.GetComboBox( IDC_NUMPARTICLES )->AddItem( L"8K Particles", UIntToPtr(NUM_PARTICLES_8K) );
    g_SampleUI.GetComboBox( IDC_NUMPARTICLES )->AddItem( L"16K Particles", UIntToPtr(NUM_PARTICLES_16K) );
    g_SampleUI.GetComboBox( IDC_NUMPARTICLES )->AddItem( L"32K Particles", UIntToPtr(NUM_PARTICLES_32K) );
    g_SampleUI.GetComboBox( IDC_NUMPARTICLES )->AddItem( L"64K Particles", UIntToPtr(NUM_PARTICLES_64K) );
    g_SampleUI.GetComboBox( IDC_NUMPARTICLES )->SetSelectedByData( UIntToPtr(g_iNumParticles) );

    g_SampleUI.AddComboBox( IDC_GRAVITY, 0, iY += 26, 170, 22 );
    g_SampleUI.GetComboBox( IDC_GRAVITY )->AddItem( L"Gravity Down", (void*)&GRAVITY_DOWN );
    g_SampleUI.GetComboBox( IDC_GRAVITY )->AddItem( L"Gravity Up", (void*)&GRAVITY_UP );
    g_SampleUI.GetComboBox( IDC_GRAVITY )->AddItem( L"Gravity Left", (void*)&GRAVITY_LEFT );
    g_SampleUI.GetComboBox( IDC_GRAVITY )->AddItem( L"Gravity Right", (void*)&GRAVITY_RIGHT );

    g_SampleUI.AddRadioButton( IDC_SIMSIMPLE, IDC_SIMMODE, L"Simple N^2", 0, iY += 26, 150, 22 );
    g_SampleUI.AddRadioButton( IDC_SIMSHARED, IDC_SIMMODE, L"Shared Memory N^2", 0, iY += 26, 150, 22 );
    g_SampleUI.AddRadioButton( IDC_SIMGRID, IDC_SIMMODE, L"Grid + Sort", 0, iY += 26, 150, 22 );
    g_SampleUI.GetRadioButton( IDC_SIMGRID )->SetChecked( true );
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF ) ||
            ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

    pDeviceSettings->d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;

    return true;
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
    g_pTxtHelper->DrawFormattedTextLine( L"%i Particles", g_iNumParticles );

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
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
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
            // Standard DXUT controls
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); break;
        case IDC_RESETSIM:
            CreateSimulationBuffers( DXUTGetD3D11Device() ); break;
        case IDC_NUMPARTICLES:
            g_iNumParticles = PtrToUint( ((CDXUTComboBox*)pControl)->GetSelectedData() );
            CreateSimulationBuffers( DXUTGetD3D11Device() );
            break;
        case IDC_GRAVITY:
            g_vGravity = *(const XMFLOAT2A*)((CDXUTComboBox*)pControl)->GetSelectedData(); break;
        case IDC_SIMSIMPLE:
            g_eSimMode = SIM_MODE_SIMPLE; break;
        case IDC_SIMSHARED:
            g_eSimMode = SIM_MODE_SHARED; break;
        case IDC_SIMGRID:
            g_eSimMode = SIM_MODE_GRID; break;
    }
}

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    if ( DeviceInfo->ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x == FALSE )
        return false;

    return true;
}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut, const D3D_SHADER_MACRO* pDefines = NULL )
{
    HRESULT hr = S_OK;

    // find the file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    // compile the shader
    ID3DBlob* pErrorBlob = NULL;
    hr = D3DX11CompileFromFile( str, pDefines, NULL, szEntryPoint, szShaderModel, dwShaderFlags, NULL, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    SAFE_RELEASE( pErrorBlob );

    return S_OK;   
}


//--------------------------------------------------------------------------------------
// Helper for creating constant buffers
//--------------------------------------------------------------------------------------
template <class T>
HRESULT CreateConstantBuffer(ID3D11Device* pd3dDevice, ID3D11Buffer** ppCB)
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = 0;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = sizeof( T );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, ppCB ) );

    return hr;
}


//--------------------------------------------------------------------------------------
// Helper for creating structured buffers with an SRV and UAV
//--------------------------------------------------------------------------------------
template <class T>
HRESULT CreateStructuredBuffer(ID3D11Device* pd3dDevice, UINT iNumElements, ID3D11Buffer** ppBuffer, ID3D11ShaderResourceView** ppSRV, ID3D11UnorderedAccessView** ppUAV, const T* pInitialData = NULL)
{
    HRESULT hr = S_OK;

    // Create SB
    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory( &bufferDesc, sizeof(bufferDesc) );
    bufferDesc.ByteWidth = iNumElements * sizeof(T);
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(T);

    D3D11_SUBRESOURCE_DATA bufferInitData;
    ZeroMemory( &bufferInitData, sizeof(bufferInitData) );
    bufferInitData.pSysMem = pInitialData;
    V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, (pInitialData)? &bufferInitData : NULL, ppBuffer ) );

    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory( &srvDesc, sizeof(srvDesc) );
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementWidth = iNumElements;
    V_RETURN( pd3dDevice->CreateShaderResourceView( *ppBuffer, &srvDesc, ppSRV ) );

    // Create UAV
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    ZeroMemory( &uavDesc, sizeof(uavDesc) );
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.NumElements = iNumElements;
    V_RETURN( pd3dDevice->CreateUnorderedAccessView( *ppBuffer, &uavDesc, ppUAV ) );

    return hr;
}


//--------------------------------------------------------------------------------------
// Create the buffers used for the simulation data
//--------------------------------------------------------------------------------------
HRESULT CreateSimulationBuffers( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;

    // Destroy the old buffers in case the number of particles has changed
    SAFE_RELEASE( g_pParticles );
    SAFE_RELEASE( g_pParticlesSRV );
    SAFE_RELEASE( g_pParticlesUAV );
    
    SAFE_RELEASE( g_pSortedParticles );
    SAFE_RELEASE( g_pSortedParticlesSRV );
    SAFE_RELEASE( g_pSortedParticlesUAV );

    SAFE_RELEASE( g_pParticleForces );
    SAFE_RELEASE( g_pParticleForcesSRV );
    SAFE_RELEASE( g_pParticleForcesUAV );
    
    SAFE_RELEASE( g_pParticleDensity );
    SAFE_RELEASE( g_pParticleDensitySRV );
    SAFE_RELEASE( g_pParticleDensityUAV );

    SAFE_RELEASE( g_pGridSRV );
    SAFE_RELEASE( g_pGridUAV );
    SAFE_RELEASE( g_pGrid );

    SAFE_RELEASE( g_pGridPingPongSRV );
    SAFE_RELEASE( g_pGridPingPongUAV );
    SAFE_RELEASE( g_pGridPingPong );

    SAFE_RELEASE( g_pGridIndicesSRV );
    SAFE_RELEASE( g_pGridIndicesUAV );
    SAFE_RELEASE( g_pGridIndices );

    // Create the initial particle positions
    // This is only used to populate the GPU buffers on creation
    const UINT iStartingWidth = (UINT)sqrt( (FLOAT)g_iNumParticles );
    Particle* particles = new Particle[ g_iNumParticles ];
    ZeroMemory( particles, sizeof(Particle) * g_iNumParticles );
    for ( UINT i = 0 ; i < g_iNumParticles ; i++ )
    {
        // Arrange the particles in a nice square
        UINT x = i % iStartingWidth;
        UINT y = i / iStartingWidth;
        particles[ i ].vPosition = XMFLOAT2( g_fInitialParticleSpacing * (FLOAT)x, g_fInitialParticleSpacing * (FLOAT)y );
    }

    // Create Structured Buffers
    V_RETURN( CreateStructuredBuffer< Particle >( pd3dDevice, g_iNumParticles, &g_pParticles, &g_pParticlesSRV, &g_pParticlesUAV, particles ) );
    DXUT_SetDebugName( g_pParticles, "Particles" );
    DXUT_SetDebugName( g_pParticlesSRV, "Particles SRV" );
    DXUT_SetDebugName( g_pParticlesUAV, "Particles UAV" );

    V_RETURN( CreateStructuredBuffer< Particle >( pd3dDevice, g_iNumParticles, &g_pSortedParticles, &g_pSortedParticlesSRV, &g_pSortedParticlesUAV, particles ) );
    DXUT_SetDebugName( g_pSortedParticles, "Sorted" );
    DXUT_SetDebugName( g_pSortedParticlesSRV, "Sorted SRV" );
    DXUT_SetDebugName( g_pSortedParticlesUAV, "Sorted UAV" );

    V_RETURN( CreateStructuredBuffer< ParticleForces >( pd3dDevice, g_iNumParticles, &g_pParticleForces, &g_pParticleForcesSRV, &g_pParticleForcesUAV ) );
    DXUT_SetDebugName( g_pParticleForces, "Forces" );
    DXUT_SetDebugName( g_pParticleForcesSRV, "Forces SRV" );
    DXUT_SetDebugName( g_pParticleForcesUAV, "Forces UAV" );

    V_RETURN( CreateStructuredBuffer< ParticleDensity >( pd3dDevice, g_iNumParticles, &g_pParticleDensity, &g_pParticleDensitySRV, &g_pParticleDensityUAV ) );
    DXUT_SetDebugName( g_pParticleDensity, "Density" );
    DXUT_SetDebugName( g_pParticleDensitySRV, "Density SRV" );
    DXUT_SetDebugName( g_pParticleDensityUAV, "Density UAV" );

    V_RETURN( CreateStructuredBuffer< UINT >( pd3dDevice, g_iNumParticles, &g_pGrid, &g_pGridSRV, &g_pGridUAV ) );
    DXUT_SetDebugName( g_pGrid, "Grid" );
    DXUT_SetDebugName( g_pGridSRV, "Grid SRV" );
    DXUT_SetDebugName( g_pGridUAV, "Grid UAV" );

    V_RETURN( CreateStructuredBuffer< UINT >( pd3dDevice, g_iNumParticles, &g_pGridPingPong, &g_pGridPingPongSRV, &g_pGridPingPongUAV ) );
    DXUT_SetDebugName( g_pGridPingPong, "PingPong" );
    DXUT_SetDebugName( g_pGridPingPongSRV, "PingPong SRV" );
    DXUT_SetDebugName( g_pGridPingPongUAV, "PingPong UAV" );

    V_RETURN( CreateStructuredBuffer< UINT2 >( pd3dDevice, NUM_GRID_INDICES, &g_pGridIndices, &g_pGridIndicesSRV, &g_pGridIndicesUAV ) );
    DXUT_SetDebugName( g_pGridIndices, "Indices" );
    DXUT_SetDebugName( g_pGridIndicesSRV, "Indices SRV" );
    DXUT_SetDebugName( g_pGridIndicesUAV, "Indices UAV" );

    delete[] particles;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    // Compile the Shaders
    ID3DBlob* pBlob = NULL;

    // Rendering Shaders
    V_RETURN( CompileShaderFromFile( L"FluidRender.hlsl", "ParticleVS", "vs_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pParticleVS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pParticleVS, "ParticleVS" );

    V_RETURN( CompileShaderFromFile( L"FluidRender.hlsl", "ParticleGS", "gs_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreateGeometryShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pParticleGS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pParticleGS, "ParticleGS" );

    V_RETURN( CompileShaderFromFile( L"FluidRender.hlsl", "ParticlePS", "ps_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pParticlePS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pParticlePS, "ParticlePS" );

    // Compute Shaders
    const char* CSTarget = (pd3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)? "cs_5_0" : "cs_4_0";
    
    CWaitDlg CompilingShadersDlg;
    CompilingShadersDlg.ShowDialog( L"Compiling Shaders..." );

    V_RETURN( CompileShaderFromFile( L"FluidCS11.hlsl", "IntegrateCS", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pIntegrateCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pIntegrateCS, "IntegrateCS" );

    V_RETURN( CompileShaderFromFile( L"FluidCS11.hlsl", "DensityCS_Simple", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDensity_SimpleCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pDensity_SimpleCS, "DensityCS_Simple" );

    V_RETURN( CompileShaderFromFile( L"FluidCS11.hlsl", "ForceCS_Simple", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pForce_SimpleCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pForce_SimpleCS, "ForceCS_Simple" );

    V_RETURN( CompileShaderFromFile( L"FluidCS11.hlsl", "DensityCS_Shared", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDensity_SharedCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pDensity_SharedCS, "DensityCS_Shared" );

    V_RETURN( CompileShaderFromFile( L"FluidCS11.hlsl", "ForceCS_Shared", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pForce_SharedCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pForce_SharedCS, "ForceCS_Shared" );

    V_RETURN( CompileShaderFromFile( L"FluidCS11.hlsl", "DensityCS_Grid", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDensity_GridCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pDensity_GridCS, "DensityCS_Grid" );

    V_RETURN( CompileShaderFromFile( L"FluidCS11.hlsl", "ForceCS_Grid", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pForce_GridCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pForce_GridCS, "ForceCS_Grid" );

    V_RETURN( CompileShaderFromFile( L"FluidCS11.hlsl", "BuildGridCS", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pBuildGridCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pBuildGridCS, "BuildGridCS" );

    V_RETURN( CompileShaderFromFile( L"FluidCS11.hlsl", "ClearGridIndicesCS", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pClearGridIndicesCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pClearGridIndicesCS, "ClearGridIndicesCS" );

    V_RETURN( CompileShaderFromFile( L"FluidCS11.hlsl", "BuildGridIndicesCS", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pBuildGridIndicesCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pBuildGridIndicesCS, "BuildGridIndicesCS" );

    V_RETURN( CompileShaderFromFile( L"FluidCS11.hlsl", "RearrangeParticlesCS", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pRearrangeParticlesCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pRearrangeParticlesCS, "RearrangeParticlesCS" );

    // Sort Shaders
    V_RETURN( CompileShaderFromFile( L"ComputeShaderSort11.hlsl", "BitonicSort", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pSortBitonic ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pSortBitonic, "BitonicSort" );

    V_RETURN( CompileShaderFromFile( L"ComputeShaderSort11.hlsl", "MatrixTranspose", CSTarget, &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pSortTranspose ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pSortTranspose, "MatrixTranspose" );

    CompilingShadersDlg.DestroyDialog();

    // Create the Simulation Buffers
    V_RETURN( CreateSimulationBuffers( pd3dDevice ) );

    // Create Constant Buffers
    V_RETURN( CreateConstantBuffer< CBSimulationConstants >( pd3dDevice, &g_pcbSimulationConstants ) );
    V_RETURN( CreateConstantBuffer< CBRenderConstants >( pd3dDevice, &g_pcbRenderConstants ) );

    V_RETURN( CreateConstantBuffer< SortCB >( pd3dDevice, &g_pSortCB ) );

    DXUT_SetDebugName( g_pcbSimulationConstants, "Simluation" );
    DXUT_SetDebugName( g_pcbRenderConstants, "Render" );
    DXUT_SetDebugName( g_pSortCB, "Sort" );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 400 );
    g_SampleUI.SetSize( 170, 300 );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// GPU Bitonic Sort
// For more information, please see the ComputeShaderSort11 sample
//--------------------------------------------------------------------------------------
void GPUSort(ID3D11DeviceContext* pd3dImmediateContext,
             ID3D11UnorderedAccessView* inUAV, ID3D11ShaderResourceView* inSRV,
             ID3D11UnorderedAccessView* tempUAV, ID3D11ShaderResourceView* tempSRV)
{
    pd3dImmediateContext->CSSetConstantBuffers( 0, 1, &g_pSortCB );

    const UINT NUM_ELEMENTS = g_iNumParticles;
    const UINT MATRIX_WIDTH = BITONIC_BLOCK_SIZE;
    const UINT MATRIX_HEIGHT = NUM_ELEMENTS / BITONIC_BLOCK_SIZE;

    // Sort the data
    // First sort the rows for the levels <= to the block size
    for( UINT level = 2 ; level <= BITONIC_BLOCK_SIZE ; level <<= 1 )
    {
        SortCB constants = { level, level, MATRIX_HEIGHT, MATRIX_WIDTH };
        pd3dImmediateContext->UpdateSubresource( g_pSortCB, 0, NULL, &constants, 0, 0 );

        // Sort the row data
        UINT UAVInitialCounts = 0;
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &inUAV, &UAVInitialCounts );
        pd3dImmediateContext->CSSetShader( g_pSortBitonic, NULL, 0 );
        pd3dImmediateContext->Dispatch( NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1 );
    }

    // Then sort the rows and columns for the levels > than the block size
    // Transpose. Sort the Columns. Transpose. Sort the Rows.
    for( UINT level = (BITONIC_BLOCK_SIZE << 1) ; level <= NUM_ELEMENTS ; level <<= 1 )
    {
        SortCB constants1 = { (level / BITONIC_BLOCK_SIZE), (level & ~NUM_ELEMENTS) / BITONIC_BLOCK_SIZE, MATRIX_WIDTH, MATRIX_HEIGHT };
        pd3dImmediateContext->UpdateSubresource( g_pSortCB, 0, NULL, &constants1, 0, 0 );

        // Transpose the data from buffer 1 into buffer 2
        ID3D11ShaderResourceView* pViewNULL = NULL;
        UINT UAVInitialCounts = 0;
        pd3dImmediateContext->CSSetShaderResources( 0, 1, &pViewNULL );
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &tempUAV, &UAVInitialCounts );
        pd3dImmediateContext->CSSetShaderResources( 0, 1, &inSRV );
        pd3dImmediateContext->CSSetShader( g_pSortTranspose, NULL, 0 );
        pd3dImmediateContext->Dispatch( MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, 1 );

        // Sort the transposed column data
        pd3dImmediateContext->CSSetShader( g_pSortBitonic, NULL, 0 );
        pd3dImmediateContext->Dispatch( NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1 );

        SortCB constants2 = { BITONIC_BLOCK_SIZE, level, MATRIX_HEIGHT, MATRIX_WIDTH };
        pd3dImmediateContext->UpdateSubresource( g_pSortCB, 0, NULL, &constants2, 0, 0 );

        // Transpose the data from buffer 2 back into buffer 1
        pd3dImmediateContext->CSSetShaderResources( 0, 1, &pViewNULL );
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &inUAV, &UAVInitialCounts );
        pd3dImmediateContext->CSSetShaderResources( 0, 1, &tempSRV );
        pd3dImmediateContext->CSSetShader( g_pSortTranspose, NULL, 0 );
        pd3dImmediateContext->Dispatch( MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, 1 );

        // Sort the row data
        pd3dImmediateContext->CSSetShader( g_pSortBitonic, NULL, 0 );
        pd3dImmediateContext->Dispatch( NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1 );
    }
}


//--------------------------------------------------------------------------------------
// GPU Fluid Simulation - Simple N^2 Algorithm
//--------------------------------------------------------------------------------------
void SimulateFluid_Simple( ID3D11DeviceContext* pd3dImmediateContext )
{
    UINT UAVInitialCounts = 0;

    // Setup
    pd3dImmediateContext->CSSetConstantBuffers( 0, 1, &g_pcbSimulationConstants );
    pd3dImmediateContext->CSSetShaderResources( 0, 1, &g_pParticlesSRV );

    // Density
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pParticleDensityUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShader( g_pDensity_SimpleCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );

    // Force
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pParticleForcesUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 1, 1, &g_pParticleDensitySRV );
    pd3dImmediateContext->CSSetShader( g_pForce_SimpleCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );

    // Integrate
    pd3dImmediateContext->CopyResource( g_pSortedParticles, g_pParticles );
    pd3dImmediateContext->CSSetShaderResources( 0, 1, &g_pSortedParticlesSRV );
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pParticlesUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 2, 1, &g_pParticleForcesSRV );
    pd3dImmediateContext->CSSetShader( g_pIntegrateCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );
}


//--------------------------------------------------------------------------------------
// GPU Fluid Simulation - Optimized N^2 Algorithm using Shared Memory
//--------------------------------------------------------------------------------------
void SimulateFluid_Shared( ID3D11DeviceContext* pd3dImmediateContext )
{
    UINT UAVInitialCounts = 0;

    // Setup
    pd3dImmediateContext->CSSetConstantBuffers( 0, 1, &g_pcbSimulationConstants );
    pd3dImmediateContext->CSSetShaderResources( 0, 1, &g_pParticlesSRV );

    // Density
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pParticleDensityUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShader( g_pDensity_SharedCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );

    // Force
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pParticleForcesUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 1, 1, &g_pParticleDensitySRV );
    pd3dImmediateContext->CSSetShader( g_pForce_SharedCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );

    // Integrate
    pd3dImmediateContext->CopyResource( g_pSortedParticles, g_pParticles );
    pd3dImmediateContext->CSSetShaderResources( 0, 1, &g_pSortedParticlesSRV );
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pParticlesUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 2, 1, &g_pParticleForcesSRV );
    pd3dImmediateContext->CSSetShader( g_pIntegrateCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );
}


//--------------------------------------------------------------------------------------
// GPU Fluid Simulation - Optimized Algorithm using a Grid + Sort
// Algorithm Overview:
//    Build Grid: For every particle, calculate a hash based on the grid cell it is in
//    Sort Grid: Sort all of the particles based on the grid ID hash
//        Particles in the same cell will now be adjacent in memory
//    Build Grid Indices: Located the start and end offsets for each cell
//    Rearrange: Rearrange the particles into the same order as the grid for easy lookup
//    Density, Force, Integrate: Perform the normal fluid simulation algorithm
//        Except now, only calculate particles from the 8 adjacent cells + current cell
//--------------------------------------------------------------------------------------
void SimulateFluid_Grid( ID3D11DeviceContext* pd3dImmediateContext )
{
    UINT UAVInitialCounts = 0;

    // Setup
    pd3dImmediateContext->CSSetConstantBuffers( 0, 1, &g_pcbSimulationConstants );
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pGridUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 0, 1, &g_pParticlesSRV );

    // Build Grid
    pd3dImmediateContext->CSSetShader( g_pBuildGridCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );

    // Sort Grid
    GPUSort(pd3dImmediateContext, g_pGridUAV, g_pGridSRV, g_pGridPingPongUAV, g_pGridPingPongSRV);

    // Setup
    pd3dImmediateContext->CSSetConstantBuffers( 0, 1, &g_pcbSimulationConstants );
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pGridIndicesUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 3, 1, &g_pGridSRV );

    // Build Grid Indices
    pd3dImmediateContext->CSSetShader( g_pClearGridIndicesCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( NUM_GRID_INDICES / SIMULATION_BLOCK_SIZE, 1, 1 );
    pd3dImmediateContext->CSSetShader( g_pBuildGridIndicesCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );

    // Setup
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pSortedParticlesUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 0, 1, &g_pParticlesSRV );
    pd3dImmediateContext->CSSetShaderResources( 3, 1, &g_pGridSRV );

    // Rearrange
    pd3dImmediateContext->CSSetShader( g_pRearrangeParticlesCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );

    // Setup
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pNullUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 0, 1, &g_pNullSRV );
    pd3dImmediateContext->CSSetShaderResources( 0, 1, &g_pSortedParticlesSRV );
    pd3dImmediateContext->CSSetShaderResources( 3, 1, &g_pGridSRV );
    pd3dImmediateContext->CSSetShaderResources( 4, 1, &g_pGridIndicesSRV );

    // Density
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pParticleDensityUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShader( g_pDensity_GridCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );

    // Force
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pParticleForcesUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 1, 1, &g_pParticleDensitySRV );
    pd3dImmediateContext->CSSetShader( g_pForce_GridCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );

    // Integrate
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pParticlesUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 2, 1, &g_pParticleForcesSRV );
    pd3dImmediateContext->CSSetShader( g_pIntegrateCS, NULL, 0 );
    pd3dImmediateContext->Dispatch( g_iNumParticles / SIMULATION_BLOCK_SIZE, 1, 1 );
}


//--------------------------------------------------------------------------------------
// GPU Fluid Simulation
//--------------------------------------------------------------------------------------
void SimulateFluid( ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime )
{
    UINT UAVInitialCounts = 0;

    // Update per-frame variables
    CBSimulationConstants pData = {};

    // Simulation Constants
    pData.iNumParticles = g_iNumParticles;
    // Clamp the time step when the simulation runs slowly to prevent numerical explosion
    pData.fTimeStep = min( g_fMaxAllowableTimeStep, fElapsedTime );
    pData.fSmoothlen = g_fSmoothlen;
    pData.fPressureStiffness = g_fPressureStiffness;
    pData.fRestDensity = g_fRestDensity;
    pData.fDensityCoef = g_fParticleMass * 315.0f / (64.0f * XM_PI * pow(g_fSmoothlen, 9));
    pData.fGradPressureCoef = g_fParticleMass * -45.0f / (XM_PI * pow(g_fSmoothlen, 6));
    pData.fLapViscosityCoef = g_fParticleMass * g_fViscosity * 45.0f / (XM_PI * pow(g_fSmoothlen, 6));

    pData.vGravity = g_vGravity;
    
    // Cells are spaced the size of the smoothing length search radius
    // That way we only need to search the 8 adjacent cells + current cell
    pData.vGridDim.x = 1.0f / g_fSmoothlen;
    pData.vGridDim.y = 1.0f / g_fSmoothlen;
    pData.vGridDim.z = 0;
    pData.vGridDim.w = 0;

    // Collision information for the map
    pData.fWallStiffness = g_fWallStiffness;
    pData.vPlanes[0] = g_vPlanes[0];
    pData.vPlanes[1] = g_vPlanes[1];
    pData.vPlanes[2] = g_vPlanes[2];
    pData.vPlanes[3] = g_vPlanes[3];

    pd3dImmediateContext->UpdateSubresource( g_pcbSimulationConstants, 0, NULL, &pData, 0, 0 );

    switch (g_eSimMode) {
        // Simple N^2 Algorithm
        case SIM_MODE_SIMPLE:
            SimulateFluid_Simple( pd3dImmediateContext );
            break;

        // Optimized N^2 Algorithm using Shared Memory
        case SIM_MODE_SHARED:
            SimulateFluid_Shared( pd3dImmediateContext );
            break;

        // Optimized Grid + Sort Algorithm
        case SIM_MODE_GRID:
            SimulateFluid_Grid( pd3dImmediateContext );
            break;
    }

    // Unset
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &g_pNullUAV, &UAVInitialCounts );
    pd3dImmediateContext->CSSetShaderResources( 0, 1, &g_pNullSRV );
    pd3dImmediateContext->CSSetShaderResources( 1, 1, &g_pNullSRV );
    pd3dImmediateContext->CSSetShaderResources( 2, 1, &g_pNullSRV );
    pd3dImmediateContext->CSSetShaderResources( 3, 1, &g_pNullSRV );
    pd3dImmediateContext->CSSetShaderResources( 4, 1, &g_pNullSRV );
}


//--------------------------------------------------------------------------------------
// GPU Fluid Rendering
//--------------------------------------------------------------------------------------
void RenderFluid( ID3D11DeviceContext* pd3dImmediateContext, float fElapsedTime )
{
    // Simple orthographic projection to display the entire map
    XMMATRIX mView = XMMatrixTranslation( -g_fMapWidth / 2.0f, -g_fMapHeight / 2.0f, 0 );
    XMMATRIX mProjection = XMMatrixOrthographicLH( g_fMapWidth, g_fMapHeight, 0, 1 );
    XMMATRIX mViewProjection = mView * mProjection;

    // Update Constants
    CBRenderConstants pData = {};

    XMStoreFloat4x4( &pData.mViewProjection, XMMatrixTranspose( mViewProjection ) );
    pData.fParticleSize = g_fParticleRenderSize;

    pd3dImmediateContext->UpdateSubresource( g_pcbRenderConstants, 0, NULL, &pData, 0, 0 );

    // Set the shaders
    pd3dImmediateContext->VSSetShader( g_pParticleVS, NULL, 0 );
    pd3dImmediateContext->GSSetShader( g_pParticleGS, NULL, 0 );
    pd3dImmediateContext->PSSetShader( g_pParticlePS, NULL, 0 );

    // Set the constant buffers
    pd3dImmediateContext->VSSetConstantBuffers( 0, 1, &g_pcbRenderConstants );
    pd3dImmediateContext->GSSetConstantBuffers( 0, 1, &g_pcbRenderConstants );
    pd3dImmediateContext->PSSetConstantBuffers( 0, 1, &g_pcbRenderConstants );

    // Setup the particles buffer and IA
    pd3dImmediateContext->VSSetShaderResources( 0, 1, &g_pParticlesSRV );
    pd3dImmediateContext->VSSetShaderResources( 1, 1, &g_pParticleDensitySRV );
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, &g_pNullBuffer, &g_iNullUINT, &g_iNullUINT );
    pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );

    // Draw the mesh
    pd3dImmediateContext->Draw( g_iNumParticles, 0 );

    // Unset the particles buffer
    pd3dImmediateContext->VSSetShaderResources( 0, 1, &g_pNullSRV );
    pd3dImmediateContext->VSSetShaderResources( 1, 1, &g_pNullSRV );
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext )
{
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Clear the render target and depth stencil
    float ClearColor[4] = { 0.05f, 0.05f, 0.05f, 0.0f };
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    SimulateFluid( pd3dImmediateContext, fElapsedTime );

    RenderFluid( pd3dImmediateContext, fElapsedTime );

    // Render the HUD
    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_HUD.OnRender( fElapsedTime );
    g_SampleUI.OnRender( fElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

    SAFE_RELEASE( g_pcbSimulationConstants );
    SAFE_RELEASE( g_pcbRenderConstants );
    SAFE_RELEASE( g_pSortCB );

    SAFE_RELEASE( g_pParticleVS );
    SAFE_RELEASE( g_pParticleGS );
    SAFE_RELEASE( g_pParticlePS );

    SAFE_RELEASE( g_pIntegrateCS );
    SAFE_RELEASE( g_pDensity_SimpleCS );
    SAFE_RELEASE( g_pForce_SimpleCS );
    SAFE_RELEASE( g_pDensity_SharedCS );
    SAFE_RELEASE( g_pForce_SharedCS );
    SAFE_RELEASE( g_pDensity_GridCS );
    SAFE_RELEASE( g_pForce_GridCS );
    SAFE_RELEASE( g_pBuildGridCS );
    SAFE_RELEASE( g_pClearGridIndicesCS );
    SAFE_RELEASE( g_pBuildGridIndicesCS );
    SAFE_RELEASE( g_pRearrangeParticlesCS );
    SAFE_RELEASE( g_pSortBitonic );
    SAFE_RELEASE( g_pSortTranspose );

    SAFE_RELEASE( g_pParticles );
    SAFE_RELEASE( g_pParticlesSRV );
    SAFE_RELEASE( g_pParticlesUAV );
    
    SAFE_RELEASE( g_pSortedParticles );
    SAFE_RELEASE( g_pSortedParticlesSRV );
    SAFE_RELEASE( g_pSortedParticlesUAV );

    SAFE_RELEASE( g_pParticleForces );
    SAFE_RELEASE( g_pParticleForcesSRV );
    SAFE_RELEASE( g_pParticleForcesUAV );
    
    SAFE_RELEASE( g_pParticleDensity );
    SAFE_RELEASE( g_pParticleDensitySRV );
    SAFE_RELEASE( g_pParticleDensityUAV );

    SAFE_RELEASE( g_pGridSRV );
    SAFE_RELEASE( g_pGridUAV );
    SAFE_RELEASE( g_pGrid );

    SAFE_RELEASE( g_pGridPingPongSRV );
    SAFE_RELEASE( g_pGridPingPongUAV );
    SAFE_RELEASE( g_pGridPingPong );

    SAFE_RELEASE( g_pGridIndicesSRV );
    SAFE_RELEASE( g_pGridIndicesUAV );
    SAFE_RELEASE( g_pGridIndices );
}
