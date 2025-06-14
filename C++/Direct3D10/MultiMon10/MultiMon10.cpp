//--------------------------------------------------------------------------------------
// File: MultiMon10.cpp
//
// Basic starting point for new Direct3D 10 samples
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTmisc.h"
#include "SDKmisc.h"
#include "resource.h"
#include "DXUTShapes.h"

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

// general windowing resources
WNDCLASS                            g_WindowClass;
bool                                g_bFullscreen = false;
IDXGIFactory*                       g_pDXGIFactory = NULL;
HWND                                g_hWndPrimary = NULL;

struct DEVICE_OBJECT
{
    UINT Ordinal;
    ID3D10Device* pDevice;
    ID3DX10Font* pFont;
    ID3DX10Sprite* pSprite;
    CDXUTTextHelper* pTxtHelper;
    ID3DX10Mesh* pMesh;
    ID3D10Effect* pEffect;

    ID3D10InputLayout* pInputLayout;
    ID3D10EffectTechnique* pRender;
    ID3D10EffectMatrixVariable* pmWorldViewProjection;
    ID3D10EffectMatrixVariable* pmWorld;
};

struct WINDOW_OBJECT
{
    HWND hWnd;
    IDXGIAdapter* pAdapter;
    IDXGIOutput* pOutput;
    IDXGISwapChain* pSwapChain;
    DEVICE_OBJECT* pDeviceObj;
    ID3D10RenderTargetView* pRenderTargetView;
    ID3D10DepthStencilView* pDepthStencilView;
    UINT Width;
    UINT Height;
    DXGI_ADAPTER_DESC AdapterDesc;
    DXGI_OUTPUT_DESC OutputDesc;
};

struct ADAPTER_OBJECT
{
    IDXGIAdapter* pDXGIAdapter;
    CGrowableArray <IDXGIOutput*> DXGIOutputArray;
};

CGrowableArray <DEVICE_OBJECT*>     g_DeviceArray;
CGrowableArray <ADAPTER_OBJECT*>    g_AdapterArray;
CGrowableArray <WINDOW_OBJECT*>     g_WindowObjects;

WCHAR g_szWindowClass[] = { L"MultiMon10Class" };
WCHAR g_szWindowedName[] = { L"MultiMon10 - Windowed" };
WCHAR g_szFullscreenName[] = { L"MultiMon10 - Fullscreen" };


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT OnD3D10CreateDevice( DEVICE_OBJECT* pDeviceObj );
void OnD3D10FrameRender( WINDOW_OBJECT* pWindowObj, double fTime, float fElapsedTime );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
void RenderText( WINDOW_OBJECT* pWindowObj );

HRESULT InitD3D10();
HRESULT EnumerateAdapters();
HRESULT EnumerateOutputs( ADAPTER_OBJECT* pAdapterObj );
HRESULT CreateWindowClass( HINSTANCE hInstance );
HRESULT CreateMonitorWindows();
HRESULT SetWindowAssociation();
HRESULT CreateDevicePerAdapter( D3D10_DRIVER_TYPE DriverType );
HRESULT CreateSwapChainPerOutput();
HRESULT CreateViewsForWindowObject( WINDOW_OBJECT* pWindowObj );
HRESULT SetupMultiMon();
void DeviceCleanup();
void FullCleanup();
void RenderToAllMonitors( double fTime, float fElapsedTime );
void PresentToAllMonitors();

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

    // Init D3D10
    if( FAILED( InitD3D10() ) )
    {
        MessageBox( NULL, L"This application requires Direct3D10 and Vista to run.  This application will now exit.",
                    L"Error", MB_OK );
        FullCleanup();
        return 1;
    }

    // Enumerate Adapters
    if( FAILED( EnumerateAdapters() ) )
    {
        MessageBox( NULL, L"Could not enumerate adapters.  This application will now exit.", L"Error", MB_OK );
        FullCleanup();
        return 1;
    }

    // Create the window class
    if( FAILED( CreateWindowClass( hInstance ) ) )
    {
        MessageBox( NULL, L"Could not create window class.  This application will now exit.", L"Error", MB_OK );
        FullCleanup();
        return 1;
    }

    // Setup the system for multi-mon
    if( FAILED( SetupMultiMon() ) )
    {
        FullCleanup();
        return 1;
    }

    // Start the timer
    CDXUTTimer Timer;
    float fElapsedTime = 0.0f;
    double fTime = 0.0f;
    Timer.Start();
    fTime = Timer.GetTime();

    // Now we're ready to receive and process Windows messages.
    bool bGotMsg;
    MSG msg;
    msg.message = WM_NULL;
    PeekMessage( &msg, NULL, 0U, 0U, PM_NOREMOVE );

    while( WM_QUIT != msg.message )
    {
        // Use PeekMessage() so we can use idle time to render the scene.
        bGotMsg = ( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) != 0 );

        if( bGotMsg )
        {
            // Translate and dispatch the message
            if( g_hWndPrimary == NULL ||
                0 == TranslateAccelerator( g_hWndPrimary, NULL, &msg ) )
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }
        else
        {
            // Render a frame during idle time (no messages are waiting)
            RenderToAllMonitors( fTime, fElapsedTime );
            PresentToAllMonitors();
        }

        fTime = Timer.GetTime();
        fElapsedTime = Timer.GetElapsedTime();
    }

    FullCleanup();

    return 0;
}

//--------------------------------------------------------------------------------------
// Create any D3D10 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT OnD3D10CreateDevice( DEVICE_OBJECT* pDeviceObj )
{
    HRESULT hr;

    V_RETURN( D3DX10CreateSprite( pDeviceObj->pDevice, 500, &pDeviceObj->pSprite ) );
    V_RETURN( D3DX10CreateFont( pDeviceObj->pDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                L"Arial", &pDeviceObj->pFont ) );
    pDeviceObj->pTxtHelper = new CDXUTTextHelper( NULL, NULL, pDeviceObj->pFont, pDeviceObj->pSprite, 15 );

    // Load the effect file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"MultiMon10.fx" ) );
    DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows
    // the shaders to be optimized and to run exactly the way they will run in
    // the release configuration of this program.
    dwShaderFlags |= D3D10_SHADER_DEBUG;
    #endif
    V_RETURN( D3DX10CreateEffectFromFile( str, NULL, NULL, "fx_4_0", dwShaderFlags, 0, pDeviceObj->pDevice, NULL,
                                              NULL, &pDeviceObj->pEffect, NULL, NULL ) );

    pDeviceObj->pRender = pDeviceObj->pEffect->GetTechniqueByName( "Render" );
    pDeviceObj->pmWorldViewProjection = pDeviceObj->pEffect->GetVariableByName( "g_mWorldViewProjection" )->AsMatrix();
    pDeviceObj->pmWorld = pDeviceObj->pEffect->GetVariableByName( "g_mWorld" )->AsMatrix();

    // Create an input layout
    const D3D10_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };
    D3D10_PASS_DESC PassDesc;
    pDeviceObj->pRender->GetPassByIndex( 0 )->GetDesc( &PassDesc );
    V_RETURN( pDeviceObj->pDevice->CreateInputLayout( layout, sizeof( layout ) / sizeof( layout[0] ),
                                                      PassDesc.pIAInputSignature,
                                                      PassDesc.IAInputSignatureSize, &pDeviceObj->pInputLayout ) );

    // Create a shape
    switch( pDeviceObj->Ordinal % 6 )
    {
        case 0:
            DXUTCreateBox( pDeviceObj->pDevice, 1, 1, 1, &pDeviceObj->pMesh );
            break;
        case 1:
            DXUTCreateCylinder( pDeviceObj->pDevice, 1, 1, 1, 8, 4, &pDeviceObj->pMesh );
            break;
        case 2:
            DXUTCreatePolygon( pDeviceObj->pDevice, 1, 6, &pDeviceObj->pMesh );
            break;
        case 3:
            DXUTCreateSphere( pDeviceObj->pDevice, 1, 16, 16, &pDeviceObj->pMesh );
            break;
        case 4:
            DXUTCreateTorus( pDeviceObj->pDevice, 0.2f, 1.5f, 8, 16, &pDeviceObj->pMesh );
            break;
        case 5:
            DXUTCreateTeapot( pDeviceObj->pDevice, &pDeviceObj->pMesh );
            break;
    };

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Called to render a frame
//--------------------------------------------------------------------------------------
void OnD3D10FrameRender( WINDOW_OBJECT* pWindowObj, double fTime, float fElapsedTime )
{
    DEVICE_OBJECT* pDeviceObj = pWindowObj->pDeviceObj;
    ID3D10Device* pd3dDevice = pWindowObj->pDeviceObj->pDevice;

    // Clear
    float ClearColor[4] = { 0.1f, 0.3f, 0.8f, 0.0f };
    pd3dDevice->ClearRenderTargetView( pWindowObj->pRenderTargetView, ClearColor );
    pd3dDevice->ClearDepthStencilView( pWindowObj->pDepthStencilView, D3D10_CLEAR_DEPTH, 1.0, 0 );

    // Setup the effects
    D3DXMATRIX mWorld;
    D3DXMATRIX mView;
    D3DXMATRIX mProj;
    D3DXMatrixRotationY( &mWorld, ( float )fTime * D3DX_PI / 2.0f );
    D3DXVECTOR3 vEye( 0,4,-4 );
    D3DXVECTOR3 vLook( 0,0,0 );
    D3DXVECTOR3 vUp( 0,1,0 );
    D3DXMatrixLookAtLH( &mView, &vEye, &vLook, &vUp );
    float fAspect = pWindowObj->Width / ( float )pWindowObj->Height;
    D3DXMatrixPerspectiveFovLH( &mProj, D3DX_PI / 3.0f, fAspect, 0.1f, 30.0f );
    D3DXMATRIX mWVP = mWorld * mView * mProj;

    pDeviceObj->pmWorldViewProjection->SetMatrix( ( float* )&mWVP );
    pDeviceObj->pmWorld->SetMatrix( ( float* )&mWorld );

    D3D10_TECHNIQUE_DESC techDesc;
    pDeviceObj->pRender->GetDesc( &techDesc );

    UINT NumSubsets;
    pDeviceObj->pMesh->GetAttributeTable( NULL, &NumSubsets );

    pd3dDevice->IASetInputLayout( pDeviceObj->pInputLayout );

    for( UINT p = 0; p < techDesc.Passes; p++ )
    {
        pDeviceObj->pRender->GetPassByIndex( p )->Apply( 0 );

        for( UINT s = 0; s < NumSubsets; s++ )
        {
            pDeviceObj->pMesh->DrawSubset( s );
        }
    }

    // Render some text
    RenderText( pWindowObj );
}

//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if( VK_ESCAPE == wParam )
            {
                PostQuitMessage( 0 );
            }
            return 0;
        case WM_CLOSE:
            PostQuitMessage( 0 );
            return 0;
        case WM_SIZE:
            // Find the swapchain for this hwnd
            for( int i = 0; i < g_WindowObjects.GetSize(); i++ )
            {
                WINDOW_OBJECT* pObj = g_WindowObjects.GetAt( i );
                if( hWnd == pObj->hWnd )
                {
                    // Cleanup the views
                    SAFE_RELEASE( pObj->pRenderTargetView );
                    SAFE_RELEASE( pObj->pDepthStencilView );

                    RECT rcCurrentClient;
                    GetClientRect( hWnd, &rcCurrentClient );

                    DXGI_SWAP_CHAIN_DESC Desc;
                    pObj->pSwapChain->GetDesc( &Desc );

                    pObj->pSwapChain->ResizeBuffers( Desc.BufferCount,
                                                     ( UINT )rcCurrentClient.right,	// passing in 0 here will automatically calculate the size from the client rect
                                                     ( UINT )rcCurrentClient.bottom,  // passing in 0 here will automatically calculate the size from the client rect
                                                     Desc.BufferDesc.Format,
                                                     0 );

                    pObj->Width = ( UINT )rcCurrentClient.right;
                    pObj->Height = ( UINT )rcCurrentClient.bottom;

                    // recreate the views
                    CreateViewsForWindowObject( pObj );

                    return 0;
                }
            }
            return 0;
    };

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}


//--------------------------------------------------------------------------------------
// Initialize D3D10
//--------------------------------------------------------------------------------------
HRESULT InitD3D10()
{
    HRESULT hr = S_OK;

    // Check for D3D10 dlls
    HMODULE hD3D10 = LoadLibrary( L"d3d10.dll" );
    HMODULE hD3DX10 = LoadLibrary( D3DX10_DLL );
    HMODULE hDXGI = LoadLibrary( L"dxgi.dll" );

    if( !hD3D10 || !hD3DX10 || !hDXGI )
        return E_FAIL;

    if( hD3D10 )
        FreeLibrary( hD3D10 );
    if( hD3DX10 )
        FreeLibrary( hD3DX10 );
    if( hDXGI )
        FreeLibrary( hDXGI );

    // Create the DXGI Factory
    hr = CreateDXGIFactory( IID_IDXGIFactory, ( void** )&g_pDXGIFactory );
    if( FAILED( hr ) )
        return hr;

    return hr;
}

//--------------------------------------------------------------------------------------
// Enumerate all D3D10 Adapters
//--------------------------------------------------------------------------------------
HRESULT EnumerateAdapters()
{
    HRESULT hr = S_OK;

    for( UINT i = 0; ; i++ )
    {
        ADAPTER_OBJECT* pAdapterObj = new ADAPTER_OBJECT;
        if( !pAdapterObj )
            return E_OUTOFMEMORY;

        pAdapterObj->pDXGIAdapter = NULL;
        hr = g_pDXGIFactory->EnumAdapters( i, &pAdapterObj->pDXGIAdapter );
        if( DXGI_ERROR_NOT_FOUND == hr )
        {
            delete pAdapterObj;
            hr = S_OK;
            break;
        }
        if( FAILED( hr ) )
        {
            delete pAdapterObj;
            return hr;
        }

        // get the description of the adapter
        DXGI_ADAPTER_DESC AdapterDesc;
        hr = pAdapterObj->pDXGIAdapter->GetDesc( &AdapterDesc );
        if( FAILED( hr ) )
        {
            delete pAdapterObj;
            return hr;
        }

        // Enumerate outputs for this adapter
        hr = EnumerateOutputs( pAdapterObj );
        if( FAILED( hr ) )
        {
            delete pAdapterObj;
            return hr;
        }

        // add the adapter to the list
        if( pAdapterObj->DXGIOutputArray.GetSize() > 0 )
            g_AdapterArray.Add( pAdapterObj );
    }

    if( g_AdapterArray.GetSize() < 1 )
        return E_FAIL;

    return hr;
}

//--------------------------------------------------------------------------------------
// Enumerate all D3D10 Outputs
//--------------------------------------------------------------------------------------
HRESULT EnumerateOutputs( ADAPTER_OBJECT* pAdapterObj )
{
    HRESULT hr = S_OK;

    for( UINT i = 0; ; i++ )
    {
        IDXGIOutput* pOutput;
        hr = pAdapterObj->pDXGIAdapter->EnumOutputs( i, &pOutput );
        if( DXGI_ERROR_NOT_FOUND == hr )
        {
            hr = S_OK;
            break;
        }
        if( FAILED( hr ) )
            return hr;

        // get the description
        DXGI_OUTPUT_DESC OutputDesc;
        hr = pOutput->GetDesc( &OutputDesc );
        if( FAILED( hr ) )
            return hr;

        // only add outputs that are attached to the desktop
        // TODO: AttachedToDesktop seems to be always 0
        /*if( !OutputDesc.AttachedToDesktop )
           {
           pOutput->Release();
           continue;
           }*/

        pAdapterObj->DXGIOutputArray.Add( pOutput );
    }

    return hr;
}

//--------------------------------------------------------------------------------------
// Creates and register the window class
//--------------------------------------------------------------------------------------
HRESULT CreateWindowClass( HINSTANCE hInstance )
{
    WCHAR szExePath[MAX_PATH];
    GetModuleFileName( NULL, szExePath, MAX_PATH );
    HICON hIcon = ExtractIcon( hInstance, szExePath, 0 );

    ZeroMemory( &g_WindowClass, sizeof( WNDCLASS ) );
    g_WindowClass.hCursor = LoadCursor( NULL, IDC_ARROW );
    g_WindowClass.hIcon = hIcon;
    g_WindowClass.hbrBackground = ( HBRUSH )GetStockObject( BLACK_BRUSH );
    g_WindowClass.style = CS_DBLCLKS;
    g_WindowClass.lpfnWndProc = MsgProc;
    g_WindowClass.hInstance = hInstance;
    g_WindowClass.lpszClassName = g_szWindowClass;

    ATOM ClassAtom = RegisterClass( &g_WindowClass );
    if( ClassAtom == 0 )
    {
        DWORD error = GetLastError();

        if( ERROR_CLASS_ALREADY_EXISTS != error )
        {
            return E_FAIL;
        }
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Creates windows for all monitors
//--------------------------------------------------------------------------------------
HRESULT CreateMonitorWindows()
{
    HRESULT hr = S_OK;

    for( int a = 0; a < g_AdapterArray.GetSize(); a++ )
    {
        ADAPTER_OBJECT* pAdapter = g_AdapterArray.GetAt( a );
        for( int o = 0; o < pAdapter->DXGIOutputArray.GetSize(); o++ )
        {
            IDXGIOutput* pOutput = pAdapter->DXGIOutputArray.GetAt( o );
            DXGI_OUTPUT_DESC OutputDesc;
            pOutput->GetDesc( &OutputDesc );
            int X = OutputDesc.DesktopCoordinates.left;
            int Y = OutputDesc.DesktopCoordinates.top;
            int Width = OutputDesc.DesktopCoordinates.right - X;
            int Height = OutputDesc.DesktopCoordinates.bottom - Y;

            WINDOW_OBJECT* pWindowObj = new WINDOW_OBJECT;
            ZeroMemory( pWindowObj, sizeof( WINDOW_OBJECT ) );

            if( g_bFullscreen )
            {
                pWindowObj->hWnd = CreateWindow( g_szWindowClass,
                                                 g_szWindowedName,
                                                 WS_POPUP,
                                                 X,
                                                 Y,
                                                 Width,
                                                 Height,
                                                 NULL,
                                                 0,
                                                 g_WindowClass.hInstance,
                                                 NULL );
            }
            else
            {
                X += 100;
                Y += 100;
                Width /= 2;
                Height /= 2;
                DWORD dwStyle = WS_OVERLAPPEDWINDOW & ~( WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME );
                pWindowObj->hWnd = CreateWindow( g_szWindowClass,
                                                 g_szWindowedName,
                                                 dwStyle,
                                                 X,
                                                 Y,
                                                 Width,
                                                 Height,
                                                 NULL,
                                                 0,
                                                 g_WindowClass.hInstance,
                                                 NULL );
            }

            if( NULL == pWindowObj->hWnd )
            {
                delete pWindowObj;
                return E_FAIL;
            }

            // show the window
            ShowWindow( pWindowObj->hWnd, SW_SHOWDEFAULT );

            // set width and height
            pWindowObj->Width = Width;
            pWindowObj->Height = Height;

            // add this to the window object array
            g_WindowObjects.Add( pWindowObj );

        }
    }

    return hr;
}

//--------------------------------------------------------------------------------------
// Set DXGI's window association
//--------------------------------------------------------------------------------------
HRESULT SetWindowAssociation()
{
    if( g_WindowObjects.GetSize() < 1 )
        return E_FAIL;

    HWND hWnd = g_WindowObjects.GetAt( 0 )->hWnd;

    // set window association
    return g_pDXGIFactory->MakeWindowAssociation( hWnd, 0 );
}

//--------------------------------------------------------------------------------------
// Creates a device per adapter
//--------------------------------------------------------------------------------------
HRESULT CreateDevicePerAdapter( D3D10_DRIVER_TYPE DriverType )
{
    HRESULT hr = S_OK;
    int iWindowObj = 0;
    for( int a = 0; a < g_AdapterArray.GetSize(); a++ )
    {
        ADAPTER_OBJECT* pAdapterObj = g_AdapterArray.GetAt( a );

        IDXGIAdapter* pAdapter = NULL;
        if( D3D10_DRIVER_TYPE_HARDWARE == DriverType )
            pAdapter = pAdapterObj->pDXGIAdapter;

        UINT CreateFlags = 0;

        // Create a device for this adapter
        ID3D10Device* pd3dDevice = NULL;
        hr = D3D10CreateDevice( pAdapter,
                                DriverType,
                                NULL,
                                CreateFlags,
                                D3D10_SDK_VERSION,
                                &pd3dDevice );
        if( FAILED( hr ) )
            return hr;

        DEVICE_OBJECT* pDeviceObj = new DEVICE_OBJECT;
        ZeroMemory( pDeviceObj, sizeof( DEVICE_OBJECT ) );
        pDeviceObj->pDevice = pd3dDevice;

        // add the device
        pDeviceObj->Ordinal = g_DeviceArray.GetSize();
        g_DeviceArray.Add( pDeviceObj );

        // Init stuff needed for the device
        OnD3D10CreateDevice( pDeviceObj );

        // go through the outputs and set the device, adapter, and output
        for( int o = 0; o < pAdapterObj->DXGIOutputArray.GetSize(); o++ )
        {
            IDXGIOutput* pOutput = pAdapterObj->DXGIOutputArray.GetAt( o );
            WINDOW_OBJECT* pWindowObj = g_WindowObjects.GetAt( iWindowObj );

            pWindowObj->pDeviceObj = pDeviceObj;
            pWindowObj->pAdapter = pAdapter;
            pWindowObj->pOutput = pOutput;
            iWindowObj ++;
        }
    }

    return hr;
}

//--------------------------------------------------------------------------------------
// Creates a swapchain per output
//--------------------------------------------------------------------------------------
HRESULT CreateSwapChainPerOutput()
{
    HRESULT hr = S_OK;
    for( int i = 0; i < g_WindowObjects.GetSize(); i++ )
    {
        WINDOW_OBJECT* pWindowObj = g_WindowObjects.GetAt( i );

        // get the dxgi device
        IDXGIDevice* pDXGIDevice = NULL;
        hr = pWindowObj->pDeviceObj->pDevice->QueryInterface( IID_IDXGIDevice, ( void** )&pDXGIDevice );
        if( FAILED( hr ) )
            return hr;

        // create a swap chain
        DXGI_SWAP_CHAIN_DESC SwapChainDesc;
        ZeroMemory( &SwapChainDesc, sizeof( DXGI_SWAP_CHAIN_DESC ) );
        SwapChainDesc.BufferDesc.Width = pWindowObj->Width;
        SwapChainDesc.BufferDesc.Height = pWindowObj->Height;
        SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        SwapChainDesc.SampleDesc.Count = 1;
        SwapChainDesc.SampleDesc.Quality = 0;
        SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        SwapChainDesc.BufferCount = 3;
        SwapChainDesc.OutputWindow = pWindowObj->hWnd;
        SwapChainDesc.Windowed = ( g_bFullscreen == false );
        SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        SwapChainDesc.Flags = 0;
        hr = g_pDXGIFactory->CreateSwapChain( pDXGIDevice, &SwapChainDesc, &pWindowObj->pSwapChain );
        pDXGIDevice->Release();
        pDXGIDevice = NULL;
        if( FAILED( hr ) )
            return hr;

        hr = CreateViewsForWindowObject( pWindowObj );
        if( FAILED( hr ) )
            return hr;
    }

    return hr;
}

//--------------------------------------------------------------------------------------
// Creates a render target view and depth stencil surface/view per swapchain
//--------------------------------------------------------------------------------------
HRESULT CreateViewsForWindowObject( WINDOW_OBJECT* pWindowObj )
{
    HRESULT hr = S_OK;

    // get the backbuffer
    ID3D10Texture2D* pBackBuffer = NULL;
    hr = pWindowObj->pSwapChain->GetBuffer( 0, IID_ID3D10Texture2D, ( void** )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    // get the backbuffer desc
    D3D10_TEXTURE2D_DESC BBDesc;
    pBackBuffer->GetDesc( &BBDesc );

    // create the render target view
    D3D10_RENDER_TARGET_VIEW_DESC RTVDesc;
    RTVDesc.Format = BBDesc.Format;
    RTVDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    RTVDesc.Texture2D.MipSlice = 0;
    hr = pWindowObj->pDeviceObj->pDevice->CreateRenderTargetView( pBackBuffer, &RTVDesc,
                                                                  &pWindowObj->pRenderTargetView );
    pBackBuffer->Release();
    pBackBuffer = NULL;
    if( FAILED( hr ) )
        return hr;

    // Create depth stencil texture
    ID3D10Texture2D* pDepthStencil = NULL;
    D3D10_TEXTURE2D_DESC descDepth;
    descDepth.Width = pWindowObj->Width;
    descDepth.Height = pWindowObj->Height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D16_UNORM;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D10_USAGE_DEFAULT;
    descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = pWindowObj->pDeviceObj->pDevice->CreateTexture2D( &descDepth, NULL, &pDepthStencil );
    if( FAILED( hr ) )
        return hr;

    // Create the depth stencil view
    D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = pWindowObj->pDeviceObj->pDevice->CreateDepthStencilView( pDepthStencil, &descDSV,
                                                                  &pWindowObj->pDepthStencilView );
    SAFE_RELEASE( pDepthStencil );
    if( FAILED( hr ) )
        return hr;

    // get various information
    if( pWindowObj->pAdapter )
        pWindowObj->pAdapter->GetDesc( &pWindowObj->AdapterDesc );
    if( pWindowObj->pOutput )
        pWindowObj->pOutput->GetDesc( &pWindowObj->OutputDesc );

    return hr;
}

//--------------------------------------------------------------------------------------
// Setup Multi-mon based upon g_MultiMonType
//--------------------------------------------------------------------------------------
HRESULT SetupMultiMon()
{
    HRESULT hr = S_OK;

    // Create a window per monitor
    hr = CreateMonitorWindows();
    if( FAILED( hr ) )
    {
        MessageBox( NULL, L"Could not create monitor windows.  This application will now exit.", L"Error", MB_OK );
        return hr;
    }

    // Set DXGI Window association for the first monitor window
    hr = SetWindowAssociation();
    if( FAILED( hr ) )
    {
        MessageBox( NULL, L"Could not set DXGI window association.  This application will now exit.", L"Error",
                    MB_OK );
        return hr;
    }

    // Create a device per adapter or device per output
    hr = CreateDevicePerAdapter( D3D10_DRIVER_TYPE_HARDWARE );
    if( FAILED( hr ) )
    {
        hr = CreateDevicePerAdapter( D3D10_DRIVER_TYPE_REFERENCE );
        if( FAILED( hr ) )
        {
            MessageBox( NULL, L"Could not create a compatible Direct3D10 device.  This application will now exit.",
                        L"Error", MB_OK );
            return hr;
        }
    }

    // Create a swap chain for each output
    hr = CreateSwapChainPerOutput();
    if( FAILED( hr ) )
    {
        MessageBox( NULL, L"Could not create Swap Chains for all outputs.  This application will now exit.", L"Error",
                    MB_OK );
        return hr;
    }

    if( g_WindowObjects.GetSize() > 0 )
    {
        WINDOW_OBJECT* pWindowObj = g_WindowObjects.GetAt( 0 );
        g_hWndPrimary = pWindowObj->hWnd;
    }

    return hr;
}

//--------------------------------------------------------------------------------------
// Cleanup the device and window based objects
//--------------------------------------------------------------------------------------
void DeviceCleanup()
{
    // Make sure we're in windowed mode, since we cannot destroy a fullscreen swapchain
    for( int w = 0; w < g_WindowObjects.GetSize(); w++ )
    {
        WINDOW_OBJECT* pWindowObj = g_WindowObjects.GetAt( w );
        pWindowObj->pSwapChain->SetFullscreenState( FALSE, NULL );
    }

    // cleanup window objects
    for( int w = 0; w < g_WindowObjects.GetSize(); w++ )
    {
        WINDOW_OBJECT* pWindowObj = g_WindowObjects.GetAt( w );
        DestroyWindow( pWindowObj->hWnd );
        SAFE_RELEASE( pWindowObj->pRenderTargetView );
        SAFE_RELEASE( pWindowObj->pDepthStencilView );
        SAFE_RELEASE( pWindowObj->pSwapChain );
        SAFE_DELETE( pWindowObj );
    }
    g_WindowObjects.RemoveAll();

    // cleanup devices
    for( int d = 0; d < g_DeviceArray.GetSize(); d++ )
    {
        DEVICE_OBJECT* pDeviceObj = g_DeviceArray.GetAt( d );

        SAFE_RELEASE( pDeviceObj->pFont );
        SAFE_RELEASE( pDeviceObj->pSprite );
        SAFE_DELETE( pDeviceObj->pTxtHelper );
        SAFE_RELEASE( pDeviceObj->pEffect );
        SAFE_RELEASE( pDeviceObj->pMesh );
        SAFE_RELEASE( pDeviceObj->pInputLayout );

        SAFE_RELEASE( pDeviceObj->pDevice );
        SAFE_DELETE( pDeviceObj );
    }
    g_DeviceArray.RemoveAll();
}

//--------------------------------------------------------------------------------------
// Cleanup everything
//--------------------------------------------------------------------------------------
void FullCleanup()
{
    // Destroy devices
    DeviceCleanup();

    // Remove enumerated objects
    for( int a = 0; a < g_AdapterArray.GetSize(); a++ )
    {
        ADAPTER_OBJECT* pAdapterObj = g_AdapterArray.GetAt( a );

        // go through the outputs
        for( int o = 0; o < pAdapterObj->DXGIOutputArray.GetSize(); o++ )
        {
            IDXGIOutput* pOutput = pAdapterObj->DXGIOutputArray.GetAt( o );
            SAFE_RELEASE( pOutput );
        }
        pAdapterObj->DXGIOutputArray.RemoveAll();

        SAFE_RELEASE( pAdapterObj->pDXGIAdapter );
        SAFE_DELETE( pAdapterObj );
    }
    g_AdapterArray.RemoveAll();
}

//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText( WINDOW_OBJECT* pWindowObj )
{
    WCHAR strTxt[MAX_PATH];

    pWindowObj->pDeviceObj->pTxtHelper->Begin();
    pWindowObj->pDeviceObj->pTxtHelper->SetInsertionPos( 5, 5 );
    pWindowObj->pDeviceObj->pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    pWindowObj->pDeviceObj->pTxtHelper->DrawTextLine( L"Multi-Mon: One device per adapter" );
    swprintf_s( strTxt, MAX_PATH, L"Adapter: %s", pWindowObj->AdapterDesc.Description );
    pWindowObj->pDeviceObj->pTxtHelper->DrawTextLine( strTxt );
    swprintf_s( strTxt, MAX_PATH, L"Output: %s", pWindowObj->OutputDesc.DeviceName );
    pWindowObj->pDeviceObj->pTxtHelper->DrawTextLine( strTxt );
    swprintf_s( strTxt, MAX_PATH, L"Device: %p", pWindowObj->pDeviceObj->pDevice );
    pWindowObj->pDeviceObj->pTxtHelper->DrawTextLine( strTxt );
    pWindowObj->pDeviceObj->pTxtHelper->SetForegroundColor( D3DXCOLOR( 0.0f, 1.0f, 0.0f, 1.0f ) );
    pWindowObj->pDeviceObj->pTxtHelper->DrawTextLine( L"" );
    pWindowObj->pDeviceObj->pTxtHelper->DrawTextLine( L"Toggle Fullscreen by pressing Alt+Enter" );
    pWindowObj->pDeviceObj->pTxtHelper->End();
}

//--------------------------------------------------------------------------------------
// Render scene to all monitors
//--------------------------------------------------------------------------------------
void RenderToAllMonitors( double fTime, float fElapsedTime )
{
    // Clear them all
    for( int w = 0; w < g_WindowObjects.GetSize(); w++ )
    {
        WINDOW_OBJECT* pWindowObj = g_WindowObjects.GetAt( w );

        // set the render target
        pWindowObj->pDeviceObj->pDevice->OMSetRenderTargets( 1, &pWindowObj->pRenderTargetView,
                                                             pWindowObj->pDepthStencilView );
        // set the viewport
        D3D10_VIEWPORT Viewport;
        Viewport.TopLeftX = 0;
        Viewport.TopLeftY = 0;
        Viewport.Width = pWindowObj->Width;
        Viewport.Height = pWindowObj->Height;
        Viewport.MinDepth = 0.0f;
        Viewport.MaxDepth = 1.0f;
        pWindowObj->pDeviceObj->pDevice->RSSetViewports( 1, &Viewport );

        // Call the render function
        OnD3D10FrameRender( pWindowObj, fTime, fElapsedTime );
    }
}

//--------------------------------------------------------------------------------------
// Render scene to all monitors
//--------------------------------------------------------------------------------------
void PresentToAllMonitors()
{
    for( int w = 0; w < g_WindowObjects.GetSize(); w++ )
    {
        WINDOW_OBJECT* pWindowObj = g_WindowObjects.GetAt( w );
        pWindowObj->pSwapChain->Present( 0, 0 );
    }
}
