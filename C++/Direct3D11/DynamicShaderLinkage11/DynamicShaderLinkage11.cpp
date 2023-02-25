//--------------------------------------------------------------------------------------
// File: DynamicShaderLinkage11.cpp
//
// This sample shows a simple example of the Microsoft Direct3D's High-Level 
// Shader Language (HLSL) using Dynamic Shader Linkage. 
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include "resource.h"

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CModelViewerCamera          g_Camera;               // A model viewing camera
CDXUTDirectionWidget        g_LightControl;
CD3DSettingsDlg             g_D3DSettingsDlg;       // Device settings dialog
CDXUTDialog                 g_HUD;                  // manages the 3D   
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls
D3DXMATRIXA16               g_mCenterMesh;
float                       g_fLightScale;
int                         g_nNumActiveLights;
int                         g_nActiveLight;
bool                        g_bShowHelp = false;    // If true, it renders the UI control text

// Lighting Settings
bool                        g_bHemiAmbientLighting = false;
bool                        g_bDirectLighting = false;
bool                        g_bLightingOnly = false;
bool                        g_bWireFrame = false;

// Resources
CDXUTTextHelper*            g_pTxtHelper = NULL;

CDXUTSDKMesh                g_Mesh11;

ID3D11InputLayout*          g_pVertexLayout11 = NULL;
ID3D11Buffer*               g_pVertexBuffer = NULL;
ID3D11Buffer*               g_pIndexBuffer = NULL;
ID3D11VertexShader*         g_pVertexShader = NULL;
ID3D11PixelShader*          g_pPixelShader = NULL;
ID3D11ClassLinkage*         g_pPSClassLinkage = NULL;   
ID3D11SamplerState*         g_pSamLinear = NULL;

ID3D11RasterizerState*      g_pRasterizerStateSolid = NULL;
ID3D11RasterizerState*      g_pRasterizerStateWireFrame = NULL;

ID3D11ShaderResourceView*   g_pEnvironmentMapSRV = NULL;
static const UINT           g_iEnvironmentMapSlot = 2;

// Shader Linkage Interface and Class variables
ID3D11ClassInstance*        g_pAmbientLightClass = NULL;
ID3D11ClassInstance*        g_pHemiAmbientLightClass = NULL;
ID3D11ClassInstance*        g_pDirectionalLightClass = NULL;
ID3D11ClassInstance*        g_pEnvironmentLightClass = NULL;


// Material Dynamic Permutation 
enum E_MATERIAL_TYPES
{
   MATERIAL_PLASTIC,
   MATERIAL_PLASTIC_TEXTURED,
   MATERIAL_PLASTIC_LIGHTING_ONLY,

   MATERIAL_ROUGH,
   MATERIAL_ROUGH_TEXTURED,
   MATERIAL_ROUGH_LIGHTING_ONLY,

   MATERIAL_TYPE_COUNT
};
char*  g_pMaterialClassNames[ MATERIAL_TYPE_COUNT ] = 
{
   "g_plasticMaterial",             // cPlasticMaterial              
   "g_plasticTexturedMaterial",     // cPlasticTexturedMaterial      
   "g_plasticLightingOnlyMaterial", // cPlasticLightingOnlyMaterial 
   "g_roughMaterial",               // cRoughMaterial        
   "g_roughTexturedMaterial",       // cRoughTexturedMaterial
   "g_roughLightingOnlyMaterial"    // cRoughLightingOnlyMaterial    
};
E_MATERIAL_TYPES            g_iMaterial = MATERIAL_PLASTIC_TEXTURED;

ID3D11ClassInstance*        g_pMaterialClasses[ MATERIAL_TYPE_COUNT ] = { NULL };

UINT                        g_iNumPSInterfaces        = 0;
UINT                        g_iAmbientLightingOffset  = 0;
UINT                        g_iDirectLightingOffset   = 0;
UINT                        g_iEnvironmentLightingOffset = 0;
UINT                        g_iMaterialOffset         = 0;
ID3D11ClassInstance**       g_dynamicLinkageArray     = NULL;

struct CB_VS_PER_OBJECT
{
    D3DXMATRIX              m_WorldViewProj;
    D3DXMATRIX              m_World;
};
UINT                        g_iCBVSPerObjectBind = 0;

ID3D11Buffer*               g_pcbVSPerObject = NULL;

struct CB_PS_PER_FRAME
{
    D3DXVECTOR4 m_vAmbientLight; // AmbientLight
    D3DXVECTOR4 m_vSkyColor;     // HemiAmbientLight
    D3DXVECTOR4 m_vGroundColor;  // HemiAmbientLight
    D3DXVECTOR4 m_vUp; 
    D3DXVECTOR4 m_vDirLightColor;// DirectionalLight
    D3DXVECTOR4 m_vDirLightDir;     
    D3DXVECTOR4 m_vEnvLight;
    D3DXVECTOR4 m_vEyeDir;
};
UINT                        g_iCBPSPerFrameBind = 0;

struct CB_PS_PER_PRIMITIVE
{
    D3DXVECTOR4   m_vObjectColorPlastic;                // Plastic -.w is Specular Power
    D3DXVECTOR4   m_vObjectColorPlasticTextured;        // Plastic -.w is Specular Power
    D3DXVECTOR4   m_vObjectColorPlasticLightingOnly;    // Plastic - Lighting Only
    D3DXVECTOR4   m_vObjectColorRough;                  // Rough Material -.w is Specular Power
    D3DXVECTOR4   m_vObjectColorRoughTextured;          // Rough Material -.w is Specular Power
    D3DXVECTOR4   m_vObjectColorRoughLightingOnly;      // Rough Material -.w is Specular Power
};
UINT                        g_iCBPSPerPrimBind = 1;

ID3D11Buffer*               g_pcbPSPerFrame = NULL;
ID3D11Buffer*               g_pcbPSPerPrim = NULL;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4
#define IDC_TOGGLEWIRE          5


// Lighting ControlsIDC_UPDATE_SCENE_RENDER
#define IDC_AMBIENT_LIGHTING_GROUP        6
#define IDC_LIGHT_CONST_AMBIENT           7
#define IDC_LIGHT_HEMI_AMBIENT            8
#define IDC_LIGHT_DIRECT                  9
#define IDC_LIGHTING_ONLY                 10

// Material Controls
#define IDC_MATERIAL_GROUP                11
#define IDC_MATERIAL_PLASTIC              12
#define IDC_MATERIAL_PLASTIC_TEXTURED     13
#define IDC_MATERIAL_ROUGH                14
#define IDC_MATERIAL_ROUGH_TEXTURED       15

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
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
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device feature level available 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );
    
    InitApp();
    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"DynamicShaderLinkage11" );

    // For now this sample only shows the Feature Level 11 usage
    DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, 640, 480 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    D3DXVECTOR3 vLightDir( -1, 1, -1 );
    D3DXVec3Normalize( &vLightDir, &vLightDir );
    g_LightControl.SetLightDirection( vLightDir );

    // Initialize dialogs
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 25;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 22, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 22, VK_F2 );
    g_HUD.AddButton( IDC_TOGGLEWIRE, L"Toggle Wires (F4)", 0, iY += 26, 170, 22, VK_F4 );

    // Material Controls
    iY = 10;
    g_SampleUI.AddRadioButton( IDC_MATERIAL_PLASTIC, IDC_MATERIAL_GROUP, L"Plastic", 0, iY += 26, 170, 22 );
    g_SampleUI.AddRadioButton( IDC_MATERIAL_PLASTIC_TEXTURED, IDC_MATERIAL_GROUP, L"Plastic Textured", 0, iY += 26, 170, 22 );
    g_SampleUI.AddRadioButton( IDC_MATERIAL_ROUGH, IDC_MATERIAL_GROUP, L"Rough", 0, iY += 26, 170, 22 );
    g_SampleUI.AddRadioButton( IDC_MATERIAL_ROUGH_TEXTURED, IDC_MATERIAL_GROUP, L"Rough Textuured", 0, iY += 26, 170, 22 );
    CDXUTRadioButton*   pRadioButton = g_SampleUI.GetRadioButton( IDC_MATERIAL_PLASTIC_TEXTURED );
    pRadioButton->SetChecked( true );

    iY += 24;
    // Lighting Controls
    g_SampleUI.AddRadioButton( IDC_LIGHT_CONST_AMBIENT, IDC_AMBIENT_LIGHTING_GROUP, L"ConstantAmbient", 0, iY += 26, 170, 22 );
    g_SampleUI.AddRadioButton( IDC_LIGHT_HEMI_AMBIENT, IDC_AMBIENT_LIGHTING_GROUP, L"Hemi Ambient", 0, iY += 26, 170, 22 );
    pRadioButton = g_SampleUI.GetRadioButton( IDC_LIGHT_CONST_AMBIENT );
    pRadioButton->SetChecked( true );

    g_SampleUI.AddCheckBox( IDC_LIGHT_DIRECT, L"Direct Lighting", 0, iY += 26, 170, 22, g_bDirectLighting );
    g_SampleUI.AddCheckBox( IDC_LIGHTING_ONLY, L"Lighting Only", 0, iY += 26, 170, 22, g_bLightingOnly );

    g_SampleUI.SetCallback( OnGUIEvent ); 

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
        if( ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
    UINT nBackBufferHeight = ( DXUTIsAppRenderingWithD3D9() ) ? DXUTGetD3D9BackBufferSurfaceDesc()->Height :
            DXUTGetDXGIBackBufferSurfaceDesc()->Height;

    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    // Draw help
    if( g_bShowHelp )
    {
        g_pTxtHelper->SetInsertionPos( 2, nBackBufferHeight - 20 * 6 );
        g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        g_pTxtHelper->DrawTextLine( L"Controls:" );

        g_pTxtHelper->SetInsertionPos( 20, nBackBufferHeight - 20 * 5 );
        g_pTxtHelper->DrawTextLine( L"Rotate model: Left mouse button\n"
                                    L"Rotate light: Right mouse button\n"
                                    L"Rotate camera: Middle mouse button\n"
                                    L"Zoom camera: Mouse wheel scroll\n" );

        g_pTxtHelper->SetInsertionPos( 350, nBackBufferHeight - 20 * 5 );
        g_pTxtHelper->DrawTextLine( L"Hide help: F1\n"
                                    L"Quit: ESC\n" );
    }
    else
    {
        g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        g_pTxtHelper->DrawTextLine( L"Press F1 for help" );
    }

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

    g_LightControl.HandleMessages( hWnd, uMsg, wParam, lParam );

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
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


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); 
            break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); 
            break;
        case IDC_CHANGEDEVICE:
            g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); 
            break;
        case IDC_TOGGLEWIRE:
            g_bWireFrame  = !g_bWireFrame;
            break;

        // Lighting Controls
        case IDC_LIGHT_CONST_AMBIENT:
            g_bHemiAmbientLighting = false;
            break;
        case IDC_LIGHT_HEMI_AMBIENT:
            g_bHemiAmbientLighting = true;
            break;
        case IDC_LIGHT_DIRECT:
            g_bDirectLighting = !g_bDirectLighting;
            break;
        case IDC_LIGHTING_ONLY:
            g_bLightingOnly = !g_bLightingOnly;
            break;

        // Material Controls
        case IDC_MATERIAL_PLASTIC:
            g_iMaterial = MATERIAL_PLASTIC;
            break;
        case IDC_MATERIAL_PLASTIC_TEXTURED:
            g_iMaterial = MATERIAL_PLASTIC_TEXTURED;
            break;
        case IDC_MATERIAL_ROUGH:
            g_iMaterial = MATERIAL_ROUGH;
            break;
        case IDC_MATERIAL_ROUGH_TEXTURED:
            g_iMaterial = MATERIAL_ROUGH_TEXTURED;
            break;

    }
 

}


//--------------------------------------------------------------------------------------
// Reject any D3D10 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}

//--------------------------------------------------------------------------------------
// Use this until D3DX11 comes online and we get some compilation helpers
//--------------------------------------------------------------------------------------
static const unsigned int MAX_INCLUDES = 5;
struct sInclude
{
   HANDLE         hFile;
   HANDLE         hFileMap;
   LARGE_INTEGER  FileSize;
   void           *pMapData;
};

class CIncludeHandler : public ID3DInclude
{
   private:
      struct sInclude   m_includeFiles[MAX_INCLUDES];
      unsigned int      m_nIncludes;

   public:
   CIncludeHandler()
   {
      // array initialization
      for ( unsigned int i=0; i<MAX_INCLUDES; i++)
      {
         m_includeFiles[i].hFile = INVALID_HANDLE_VALUE;
         m_includeFiles[i].hFileMap = INVALID_HANDLE_VALUE;
         m_includeFiles[i].pMapData = NULL;
      }
      m_nIncludes = 0;
   }
   ~CIncludeHandler()
   {
      for (unsigned int i=0; i<m_nIncludes; i++)
      {
         UnmapViewOfFile( m_includeFiles[i].pMapData );

         if ( m_includeFiles[i].hFileMap != INVALID_HANDLE_VALUE)
            CloseHandle( m_includeFiles[i].hFileMap );

         if ( m_includeFiles[i].hFile != INVALID_HANDLE_VALUE)
            CloseHandle( m_includeFiles[i].hFile );
      }

      m_nIncludes = 0;
   }

   STDMETHOD(Open(
     D3D_INCLUDE_TYPE IncludeType,
     LPCSTR pFileName,
     LPCVOID pParentData,
     LPCVOID *ppData,
     UINT *pBytes
   ))
   {
      unsigned int   incIndex = m_nIncludes+1;

      // Make sure we have enough room for this include file
      if ( incIndex >= MAX_INCLUDES )
        return E_FAIL;

      // try to open the file
      m_includeFiles[incIndex].hFile  = CreateFileA( pFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                             FILE_FLAG_SEQUENTIAL_SCAN, NULL );
      if( INVALID_HANDLE_VALUE == m_includeFiles[incIndex].hFile )
      {
        return E_FAIL;
      }

      // Get the file size
      GetFileSizeEx( m_includeFiles[incIndex].hFile, &m_includeFiles[incIndex].FileSize );

      // Use Memory Mapped File I/O for the header data
      m_includeFiles[incIndex].hFileMap = CreateFileMappingA( m_includeFiles[incIndex].hFile, NULL, PAGE_READONLY, m_includeFiles[incIndex].FileSize.HighPart, m_includeFiles[incIndex].FileSize.LowPart, pFileName);
      if( m_includeFiles[incIndex].hFileMap == NULL  )
      {
        if (m_includeFiles[incIndex].hFile != INVALID_HANDLE_VALUE)
           CloseHandle( m_includeFiles[incIndex].hFile );
        return E_FAIL;
      }

      // Create Map view
      *ppData = MapViewOfFile( m_includeFiles[incIndex].hFileMap, FILE_MAP_READ, 0, 0, 0 );
      *pBytes = m_includeFiles[incIndex].FileSize.LowPart;

      // Success - Increment the include file count
      m_nIncludes= incIndex;

      return S_OK;
   }

   STDMETHOD(Close( LPCVOID pData ))
   {
      // Defer Closure until the container destructor 
      return S_OK;
   }
};

HRESULT CompileShaderFromFile( WCHAR* szFileName, DWORD flags, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut, BOOL downLevel )
{
    HRESULT hr = S_OK;

#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    flags |= D3DCOMPILE_DEBUG;
#endif

    // find the file
    WCHAR str[MAX_PATH];
    WCHAR workingPath[MAX_PATH], filePath[MAX_PATH];
    WCHAR *strLastSlash = NULL;
    bool  resetCurrentDir = false;

    // Get the current working directory so we can restore it later
    UINT nBytes = GetCurrentDirectory( MAX_PATH, workingPath );
    if (nBytes==MAX_PATH)
    {
      return E_FAIL;
    }

    // Check we can find the file first
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

    // Check if the file is in the current working directory
    wcscpy_s( filePath, MAX_PATH, str );

    strLastSlash = wcsrchr( filePath, TEXT( '\\' ) );
    if( strLastSlash )
    {
        // Chop the exe name from the exe path
        *strLastSlash = 0;

        SetCurrentDirectory( filePath );
        resetCurrentDir  = true;
    }

    // Create an Include handler instance
    CIncludeHandler* pIncludeHandler = new CIncludeHandler;

    // Compile the shader using optional defines and an include handler for header processing
    static const D3D_SHADER_MACRO Shader_Macros[] =  { "STATIC_PERMUTE", "1", NULL, NULL };

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( szFileName, (downLevel) ? &Shader_Macros[0] : NULL, pIncludeHandler, szEntryPoint, szShaderModel, 
        flags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );

    delete pIncludeHandler;

    // Restore the current working directory if we need to 
    if ( resetCurrentDir )
    {
        SetCurrentDirectory( workingPath );
    }

    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    SAFE_RELEASE( pErrorBlob );

    return hr;
}

//--------------------------------------------------------------------------------------
// Create any D3D10 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr = S_OK;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    D3DXVECTOR3 vCenter( 0.25767413f, -28.503521f, 111.00689f );
    FLOAT fObjectRadius = 378.15607f;

    D3DXMatrixTranslation( &g_mCenterMesh, -vCenter.x, -vCenter.y, -vCenter.z );
    D3DXMATRIXA16 m;
    D3DXMatrixRotationY( &m, D3DX_PI );
    g_mCenterMesh *= m;
    D3DXMatrixRotationX( &m, D3DX_PI / 2.0f );
    g_mCenterMesh *= m;

    // Init the UI widget for directional lighting
    V_RETURN( CDXUTDirectionWidget::StaticOnD3D11CreateDevice( pd3dDevice,  pd3dImmediateContext ) );
    g_LightControl.SetRadius( fObjectRadius );

    // Compile the shaders to a model based on the feature level we acquired
    ID3DBlob* pVertexShaderBuffer = NULL;
    ID3DBlob* pPixelShaderBuffer = NULL;
  
    D3D_FEATURE_LEVEL   supportedFeatureLevel = DXUTGetD3D11DeviceFeatureLevel();
    if (supportedFeatureLevel >= D3D_FEATURE_LEVEL_11_0)
    {
       // We are going to use Dynamic shader linkage with SM5 so we need to create some class linkage libraries 
       V_RETURN( pd3dDevice->CreateClassLinkage( &g_pPSClassLinkage ) );
       DXUT_SetDebugName( g_pPSClassLinkage, "PS" );

       V_RETURN( CompileShaderFromFile( L"DynamicShaderLinkage11_VS.hlsl", D3DCOMPILE_ENABLE_STRICTNESS, 
                                                   "VSMain", "vs_5_0" , &pVertexShaderBuffer, false ) );
       V_RETURN( CompileShaderFromFile( L"DynamicShaderLinkage11_PS.hlsl", D3DCOMPILE_ENABLE_STRICTNESS, // | D3DCOMPILE_OPTIMIZATION_LEVEL0, 
                                                   "PSMain", "ps_5_0" , &pPixelShaderBuffer, false ) );

       V_RETURN( pd3dDevice->CreateVertexShader( pVertexShaderBuffer->GetBufferPointer(),
                                                 pVertexShaderBuffer->GetBufferSize(), NULL, &g_pVertexShader ) );
       V_RETURN( pd3dDevice->CreatePixelShader( pPixelShaderBuffer->GetBufferPointer(),
                                                pPixelShaderBuffer->GetBufferSize(), g_pPSClassLinkage, &g_pPixelShader ) );

       DXUT_SetDebugName( g_pVertexShader, "VSMain" );
       DXUT_SetDebugName( g_pPixelShader, "PSMain" );

       // use shader reflection to get data locations for the interface array 
       ID3D11ShaderReflection* pReflector = NULL; 
       V_RETURN( D3DReflect( pPixelShaderBuffer->GetBufferPointer(), pPixelShaderBuffer->GetBufferSize(), 
                                    IID_ID3D11ShaderReflection, (void**) &pReflector) );

       g_iNumPSInterfaces = pReflector->GetNumInterfaceSlots(); 
       g_dynamicLinkageArray = (ID3D11ClassInstance**) malloc( sizeof(ID3D11ClassInstance*) * g_iNumPSInterfaces );
       if (g_dynamicLinkageArray == NULL)
          return E_FAIL;

       ID3D11ShaderReflectionVariable* pAmbientLightingVar = pReflector->GetVariableByName("g_abstractAmbientLighting");
       g_iAmbientLightingOffset = pAmbientLightingVar->GetInterfaceSlot(0);

       ID3D11ShaderReflectionVariable* pDirectLightingVar = pReflector->GetVariableByName("g_abstractDirectLighting");
       g_iDirectLightingOffset = pDirectLightingVar->GetInterfaceSlot(0);

       ID3D11ShaderReflectionVariable* pEnvironmentLightingVar = pReflector->GetVariableByName("g_abstractEnvironmentLighting");
       g_iEnvironmentLightingOffset = pEnvironmentLightingVar->GetInterfaceSlot(0);

       ID3D11ShaderReflectionVariable* pMaterialVar = pReflector->GetVariableByName("g_abstractMaterial");
       g_iMaterialOffset = pMaterialVar->GetInterfaceSlot(0);

       // Create a linkage array for Dyamically linking the interface
       // Get the abstract class interfaces so we can dynamically permute and assign linkages
       g_pPSClassLinkage->GetClassInstance( "g_ambientLight", 0, &g_pAmbientLightClass );
       g_pPSClassLinkage->GetClassInstance( "g_hemiAmbientLight", 0, &g_pHemiAmbientLightClass );
       g_pPSClassLinkage->GetClassInstance( "g_directionalLight",0, &g_pDirectionalLightClass );
       g_pPSClassLinkage->GetClassInstance( "g_environmentLight",0, &g_pEnvironmentLightClass );

       // Acquire the material Class Instances for all possible material settings
       for( UINT i=0; i < MATERIAL_TYPE_COUNT; i++)
       {
         g_pPSClassLinkage->GetClassInstance( g_pMaterialClassNames[i], 0, &g_pMaterialClasses[i] );     
       }      

       SAFE_RELEASE( pReflector );
    }
    else // Lower feature levels than 11 have no support for Dynamic Shader Linkage - need to use a static setting
    {
       // To fully support feature levels without DLS, we should compile the shader multiple times for each render setting.

       V_RETURN( CompileShaderFromFile( L"DynamicShaderLinkage11_VS.hlsl",  D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,
                                              "VSMain", "vs_4_0" , &pVertexShaderBuffer, true ) );
       V_RETURN( CompileShaderFromFile( L"DynamicShaderLinkage11_PS.hlsl",  D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY,
                                              "PSMain", "ps_4_0" , &pPixelShaderBuffer, true ) );

       // Create the shaders without Dynamic Shader Linkage - only support static settings
       V_RETURN( pd3dDevice->CreateVertexShader( pVertexShaderBuffer->GetBufferPointer(),
                                                 pVertexShaderBuffer->GetBufferSize(), NULL, &g_pVertexShader ) );
       V_RETURN( pd3dDevice->CreatePixelShader( pPixelShaderBuffer->GetBufferPointer(),
                                                pPixelShaderBuffer->GetBufferSize(), NULL, &g_pPixelShader ) );

       DXUT_SetDebugName( g_pVertexShader, "VSMain0" );
       DXUT_SetDebugName( g_pPixelShader, "PSMain0" );
    }

    // Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R10G10B10A2_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "TEXCOORD",  0, DXGI_FORMAT_R16G16_FLOAT,    0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "TANGENT",  0, DXGI_FORMAT_R10G10B10A2_UNORM,   0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BINORMAL", 0, DXGI_FORMAT_R10G10B10A2_UNORM,   0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }

    };

    V_RETURN( pd3dDevice->CreateInputLayout( layout, ARRAYSIZE( layout ), pVertexShaderBuffer->GetBufferPointer(),
                                             pVertexShaderBuffer->GetBufferSize(), &g_pVertexLayout11 ) );
    DXUT_SetDebugName( g_pVertexLayout11, "Primary" );

    SAFE_RELEASE( pVertexShaderBuffer );
    SAFE_RELEASE( pPixelShaderBuffer );

    // Load the mesh
    V_RETURN( g_Mesh11.Create( pd3dDevice, L"Squid\\squid.sdkmesh", false ) ); 

    // Create a sampler state
    D3D11_SAMPLER_DESC SamDesc;
    SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.MipLODBias = 0.0f;
    SamDesc.MaxAnisotropy = 1;
    SamDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    SamDesc.BorderColor[0] = SamDesc.BorderColor[1] = SamDesc.BorderColor[2] = SamDesc.BorderColor[3] = 0;
    SamDesc.MinLOD = 0;
    SamDesc.MaxLOD = D3D11_FLOAT32_MAX;
    V_RETURN( pd3dDevice->CreateSamplerState( &SamDesc, &g_pSamLinear ) );
    DXUT_SetDebugName( g_pSamLinear, "Linear" );

    // Setup constant buffers
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;

    // Vertex shader buffer
    Desc.ByteWidth = sizeof( CB_VS_PER_OBJECT );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbVSPerObject ) );
    DXUT_SetDebugName( g_pcbVSPerObject, "CB_VS_PER_OBJECT" );

    // Pixel Shader buffers
    Desc.ByteWidth = sizeof( CB_PS_PER_FRAME );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbPSPerFrame ) );
    DXUT_SetDebugName( g_pcbPSPerFrame, "CB_PS_PER_FRAME" );

    Desc.ByteWidth = sizeof( CB_PS_PER_PRIMITIVE );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbPSPerPrim ) );
    DXUT_SetDebugName( g_pcbPSPerPrim, "CB_PS_PER_PRIMITIVE" );

    // Load a HDR Environment for reflections
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"Light Probes\\uffizi_cross.dds" ) );
    V_RETURN( D3DX11CreateShaderResourceViewFromFile( pd3dDevice, str, NULL, NULL, &g_pEnvironmentMapSRV, NULL ));
    DXUT_SetDebugName( g_pEnvironmentMapSRV, "uffizi_cross.dds" );

    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye( 0.0f, 0.0f, -50.0f );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, -0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );
    g_Camera.SetRadius( fObjectRadius , fObjectRadius , fObjectRadius );

   // Create Rasterizer State Objects for WireFrame / Solid rendering
    D3D11_RASTERIZER_DESC RSDesc;
    RSDesc.AntialiasedLineEnable = FALSE;
    RSDesc.CullMode = D3D11_CULL_BACK;
    RSDesc.DepthBias = 0;
    RSDesc.DepthBiasClamp = 0.0f;
    RSDesc.DepthClipEnable = TRUE;
    RSDesc.FillMode = D3D11_FILL_SOLID;
    RSDesc.FrontCounterClockwise = FALSE;
    RSDesc.MultisampleEnable = TRUE;
    RSDesc.ScissorEnable = FALSE;
    RSDesc.SlopeScaledDepthBias = 0.0f;
    V_RETURN( pd3dDevice->CreateRasterizerState( &RSDesc, &g_pRasterizerStateSolid ) );
    DXUT_SetDebugName( g_pRasterizerStateSolid, "Solid" );

    RSDesc.FillMode = D3D11_FILL_WIREFRAME ;
    V_RETURN( pd3dDevice->CreateRasterizerState( &RSDesc, &g_pRasterizerStateWireFrame ) );
    DXUT_SetDebugName( g_pRasterizerStateWireFrame, "Wireframe" );

    return hr;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr = S_OK;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 2.0f, 4000.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300 );
    g_SampleUI.SetSize( 170, 300 );

    return hr;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext )
{
    HRESULT hr;

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Clear the render target and depth stencil
    float ClearColor[4] = { 0.0f, 0.25f, 0.25f, 0.55f };
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    D3DXMATRIX mWorldViewProjection;
    D3DXVECTOR3 vLightDir;
    D3DXMATRIX mWorld;
    D3DXMATRIX mView;
    D3DXMATRIX mProj;

    // Get the projection & view matrix from the camera class
    mWorld = *g_Camera.GetWorldMatrix();
    mProj  = *g_Camera.GetProjMatrix();
    mView  = *g_Camera.GetViewMatrix();

    // Get the light direction
    vLightDir = g_LightControl.GetLightDirection();

    // Render the light arrow so the user can visually see the light dir
    //D3DXCOLOR arrowColor = ( i == g_nActiveLight ) ? D3DXVECTOR4( 1, 1, 0, 1 ) : D3DXVECTOR4( 1, 1, 1, 1 );
    D3DXCOLOR arrowColor = D3DXVECTOR4( 1, 1, 0, 1 );
    V( g_LightControl.OnRender11( arrowColor, &mView, &mProj, g_Camera.GetEyePt() ) );

    // Per frame cb update
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    V( pd3dImmediateContext->Map( g_pcbPSPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    CB_PS_PER_FRAME* pPerFrame = ( CB_PS_PER_FRAME* )MappedResource.pData;
    // Ambient Light
    float fLightColor = 0.1f;
    pPerFrame->m_vAmbientLight = D3DXVECTOR4( fLightColor , fLightColor, fLightColor, 1.0f);

    // Hemi Ambient Light
    fLightColor = 0.3f;
    pPerFrame->m_vSkyColor = D3DXVECTOR4( fLightColor , fLightColor, fLightColor + 0.1f, 1.0f);
    fLightColor = 0.05f;
    pPerFrame->m_vGroundColor = D3DXVECTOR4( fLightColor, fLightColor, fLightColor, 1.0f);
    pPerFrame->m_vUp = D3DXVECTOR4( 0.0f, 1.0f, 0.0f, 1.0f);

    // Directional Light
    fLightColor = 1.0f;
    pPerFrame->m_vDirLightColor = D3DXVECTOR4( fLightColor , fLightColor , fLightColor , 1.0f);
    pPerFrame->m_vDirLightDir = D3DXVECTOR4( vLightDir.x, vLightDir.y, vLightDir.z, 1.0f );

    // Environement Light - color comes from the texture
    pPerFrame->m_vEnvLight = D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 1.0f );

    // Setup the Eye based on the DXUT camera
    D3DXVECTOR3 vEyePt = *g_Camera.GetEyePt();
    D3DXVECTOR3 vDir = *g_Camera.GetLookAtPt() - vEyePt;
    pPerFrame->m_vEyeDir = D3DXVECTOR4( vDir.x, vDir.y, vDir.z, 1.0f );

    pd3dImmediateContext->Unmap( g_pcbPSPerFrame, 0 );

    pd3dImmediateContext->PSSetConstantBuffers( g_iCBPSPerFrameBind, 1, &g_pcbPSPerFrame );

    //Get the mesh
    //IA setup
    pd3dImmediateContext->IASetInputLayout( g_pVertexLayout11 );
    UINT Strides[1];
    UINT Offsets[1];
    ID3D11Buffer* pVB[1];
    pVB[0] = g_Mesh11.GetVB11( 0, 0 );
    Strides[0] = ( UINT )g_Mesh11.GetVertexStride( 0, 0 );
    Offsets[0] = 0;
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    pd3dImmediateContext->IASetIndexBuffer( g_Mesh11.GetIB11( 0 ), g_Mesh11.GetIBFormat11( 0 ), 0 );
 
    // Set the per object constant data
    mWorldViewProjection = mWorld * mView * mProj;

    // VS Per object
    V( pd3dImmediateContext->Map( g_pcbVSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    CB_VS_PER_OBJECT* pVSPerObject = ( CB_VS_PER_OBJECT* )MappedResource.pData;
    D3DXMatrixTranspose( &pVSPerObject->m_WorldViewProj, &mWorldViewProjection );
    D3DXMatrixTranspose( &pVSPerObject->m_World, &mWorld );

    pd3dImmediateContext->Unmap( g_pcbVSPerObject, 0 );
    pd3dImmediateContext->VSSetConstantBuffers( g_iCBVSPerObjectBind, 1, &g_pcbVSPerObject );

    if ( g_dynamicLinkageArray )
    {
        // Setup the Shader Linkage based on the user settings for Lighting
        // Ambient Lighting First - Constant or Hemi?
        if ( g_bHemiAmbientLighting )    
          g_dynamicLinkageArray[g_iAmbientLightingOffset] = g_pHemiAmbientLightClass;
        else
          g_dynamicLinkageArray[g_iAmbientLightingOffset] = g_pAmbientLightClass;
    
        // Direct Light - None or Directional 
        if (g_bDirectLighting) 
        {
           g_dynamicLinkageArray[g_iDirectLightingOffset] = g_pDirectionalLightClass;

        }
        else
        {
          // Disable ALL Direct Lighting
          g_dynamicLinkageArray[g_iDirectLightingOffset] = g_pAmbientLightClass;
        }
 
        // Setup the selected material class instance
        switch( g_iMaterial )
        {
            case MATERIAL_PLASTIC:
            case MATERIAL_PLASTIC_TEXTURED:
                 {       
                     // Bind the Environment light for reflections
                     g_dynamicLinkageArray[g_iEnvironmentLightingOffset] = g_pEnvironmentLightClass;
                     if (g_bLightingOnly)
                        g_dynamicLinkageArray[g_iMaterialOffset] = g_pMaterialClasses[ MATERIAL_PLASTIC_LIGHTING_ONLY ];
                     else
                        g_dynamicLinkageArray[g_iMaterialOffset] = g_pMaterialClasses[ g_iMaterial ] ;
                     break;
                 }
            case MATERIAL_ROUGH:
            case MATERIAL_ROUGH_TEXTURED:
                {
                    // UnBind the Environment light 
                    g_dynamicLinkageArray[g_iEnvironmentLightingOffset] = g_pAmbientLightClass;
                    if (g_bLightingOnly)
                        g_dynamicLinkageArray[g_iMaterialOffset] = g_pMaterialClasses[ MATERIAL_ROUGH_LIGHTING_ONLY ];
                    else
                        g_dynamicLinkageArray[g_iMaterialOffset] = g_pMaterialClasses[ g_iMaterial ] ;
                    break;
                }
        }
    }

    // PS Per Prim
    int iSpecPower = 128;

    V( pd3dImmediateContext->Map( g_pcbPSPerPrim, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
    CB_PS_PER_PRIMITIVE* pPSPerPrim = ( CB_PS_PER_PRIMITIVE* )MappedResource.pData;

    pPSPerPrim->m_vObjectColorPlastic             = D3DXVECTOR4( 1, 0, 0.5, 0 );  // Shiny Plastic 
    memcpy( &(pPSPerPrim->m_vObjectColorPlastic.w), &iSpecPower, sizeof(int));
    pPSPerPrim->m_vObjectColorPlastic.w           = (int)255;             
    pPSPerPrim->m_vObjectColorPlasticTextured     = D3DXVECTOR4( 1, 0, 0.5, 0 );  // Shiny Plastic with Textures 
    memcpy( &(pPSPerPrim->m_vObjectColorPlasticTextured.w), &iSpecPower, sizeof(int));
    pPSPerPrim->m_vObjectColorPlasticLightingOnly = D3DXVECTOR4( 1, 1, 1, (int)255 );    // Lighting Only Plastic
    memcpy( &(pPSPerPrim->m_vObjectColorPlasticLightingOnly.w), &iSpecPower, sizeof(int));

    iSpecPower = 6;
    pPSPerPrim->m_vObjectColorRough               = D3DXVECTOR4( 0, .5, 1, 0 );     // Rough Material 
    memcpy( &(pPSPerPrim->m_vObjectColorRough.w), &iSpecPower, sizeof(int));
    pPSPerPrim->m_vObjectColorRoughTextured       = D3DXVECTOR4( 0, .5, 1, 0 );     // Rough Material with Textures
    memcpy( &(pPSPerPrim->m_vObjectColorRoughTextured.w), &iSpecPower, sizeof(int));
    pPSPerPrim->m_vObjectColorRoughLightingOnly   = D3DXVECTOR4( 1, 1, 1, 0 );    // Lighting Only Rough
    memcpy( &(pPSPerPrim->m_vObjectColorRoughLightingOnly.w), &iSpecPower, sizeof(int));

    pd3dImmediateContext->Unmap( g_pcbPSPerPrim, 0 );
  
    pd3dImmediateContext->PSSetConstantBuffers( g_iCBPSPerPrimBind, 1, &g_pcbPSPerPrim );

    // Set the shaders 
    pd3dImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
    pd3dImmediateContext->PSSetShader( g_pPixelShader, g_dynamicLinkageArray, g_iNumPSInterfaces );

    // Set the Environment Map
    pd3dImmediateContext->PSSetShaderResources( g_iEnvironmentMapSlot, 1 , &g_pEnvironmentMapSRV );

    //Render
    pd3dImmediateContext->PSSetSamplers( 0, 1, &g_pSamLinear );
    if (g_bWireFrame)
      pd3dImmediateContext->RSSetState( g_pRasterizerStateWireFrame );
    else
      pd3dImmediateContext->RSSetState( g_pRasterizerStateSolid );

    g_Mesh11.Render( pd3dImmediateContext, 0, 1, INVALID_SAMPLER_SLOT);

    // Tell the UI items to render 
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
    // Clean up shader linkage
    if (g_dynamicLinkageArray != NULL)
    {
      free( g_dynamicLinkageArray );
      g_dynamicLinkageArray = NULL;
    }

    // Clean up shader Linkage
    SAFE_RELEASE( g_pPSClassLinkage );
    
 	// Release the dynamic shader interfaces 
    SAFE_RELEASE( g_pAmbientLightClass );
    SAFE_RELEASE( g_pHemiAmbientLightClass );
    SAFE_RELEASE( g_pDirectionalLightClass );
    SAFE_RELEASE( g_pEnvironmentLightClass );

    for( UINT i=0; i < MATERIAL_TYPE_COUNT; i++)
    {
      SAFE_RELEASE( g_pMaterialClasses[i] );     
    }      

    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    CDXUTDirectionWidget::StaticOnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

    g_Mesh11.Destroy();

    SAFE_RELEASE( g_pSamLinear );
    SAFE_RELEASE( g_pVertexLayout11 );
    SAFE_RELEASE( g_pVertexBuffer );
    SAFE_RELEASE( g_pIndexBuffer );
    SAFE_RELEASE( g_pVertexShader );
    SAFE_RELEASE( g_pPixelShader );

    SAFE_RELEASE( g_pcbVSPerObject );

    SAFE_RELEASE( g_pcbPSPerFrame );
    SAFE_RELEASE( g_pcbPSPerPrim );

    SAFE_RELEASE( g_pRasterizerStateSolid );
    SAFE_RELEASE( g_pRasterizerStateWireFrame );

    SAFE_RELEASE( g_pEnvironmentMapSRV );
}



