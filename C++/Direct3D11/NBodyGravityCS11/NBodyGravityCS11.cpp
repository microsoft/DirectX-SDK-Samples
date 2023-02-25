//--------------------------------------------------------------------------------------
// File: NBodyGravityCS11.cpp
//
// Demonstrates how to use Compute Shader to do n-body gravity computation
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTgui.h"
#include "SDKmisc.h"
#include "DXUTcamera.h"
#include "DXUTsettingsdlg.h"
#include <d3dx11.h>
#include <commdlg.h>
#include "resource.h"
#include "WaitDlg.h"

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager          g_DialogResourceManager;    // manager for shared resources of dialogs
CModelViewerCamera                  g_Camera;                   // A model viewing camera
CD3DSettingsDlg                     g_D3DSettingsDlg;           // Device settings dialog
CDXUTDialog                         g_HUD;                      // dialog for standard controls
CDXUTDialog                         g_SampleUI;                 // dialog for sample specific controls
CDXUTTextHelper*                    g_pTxtHelper = NULL;

ID3D11VertexShader*                 g_pRenderParticlesVS = NULL;
ID3D11GeometryShader*               g_pRenderParticlesGS = NULL;
ID3D11PixelShader*                  g_pRenderParticlesPS = NULL;
ID3D11SamplerState*                 g_pSampleStateLinear = NULL;
ID3D11BlendState*                   g_pBlendingStateParticle = NULL;
ID3D11DepthStencilState*            g_pDepthStencilState = NULL;

ID3D11ComputeShader*                g_pCalcCS = NULL;
ID3D11Buffer*                       g_pcbCS = NULL;

ID3D11Buffer*                       g_pParticlePosVelo0 = NULL;
ID3D11Buffer*                       g_pParticlePosVelo1 = NULL;
ID3D11ShaderResourceView*           g_pParticlePosVeloRV0 = NULL;
ID3D11ShaderResourceView*           g_pParticlePosVeloRV1 = NULL;
ID3D11UnorderedAccessView*          g_pParticlePosVeloUAV0 = NULL;
ID3D11UnorderedAccessView*          g_pParticlePosVeloUAV1 = NULL;
ID3D11Buffer*                       g_pParticleBuffer = NULL;
ID3D11InputLayout*                  g_pParticleVertexLayout = NULL;

ID3D11Buffer*                       g_pcbGS = NULL;

ID3D11ShaderResourceView*           g_pParticleTexRV = NULL;

float                               g_fSpread = 400.0f;

struct PARTICLE_VERTEX
{
    D3DXVECTOR4 Color;
};
#define MAX_PARTICLES 10000         // the number of particles in the n-body simulation

struct CB_GS
{
    D3DXMATRIX m_WorldViewProj;
    D3DXMATRIX m_InvView;
};

struct CB_CS
{
    UINT param[4];
    float paramf[4];
};

struct PARTICLE
{
    D3DXVECTOR4 pos;
    D3DXVECTOR4 velo;
};

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4
#define IDC_RESETPARTICLES      5

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
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

void InitApp();
void RenderText();

//--------------------------------------------------------------------------------------
// Find and compile the specified shader
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
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

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( str, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
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
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackFrameMove( OnFrameMove );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    InitApp();

    DXUTInit( true, true );                 // Use this line instead to try to create a hardware device

    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"NBodyGravityCS11" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 800, 600 );
    DXUTMainLoop();                      // Enter into the DXUT render loop

    return DXUTGetExitCode();
}

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 23 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 23, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 23, VK_F2 );
    g_HUD.AddButton( IDC_RESETPARTICLES, L"Reset particles", 0, iY += 26, 170, 22, VK_F2 );
    g_SampleUI.SetCallback( OnGUIEvent ); 
}

HRESULT CreateParticleBuffer( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC vbdesc =
    {
        MAX_PARTICLES * sizeof( PARTICLE_VERTEX ),
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_VERTEX_BUFFER,
        0,
        0
    };
    D3D11_SUBRESOURCE_DATA vbInitData;
    ZeroMemory( &vbInitData, sizeof( D3D11_SUBRESOURCE_DATA ) );

    PARTICLE_VERTEX* pVertices = new PARTICLE_VERTEX[ MAX_PARTICLES ];
    if( !pVertices )
        return E_OUTOFMEMORY;

    for( UINT i = 0; i < MAX_PARTICLES; i++ )
    {
        pVertices[i].Color = D3DXVECTOR4( 1, 1, 0.2f, 1 );
    }

    vbInitData.pSysMem = pVertices;
    V_RETURN( pd3dDevice->CreateBuffer( &vbdesc, &vbInitData, &g_pParticleBuffer ) );
    DXUT_SetDebugName( g_pParticleBuffer, "Particles" );
    SAFE_DELETE_ARRAY( pVertices );

    return hr;
}

//--------------------------------------------------------------------------------------
float RPercent()
{
    float ret = ( float )( ( rand() % 10000 ) - 5000 );
    return ret / 5000.0f;
}

float
normalize(D3DXVECTOR3& vector)
{
    float dist = sqrtf(vector.x*vector.x + vector.y*vector.y + vector.z*vector.z);
    if (dist > 1e-6)
    {
        vector.x /= dist;
        vector.y /= dist;
        vector.z /= dist;
    }
    return dist;
}

//--------------------------------------------------------------------------------------
// This helper function loads a group of particles
//--------------------------------------------------------------------------------------
void LoadParticles( PARTICLE* pParticles,
                   D3DXVECTOR3 Center, D3DXVECTOR4 Velocity, float Spread, UINT NumParticles )
{
    for( UINT i = 0; i < NumParticles; i++ )
    {
        D3DXVECTOR3 Delta( Spread, Spread, Spread );

        while( D3DXVec3LengthSq( &Delta ) > Spread * Spread )
        {
            Delta.x = RPercent() * Spread;
            Delta.y = RPercent() * Spread;
            Delta.z = RPercent() * Spread;
        }

        pParticles[i].pos.x = Center.x + Delta.x;
        pParticles[i].pos.y = Center.y + Delta.y;
        pParticles[i].pos.z = Center.z + Delta.z;
        pParticles[i].pos.w = 10000.0f * 10000.0f;

        pParticles[i].velo = Velocity;
    }    
}

HRESULT CreateParticlePosVeloBuffers( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.ByteWidth = MAX_PARTICLES * sizeof(PARTICLE);
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(PARTICLE);
    desc.Usage = D3D11_USAGE_DEFAULT;

    // Initialize the data in the buffers
    PARTICLE* pData1 = new PARTICLE[ MAX_PARTICLES ];
    if( !pData1 )
        return E_OUTOFMEMORY;    

    srand( GetTickCount() );   

#if 1
    // Disk Galaxy Formation
    float fCenterSpread = g_fSpread * 0.50f;
    LoadParticles( pData1,
        D3DXVECTOR3( fCenterSpread, 0, 0 ), D3DXVECTOR4( 0, 0, -20, 1/10000.0f / 10000.0f ),
        g_fSpread, MAX_PARTICLES / 2 );
    LoadParticles( &pData1[MAX_PARTICLES / 2],
        D3DXVECTOR3( -fCenterSpread, 0, 0 ), D3DXVECTOR4( 0, 0, 20, 1/10000.0f / 10000.0f ),
        g_fSpread, MAX_PARTICLES / 2 );    
#else
    // Disk Galaxy Formation with impacting third cluster
    LoadParticles( pData1, 
        D3DXVECTOR3(g_fSpread,0,0), D3DXVECTOR4(0,0,-8,1/10000.0f / 10000.0f),
        g_fSpread, MAX_PARTICLES/3 );
    LoadParticles( &pData1[MAX_PARTICLES/3],
        D3DXVECTOR3(-g_fSpread,0,0), D3DXVECTOR4(0,0,8,1/10000.0f / 10000.0f),
        g_fSpread, MAX_PARTICLES/2 );
    LoadParticles( &pData1[2*(MAX_PARTICLES/3)],
        D3DXVECTOR3(0,0,g_fSpread*15.0f), D3DXVECTOR4(0,0,-60,1/10000.0f / 10000.0f),
        g_fSpread, MAX_PARTICLES - 2*(MAX_PARTICLES/3) );
#endif

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = pData1;
    V_RETURN( pd3dDevice->CreateBuffer( &desc, &InitData, &g_pParticlePosVelo0 ) );
    V_RETURN( pd3dDevice->CreateBuffer( &desc, &InitData, &g_pParticlePosVelo1 ) );
    DXUT_SetDebugName( g_pParticlePosVelo0, "PosVelo0" );
    DXUT_SetDebugName( g_pParticlePosVelo1, "PosVelo1" );
    SAFE_DELETE_ARRAY( pData1 );

    D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
    ZeroMemory( &DescRV, sizeof( DescRV ) );
    DescRV.Format = DXGI_FORMAT_UNKNOWN;
    DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    DescRV.Buffer.FirstElement = 0;
    DescRV.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pParticlePosVelo0, &DescRV, &g_pParticlePosVeloRV0 ) );
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pParticlePosVelo1, &DescRV, &g_pParticlePosVeloRV1 ) );
    DXUT_SetDebugName( g_pParticlePosVeloRV0, "PosVelo0 SRV" );
    DXUT_SetDebugName( g_pParticlePosVeloRV1, "PosVelo1 SRV" );

    D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
    ZeroMemory( &DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
    DescUAV.Format = DXGI_FORMAT_UNKNOWN;
    DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    DescUAV.Buffer.FirstElement = 0;
    DescUAV.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;
    V_RETURN( pd3dDevice->CreateUnorderedAccessView( g_pParticlePosVelo0, &DescUAV, &g_pParticlePosVeloUAV0 ) );
    V_RETURN( pd3dDevice->CreateUnorderedAccessView( g_pParticlePosVelo1, &DescUAV, &g_pParticlePosVeloUAV1 ) );
    DXUT_SetDebugName( g_pParticlePosVeloUAV0, "PosVelo0 UAV" );
    DXUT_SetDebugName( g_pParticlePosVeloUAV1, "PosVelo1 UAV" );

    return hr;
}

bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    assert( pDeviceSettings->ver == DXUT_D3D11_DEVICE );

    // For the first device created if it is a REF device, optionally display a warning dialog box
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

    return true;
}

template <class T>
void SWAP( T* &x, T* &y )
{
    T* temp = x;
    x = y;
    y = temp;
}

//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();

    int dimx = int(ceil(MAX_PARTICLES/128.0f));
    
    {
        pd3dImmediateContext->CSSetShader( g_pCalcCS, NULL, 0 );

        // For CS input            
        ID3D11ShaderResourceView* aRViews[ 1 ] = { g_pParticlePosVeloRV0 };
        pd3dImmediateContext->CSSetShaderResources( 0, 1, aRViews );

        // For CS output
        ID3D11UnorderedAccessView* aUAViews[ 1 ] = { g_pParticlePosVeloUAV1 };
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, aUAViews, (UINT*)(&aUAViews) );

        // For CS constant buffer
        D3D11_MAPPED_SUBRESOURCE MappedResource;
        V( pd3dImmediateContext->Map( g_pcbCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
        CB_CS* pcbCS = ( CB_CS* )MappedResource.pData;
        pcbCS->param[0] = MAX_PARTICLES;
        pcbCS->param[1] = dimx;      
        pcbCS->paramf[0] = 0.1f;
        pcbCS->paramf[1] = 1;
        pd3dImmediateContext->Unmap( g_pcbCS, 0 );
        ID3D11Buffer* ppCB[1] = { g_pcbCS };
        pd3dImmediateContext->CSSetConstantBuffers( 0, 1, ppCB );

        // Run the CS
        pd3dImmediateContext->Dispatch( dimx, 1, 1 );

        // Unbind resources for CS
        ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
        pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, ppUAViewNULL, (UINT*)(&aUAViews) );
        ID3D11ShaderResourceView* ppSRVNULL[1] = { NULL };
        pd3dImmediateContext->CSSetShaderResources( 0, 1, ppSRVNULL );

        //pd3dImmediateContext->CSSetShader( NULL, NULL, 0 );

        SWAP( g_pParticlePosVelo0, g_pParticlePosVelo1 );
        SWAP( g_pParticlePosVeloRV0, g_pParticlePosVeloRV1 );
        SWAP( g_pParticlePosVeloUAV0, g_pParticlePosVeloUAV1 );
    }

    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}

//--------------------------------------------------------------------------------------
// Before handling window messages, DXUT passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then DXUT will not process this message.
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

    // Pass all windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
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
        g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); break;

    case IDC_RESETPARTICLES:
        {
            SAFE_RELEASE(g_pParticlePosVelo0);
            SAFE_RELEASE(g_pParticlePosVelo1);
            SAFE_RELEASE(g_pParticlePosVeloRV0);
            SAFE_RELEASE(g_pParticlePosVeloRV1);
            SAFE_RELEASE(g_pParticlePosVeloUAV0);
            SAFE_RELEASE(g_pParticlePosVeloUAV1);
            CreateParticlePosVeloBuffers(DXUTGetD3D11Device());
            break;
        }
    }
}

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                      DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // reject any device which doesn't support CS4x
    if ( DeviceInfo->ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x == FALSE )
        return false;
    
    return true;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;

    static bool bFirstOnCreateDevice = true;

    // Warn the user that in order to support CS4x, a non-hardware device has been created, continue or quit?
    if ( DXUTGetDeviceSettings().d3d11.DriverType != D3D_DRIVER_TYPE_HARDWARE && bFirstOnCreateDevice )
    {
        if ( MessageBox( 0, L"CS4x capability is missing. "\
                            L"In order to continue, a non-hardware device has been created, "\
                            L"it will be very slow, continue?", L"Warning", MB_ICONEXCLAMATION | MB_YESNO ) != IDYES )
            return E_FAIL;
    }

    CWaitDlg CompilingShadersDlg;
    CompilingShadersDlg.ShowDialog( L"Compiling Shaders..." );

    bFirstOnCreateDevice = false;

    D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS ho;
    V_RETURN( pd3dDevice->CheckFeatureSupport( D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &ho, sizeof(ho) ) );

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    ID3DBlob* pBlobRenderParticlesVS = NULL;
    ID3DBlob* pBlobRenderParticlesGS = NULL;
    ID3DBlob* pBlobRenderParticlesPS = NULL;
    ID3DBlob* pBlobCalcCS = NULL;

    // Create the shaders
    V_RETURN( CompileShaderFromFile( L"ParticleDraw.hlsl", "VSParticleDraw", "vs_4_0", &pBlobRenderParticlesVS ) );
    V_RETURN( CompileShaderFromFile( L"ParticleDraw.hlsl", "GSParticleDraw", "gs_4_0", &pBlobRenderParticlesGS ) );
    V_RETURN( CompileShaderFromFile( L"ParticleDraw.hlsl", "PSParticleDraw", "ps_4_0", &pBlobRenderParticlesPS ) );
    V_RETURN( CompileShaderFromFile( L"NBodyGravityCS11.hlsl", "CSMain", "cs_4_0", &pBlobCalcCS ) );

    V_RETURN( pd3dDevice->CreateVertexShader( pBlobRenderParticlesVS->GetBufferPointer(), pBlobRenderParticlesVS->GetBufferSize(), NULL, &g_pRenderParticlesVS ) );
    DXUT_SetDebugName( g_pRenderParticlesVS, "VSParticleDraw" );

    V_RETURN( pd3dDevice->CreateGeometryShader( pBlobRenderParticlesGS->GetBufferPointer(), pBlobRenderParticlesGS->GetBufferSize(), NULL, &g_pRenderParticlesGS ) );
    DXUT_SetDebugName( g_pRenderParticlesGS, "GSParticleDraw" );

    V_RETURN( pd3dDevice->CreatePixelShader( pBlobRenderParticlesPS->GetBufferPointer(), pBlobRenderParticlesPS->GetBufferSize(), NULL, &g_pRenderParticlesPS ) );
    DXUT_SetDebugName( g_pRenderParticlesPS, "PSParticleDraw" );

    V_RETURN( pd3dDevice->CreateComputeShader( pBlobCalcCS->GetBufferPointer(), pBlobCalcCS->GetBufferSize(), NULL, &g_pCalcCS ) );
    DXUT_SetDebugName( g_pCalcCS, "CSMain" );

    // Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    V_RETURN( pd3dDevice->CreateInputLayout( layout, sizeof( layout ) / sizeof( layout[0] ),
        pBlobRenderParticlesVS->GetBufferPointer(), pBlobRenderParticlesVS->GetBufferSize(), &g_pParticleVertexLayout ) );
    DXUT_SetDebugName( g_pParticleVertexLayout, "Particles" );
    
    SAFE_RELEASE( pBlobRenderParticlesVS );
    SAFE_RELEASE( pBlobRenderParticlesGS );
    SAFE_RELEASE( pBlobRenderParticlesPS );
    SAFE_RELEASE( pBlobCalcCS );

    V_RETURN( CreateParticleBuffer( pd3dDevice ) );
    V_RETURN( CreateParticlePosVeloBuffers( pd3dDevice ) );

    // Setup constant buffer
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = sizeof( CB_GS );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbGS ) );
    DXUT_SetDebugName( g_pcbGS, "CB_GS" );

    Desc.ByteWidth = sizeof( CB_CS );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbCS ) );
    DXUT_SetDebugName( g_pcbCS, "CB_CS" );

    // Load the Particle Texture
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"misc\\Particle.dds" ) );
    V_RETURN( D3DX11CreateShaderResourceViewFromFile( pd3dDevice, str, NULL, NULL, &g_pParticleTexRV, NULL ) );
    DXUT_SetDebugName( g_pParticleTexRV, "Particle.dds" );

    D3D11_SAMPLER_DESC SamplerDesc;
    ZeroMemory( &SamplerDesc, sizeof(SamplerDesc) );
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    V_RETURN( pd3dDevice->CreateSamplerState( &SamplerDesc, &g_pSampleStateLinear ) );
    DXUT_SetDebugName( g_pSampleStateLinear, "Linear" );

    D3D11_BLEND_DESC BlendStateDesc;
    ZeroMemory( &BlendStateDesc, sizeof(BlendStateDesc) );
    BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
    BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
    V_RETURN( pd3dDevice->CreateBlendState( &BlendStateDesc, &g_pBlendingStateParticle ) );
    DXUT_SetDebugName( g_pBlendingStateParticle, "Blending" );

    D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
    ZeroMemory( &DepthStencilDesc, sizeof(DepthStencilDesc) );
    DepthStencilDesc.DepthEnable = FALSE;
    DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    pd3dDevice->CreateDepthStencilState( &DepthStencilDesc, &g_pDepthStencilState );
    DXUT_SetDebugName( g_pDepthStencilState, "DepthOff" );

    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye( -g_fSpread * 2, g_fSpread * 4, -g_fSpread * 3 );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, 0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );

    CompilingShadersDlg.DestroyDialog();

    return S_OK;
}

HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 10.0f, 500000.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( 0, MOUSE_WHEEL, MOUSE_LEFT_BUTTON | MOUSE_MIDDLE_BUTTON | MOUSE_RIGHT_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300 );
    g_SampleUI.SetSize( 170, 300 );

    return hr;
}

void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();    
}

void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    g_pTxtHelper->End();
}

//--------------------------------------------------------------------------------------
bool RenderParticles( ID3D11DeviceContext* pd3dImmediateContext, D3DXMATRIX& mView, D3DXMATRIX& mProj )
{
    D3DXMATRIX mWorldViewProj;

    ID3D11BlendState *pBlendState0 = NULL;
    ID3D11DepthStencilState *pDepthStencilState0 = NULL;
    UINT SampleMask0, StencilRef0;
    D3DXVECTOR4 BlendFactor0;
    pd3dImmediateContext->OMGetBlendState( &pBlendState0, &BlendFactor0.x, &SampleMask0 );
    pd3dImmediateContext->OMGetDepthStencilState( &pDepthStencilState0, &StencilRef0 );

    pd3dImmediateContext->VSSetShader( g_pRenderParticlesVS, NULL, 0 );
    pd3dImmediateContext->GSSetShader( g_pRenderParticlesGS, NULL, 0 );
    pd3dImmediateContext->PSSetShader( g_pRenderParticlesPS, NULL, 0 );
    
    pd3dImmediateContext->IASetInputLayout( g_pParticleVertexLayout );

    // Set IA parameters
    ID3D11Buffer* pBuffers[1] = { g_pParticleBuffer };
    UINT stride[1] = { sizeof( PARTICLE_VERTEX ) };
    UINT offset[1] = { 0 };
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, pBuffers, stride, offset );
    pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );

    ID3D11ShaderResourceView* aRViews[ 1 ] = { g_pParticlePosVeloRV0 };
    pd3dImmediateContext->VSSetShaderResources( 0, 1, aRViews );

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map( g_pcbGS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    CB_GS* pCBGS = ( CB_GS* )MappedResource.pData; 
    D3DXMatrixMultiply( &pCBGS->m_WorldViewProj, &mView, &mProj );   
    D3DXMatrixInverse( &pCBGS->m_InvView, NULL, &mView );
    pd3dImmediateContext->Unmap( g_pcbGS, 0 );
    pd3dImmediateContext->GSSetConstantBuffers( 0, 1, &g_pcbGS );

    pd3dImmediateContext->PSSetShaderResources( 0, 1, &g_pParticleTexRV );
    pd3dImmediateContext->PSSetSamplers( 0, 1, &g_pSampleStateLinear );

    pd3dImmediateContext->OMSetBlendState( g_pBlendingStateParticle, D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF  );
    pd3dImmediateContext->OMSetDepthStencilState( g_pDepthStencilState, 0 );

    pd3dImmediateContext->Draw( MAX_PARTICLES, 0 );

    ID3D11ShaderResourceView* ppSRVNULL[1] = { NULL };
    pd3dImmediateContext->VSSetShaderResources( 0, 1, ppSRVNULL );
    pd3dImmediateContext->PSSetShaderResources( 0, 1, ppSRVNULL );

    /*ID3D11Buffer* ppBufNULL[1] = { NULL };
    pd3dImmediateContext->GSSetConstantBuffers( 0, 1, ppBufNULL );*/

    pd3dImmediateContext->GSSetShader( NULL, NULL, 0 );
    pd3dImmediateContext->OMSetBlendState( pBlendState0, &BlendFactor0.x, SampleMask0 ); SAFE_RELEASE(pBlendState0);
    pd3dImmediateContext->OMSetDepthStencilState( pDepthStencilState0, StencilRef0 ); SAFE_RELEASE(pDepthStencilState0);

    return true;
}

void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext )
{
    float ClearColor[4] = { 0.0, 0.0, 0.0, 0.0 };

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }
    
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    D3DXMATRIX mView;
    D3DXMATRIX mProj;

    // Get the projection & view matrix from the camera class
    mProj = *g_Camera.GetProjMatrix();
    mView = *g_Camera.GetViewMatrix();

    // Render the particles
    RenderParticles( pd3dImmediateContext, mView, mProj );

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_HUD.OnRender( fElapsedTime );
    g_SampleUI.OnRender( fElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();

    // The following could be used to output fps stats into debug output window,
    // which is useful because you can then turn off all UI rendering as they cloud performance
    /*static DWORD dwTimefirst = GetTickCount();
    if ( GetTickCount() - dwTimefirst > 5000 )
    {    
        OutputDebugString( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
        OutputDebugString( L"\n" );
        dwTimefirst = GetTickCount();
    }*/
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

    SAFE_RELEASE( g_pParticleBuffer ); 
    SAFE_RELEASE( g_pParticleVertexLayout );

    SAFE_RELEASE( g_pParticlePosVelo0 );
    SAFE_RELEASE( g_pParticlePosVelo1 );
    SAFE_RELEASE( g_pParticlePosVeloRV0 );
    SAFE_RELEASE( g_pParticlePosVeloRV1 );
    SAFE_RELEASE( g_pParticlePosVeloUAV0 );
    SAFE_RELEASE( g_pParticlePosVeloUAV1 );

    SAFE_RELEASE( g_pcbGS );
    SAFE_RELEASE( g_pcbCS );

    SAFE_RELEASE( g_pParticleTexRV );

    SAFE_RELEASE( g_pRenderParticlesVS );
    SAFE_RELEASE( g_pRenderParticlesGS );
    SAFE_RELEASE( g_pRenderParticlesPS );
    SAFE_RELEASE( g_pCalcCS );
    SAFE_RELEASE( g_pSampleStateLinear );
    SAFE_RELEASE( g_pBlendingStateParticle );
    SAFE_RELEASE( g_pDepthStencilState );
}
