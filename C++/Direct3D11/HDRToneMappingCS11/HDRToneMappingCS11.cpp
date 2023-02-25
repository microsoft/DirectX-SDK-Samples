//--------------------------------------------------------------------------------------
// File: HDRToneMappingCS11.cpp
//
// Demonstrates how to use Compute Shader to do post-processing
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"
#include "skybox11.h"
#include <D3DX11tex.h>
#include <D3DX11.h>
#include <D3DX11core.h>
#include <D3DX11async.h>

#define NUM_TONEMAP_TEXTURES  5       // Number of stages in the 3x3 down-scaling for post-processing in PS
static const int ToneMappingTexSize = (int)pow(3.0f, NUM_TONEMAP_TEXTURES-1);
#define NUM_BLOOM_TEXTURES 2

enum POSTPROCESS_MODE
{
    PM_COMPUTE_SHADER,
    PM_PIXEL_SHADER,
};
POSTPROCESS_MODE g_ePostProcessMode = PM_COMPUTE_SHADER;// Stores which path is currently used for post-processing

CDXUTDialogResourceManager  g_DialogResourceManager;    // Manager for shared resources of dialogs
CModelViewerCamera          g_Camera;                   // A model viewing camera
CD3DSettingsDlg             g_D3DSettingsDlg;           // Device settings dialog
CDXUTDialog                 g_HUD;                      // Dialog for standard controls
CDXUTDialog                 g_SampleUI;                 // Dialog for sample specific controls
CSkybox11                   g_Skybox;

CDXUTTextHelper*            g_pTxtHelper = NULL;

// Shaders used in CS path
ID3D11ComputeShader*        g_pReduceTo1DCS = NULL;
ID3D11ComputeShader*        g_pReduceToSingleCS = NULL;
ID3D11ComputeShader*        g_pBrightPassAndHorizFilterCS = NULL;
ID3D11ComputeShader*        g_pVertFilterCS = NULL;
ID3D11ComputeShader*        g_pHorizFilterCS = NULL;
ID3D11PixelShader*          g_pDumpBufferPS = NULL;

// Blooming effect intermediate buffers used in CS path
ID3D11Buffer*               g_apBufBloom11[NUM_BLOOM_TEXTURES];         
ID3D11ShaderResourceView*   g_apBufBloomRV11[NUM_BLOOM_TEXTURES];
ID3D11UnorderedAccessView*  g_apBufBloomUAV11[NUM_BLOOM_TEXTURES];

ID3D11Texture2D*            g_pTexRender11 = NULL;          // Render target texture for the skybox
ID3D11Texture2D*            g_pTexRenderMS11 = NULL;        // Render target texture for the skybox when multi sampling is on
ID3D11Texture2D*            g_pMSDS11 = NULL;
ID3D11Texture2D*            g_pTexBlurred11 = NULL;         // Intermediate texture used in full screen blur
ID3D11RenderTargetView*     g_pTexRenderRTV11 = NULL;       
ID3D11RenderTargetView*     g_pMSRTV11 = NULL;
ID3D11RenderTargetView*     g_pTexBlurredRTV11 = NULL;
ID3D11DepthStencilView*     g_pMSDSV11 = NULL;
ID3D11ShaderResourceView*   g_pTexRenderRV11 = NULL;        
ID3D11ShaderResourceView*   g_pTexBlurredRV11 = NULL;

// Stuff used for drawing the "full screen quad"
struct SCREEN_VERTEX
{
    D3DXVECTOR4 pos;
    D3DXVECTOR2 tex;
};
ID3D11Buffer*               g_pScreenQuadVB = NULL;
ID3D11InputLayout*          g_pQuadLayout = NULL;
ID3D11VertexShader*         g_pQuadVS = NULL;
ID3D11PixelShader*          g_pFinalPassPS = NULL;
ID3D11PixelShader*          g_pFinalPassForCPUReductionPS = NULL;

// Constant buffer layout for transferring data to the CS
struct CB_CS
{
    UINT param[4];
};
UINT                        g_iCBCSBind = 0;

// Constant buffer layout for transferring data to the PS
struct CB_PS
{
    float param[4];
};
UINT                        g_iCBPSBind = 0;

// Constant buffer layout for transferring data to the PS for bloom effect
struct CB_Bloom_PS
{
    D3DXVECTOR4 avSampleOffsets[15];
    D3DXVECTOR4 avSampleWeights[15];
};
UINT                        g_iCBBloomPSBind = 0;

// Constant buffer layout for transferring data to the CS for horizontal and vertical convolution
struct CB_filter
{
    D3DXVECTOR4  avSampleWeights[15];
    union
    {
        struct
        {
            int outputsize[2];
        } o;        
        struct 
        {
            UINT    outputwidth;
            float   finverse;
        } uf;
    };    
    int     inputsize[2];
};

ID3D11Buffer*               g_pcbCS = NULL;                 // Constant buffer for passing parameters into the CS
ID3D11Buffer*               g_pcbBloom = NULL;              // Constant buffer for passing parameters into the PS for bloom effect
ID3D11Buffer*               g_pcbFilterCS = NULL;           // Constant buffer for passing parameters into the CS for horizontal and vertical convolution

ID3D11Buffer*               g_pBufferReduction0 = NULL;     // Two StructuredBuffer used for ping-ponging in the CS reduction operation
ID3D11Buffer*               g_pBufferReduction1 = NULL;
ID3D11Buffer*               g_pBufferBlur0 = NULL;          // Two buffer used in full screen blur in CS path
ID3D11Buffer*               g_pBufferBlur1 = NULL;
ID3D11Buffer*               g_pBufferCPURead = NULL;        // Buffer for reduction on CPU

ID3D11UnorderedAccessView*  g_pReductionUAView0 = NULL;     // UAV of the corresponding buffer object defined above
ID3D11UnorderedAccessView*  g_pReductionUAView1 = NULL;
ID3D11UnorderedAccessView*  g_pBlurUAView0 = NULL;
ID3D11UnorderedAccessView*  g_pBlurUAView1 = NULL;

ID3D11ShaderResourceView*   g_pReductionRV0 = NULL;         // SRV of the corresponding buffer object defined above
ID3D11ShaderResourceView*   g_pReductionRV1 = NULL;
ID3D11ShaderResourceView*   g_pBlurRV0 = NULL;
ID3D11ShaderResourceView*   g_pBlurRV1 = NULL;

bool                        g_bBloom = false;               // Bloom effect on/off
bool                        g_bFullScrBlur = false;         // Full screen blur on/off
bool                        g_bPostProcessON = true;        // All post-processing effect on/off
bool                        g_bCPUReduction = false;        // CPU reduction on/off

float                       g_fCPUReduceResult = 0;         // CPU reduction result

CDXUTStatic*                g_pStaticTech = NULL;           // Sample specific UI
CDXUTComboBox*              g_pComboBoxTech = NULL;
CDXUTCheckBox*              g_pCheckBloom = NULL;
CDXUTCheckBox*              g_pCheckScrBlur = NULL;

ID3D11Texture2D*            g_apTexToneMap11[NUM_TONEMAP_TEXTURES];     // Tone mapping calculation textures used in PS path
ID3D11ShaderResourceView*   g_apTexToneMapRV11[NUM_TONEMAP_TEXTURES];
ID3D11RenderTargetView*     g_apTexToneMapRTV11[NUM_TONEMAP_TEXTURES];
ID3D11Texture2D*            g_pTexBrightPass11 = NULL;                  // Bright pass filter texture used in PS path
ID3D11ShaderResourceView*   g_pTexBrightPassRV11 = NULL;
ID3D11RenderTargetView*     g_pTexBrightPassRTV11 = NULL;
ID3D11Texture2D*            g_apTexBloom11[NUM_BLOOM_TEXTURES];         // Blooming effect intermediate textures used in PS path
ID3D11ShaderResourceView*   g_apTexBloomRV11[NUM_BLOOM_TEXTURES];
ID3D11RenderTargetView*     g_apTexBloomRTV11[NUM_BLOOM_TEXTURES];
ID3D11PixelShader*          g_pDownScale2x2LumPS = NULL;                // Shaders in PS path
ID3D11PixelShader*          g_pDownScale3x3PS = NULL;
ID3D11PixelShader*          g_pOldFinalPassPS = NULL;
ID3D11PixelShader*          g_pDownScale3x3BrightPassPS = NULL;
ID3D11PixelShader*          g_pBloomPS = NULL;

ID3D11SamplerState*         g_pSampleStatePoint = NULL;
ID3D11SamplerState*         g_pSampleStateLinear = NULL;


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4
#define IDC_POSTPROCESS_MODE    5
#define IDC_BLOOM               6
#define IDC_POSTPROCESSON       7
#define IDC_SCREENBLUR          8

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

    // Disable gamma correction on this sample
    DXUTSetIsInGammaCorrectMode( false );

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

    DXUTSetCursorSettings( true, true );    // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"HDRToneMappingCS11" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 640, 480 );
    DXUTMainLoop();                         // Enter into the DXUT render loop

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

    g_HUD.SetCallback( OnGUIEvent ); int iY = 30;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 23 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 23, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 23, VK_F2 );

    g_SampleUI.AddCheckBox( IDC_POSTPROCESSON, L"(P)ost process on:", -20, 150-50, 140, 18, g_bPostProcessON, 'P' );

    g_SampleUI.AddStatic( 0, L"Post processing (t)ech", 0, 150-20, 105, 25, false, &g_pStaticTech );
    g_SampleUI.AddComboBox( IDC_POSTPROCESS_MODE, 0, 150, 150, 24, 'T', false, &g_pComboBoxTech );
    g_pComboBoxTech->AddItem( L"Compute Shader", IntToPtr(PM_COMPUTE_SHADER) );
    g_pComboBoxTech->AddItem( L"Pixel Shader", IntToPtr(PM_PIXEL_SHADER) );

    g_SampleUI.AddCheckBox( IDC_BLOOM, L"Show (B)loom", 0, 195, 140, 18, g_bBloom, 'B', false, &g_pCheckBloom );
    g_SampleUI.AddCheckBox( IDC_SCREENBLUR, L"Full (S)creen Blur", 0, 195+20, 140, 18, g_bFullScrBlur, 'S', false, &g_pCheckScrBlur );

    g_SampleUI.SetCallback( OnGUIEvent ); 
}

//--------------------------------------------------------------------------------------
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// DXUT will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise CreateDevice() will fail.  
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    assert( pDeviceSettings->ver == DXUT_D3D11_DEVICE );
    
    // Add UAC flag to back buffer Texture2D resource, so we can create an UAV on the back buffer of the swap chain,
    // then it can be bound as the output resource of the CS
    // However, as CS4.0 cannot output to textures, this is taken out when the sample has been ported to CS4.0
    //pDeviceSettings->d3d11.sd.BufferUsage |= DXGI_USAGE_UNORDERED_ACCESS;

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

void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.0f, 1.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    g_pTxtHelper->End();
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

        case IDC_BLOOM:
            g_bBloom = !g_bBloom; break;
        case IDC_POSTPROCESSON:
            g_bPostProcessON = !g_bPostProcessON; 
            g_pStaticTech->SetEnabled( g_bPostProcessON );
            g_pComboBoxTech->SetEnabled( g_bPostProcessON );
            g_pCheckBloom->SetEnabled( g_bPostProcessON );
            g_pCheckScrBlur->SetEnabled( g_bPostProcessON );
            break;
        case IDC_SCREENBLUR:
            g_bFullScrBlur = !g_bFullScrBlur;
            break;

        case IDC_POSTPROCESS_MODE:
        {
            CDXUTComboBox* pComboBox = ( CDXUTComboBox* )pControl;
            g_ePostProcessMode = ( POSTPROCESS_MODE )( int )PtrToInt( pComboBox->GetSelectedData() );

            break;
        }        
    }

}

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
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

    bFirstOnCreateDevice = false;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    WCHAR strPath[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( strPath, MAX_PATH, L"Light Probes\\uffizi_cross.dds" ) );

    ID3D11Texture2D* pCubeTexture = NULL;
    ID3D11ShaderResourceView* pCubeRV = NULL;
    UINT SupportCaps = 0;

    pd3dDevice->CheckFormatSupport( DXGI_FORMAT_R32G32B32A32_FLOAT, &SupportCaps );
    if( SupportCaps & D3D11_FORMAT_SUPPORT_TEXTURECUBE &&
        SupportCaps & D3D11_FORMAT_SUPPORT_RENDER_TARGET &&
        SupportCaps & D3D11_FORMAT_SUPPORT_TEXTURE2D )
    {
        D3DX11_IMAGE_LOAD_INFO LoadInfo;
        LoadInfo.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        V_RETURN( D3DX11CreateShaderResourceViewFromFile( pd3dDevice, strPath, &LoadInfo, NULL, &pCubeRV, NULL ) );
        DXUT_SetDebugName( pCubeRV, "uffizi_cross.dds" );
        pCubeRV->GetResource( ( ID3D11Resource** )&pCubeTexture );
        DXUT_SetDebugName( pCubeTexture, "uffizi_cross.dds" );
        V_RETURN( g_Skybox.OnD3D11CreateDevice( pd3dDevice, 50, pCubeTexture, pCubeRV ) );
    }
    else
        return E_FAIL;
    
    ID3DBlob* pBlob = NULL;

    // Create the shaders
    V_RETURN( CompileShaderFromFile( L"ReduceTo1DCS.hlsl", "CSMain", "cs_4_0", &pBlob ) );   
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pReduceTo1DCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pReduceTo1DCS, "CSMain" );

    V_RETURN( CompileShaderFromFile( L"ReduceToSingleCS.hlsl", "CSMain", "cs_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pReduceToSingleCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pReduceToSingleCS, "CSMain" );
    
    V_RETURN( CompileShaderFromFile( L"FinalPass.hlsl", "PSFinalPass", "ps_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pFinalPassPS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pFinalPassPS, "PSFinalPass" );

    V_RETURN( CompileShaderFromFile( L"FinalPass.hlsl", "PSFinalPassForCPUReduction", "ps_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pFinalPassForCPUReductionPS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pFinalPassForCPUReductionPS, "PSFinalPassForCPUReduction" );

    V_RETURN( CompileShaderFromFile( L"PSApproach.hlsl", "DownScale2x2_Lum", "ps_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDownScale2x2LumPS ) );    
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pDownScale2x2LumPS, "DownScale2x2_Lum" );

    V_RETURN( CompileShaderFromFile( L"PSApproach.hlsl", "DownScale3x3", "ps_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDownScale3x3PS ) );  
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pDownScale3x3PS, "DownScale3x3" );

    V_RETURN( CompileShaderFromFile( L"PSApproach.hlsl", "FinalPass", "ps_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pOldFinalPassPS ) );  
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pOldFinalPassPS, "FinalPass" );

    V_RETURN( CompileShaderFromFile( L"PSApproach.hlsl", "DownScale3x3_BrightPass", "ps_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDownScale3x3BrightPassPS ) );  
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pDownScale3x3BrightPassPS, "DownScale3x3_BrightPass" );

    V_RETURN( CompileShaderFromFile( L"PSApproach.hlsl", "Bloom", "ps_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pBloomPS ) );  
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pBloomPS, "Bloom" );

    V_RETURN( CompileShaderFromFile( L"BrightPassAndHorizFilterCS.hlsl", "CSMain", "cs_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pBrightPassAndHorizFilterCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pBrightPassAndHorizFilterCS, "CSMain" );

    V_RETURN( CompileShaderFromFile( L"FilterCS.hlsl", "CSVerticalFilter", "cs_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pVertFilterCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pVertFilterCS, "CSVerticalFilter" );

    V_RETURN( CompileShaderFromFile( L"FilterCS.hlsl", "CSHorizFilter", "cs_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreateComputeShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pHorizFilterCS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pHorizFilterCS, "CSHorizFilter" );

    V_RETURN( CompileShaderFromFile( L"DumpToTexture.hlsl", "PSDump", "ps_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreatePixelShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pDumpBufferPS ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pDumpBufferPS, "PSDump" );

    V_RETURN( CompileShaderFromFile( L"FinalPass.hlsl", "QuadVS", "vs_4_0", &pBlob ) );
    V_RETURN( pd3dDevice->CreateVertexShader( pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pQuadVS ) );
    DXUT_SetDebugName( g_pQuadVS, "QuadVS" );

    const D3D11_INPUT_ELEMENT_DESC quadlayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    V_RETURN( pd3dDevice->CreateInputLayout( quadlayout, 2, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &g_pQuadLayout ) );
    SAFE_RELEASE( pBlob );
    DXUT_SetDebugName( g_pQuadLayout, "Quad" );

    // Setup constant buffers
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;    
    Desc.ByteWidth = sizeof( CB_CS );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbCS ) );
    DXUT_SetDebugName( g_pcbCS, "CB_CS" );

    Desc.ByteWidth = sizeof( CB_Bloom_PS );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbBloom ) );
    DXUT_SetDebugName( g_pcbBloom, "CB_Bloom_PS" );

    Desc.ByteWidth = sizeof( CB_filter );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbFilterCS ) );
    DXUT_SetDebugName( g_pcbFilterCS, "CB_filter" );

    // Samplers
    D3D11_SAMPLER_DESC SamplerDesc;
    ZeroMemory( &SamplerDesc, sizeof(SamplerDesc) );
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    V_RETURN( pd3dDevice->CreateSamplerState( &SamplerDesc, &g_pSampleStateLinear ) );
    DXUT_SetDebugName( g_pSampleStateLinear, "Linear" );

    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    V_RETURN( pd3dDevice->CreateSamplerState( &SamplerDesc, &g_pSampleStatePoint ) );
    DXUT_SetDebugName( g_pSampleStatePoint, "Point" );

    // Create a screen quad for render to texture operations
    SCREEN_VERTEX svQuad[4];
    svQuad[0].pos = D3DXVECTOR4( -1.0f, 1.0f, 0.5f, 1.0f );
    svQuad[0].tex = D3DXVECTOR2( 0.0f, 0.0f );
    svQuad[1].pos = D3DXVECTOR4( 1.0f, 1.0f, 0.5f, 1.0f );
    svQuad[1].tex = D3DXVECTOR2( 1.0f, 0.0f );
    svQuad[2].pos = D3DXVECTOR4( -1.0f, -1.0f, 0.5f, 1.0f );
    svQuad[2].tex = D3DXVECTOR2( 0.0f, 1.0f );
    svQuad[3].pos = D3DXVECTOR4( 1.0f, -1.0f, 0.5f, 1.0f );
    svQuad[3].tex = D3DXVECTOR2( 1.0f, 1.0f );

    D3D11_BUFFER_DESC vbdesc =
    {
        4 * sizeof( SCREEN_VERTEX ),
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_VERTEX_BUFFER,
        0,
        0
    };
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = svQuad;
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;
    V_RETURN( pd3dDevice->CreateBuffer( &vbdesc, &InitData, &g_pScreenQuadVB ) );
    DXUT_SetDebugName( g_pScreenQuadVB, "ScreenQuad" );

    // Setup the camera   
    //D3DXVECTOR3 vecEye( 0.0f, 0.5f, -3.0f );
    D3DXVECTOR3 vecEye( 0.0f, -10.5f, -3.0f );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, 0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );

    return S_OK;
}

HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    g_Skybox.OnD3D11ResizedSwapChain( pBackBufferSurfaceDesc );

    // Create the render target texture
    // Our skybox will be rendered to this texture for later post-process
    D3D11_TEXTURE2D_DESC Desc;
    ZeroMemory( &Desc, sizeof( D3D11_TEXTURE2D_DESC ) );
    Desc.ArraySize = 1;
    Desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    Desc.Width = pBackBufferSurfaceDesc->Width;
    Desc.Height = pBackBufferSurfaceDesc->Height;
    Desc.MipLevels = 1;
    Desc.SampleDesc.Count = 1;
    V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pTexRender11 ) );
    V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pTexBlurred11 ) );    

    DXUT_SetDebugName( g_pTexRender11, "Render" );
    DXUT_SetDebugName( g_pTexBlurred11, "Blurred" );

    // Create the render target view
    D3D11_RENDER_TARGET_VIEW_DESC DescRT;
    DescRT.Format = Desc.Format;
    DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    DescRT.Texture2D.MipSlice = 0;
    V_RETURN( pd3dDevice->CreateRenderTargetView( g_pTexRender11, &DescRT, &g_pTexRenderRTV11 ) );
    V_RETURN( pd3dDevice->CreateRenderTargetView( g_pTexBlurred11, &DescRT, &g_pTexBlurredRTV11 ) );

    DXUT_SetDebugName( g_pTexRenderRTV11, "Render RTV" );
    DXUT_SetDebugName( g_pTexBlurredRTV11, "Blurred RTV" );

    if ( pBackBufferSurfaceDesc->SampleDesc.Count > 1 )
    {
        // If multi sampling is on, we create the multi sample floating render target
        D3D11_TEXTURE2D_DESC DescMS = Desc;
        DescMS.BindFlags = D3D11_BIND_RENDER_TARGET;
        DescMS.SampleDesc.Count = pBackBufferSurfaceDesc->SampleDesc.Count;
        DescMS.SampleDesc.Quality = pBackBufferSurfaceDesc->SampleDesc.Quality;
        V_RETURN( pd3dDevice->CreateTexture2D( &DescMS, NULL, &g_pTexRenderMS11 ) );
        DXUT_SetDebugName( g_pTexRenderMS11, "MSAA RT" );

        // Render target view for multi-sampling
        D3D11_RENDER_TARGET_VIEW_DESC DescMSRT = DescRT;
        DescMSRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        V_RETURN( pd3dDevice->CreateRenderTargetView( g_pTexRenderMS11, &DescMSRT, &g_pMSRTV11 ) );
        DXUT_SetDebugName( g_pMSRTV11, "MSAA SRV" );

        // Depth stencil texture for multi-sampling
        DescMS.Format = DXGI_FORMAT_D32_FLOAT;        
        DescMS.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        DescMS.CPUAccessFlags = 0;
        DescMS.MiscFlags = 0;
        V_RETURN( pd3dDevice->CreateTexture2D( &DescMS, NULL, &g_pMSDS11 ) );
        DXUT_SetDebugName( g_pMSDS11, "MSAA Depth RT" );

        // Depth stencil view for multi-sampling
        D3D11_DEPTH_STENCIL_VIEW_DESC DescDS;
        ZeroMemory( &DescDS, sizeof(DescDS) );
        DescDS.Format = DXGI_FORMAT_D32_FLOAT;
        DescDS.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        V_RETURN( pd3dDevice->CreateDepthStencilView( g_pMSDS11, &DescDS, &g_pMSDSV11 ) );
        DXUT_SetDebugName( g_pMSDSV11, "MSAA Depth DSV" );
    }

    // Create the resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
    DescRV.Format = Desc.Format;
    DescRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    DescRV.Texture2D.MipLevels = 1;
    DescRV.Texture2D.MostDetailedMip = 0;
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pTexRender11, &DescRV, &g_pTexRenderRV11 ) );
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pTexBlurred11, &DescRV, &g_pTexBlurredRV11 ) );

    DXUT_SetDebugName( g_pTexRenderRV11, "Render SRV" );
    DXUT_SetDebugName( g_pTexBlurredRV11, "Blurred SRV" );

    // Create the buffers used in full screen blur for CS path
    {
        D3D11_BUFFER_DESC DescBuffer;
        ZeroMemory( &DescBuffer, sizeof(DescBuffer) );
        DescBuffer.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        DescBuffer.ByteWidth = sizeof(D3DXVECTOR4) * pBackBufferSurfaceDesc->Width * pBackBufferSurfaceDesc->Height;
        DescBuffer.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        DescBuffer.StructureByteStride = sizeof(D3DXVECTOR4);
        DescBuffer.Usage = D3D11_USAGE_DEFAULT;
        V_RETURN( pd3dDevice->CreateBuffer( &DescBuffer, NULL, &g_pBufferBlur0 ) );
        V_RETURN( pd3dDevice->CreateBuffer( &DescBuffer, NULL, &g_pBufferBlur1 ) );

        DXUT_SetDebugName( g_pBufferBlur0, "Blur0" );
        DXUT_SetDebugName( g_pBufferBlur1, "Blur1" );

        D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
        ZeroMemory( &DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
        DescUAV.Format = DXGI_FORMAT_UNKNOWN;
        DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        DescUAV.Buffer.FirstElement = 0;
        DescUAV.Buffer.NumElements = DescBuffer.ByteWidth / DescBuffer.StructureByteStride;
        V_RETURN( pd3dDevice->CreateUnorderedAccessView( g_pBufferBlur0, &DescUAV, &g_pBlurUAView0 ) );
        V_RETURN( pd3dDevice->CreateUnorderedAccessView( g_pBufferBlur1, &DescUAV, &g_pBlurUAView1 ) );

        DXUT_SetDebugName( g_pBlurUAView0, "Blur0 UAV" );
        DXUT_SetDebugName( g_pBlurUAView1, "Blur1 UAV" );

        D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
        ZeroMemory( &DescRV, sizeof( DescRV ) );
        DescRV.Format = DXGI_FORMAT_UNKNOWN;
        DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        DescRV.Buffer.FirstElement = DescUAV.Buffer.FirstElement;
        DescRV.Buffer.NumElements = DescUAV.Buffer.NumElements;
        V_RETURN( pd3dDevice->CreateShaderResourceView( g_pBufferBlur0, &DescRV, &g_pBlurRV0 ) );
        V_RETURN( pd3dDevice->CreateShaderResourceView( g_pBufferBlur1, &DescRV, &g_pBlurRV1 ) );

        DXUT_SetDebugName( g_pBlurRV0, "Blur0 SRV" );
        DXUT_SetDebugName( g_pBlurRV1, "Blur1 SRV" );
    }

    // Create two buffers for ping-ponging in the reduction operation used for calculating luminance
    D3D11_BUFFER_DESC DescBuffer;
    ZeroMemory( &DescBuffer, sizeof(D3D11_BUFFER_DESC) );
    DescBuffer.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    DescBuffer.ByteWidth = int(ceil(pBackBufferSurfaceDesc->Width / 8.0f) * ceil(pBackBufferSurfaceDesc->Height / 8.0f)) * sizeof(float);
    DescBuffer.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    DescBuffer.StructureByteStride = sizeof(float);
    DescBuffer.Usage = D3D11_USAGE_DEFAULT;
    V_RETURN( pd3dDevice->CreateBuffer( &DescBuffer, NULL, &g_pBufferReduction0 ) );
    V_RETURN( pd3dDevice->CreateBuffer( &DescBuffer, NULL, &g_pBufferReduction1 ) );

    DXUT_SetDebugName( g_pBufferReduction0, "Reduction0" );
    DXUT_SetDebugName( g_pBufferReduction1, "Reduction1" );

    // This Buffer is for reduction on CPU
    DescBuffer.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    DescBuffer.Usage = D3D11_USAGE_STAGING;
    DescBuffer.BindFlags = 0;
    V_RETURN( pd3dDevice->CreateBuffer( &DescBuffer, NULL, &g_pBufferCPURead ) );
    DXUT_SetDebugName( g_pBufferCPURead, "CPU Read" );
    
    // Create UAV on the above two buffers object
    D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
    ZeroMemory( &DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
    DescUAV.Format = DXGI_FORMAT_UNKNOWN;
    DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    DescUAV.Buffer.FirstElement = 0;
    DescUAV.Buffer.NumElements = DescBuffer.ByteWidth / sizeof(float);
    V_RETURN( pd3dDevice->CreateUnorderedAccessView( g_pBufferReduction0, &DescUAV, &g_pReductionUAView0 ) );
    V_RETURN( pd3dDevice->CreateUnorderedAccessView( g_pBufferReduction1, &DescUAV, &g_pReductionUAView1 ) );

    DXUT_SetDebugName( g_pReductionUAView0, "Reduction0 UAV" );
    DXUT_SetDebugName( g_pReductionUAView1, "Reduction1 UAV" );

    // Create resource view for the two buffers object
    ZeroMemory( &DescRV, sizeof( DescRV ) );
    DescRV.Format = DXGI_FORMAT_UNKNOWN;
    DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    DescRV.Buffer.FirstElement = DescUAV.Buffer.FirstElement;
    DescRV.Buffer.NumElements = DescUAV.Buffer.NumElements;
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pBufferReduction0, &DescRV, &g_pReductionRV0 ) );
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pBufferReduction1, &DescRV, &g_pReductionRV1 ) );

    DXUT_SetDebugName( g_pReductionRV0, "Reduction0 SRV" );
    DXUT_SetDebugName( g_pReductionRV1, "Reduction1 SRV" );

    // Textures for tone mapping for the PS path
    int nSampleLen = 1;
    for( int i = 0; i < NUM_TONEMAP_TEXTURES; i++ )
    {
        D3D11_TEXTURE2D_DESC tmdesc;
        ZeroMemory( &tmdesc, sizeof( D3D11_TEXTURE2D_DESC ) );
        tmdesc.ArraySize = 1;
        tmdesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        tmdesc.Usage = D3D11_USAGE_DEFAULT;
        tmdesc.Format = DXGI_FORMAT_R32_FLOAT;
        tmdesc.Width = nSampleLen;
        tmdesc.Height = nSampleLen;
        tmdesc.MipLevels = 1;
        tmdesc.SampleDesc.Count = 1;
        V_RETURN( pd3dDevice->CreateTexture2D( &tmdesc, NULL, &g_apTexToneMap11[i] ) );
        DXUT_SetDebugName( g_apTexToneMap11[i], "ToneMap" );

        // Create the render target view
        D3D11_RENDER_TARGET_VIEW_DESC DescRT;
        DescRT.Format = tmdesc.Format;
        DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        DescRT.Texture2D.MipSlice = 0;
        V_RETURN( pd3dDevice->CreateRenderTargetView( g_apTexToneMap11[i], &DescRT, &g_apTexToneMapRTV11[i] ) );
        DXUT_SetDebugName( g_apTexToneMapRTV11[i], "ToneMap RTV" );

        // Create the shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
        DescRV.Format = tmdesc.Format;
        DescRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        DescRV.Texture2D.MipLevels = 1;
        DescRV.Texture2D.MostDetailedMip = 0;
        V_RETURN( pd3dDevice->CreateShaderResourceView( g_apTexToneMap11[i], &DescRV, &g_apTexToneMapRV11[i] ) );
        DXUT_SetDebugName( g_apTexToneMapRV11[i], "ToneMap SRV" );

        nSampleLen *= 3;
    }

    // Create the temporary blooming effect textures for PS path and buffers for CS path
    for( int i = 0; i < NUM_BLOOM_TEXTURES; i++ )
    {
        // Texture for blooming effect in PS path
        D3D11_TEXTURE2D_DESC bmdesc;
        ZeroMemory( &bmdesc, sizeof( D3D11_TEXTURE2D_DESC ) );
        bmdesc.ArraySize = 1;
        bmdesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        bmdesc.Usage = D3D11_USAGE_DEFAULT;
        bmdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        bmdesc.Width = pBackBufferSurfaceDesc->Width / 8;
        bmdesc.Height = pBackBufferSurfaceDesc->Height / 8;
        bmdesc.MipLevels = 1;
        bmdesc.SampleDesc.Count = 1;
        V_RETURN( pd3dDevice->CreateTexture2D( &bmdesc, NULL, &g_apTexBloom11[i] ) );
        DXUT_SetDebugName( g_apTexBloom11[i], "PSBloom" );

        // Create the render target view
        D3D11_RENDER_TARGET_VIEW_DESC DescRT;
        DescRT.Format = bmdesc.Format;
        DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        DescRT.Texture2D.MipSlice = 0;
        V_RETURN( pd3dDevice->CreateRenderTargetView( g_apTexBloom11[i], &DescRT, &g_apTexBloomRTV11[i] ) );
        DXUT_SetDebugName( g_apTexBloomRTV11[i], "PSBloom RTV" );

        // Create the shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
        DescRV.Format = bmdesc.Format;
        DescRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        DescRV.Texture2D.MipLevels = 1;
        DescRV.Texture2D.MostDetailedMip = 0;
        V_RETURN( pd3dDevice->CreateShaderResourceView( g_apTexBloom11[i], &DescRV, &g_apTexBloomRV11[i] ) );
        DXUT_SetDebugName( g_apTexBloomRV11[i], "PSBloom SRV" );

        // Buffers for blooming effect in CS path
        D3D11_BUFFER_DESC bufdesc;
        ZeroMemory( &bufdesc, sizeof(bufdesc) );
        bufdesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        bufdesc.ByteWidth = pBackBufferSurfaceDesc->Width / 8 * pBackBufferSurfaceDesc->Height / 8 * sizeof(D3DXVECTOR4);
        bufdesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bufdesc.StructureByteStride = sizeof(D3DXVECTOR4);
        bufdesc.Usage = D3D11_USAGE_DEFAULT;
        V_RETURN( pd3dDevice->CreateBuffer( &bufdesc, NULL, &g_apBufBloom11[i] ) );
        DXUT_SetDebugName( g_apBufBloom11[i], "CSBloom" );

        ZeroMemory( &DescRV, sizeof(DescRV) );
        DescRV.Format = DXGI_FORMAT_UNKNOWN;
        DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        DescRV.Buffer.FirstElement = 0;
        DescRV.Buffer.NumElements = bufdesc.ByteWidth / bufdesc.StructureByteStride;
        V_RETURN( pd3dDevice->CreateShaderResourceView( g_apBufBloom11[i], &DescRV, &g_apBufBloomRV11[i] ) );
        DXUT_SetDebugName( g_apBufBloomRV11[i], "CSBloom RTV" );

        D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
        ZeroMemory( &DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
        DescUAV.Format = DXGI_FORMAT_UNKNOWN;
        DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        DescUAV.Buffer.FirstElement = 0;
        DescUAV.Buffer.NumElements = DescRV.Buffer.NumElements;
        V_RETURN( pd3dDevice->CreateUnorderedAccessView( g_apBufBloom11[i], &DescUAV, &g_apBufBloomUAV11[i] ) );
        DXUT_SetDebugName( g_apBufBloomUAV11[i], "CSBloom UAV" );
    }

    // Create the bright pass texture for PS path
    Desc.Width /= 8;
    Desc.Height /= 8;
    Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    V_RETURN( pd3dDevice->CreateTexture2D( &Desc, NULL, &g_pTexBrightPass11 ) );
    DXUT_SetDebugName( g_pTexBrightPass11, "BrightPass" );

    // Create the render target view
    DescRT.Format = Desc.Format;
    DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    DescRT.Texture2D.MipSlice = 0;
    V_RETURN( pd3dDevice->CreateRenderTargetView( g_pTexBrightPass11, &DescRT, &g_pTexBrightPassRTV11 ) );
    DXUT_SetDebugName( g_pTexBrightPassRTV11, "BrightPass RTV" );

    // Create the resource view
    DescRV.Format = Desc.Format;
    DescRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    DescRV.Texture2D.MipLevels = 1;
    DescRV.Texture2D.MostDetailedMip = 0;
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pTexBrightPass11, &DescRV, &g_pTexBrightPassRV11 ) );
    DXUT_SetDebugName( g_pTexBrightPassRV11, "BrightPass SRV" );

    // Setup the camera's projection parameters    
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 0.1f, 5000.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 240 );
    g_SampleUI.SetSize( 150, 110 );

    return S_OK;
}

template <class T>
void SWAP( T* &x, T* &y )
{
    T* temp = x;
    x = y;
    y = temp;
}

void DrawFullScreenQuad11( ID3D11DeviceContext* pd3dImmediateContext, 
                           ID3D11PixelShader* pPS,
                           UINT Width, UINT Height )
{
    // Save the old viewport
    D3D11_VIEWPORT vpOld[D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX];
    UINT nViewPorts = 1;
    pd3dImmediateContext->RSGetViewports( &nViewPorts, vpOld );

    // Setup the viewport to match the backbuffer
    D3D11_VIEWPORT vp;
    vp.Width = (float)Width;
    vp.Height = (float)Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pd3dImmediateContext->RSSetViewports( 1, &vp );

    UINT strides = sizeof( SCREEN_VERTEX );
    UINT offsets = 0;
    ID3D11Buffer* pBuffers[1] = { g_pScreenQuadVB };

    pd3dImmediateContext->IASetInputLayout( g_pQuadLayout );
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, pBuffers, &strides, &offsets );
    pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

    pd3dImmediateContext->VSSetShader( g_pQuadVS, NULL, 0 );
    pd3dImmediateContext->PSSetShader( pPS, NULL, 0 );
    pd3dImmediateContext->Draw( 4, 0 );

    // Restore the Old viewport
    pd3dImmediateContext->RSSetViewports( nViewPorts, vpOld );
}

//--------------------------------------------------------------------------------------
// Measure the average luminance of the rendered skybox in PS path
//--------------------------------------------------------------------------------------
HRESULT MeasureLuminancePS11( ID3D11DeviceContext* pd3dImmediateContext )
{
    ID3D11ShaderResourceView* pTexSrc = NULL;
    ID3D11ShaderResourceView* pTexDest = NULL;
    ID3D11RenderTargetView* pSurfDest = NULL;

    //-------------------------------------------------------------------------
    // Initial sampling pass to convert the image to the log of the grayscale
    //-------------------------------------------------------------------------
    pTexSrc = g_pTexRenderRV11;
    pTexDest = g_apTexToneMapRV11[NUM_TONEMAP_TEXTURES - 1];
    pSurfDest = g_apTexToneMapRTV11[NUM_TONEMAP_TEXTURES - 1];

    D3D11_TEXTURE2D_DESC descSrc;
    g_pTexRender11->GetDesc( &descSrc );
    D3D11_TEXTURE2D_DESC descDest;
    g_apTexToneMap11[NUM_TONEMAP_TEXTURES - 1]->GetDesc( &descDest );

    ID3D11RenderTargetView* aRTViews[ 1 ] = { pSurfDest };
    pd3dImmediateContext->OMSetRenderTargets( 1, aRTViews, NULL );             
    ID3D11ShaderResourceView* aRViews[ 1 ] = { pTexSrc };
    pd3dImmediateContext->PSSetShaderResources( 0, 1, aRViews );

    ID3D11SamplerState* aSamplers[] = { g_pSampleStatePoint };
    pd3dImmediateContext->PSSetSamplers( 0, 1, aSamplers );

    DrawFullScreenQuad11( pd3dImmediateContext, g_pDownScale2x2LumPS, descDest.Width, descDest.Height );

    ID3D11ShaderResourceView* ppSRVNULL[1] = { NULL };
    pd3dImmediateContext->PSSetShaderResources( 0, 1, ppSRVNULL );

    //-------------------------------------------------------------------------
    // Iterate through the remaining tone map textures
    //------------------------------------------------------------------------- 
    for( int i = NUM_TONEMAP_TEXTURES - 1; i > 0; i-- )
    {
        // Cycle the textures
        pTexSrc = g_apTexToneMapRV11[i];
        pTexDest = g_apTexToneMapRV11[i - 1];
        pSurfDest = g_apTexToneMapRTV11[i - 1];

        D3D11_TEXTURE2D_DESC desc;
        g_apTexToneMap11[i]->GetDesc( &desc );

        ID3D11RenderTargetView* aRTViews[ 1 ] = { pSurfDest };
        pd3dImmediateContext->OMSetRenderTargets( 1, aRTViews, NULL );

        ID3D11ShaderResourceView* aRViews[ 1 ] = { pTexSrc };
        pd3dImmediateContext->PSSetShaderResources( 0, 1, aRViews );

        DrawFullScreenQuad11( pd3dImmediateContext, g_pDownScale3x3PS, desc.Width / 3, desc.Height / 3 );

        ID3D11ShaderResourceView* ppSRVNULL[1] = { NULL };
        pd3dImmediateContext->PSSetShaderResources( 0, 1, ppSRVNULL );
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Bright pass for bloom effect in PS path
//--------------------------------------------------------------------------------------
HRESULT BrightPassFilterPS11( ID3D11DeviceContext* pd3dImmediateContext, const DXGI_SURFACE_DESC* pBackBufferDesc )
{
    ID3D11RenderTargetView* aRTViews[ 1 ] = { g_pTexBrightPassRTV11 };
    pd3dImmediateContext->OMSetRenderTargets( 1, aRTViews, NULL );

    ID3D11ShaderResourceView* aRViews[ 2 ] = { g_pTexRenderRV11, g_apTexToneMapRV11[0] };
    pd3dImmediateContext->PSSetShaderResources( 0, 2, aRViews );  

    ID3D11SamplerState* aSamplers[] = { g_pSampleStatePoint };
    pd3dImmediateContext->PSSetSamplers( 0, 1, aSamplers );

    DrawFullScreenQuad11( pd3dImmediateContext, g_pDownScale3x3BrightPassPS, pBackBufferDesc->Width / 8, pBackBufferDesc->Height / 8 );

    ID3D11ShaderResourceView* ppSRVNULL[2] = { NULL, NULL };
    pd3dImmediateContext->PSSetShaderResources( 0, 2, ppSRVNULL );

    // Debug texture out
    //D3DX11SaveTextureToFile( pd3dImmediateContext, g_pTexBrightPass11, D3DX11_IFF_BMP, L"d:\\g_pTexBrightPass11.bmp" );

    return S_OK;
}

float GaussianDistribution( float x, float y, float rho )
{
    float g = 1.0f / sqrtf( 2.0f * D3DX_PI * rho * rho );
    g *= expf( -( x * x + y * y ) / ( 2 * rho * rho ) );

    return g;
}

HRESULT GetSampleOffsets_Bloom_D3D11( DWORD dwD3DTexSize,
                                     float afTexCoordOffset[15],
                                     D3DXVECTOR4* avColorWeight,
                                     float fDeviation, float fMultiplier )
{
    int i = 0;
    float tu = 1.0f / ( float )dwD3DTexSize;

    // Fill the center texel
    float weight = 1.0f * GaussianDistribution( 0, 0, fDeviation );
    avColorWeight[7] = D3DXVECTOR4( weight, weight, weight, 1.0f );

    afTexCoordOffset[7] = 0.0f;

    // Fill one side
    for( i = 1; i < 8; i++ )
    {
        weight = fMultiplier * GaussianDistribution( ( float )i, 0, fDeviation );
        afTexCoordOffset[7-i] = -i * tu;

        avColorWeight[7-i] = D3DXVECTOR4( weight, weight, weight, 1.0f );
    }

    // Copy to the other side
    for( i = 8; i < 15; i++ )
    {
        avColorWeight[i] = avColorWeight[14 - i];
        afTexCoordOffset[i] = -afTexCoordOffset[14 - i];
    }

    // Debug convolution kernel which doesn't transform input data
    /*ZeroMemory( avColorWeight, sizeof(D3DXVECTOR4)*15 );
    avColorWeight[7] = D3DXVECTOR4( 1, 1, 1, 1 );*/

    return S_OK;
}

HRESULT GetSampleWeights_D3D11( D3DXVECTOR4* avColorWeight,
                                float fDeviation, float fMultiplier )
{
    // Fill the center texel
    float weight = 1.0f * GaussianDistribution( 0, 0, fDeviation );
    avColorWeight[7] = D3DXVECTOR4( weight, weight, weight, 1.0f );

    // Fill the right side
    for( int i = 1; i < 8; i++ )
    {
        weight = fMultiplier * GaussianDistribution( ( float )i, 0, fDeviation );
        avColorWeight[7-i] = D3DXVECTOR4( weight, weight, weight, 1.0f );
    }

    // Copy to the left side
    for( int i = 8; i < 15; i++ )
    {
        avColorWeight[i] = avColorWeight[14 - i];
    }

    // Debug convolution kernel which doesn't transform input data
    /*ZeroMemory( avColorWeight, sizeof(D3DXVECTOR4)*15 );
    avColorWeight[7] = D3DXVECTOR4( 1, 1, 1, 1 );*/

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Blur using a separable convolution kernel in PS path
//--------------------------------------------------------------------------------------
HRESULT BlurPS11( ID3D11DeviceContext* pd3dImmediateContext, DWORD dwWidth, DWORD dwHeight,
                  ID3D11ShaderResourceView* pFromRV,
                  ID3D11ShaderResourceView* pAuxRV, ID3D11RenderTargetView* pAuxRTV,
                  ID3D11RenderTargetView* pToRTV )
{
    HRESULT hr = S_OK;
    int i = 0;

    // Horizontal Blur
    D3D11_MAPPED_SUBRESOURCE MappedResource;            
    V( pd3dImmediateContext->Map( g_pcbBloom, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
        CB_Bloom_PS* pcbBloom = ( CB_Bloom_PS* )MappedResource.pData;
        D3DXVECTOR4* avSampleOffsets = pcbBloom->avSampleOffsets;
        D3DXVECTOR4* avSampleWeights = pcbBloom->avSampleWeights;

        float afSampleOffsets[15];
        hr = GetSampleOffsets_Bloom_D3D11( dwWidth, afSampleOffsets, avSampleWeights, 3.0f, 1.25f );
        for( i = 0; i < 15; i++ )
        {
            avSampleOffsets[i] = D3DXVECTOR4( afSampleOffsets[i], 0.0f, 0.0f, 0.0f );
        }
    pd3dImmediateContext->Unmap( g_pcbBloom, 0 );

    ID3D11Buffer* ppCB[1] = { g_pcbBloom };
    pd3dImmediateContext->PSSetConstantBuffers( g_iCBBloomPSBind, 1, ppCB );

    ID3D11RenderTargetView* aRTViews[ 1 ] = { pAuxRTV };
    pd3dImmediateContext->OMSetRenderTargets( 1, aRTViews, NULL );

    ID3D11ShaderResourceView* pViews[1] = {pFromRV};
    pd3dImmediateContext->PSSetShaderResources( 0, 1, pViews );
    ID3D11SamplerState* aSamplers[] = { g_pSampleStatePoint };
    pd3dImmediateContext->PSSetSamplers( 0, 1, aSamplers );
    DrawFullScreenQuad11( pd3dImmediateContext, g_pBloomPS, dwWidth, dwHeight );
    ID3D11ShaderResourceView* pViewsNull[4] = {0,0,0,0};
    pd3dImmediateContext->PSSetShaderResources( 0, 4, pViewsNull );

    // Debug texture out
    //D3DX11SaveTextureToFile( pd3dImmediateContext, g_apTexBloom11[1], D3DX11_IFF_BMP, L"d:\\g_apTexBloom111.bmp" );

    // Vertical Blur
    V( pd3dImmediateContext->Map( g_pcbBloom, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
        pcbBloom = ( CB_Bloom_PS* )MappedResource.pData;
        avSampleOffsets = pcbBloom->avSampleOffsets;
        avSampleWeights = pcbBloom->avSampleWeights;
        hr = GetSampleOffsets_Bloom_D3D11( dwHeight, afSampleOffsets, avSampleWeights, 3.0f, 1.25f );
        for( i = 0; i < 15; i++ )
        {
            avSampleOffsets[i] = D3DXVECTOR4( 0.0f, afSampleOffsets[i], 0.0f, 0.0f );
        }
    pd3dImmediateContext->Unmap( g_pcbBloom, 0 );

    ppCB[0] = g_pcbBloom;
    pd3dImmediateContext->PSSetConstantBuffers( g_iCBBloomPSBind, 1, ppCB );

    aRTViews[ 0 ] = pToRTV;
    pd3dImmediateContext->OMSetRenderTargets( 1, aRTViews, NULL );

    pViews[0] = pAuxRV;
    pd3dImmediateContext->PSSetShaderResources( 0, 1, pViews );
    DrawFullScreenQuad11( pd3dImmediateContext, g_pBloomPS, dwWidth, dwHeight );
    pd3dImmediateContext->PSSetShaderResources( 0, 4, pViewsNull );

    ID3D11Buffer* ppCBNull[1] = { NULL };
    pd3dImmediateContext->PSSetConstantBuffers( g_iCBBloomPSBind, 1, ppCBNull );

    // Debug texture out
    //D3DX11SaveTextureToFile( pd3dImmediateContext, g_apTexBloom11[0], D3DX11_IFF_BMP, L"d:\\g_apTexBloom110.bmp" );

    return hr;
}

//--------------------------------------------------------------------------------------
// Bloom effect in PS path
//--------------------------------------------------------------------------------------
HRESULT RenderBloomPS11( ID3D11DeviceContext* pd3dImmediateContext, const DXGI_SURFACE_DESC* pBackBufferDesc )
{
    return BlurPS11( pd3dImmediateContext, pBackBufferDesc->Width/8, pBackBufferDesc->Height/8,
                     g_pTexBrightPassRV11,
                     g_apTexBloomRV11[1], g_apTexBloomRTV11[1],
                     g_apTexBloomRTV11[0] );
}

//--------------------------------------------------------------------------------------
// Full screen blur effect in PS path
//--------------------------------------------------------------------------------------
HRESULT FullScrBlurPS11( ID3D11DeviceContext* pd3dImmediateContext, const DXGI_SURFACE_DESC* pBackBufferDesc )
{
    return BlurPS11( pd3dImmediateContext, pBackBufferDesc->Width, pBackBufferDesc->Height,
                     g_pTexRenderRV11,
                     g_pTexBlurredRV11, g_pTexBlurredRTV11,
                     g_pTexRenderRTV11 );
}

//--------------------------------------------------------------------------------------
// Helper function which makes CS invocation more convenient
//--------------------------------------------------------------------------------------
void RunComputeShader( ID3D11DeviceContext* pd3dImmediateContext,
                       ID3D11ComputeShader* pComputeShader,
                       UINT nNumViews, ID3D11ShaderResourceView** pShaderResourceViews, 
                       ID3D11Buffer* pCBCS, void* pCSData, DWORD dwNumDataBytes,
                       ID3D11UnorderedAccessView* pUnorderedAccessView,
                       UINT X, UINT Y, UINT Z )
{
    HRESULT hr = S_OK;
    
    pd3dImmediateContext->CSSetShader( pComputeShader, NULL, 0 );
    pd3dImmediateContext->CSSetShaderResources( 0, nNumViews, pShaderResourceViews );
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, &pUnorderedAccessView, NULL );
    if ( pCBCS )
    {
        D3D11_MAPPED_SUBRESOURCE MappedResource;
        V( pd3dImmediateContext->Map( pCBCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
        memcpy( MappedResource.pData, pCSData, dwNumDataBytes );
        pd3dImmediateContext->Unmap( pCBCS, 0 );
        ID3D11Buffer* ppCB[1] = { pCBCS };
        pd3dImmediateContext->CSSetConstantBuffers( 0, 1, ppCB );
    }

    pd3dImmediateContext->Dispatch( X, Y, Z );

    ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
    pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, ppUAViewNULL, NULL );

    ID3D11ShaderResourceView* ppSRVNULL[3] = { NULL, NULL, NULL };
    pd3dImmediateContext->CSSetShaderResources( 0, 3, ppSRVNULL );
    ID3D11Buffer* ppBufferNULL[1] = { NULL };
    pd3dImmediateContext->CSSetConstantBuffers( 0, 1, ppBufferNULL );
}

//--------------------------------------------------------------------------------------
// Debug function which copies a GPU buffer to a CPU readable buffer
//--------------------------------------------------------------------------------------
ID3D11Buffer* CreateAndCopyToDebugBuf( ID3D11Device* pDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer* pBuffer )
{
    ID3D11Buffer* debugbuf = NULL;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    pBuffer->GetDesc( &desc );
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;
    pDevice->CreateBuffer(&desc, NULL, &debugbuf);
    DXUT_SetDebugName( debugbuf, "Debug" );

    pd3dImmediateContext->CopyResource( debugbuf, pBuffer );

    return debugbuf;
}

// Define this to do full pixel reduction.
// If this is on, the same flag must also be on in ReduceTo1DCS.hlsl.
//#define CS_FULL_PIXEL_REDUCITON 

//--------------------------------------------------------------------------------------
// Measure the average luminance of the rendered skybox in CS path
//--------------------------------------------------------------------------------------
HRESULT MeasureLuminanceCS11( ID3D11DeviceContext* pd3dImmediateContext, const DXGI_SURFACE_DESC* pBackBufferDesc )
{
    HRESULT hr;
    
#ifdef CS_FULL_PIXEL_REDUCITON
    int dimx = int(ceil(pBackBufferDesc->Width/8.0f));
    dimx = int(ceil(dimx/2.0f));
    int dimy = int(ceil(pBackBufferDesc->Height/8.0f));
    dimy = int(ceil(dimy/2.0f));
#else
    int dimx = int(ceil(ToneMappingTexSize/8.0f));
    int dimy = dimx;
#endif

    // First CS pass, reduce the render target texture into a 1D buffer
    {
        ID3D11ShaderResourceView* aRViews[ 1 ] = { g_pTexRenderRV11 };
        CB_CS cbCS = { dimx, dimy, pBackBufferDesc->Width, pBackBufferDesc->Height };
        RunComputeShader( pd3dImmediateContext, 
                          g_pReduceTo1DCS, 
                          1, aRViews,
                          g_pcbCS, &cbCS, sizeof(cbCS),
                          g_pReductionUAView0,
                          dimx, dimy, 1 ); 

        // Turn on this and set a breakpoint right after the call to Map to see what's written to g_pBufferReduction0
#if 0
        ID3D11Buffer* pDebugBuf = CreateAndCopyToDebugBuf( DXUTGetD3D11Device(), pd3dImmediateContext, g_pBufferReduction0 );
        D3D11_MAPPED_SUBRESOURCE MappedResource;     
        V( pd3dImmediateContext->Map( pDebugBuf, 0, D3D11_MAP_READ, 0, &MappedResource ) );
        pd3dImmediateContext->Unmap( pDebugBuf, 0 );
        SAFE_RELEASE( pDebugBuf );        
#endif
    }

    // Reduction CS passes, the reduction result will be in the first element of g_pTex1DReduction1
    {
        if ( !g_bCPUReduction )
        {
            int dim = dimx*dimy;
            int nNumToReduce = dim;
            dim = int( ceil(dim/128.0f) );
            if ( nNumToReduce > 1 )
            {
                for (;;)
                {
                    ID3D11ShaderResourceView* aRViews[ 1 ] = { g_pReductionRV0 };
                    CB_CS cbCS = { nNumToReduce, dim, 0, 0 };
                    RunComputeShader( pd3dImmediateContext,
                                      g_pReduceToSingleCS,
                                      1, aRViews,
                                      g_pcbCS, &cbCS, sizeof(cbCS),
                                      g_pReductionUAView1, 
                                      dim, 1, 1 );                               

                    nNumToReduce = dim;
                    dim = int( ceil(dim/128.0f) );

                    if ( nNumToReduce == 1 )
                        break;

                    SWAP( g_pBufferReduction0, g_pBufferReduction1 );
                    SWAP( g_pReductionUAView0, g_pReductionUAView1 );
                    SWAP( g_pReductionRV0, g_pReductionRV1 );
                }
            } else
            {
                SWAP( g_pBufferReduction0, g_pBufferReduction1 );
                SWAP( g_pReductionUAView0, g_pReductionUAView1 );
                SWAP( g_pReductionRV0, g_pReductionRV1 );
            }
        } else
        {
            // read back to CPU and reduce on the CPU
            D3D11_BOX box;
            box.left = 0;
            box.right = sizeof(float) * dimx * dimy;
            box.top = 0;
            box.bottom = 1;
            box.front = 0;
            box.back = 1;
            pd3dImmediateContext->CopySubresourceRegion( g_pBufferCPURead, 0, 0, 0, 0, g_pBufferReduction0, 0, &box );
            D3D11_MAPPED_SUBRESOURCE MappedResource; 
            V( pd3dImmediateContext->Map( g_pBufferCPURead, 0, D3D11_MAP_READ, 0, &MappedResource ) );       
            float *pData = (float*)MappedResource.pData;
            g_fCPUReduceResult = 0;
            for ( int i = 0; i < dimx * dimy; ++i )
            {
                g_fCPUReduceResult += pData[i];
            }
            pd3dImmediateContext->Unmap( g_pBufferCPURead, 0 );                       
        }        
    }    

    // Turn on this and set a breakpoint right after the call to Map to see what is in g_pBufferReduction1 and g_pBufferReduction0
#if 0
    ID3D11Buffer* pDebugBuf = CreateAndCopyToDebugBuf( DXUTGetD3D11Device(), pd3dImmediateContext, g_pBufferReduction1 );
    D3D11_MAPPED_SUBRESOURCE MappedResource;     
    V( pd3dImmediateContext->Map( pDebugBuf, 0, D3D11_MAP_READ, 0, &MappedResource ) );
    pd3dImmediateContext->Unmap( pDebugBuf, 0 );
    SAFE_RELEASE( pDebugBuf );

    pDebugBuf = CreateAndCopyToDebugBuf( DXUTGetD3D11Device(), pd3dImmediateContext, g_pBufferReduction0 );    
    V( pd3dImmediateContext->Map( pDebugBuf, 0, D3D11_MAP_READ, 0, &MappedResource ) );
    pd3dImmediateContext->Unmap( pDebugBuf, 0 );
    SAFE_RELEASE( pDebugBuf );
#endif

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Bloom effect in CS path
//--------------------------------------------------------------------------------------
HRESULT BloomCS11( ID3D11DeviceContext* pd3dImmediateContext, const DXGI_SURFACE_DESC* pBackBufferDesc )
{
    // Bright pass and horizontal blur
    ID3D11ShaderResourceView* aRViews[ 2 ] = { g_pTexRenderRV11, g_pReductionRV1 };
    CB_filter cbFilter;
    GetSampleWeights_D3D11( cbFilter.avSampleWeights, 3.0f, 1.25f );
    cbFilter.uf.outputwidth = pBackBufferDesc->Width / 8;    
#ifdef CS_FULL_PIXEL_REDUCITON
    cbFilter.uf.finverse = 1.0f / (pBackBufferDesc->Width*pBackBufferDesc->Height);
#else
    cbFilter.uf.finverse = 1.0f / (ToneMappingTexSize*ToneMappingTexSize);
#endif
    cbFilter.inputsize[0] = pBackBufferDesc->Width;
    cbFilter.inputsize[1] = pBackBufferDesc->Height;
    RunComputeShader( pd3dImmediateContext, 
                      g_pBrightPassAndHorizFilterCS,
                      2, aRViews,
                      g_pcbFilterCS, &cbFilter, sizeof(cbFilter),
                      g_apBufBloomUAV11[1],
                      int(ceil((float)cbFilter.uf.outputwidth / (128 - 7 * 2))), pBackBufferDesc->Height / 8, 1 );

    // Vertical blur
    aRViews[0] = g_apBufBloomRV11[1];
    cbFilter.o.outputsize[0] = pBackBufferDesc->Width / 8;
    cbFilter.o.outputsize[1] = pBackBufferDesc->Height / 8;    
    cbFilter.inputsize[0] = pBackBufferDesc->Width / 8;
    cbFilter.inputsize[1] = pBackBufferDesc->Height / 8;
    RunComputeShader( pd3dImmediateContext,
                      g_pVertFilterCS,
                      1, aRViews,
                      g_pcbFilterCS, &cbFilter, sizeof(cbFilter),
                      g_apBufBloomUAV11[0],
                      pBackBufferDesc->Width / 8, int(ceil((float)cbFilter.o.outputsize[1] / (128 - 7 * 2))), 1 );  

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Full screen blur effect in CS path
//--------------------------------------------------------------------------------------
HRESULT FullScrBlurCS11( ID3D11DeviceContext* pd3dImmediateContext, const DXGI_SURFACE_DESC* pBackBufferDesc )
{
    HRESULT hr = S_OK;
    
    ID3D11ShaderResourceView* aRViews[ 2 ] = { NULL, g_pTexRenderRV11 };
    CB_filter cbFilter;
    GetSampleWeights_D3D11( cbFilter.avSampleWeights, 3.0f, 1.25f );
    cbFilter.o.outputsize[0] = pBackBufferDesc->Width;
    cbFilter.o.outputsize[1] = pBackBufferDesc->Height;
    cbFilter.inputsize[0] = pBackBufferDesc->Width;
    cbFilter.inputsize[1] = pBackBufferDesc->Height;
    RunComputeShader( pd3dImmediateContext, 
                      g_pHorizFilterCS,
                      2, aRViews,
                      g_pcbFilterCS, &cbFilter, sizeof(cbFilter),
                      g_pBlurUAView0,
                      int(ceil((float)pBackBufferDesc->Width / (128 - 7 * 2))), pBackBufferDesc->Height, 1 );

    aRViews[ 0 ] = g_pBlurRV0;
    RunComputeShader( pd3dImmediateContext, 
                      g_pVertFilterCS,
                      1, aRViews,
                      g_pcbFilterCS, &cbFilter, sizeof(cbFilter),
                      g_pBlurUAView1,
                      pBackBufferDesc->Width, int(ceil((float)pBackBufferDesc->Height / (128 - 7 * 2))), 1 );

    return hr;
}

//--------------------------------------------------------------------------------------
// Convert buffer result output from CS to a texture, used in CS path
//--------------------------------------------------------------------------------------
HRESULT DumpToTexture( ID3D11DeviceContext* pd3dImmediateContext, DWORD dwWidth, DWORD dwHeight,
                       ID3D11ShaderResourceView* pFromRV, ID3D11RenderTargetView* pToRTV )
{
    HRESULT hr = S_OK;
    
    ID3D11ShaderResourceView* aRViews[ 1 ] = { pFromRV };
    pd3dImmediateContext->PSSetShaderResources( 0, 1, aRViews );

    ID3D11RenderTargetView* aRTViews[ 1 ] = { pToRTV };
    pd3dImmediateContext->OMSetRenderTargets( 1, aRTViews, NULL );          

    D3D11_MAPPED_SUBRESOURCE MappedResource;            
    V( pd3dImmediateContext->Map( g_pcbCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    UINT* p = (UINT*)MappedResource.pData;
    p[0] = dwWidth;
    p[1] = dwHeight;
    pd3dImmediateContext->Unmap( g_pcbCS, 0 );
    ID3D11Buffer* ppCB[1] = { g_pcbCS };
    pd3dImmediateContext->PSSetConstantBuffers( g_iCBPSBind, 1, ppCB );

    DrawFullScreenQuad11( pd3dImmediateContext, g_pDumpBufferPS, dwWidth, dwHeight );

    return hr;
}

void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext )
{
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

    const DXGI_SURFACE_DESC* pBackBufferDesc = DXUTGetDXGIBackBufferSurfaceDesc();

    // Store off original render target, this is the back buffer of the swap chain
    ID3D11RenderTargetView* pOrigRTV = NULL;
    ID3D11DepthStencilView* pOrigDSV = NULL;
    pd3dImmediateContext->OMGetRenderTargets( 1, &pOrigRTV, &pOrigDSV );

    float ClearColor[4] = { 0.3f, 0.3f, 0.3f, 1.0f } ; // red, green, blue, alpha

    // Set the render target to our own texture
    if ( g_bPostProcessON )
    {
        if ( pBackBufferDesc->SampleDesc.Count > 1 )
        {
            ID3D11RenderTargetView* aRTViews[ 1 ] = { g_pMSRTV11 };
            pd3dImmediateContext->OMSetRenderTargets( 1, aRTViews, g_pMSDSV11 );

            pd3dImmediateContext->ClearRenderTargetView( g_pMSRTV11, ClearColor );
            pd3dImmediateContext->ClearDepthStencilView( g_pMSDSV11, D3D11_CLEAR_DEPTH, 1.0, 0 );
        } 
        else
        {
            ID3D11RenderTargetView* aRTViews[ 1 ] = { g_pTexRenderRTV11 };
            pd3dImmediateContext->OMSetRenderTargets( 1, aRTViews, pOrigDSV );

            pd3dImmediateContext->ClearRenderTargetView( g_pTexRenderRTV11, ClearColor );            
        }                
    } 
    
    pd3dImmediateContext->ClearRenderTargetView( pOrigRTV, ClearColor );
    pd3dImmediateContext->ClearDepthStencilView( pOrigDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );      

    HRESULT hr;
    D3DXMATRIXA16 mWorld;
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;
    D3DXMATRIXA16 mWorldViewProjection;

    // Get the projection & view matrix from the camera class
    mWorld = *g_Camera.GetWorldMatrix();
    mView = *g_Camera.GetViewMatrix();
    mProj = *g_Camera.GetProjMatrix();
    
    mWorldViewProjection = mWorld * mView * mProj;

    g_Skybox.D3D11Render( &mWorldViewProjection, pd3dImmediateContext );

    if ( g_bPostProcessON && pBackBufferDesc->SampleDesc.Count > 1 )
    {
        D3D11_TEXTURE2D_DESC Desc;
        g_pTexRender11->GetDesc( &Desc );
        pd3dImmediateContext->ResolveSubresource( g_pTexRender11, D3D11CalcSubresource( 0, 0, 1 ), g_pTexRenderMS11,
                                                  D3D11CalcSubresource( 0, 0, 1 ), Desc.Format );
        ID3D11RenderTargetView* aRTViews[ 1 ] = { NULL };
        pd3dImmediateContext->OMSetRenderTargets( 1, aRTViews, pOrigDSV );
    }

    if ( g_bPostProcessON )
    {
        // g_pTexRender11 is bound as the render target, release it here,
        // as it will be used later as the input texture to the CS
        ID3D11RenderTargetView* ppRTVNULL[1] = { NULL };
        pd3dImmediateContext->OMSetRenderTargets( 1, ppRTVNULL, pOrigDSV );

        if ( g_ePostProcessMode == PM_COMPUTE_SHADER )
        {
            MeasureLuminanceCS11( pd3dImmediateContext, pBackBufferDesc );
            if ( g_bFullScrBlur )
                FullScrBlurCS11( pd3dImmediateContext, pBackBufferDesc );

            if ( g_bBloom )
            {
                BloomCS11( pd3dImmediateContext, pBackBufferDesc );
            }

            DumpToTexture( pd3dImmediateContext, pBackBufferDesc->Width/8, pBackBufferDesc->Height/8,
                           g_apBufBloomRV11[0], g_apTexBloomRTV11[0] );

            if ( g_bFullScrBlur )
                DumpToTexture( pd3dImmediateContext, pBackBufferDesc->Width, pBackBufferDesc->Height,
                               g_pBlurRV1, g_pTexRenderRTV11 );
        } else //if ( g_ePostProcessMode == PM_PIXEL_SHADER )
        {
            MeasureLuminancePS11( pd3dImmediateContext );           

            if ( g_bBloom )
            {
                BrightPassFilterPS11( pd3dImmediateContext, pBackBufferDesc );
                RenderBloomPS11( pd3dImmediateContext, pBackBufferDesc );
            }

            if ( g_bFullScrBlur )
                FullScrBlurPS11( pd3dImmediateContext, pBackBufferDesc );
        }

        // Restore original render targets
        ID3D11RenderTargetView* aRTViews[ 1 ] = { pOrigRTV };
        pd3dImmediateContext->OMSetRenderTargets( 1, aRTViews, pOrigDSV );        

        // Tone-mapping
        if ( g_ePostProcessMode == PM_COMPUTE_SHADER )
        {
            if ( !g_bCPUReduction )
            {
                ID3D11ShaderResourceView* aRViews[ 3 ] = { g_pTexRenderRV11, g_pReductionRV1, g_bBloom ? g_apTexBloomRV11[0] : NULL };
                pd3dImmediateContext->PSSetShaderResources( 0, 3, aRViews );

                D3D11_MAPPED_SUBRESOURCE MappedResource;            
                V( pd3dImmediateContext->Map( g_pcbCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
                CB_PS* pcbCS = ( CB_PS* )MappedResource.pData;
#ifdef CS_FULL_PIXEL_REDUCITON
                pcbCS->param[0] = 1.0f / (pBackBufferDesc->Width*pBackBufferDesc->Height);
#else
                pcbCS->param[0] = 1.0f / (ToneMappingTexSize*ToneMappingTexSize);
#endif
                pd3dImmediateContext->Unmap( g_pcbCS, 0 );
                ID3D11Buffer* ppCB[1] = { g_pcbCS };
                pd3dImmediateContext->PSSetConstantBuffers( g_iCBPSBind, 1, ppCB );

                ID3D11SamplerState* aSamplers[] = { g_pSampleStatePoint, g_pSampleStateLinear };
                pd3dImmediateContext->PSSetSamplers( 0, 2, aSamplers );

                DrawFullScreenQuad11( pd3dImmediateContext, g_pFinalPassPS, pBackBufferDesc->Width, pBackBufferDesc->Height );
            } else
            {
                ID3D11ShaderResourceView* aRViews[ 1 ] = { g_pTexRenderRV11 };
                pd3dImmediateContext->PSSetShaderResources( 0, 1, aRViews );

                D3D11_MAPPED_SUBRESOURCE MappedResource;            
                V( pd3dImmediateContext->Map( g_pcbCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
                CB_PS* pcbCS = ( CB_PS* )MappedResource.pData;
#ifdef CS_FULL_PIXEL_REDUCITON
                pcbCS->param[0] = g_fCPUReduceResult / (pBackBufferDesc->Width*pBackBufferDesc->Height);
#else
                pcbCS->param[0] = g_fCPUReduceResult / (ToneMappingTexSize*ToneMappingTexSize);
#endif
                pd3dImmediateContext->Unmap( g_pcbCS, 0 );
                ID3D11Buffer* ppCB[1] = { g_pcbCS };
                pd3dImmediateContext->PSSetConstantBuffers( g_iCBPSBind, 1, ppCB );

                DrawFullScreenQuad11( pd3dImmediateContext, g_pFinalPassForCPUReductionPS, pBackBufferDesc->Width, pBackBufferDesc->Height );                
            }            
        } else //if ( g_ePostProcessMode == PM_PIXEL_SHADER )
        {
            ID3D11ShaderResourceView* aRViews[ 3 ] = { g_pTexRenderRV11, g_apTexToneMapRV11[0], g_bBloom ? g_apTexBloomRV11[0] : NULL };
            pd3dImmediateContext->PSSetShaderResources( 0, 3, aRViews );

            ID3D11SamplerState* aSamplers[] = { g_pSampleStatePoint, g_pSampleStateLinear };
            pd3dImmediateContext->PSSetSamplers( 0, 2, aSamplers );

            DrawFullScreenQuad11( pd3dImmediateContext, g_pOldFinalPassPS, pBackBufferDesc->Width, pBackBufferDesc->Height );
        }

        ID3D11ShaderResourceView* ppSRVNULL[3] = { NULL, NULL, NULL };
        pd3dImmediateContext->PSSetShaderResources( 0, 3, ppSRVNULL );  
    }

    SAFE_RELEASE( pOrigRTV );
    SAFE_RELEASE( pOrigDSV );

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_HUD.OnRender( fElapsedTime );
    g_SampleUI.OnRender( fElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();
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

    g_Skybox.OnD3D11DestroyDevice();

    SAFE_RELEASE( g_pFinalPassPS );
    SAFE_RELEASE( g_pFinalPassForCPUReductionPS );
    SAFE_RELEASE( g_pReduceTo1DCS );
    SAFE_RELEASE( g_pReduceToSingleCS );
    SAFE_RELEASE( g_pBrightPassAndHorizFilterCS );
    SAFE_RELEASE( g_pVertFilterCS );
    SAFE_RELEASE( g_pHorizFilterCS );
    SAFE_RELEASE( g_pDownScale2x2LumPS );
    SAFE_RELEASE( g_pDownScale3x3PS );
    SAFE_RELEASE( g_pOldFinalPassPS );
    SAFE_RELEASE( g_pDownScale3x3BrightPassPS );
    SAFE_RELEASE( g_pBloomPS );
    SAFE_RELEASE( g_pDumpBufferPS );

    SAFE_RELEASE( g_pcbCS );
    SAFE_RELEASE( g_pcbBloom );
    SAFE_RELEASE( g_pcbFilterCS );

    SAFE_RELEASE( g_pSampleStateLinear );
    SAFE_RELEASE( g_pSampleStatePoint );

    SAFE_RELEASE( g_pScreenQuadVB );
    SAFE_RELEASE( g_pQuadVS );
    SAFE_RELEASE( g_pQuadLayout );
}

void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();

    SAFE_RELEASE( g_pTexRender11 );
    SAFE_RELEASE( g_pTexRenderMS11 );
    SAFE_RELEASE( g_pMSDS11 );
    SAFE_RELEASE( g_pTexBlurred11 );
    SAFE_RELEASE( g_pTexRenderRTV11 );
    SAFE_RELEASE( g_pMSRTV11 );
    SAFE_RELEASE( g_pMSDSV11 );
    SAFE_RELEASE( g_pTexBlurredRTV11 );
    SAFE_RELEASE( g_pTexRenderRV11 );
    SAFE_RELEASE( g_pTexBlurredRV11 );

    SAFE_RELEASE( g_pBufferReduction0 );
    SAFE_RELEASE( g_pBufferReduction1 );
    SAFE_RELEASE( g_pBufferBlur0 );
    SAFE_RELEASE( g_pBufferBlur1 );
    SAFE_RELEASE( g_pBufferCPURead );
    SAFE_RELEASE( g_pReductionUAView0 );
    SAFE_RELEASE( g_pReductionUAView1 );
    SAFE_RELEASE( g_pBlurUAView0 );
    SAFE_RELEASE( g_pBlurUAView1 );
    SAFE_RELEASE( g_pReductionRV0 );
    SAFE_RELEASE( g_pReductionRV1 );
    SAFE_RELEASE( g_pBlurRV0 );
    SAFE_RELEASE( g_pBlurRV1 );

    for( int i = 0; i < NUM_TONEMAP_TEXTURES; i++ )
    {
        SAFE_RELEASE( g_apTexToneMap11[i] ); // Tone mapping calculation textures
        SAFE_RELEASE( g_apTexToneMapRV11[i] );
        SAFE_RELEASE( g_apTexToneMapRTV11[i] );
    }
    for( int i = 0; i < NUM_BLOOM_TEXTURES; i++ )
    {
        SAFE_RELEASE( g_apTexBloom11[i] );     // Blooming effect intermediate texture
        SAFE_RELEASE( g_apTexBloomRV11[i] );
        SAFE_RELEASE( g_apTexBloomRTV11[i] );

        SAFE_RELEASE( g_apBufBloom11[i] );
        SAFE_RELEASE( g_apBufBloomRV11[i] );
        SAFE_RELEASE( g_apBufBloomUAV11[i] );
    }

    SAFE_RELEASE( g_pTexBrightPassRV11 );
    SAFE_RELEASE( g_pTexBrightPassRTV11 );
    SAFE_RELEASE( g_pTexBrightPass11 );

    g_Skybox.OnD3D11ReleasingSwapChain();
}
