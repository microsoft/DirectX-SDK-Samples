//--------------------------------------------------------------------------------------
// File: game.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "SDKmisc.h"
#include <stdio.h>
#include "game.h"
#include "audio.h"
#include "resource.h"


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_STATIC              1
#define IDC_AUDIO               2
#define IDC_VIDEO               3
#define IDC_RESUME              4
#define IDC_QUIT                5
#define IDC_BACK                8
#define IDC_SOUNDFX_SCALE       6
#define IDC_MUSIC_SCALE         7
#define IDC_RESOLUTION          9
#define IDC_ANTI_ALIASING       10
#define IDC_MAX_DROIDS          11
#define IDC_HIGH_MODEL_RES      12
#define IDC_MAX_DROIDS_TEXT     13
#define IDC_APPLY               14
#define IDC_FULLSCREEN          15
#define IDC_ASPECT              16

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
void ComputeMeshScaling( CDXUTXFileMesh& Mesh, D3DXMATRIX* pmScalingCenter, float fNewRadius );
void ComputeMeshScalingBox( CDXUTXFileMesh& Mesh, D3DXMATRIX* pmScalingCenter, D3DXVECTOR3 vNewMin,
                            D3DXVECTOR3 vNewMax );
void CreateAmmo( int nIndex, D3DXVECTOR4 Pos, D3DXVECTOR4 Vel );
void CreateDroid();
void SetEffectTechnique();
void RenderText();
void ToggleMenu();
void UpdateAspectRatioList( DXUTDeviceSettings* pDS );
void UpdateResolutionList( DXUTDeviceSettings* pDS );


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CFirstPersonCamera  g_Camera;
RENDER_STATE        g_Render;
GAME_STATE          g_GameState;

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    srand( 0 );

    g_Render.pEffect = NULL;
    g_Render.pDefaultTex = NULL;
    g_Render.UseFixedFunction = 0.0f;
    g_Render.ForceShader = 0;
    g_Render.MaximumResolution = 4096.0f;
    g_Render.DisableSpecular = 0.0f;
    g_Render.bDetectOptimalSettings = true;

    // Initialize dialogs
    g_Render.MainMenuDlg.Init( &g_Render.DialogResourceManager );
    g_Render.MainMenuDlg.SetCallback( OnGUIEvent ); int iY = ( ( 300 - 30 * 6 ) / 2 );
    g_Render.MainMenuDlg.AddButton( IDC_AUDIO, L"Audio", ( 250 - 125 ) / 2, iY += 30, 125, 22 );
    g_Render.MainMenuDlg.AddButton( IDC_VIDEO, L"Video", ( 250 - 125 ) / 2, iY += 30, 125, 22 );
    g_Render.MainMenuDlg.AddButton( IDC_RESUME, L"Resume", ( 250 - 125 ) / 2, iY += 30, 125, 22 );
    g_Render.MainMenuDlg.AddButton( IDC_QUIT, L"Quit", ( 250 - 125 ) / 2, iY += 60, 125, 22, 'Q' );

    g_Render.AudioMenuDlg.Init( &g_Render.DialogResourceManager );
    g_Render.AudioMenuDlg.SetCallback( OnGUIEvent ); iY = 60;
    g_Render.AudioMenuDlg.AddStatic( IDC_STATIC, L"Music Volume", ( 250 - 125 ) / 2, iY += 24, 125, 22 );
    g_Render.AudioMenuDlg.AddSlider( IDC_MUSIC_SCALE, ( 250 - 100 ) / 2, iY += 24, 100, 22, 0, 100, 100 );
    g_Render.AudioMenuDlg.AddStatic( IDC_STATIC, L"Sound Effects Volume", ( 250 - 125 ) / 2, iY += 35, 125, 22 );
    g_Render.AudioMenuDlg.AddSlider( IDC_SOUNDFX_SCALE, ( 250 - 100 ) / 2, iY += 24, 100, 22, 0, 100, 100 );
    g_Render.AudioMenuDlg.AddButton( IDC_BACK, L"Back", ( 250 - 125 ) / 2, iY += 40, 125, 22 );

    g_Render.VideoMenuDlg.Init( &g_Render.DialogResourceManager );
    g_Render.VideoMenuDlg.SetCallback( OnGUIEvent ); iY = 0;
    g_Render.VideoMenuDlg.AddCheckBox( IDC_FULLSCREEN, L"Full screen", ( 250 - 200 ) / 2, iY += 30, 200, 22, true );
    g_Render.VideoMenuDlg.AddStatic( IDC_STATIC, L"Aspect:", 50, iY += 22, 50, 22 );
    g_Render.VideoMenuDlg.AddComboBox( IDC_ASPECT, 100, iY, 100, 22 );
    g_Render.VideoMenuDlg.AddStatic( IDC_STATIC, L"Resolution:", 30, iY += 22, 75, 22 );
    g_Render.VideoMenuDlg.AddComboBox( IDC_RESOLUTION, 100, iY, 125, 22 );
    g_Render.VideoMenuDlg.AddCheckBox( IDC_ANTI_ALIASING, L"Anti-Aliasing", ( 250 - 200 ) / 2, iY += 26, 200, 22,
                                       false );
    g_Render.VideoMenuDlg.AddCheckBox( IDC_HIGH_MODEL_RES, L"High res models", ( 250 - 200 ) / 2, iY += 26, 200, 22,
                                       true );
    g_Render.VideoMenuDlg.AddStatic( IDC_MAX_DROIDS_TEXT, L"Max Droids", ( 250 - 125 ) / 2, iY += 26, 125, 22 );
    g_Render.VideoMenuDlg.AddSlider( IDC_MAX_DROIDS, ( 250 - 150 ) / 2, iY += 22, 150, 22, 1, MAX_DROID, 10 );
    g_Render.VideoMenuDlg.AddButton( IDC_APPLY, L"Apply", ( 250 - 125 ) / 2, iY += 35, 125, 22 );
    g_Render.VideoMenuDlg.AddButton( IDC_BACK, L"Back", ( 250 - 125 ) / 2, iY += 30, 125, 22 );

    // Setup the camera
    D3DXVECTOR3 MinBound( g_MinBound.x + CAMERA_SIZE, g_MinBound.y + CAMERA_SIZE, g_MinBound.z + CAMERA_SIZE );
    D3DXVECTOR3 MaxBound( g_MaxBound.x - CAMERA_SIZE, g_MaxBound.y - CAMERA_SIZE, g_MaxBound.z - CAMERA_SIZE );
    g_Camera.SetClipToBoundary( true, &MinBound, &MaxBound );
    g_Camera.SetEnableYAxisMovement( false );
    g_Camera.SetRotateButtons( false, false, true );
    g_Camera.SetScalers( 0.001f, 4.0f );
    D3DXVECTOR3 vecEye( 0.0f, -GROUND_Y + 0.7f, 0.0f );
    D3DXVECTOR3 vecAt ( 0.0f, -GROUND_Y + 0.7f, 1.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );

    ZeroMemory( &g_GameState, sizeof( GAME_STATE ) );
    g_GameState.gameMode = GAME_RUNNING;
    g_GameState.nAmmoCount = 0;
    g_GameState.fAmmoColorLerp = 1000.0f;
    g_GameState.BlendFromColor = D3DXCOLOR( 0.6f, 0, 0, 1 );
    g_GameState.bAutoAddDroids = false;
    g_GameState.bDroidMove = true;

    // Store the rcWork of each monitor before a fullscreen D3D device is created 
    // This is used later to ensure the supported window mode 
    // resolutions will fit inside the desktop
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    UINT numAdapters = pD3D->GetAdapterCount();
    for( UINT adapterOrdinal = 0; adapterOrdinal < numAdapters && adapterOrdinal < 10; adapterOrdinal++ )
    {
        MONITORINFO miAdapter;
        miAdapter.cbSize = sizeof( MONITORINFO );
        DXUTGetMonitorInfo( pD3D->GetAdapterMonitor( adapterOrdinal ), &miAdapter );
        g_Render.rcAdapterWork[adapterOrdinal] = miAdapter.rcWork;
    }

    // Make a list of supported windowed mode resolutions.  
    // The list of fullscreen mode resolutions are gathered from the D3D device directly.
    D3DDISPLAYMODE dm = {0, 0, 0, D3DFMT_UNKNOWN};
    dm.Width = 640; dm.Height = 480; g_Render.aWindowedDMList.Add( dm ); // 4:3
    dm.Width = 800; dm.Height = 600; g_Render.aWindowedDMList.Add( dm ); // 4:3
    dm.Width = 1024; dm.Height = 768; g_Render.aWindowedDMList.Add( dm ); // 4:3
    dm.Width = 1280; dm.Height = 960; g_Render.aWindowedDMList.Add( dm ); // 4:3
    dm.Width = 1600; dm.Height = 1200; g_Render.aWindowedDMList.Add( dm ); // 4:3

    dm.Width = 852; dm.Height = 480; g_Render.aWindowedDMList.Add( dm ); // 16:9
    dm.Width = 1067; dm.Height = 600; g_Render.aWindowedDMList.Add( dm ); // 16:9
    dm.Width = 1280; dm.Height = 720; g_Render.aWindowedDMList.Add( dm ); // 16:9
    dm.Width = 1920; dm.Height = 1080; g_Render.aWindowedDMList.Add( dm ); // 16:9
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

    // No fallback defined by this app, so reject any device that 
    // doesn't support at least ps2.0
    if( pCaps->PixelShaderVersion < D3DPS_VERSION( 2, 0 ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
static int __cdecl SortAspectRatios( const void* arg1, const void* arg2 )
{
    float* pArg1 = ( float* )arg1;
    float* pArg2 = ( float* )arg2;

    if( *pArg1 > *pArg2 )
        return 1;
    if( *pArg1 < *pArg2 )
        return -1;
    return 0;
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

    // Look at the caps, and choose reasonable defaults.  Only do this
    // upon changing devices (not when the user changes the settings)
    if( g_Render.bDetectOptimalSettings )
    {
        if( caps.VertexShaderVersion < D3DVS_VERSION( 1, 1 ) )
        {
            g_GameState.nMaxDroids = 5;
            g_Render.UseHighResModels = false;
            g_Render.UseAnisotropicFilter = 0;
        }
        else if( caps.VertexShaderVersion < D3DVS_VERSION( 2, 0 ) )
        {
            g_GameState.nMaxDroids = 10;
            g_Render.UseHighResModels = false;
            g_Render.UseAnisotropicFilter = 1;
        }
        else
        {
            g_GameState.nMaxDroids = 10;
            g_Render.UseHighResModels = true;
            g_Render.UseAnisotropicFilter = 1;
        }
    }

    while( g_GameState.nDroidCount < g_GameState.nMaxDroids )
        CreateDroid();

    // Wipe aspect/resolution lists if switching adapters so currently selected choice doesn't remain
    static UINT s_nAdapterOrdinal = 0xFFFF;
    if( pDeviceSettings->d3d9.AdapterOrdinal != s_nAdapterOrdinal )
    {
        s_nAdapterOrdinal = pDeviceSettings->d3d9.AdapterOrdinal;
        g_Render.VideoMenuDlg.GetComboBox( IDC_ASPECT )->RemoveAllItems();
        g_Render.VideoMenuDlg.GetComboBox( IDC_RESOLUTION )->RemoveAllItems();
        UpdateAspectRatioList( pDeviceSettings );
    }

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }

    pDeviceSettings->d3d9.pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    return true;
}


struct DROID_VERTEX
{
    D3DXVECTOR3 pos;
    D3DXVECTOR3 normal;
    D3DXVECTOR2 uv;
};


//--------------------------------------------------------------------------------------
HRESULT SplitIntoSeperateTriangles( IDirect3DDevice9* pd3dDevice, ID3DXMesh* pInMesh, CDXUTXFileMesh* pOutMesh )
{
    HRESULT hr;
    DWORD dwNumFaces = pInMesh->GetNumFaces();
    DWORD dwOutNumVerts = dwNumFaces * 3;
    D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];
    pInMesh->GetDeclaration( decl );

    ID3DXMesh* pNewMesh = NULL;
    hr = D3DXCreateMesh( dwNumFaces, dwOutNumVerts, D3DXMESH_32BIT | D3DXMESH_MANAGED, decl, pd3dDevice, &pNewMesh );
    if( FAILED( hr ) )
        return hr;

    WORD* pInIndexData;
    DWORD* pNewIndexData;
    DROID_VERTEX* pInVertexData;
    DROID_VERTEX* pNewVertexData;
    DWORD* pInAttribData;
    DWORD* pNewAttribData;
    pInMesh->LockAttributeBuffer( D3DLOCK_READONLY, ( DWORD** )&pInAttribData );
    pInMesh->LockVertexBuffer( D3DLOCK_READONLY, ( VOID** )&pInVertexData );
    pInMesh->LockIndexBuffer( D3DLOCK_READONLY, ( VOID** )&pInIndexData );
    {
        pNewMesh->LockAttributeBuffer( 0, ( DWORD** )&pNewAttribData );
        pNewMesh->LockIndexBuffer( 0, ( VOID** )&pNewIndexData );
        pNewMesh->LockVertexBuffer( 0, ( VOID** )&pNewVertexData );
        for( DWORD iFace = 0; iFace < dwNumFaces; iFace++ )
        {
            DWORD iV1 = pInIndexData[ iFace * 3 + 0 ];
            DWORD iV2 = pInIndexData[ iFace * 3 + 1 ];
            DWORD iV3 = pInIndexData[ iFace * 3 + 2 ];

            pNewAttribData[ iFace ] = 0;

            D3DXVECTOR3 vA = pInVertexData[ iV1 ].pos - pInVertexData[ iV2 ].pos;
            D3DXVECTOR3 vB = pInVertexData[ iV1 ].pos - pInVertexData[ iV3 ].pos;
            D3DXVECTOR3 vFaceNormal;
            D3DXVec3Cross( &vFaceNormal, &vA, &vB );
            D3DXVec3Normalize( &vFaceNormal, &vFaceNormal );

            pNewIndexData[ iFace * 3 + 0 ] = iFace * 3 + 0;
            pNewVertexData[ iFace * 3 + 0 ].pos = pInVertexData[ iV1 ].pos;
            pNewVertexData[ iFace * 3 + 0 ].normal = vFaceNormal; // pInVertexData[ iV1 ].normal;
            pNewVertexData[ iFace * 3 + 0 ].uv = pInVertexData[ iV1 ].uv;

            pNewIndexData[ iFace * 3 + 1 ] = iFace * 3 + 1;
            pNewVertexData[ iFace * 3 + 1 ].pos = pInVertexData[ iV2 ].pos;
            pNewVertexData[ iFace * 3 + 1 ].normal = vFaceNormal; // pInVertexData[ iV2 ].normal;
            pNewVertexData[ iFace * 3 + 1 ].uv = pInVertexData[ iV2 ].uv;

            pNewIndexData[ iFace * 3 + 2 ] = iFace * 3 + 2;
            pNewVertexData[ iFace * 3 + 2 ].pos = pInVertexData[ iV3 ].pos;
            pNewVertexData[ iFace * 3 + 2 ].normal = vFaceNormal; // pInVertexData[ iV3 ].normal;
            pNewVertexData[ iFace * 3 + 2 ].uv = pInVertexData[ iV3 ].uv;
        }
        pNewMesh->UnlockIndexBuffer();
        pNewMesh->UnlockVertexBuffer();
        pNewMesh->UnlockAttributeBuffer();
    }
    pInMesh->UnlockAttributeBuffer();
    pInMesh->UnlockVertexBuffer();
    pInMesh->UnlockIndexBuffer();

    DWORD dwMaterials = 1;
    D3DXMATERIAL d3dxMaterials[1];
    d3dxMaterials[0].MatD3D = g_Render.meshDroid.m_pMaterials[0];
    d3dxMaterials[0].pTextureFilename = "media\\droid\\EvilDrone_Diff.jpg";

    hr = pOutMesh->Create( pd3dDevice, pNewMesh, d3dxMaterials, dwMaterials );
    SAFE_RELEASE( pNewMesh );

    return hr;
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

    V_RETURN( g_Render.DialogResourceManager.OnD3D9CreateDevice( pd3dDevice ) );

    // Initialize the font
    V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                              L"Arial", &g_Render.pFont ) );

    // Read the D3DX effect file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"Main.fx" ) );

    // If this fails, there should be debug output as to 
    // they the .fx file failed to compile
    V_RETURN( D3DXCreateEffectFromFile( pd3dDevice, str, NULL, NULL, D3DXFX_NOT_CLONEABLE,
                                        NULL, &g_Render.pEffect, NULL ) );

    // Obtain handles
    g_Render.hMatW = g_Render.pEffect->GetParameterByName( NULL, "g_mWorld" );
    g_Render.hMatV = g_Render.pEffect->GetParameterByName( NULL, "g_mView" );
    g_Render.hMatP = g_Render.pEffect->GetParameterByName( NULL, "g_mProj" );
    g_Render.hMatWV = g_Render.pEffect->GetParameterByName( NULL, "g_mWorldView" );
    g_Render.hMatWVP = g_Render.pEffect->GetParameterByName( NULL, "g_mWorldViewProj" );
    g_Render.hDiffuse = g_Render.pEffect->GetParameterByName( NULL, "g_matDiffuse" );
    g_Render.hSpecular = g_Render.pEffect->GetParameterByName( NULL, "g_matSpecular" );
    g_Render.hDiffuseTexture = g_Render.pEffect->GetParameterByName( NULL, "g_DiffuseTexture" );
    g_Render.hNormalMap = g_Render.pEffect->GetParameterByName( NULL, "g_NormalMapTexture" );
    g_Render.hPower = g_Render.pEffect->GetParameterByName( NULL, "g_matPower" );
    g_Render.hAnimation = g_Render.pEffect->GetParameterByName( NULL, "g_fAnimation" );

    // Create mesh
    WCHAR wsz[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( wsz, MAX_PATH, L"room\\droidroom.x" ) );
    g_Render.meshCell.Create( pd3dDevice, wsz );

    V_RETURN( DXUTFindDXSDKMediaFileCch( wsz, MAX_PATH, L"ammo.x" ) );
    g_Render.meshAmmo.Create( pd3dDevice, wsz );

    V_RETURN( DXUTFindDXSDKMediaFileCch( wsz, MAX_PATH, L"droid\\evildrone.x" ) );
    g_Render.meshDroid.Create( pd3dDevice, wsz );

    V_RETURN( DXUTFindDXSDKMediaFileCch( wsz, MAX_PATH, L"droid\\evildrone-low.x" ) );
    g_Render.meshDroidLow.Create( pd3dDevice, wsz );

    // Create a new vertex declaration to hold all the required data
    const D3DVERTEXELEMENT9 VertexDecl[] =
    {
        { 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
        { 0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,  0 },
        { 0, 36, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };

    // Make a new CDXUTMesh by splitting the droid mesh into seperate triangles with vertex normals set to the face normal
    SplitIntoSeperateTriangles( pd3dDevice, g_Render.meshDroid.GetMesh(), &g_Render.meshDroidExplosion );
    SplitIntoSeperateTriangles( pd3dDevice, g_Render.meshDroidLow.GetMesh(), &g_Render.meshDroidExplosionLow );

    g_Render.meshCell.SetVertexDecl( pd3dDevice, VertexDecl );
    g_Render.meshAmmo.SetVertexDecl( pd3dDevice, VertexDecl );
    g_Render.meshDroid.SetVertexDecl( pd3dDevice, VertexDecl );
    g_Render.meshDroidExplosion.SetVertexDecl( pd3dDevice, VertexDecl );
    g_Render.meshDroidLow.SetVertexDecl( pd3dDevice, VertexDecl );
    g_Render.meshDroidExplosionLow.SetVertexDecl( pd3dDevice, VertexDecl );

    // Create the 1x1 white default texture
    V_RETURN( pd3dDevice->CreateTexture( 1, 1, 1, 0, D3DFMT_A8R8G8B8,
                                         D3DPOOL_MANAGED, &g_Render.pDefaultTex, NULL ) );
    D3DLOCKED_RECT lr;
    V_RETURN( g_Render.pDefaultTex->LockRect( 0, &lr, NULL, 0 ) );
    *( LPDWORD )lr.pBits = D3DCOLOR_RGBA( 255, 255, 255, 255 );
    V_RETURN( g_Render.pDefaultTex->UnlockRect( 0 ) );

    // Create a 1x1 default normal map texture
    V_RETURN( pd3dDevice->CreateTexture( 1, 1, 1, 0, D3DFMT_A8R8G8B8,
                                         D3DPOOL_MANAGED, &g_Render.pDefaultNormalMap, NULL ) );
    V_RETURN( g_Render.pDefaultNormalMap->LockRect( 0, &lr, NULL, 0 ) );
    *( LPDWORD )lr.pBits = D3DCOLOR_RGBA( 127, 127, 255, 0 );
    V_RETURN( g_Render.pDefaultNormalMap->UnlockRect( 0 ) );

    // Compute the scaling matrix for the ball mesh.
    ComputeMeshScalingBox( g_Render.meshCell, &g_Render.mCellWorld, g_MinBound + D3DXVECTOR3( -0.2f, -0.2f, -0.2f ),
                           g_MaxBound + D3DXVECTOR3( 0.2f, 0.2f, 0.2f ) );
    ComputeMeshScaling( g_Render.meshAmmo, &g_Render.mAmmoWorld, AMMO_SIZE * 0.5f );
    ComputeMeshScaling( g_Render.meshDroid, &g_Render.mDroidWorld, DROID_SIZE * 0.5f );

    //
    // Initialize feature usage settings in the effect object according to the config flags.
    //

    // Rendering technique (FF or shader)
    SetEffectTechnique();
    g_Render.pEffect->SetBool( "g_bUseSpecular", !g_Render.DisableSpecular );
    g_Render.pEffect->SetBool( "g_bUseAnisotropic", g_Render.UseAnisotropicFilter );
    g_Render.pEffect->SetInt( "g_MinFilter", g_Render.UseAnisotropicFilter ? D3DTEXF_ANISOTROPIC : D3DTEXF_LINEAR );
    g_Render.pEffect->SetInt( "g_MaxAnisotropy", g_Render.UseAnisotropicFilter ? 8 : 1 );
    g_Render.pEffect->SetTexture( g_Render.hNormalMap, g_Render.pDefaultNormalMap );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Compute a matrix that scales Mesh to a specified size and centers around origin
//--------------------------------------------------------------------------------------
void ComputeMeshScaling( CDXUTXFileMesh& Mesh, D3DXMATRIX* pmScalingCenter, float fNewRadius )
{
    LPVOID pVerts = NULL;
    D3DXMatrixIdentity( pmScalingCenter );
    if( SUCCEEDED( Mesh.GetMesh()->LockVertexBuffer( 0, &pVerts ) ) )
    {
        D3DXVECTOR3 vCtr;
        float fRadius;
        if( SUCCEEDED( D3DXComputeBoundingSphere( ( const D3DXVECTOR3* )pVerts,
                                                  Mesh.GetMesh()->GetNumVertices(),
                                                  Mesh.GetMesh()->GetNumBytesPerVertex(),
                                                  &vCtr, &fRadius ) ) )
        {
            D3DXMatrixTranslation( pmScalingCenter, -vCtr.x, -vCtr.y, -vCtr.z );
            D3DXMATRIXA16 m;
            D3DXMatrixScaling( &m, fNewRadius / fRadius,
                               fNewRadius / fRadius,
                               fNewRadius / fRadius );
            D3DXMatrixMultiply( pmScalingCenter, pmScalingCenter, &m );
        }
        Mesh.GetMesh()->UnlockVertexBuffer();
    }
}


//--------------------------------------------------------------------------------------
// Compute a matrix that scales Mesh to a specified size and centers around origin
//--------------------------------------------------------------------------------------
void ComputeMeshScalingBox( CDXUTXFileMesh& Mesh, D3DXMATRIX* pmScalingCenter, D3DXVECTOR3 vNewMin,
                            D3DXVECTOR3 vNewMax )
{
    LPVOID pVerts = NULL;
    D3DXMatrixIdentity( pmScalingCenter );
    if( SUCCEEDED( Mesh.GetMesh()->LockVertexBuffer( 0, &pVerts ) ) )
    {
        D3DXVECTOR3 vCurMin;
        D3DXVECTOR3 vCurMax;
        D3DXVECTOR3 vCurCtr;
        D3DXVECTOR3 vNewCtr;

        if( SUCCEEDED( D3DXComputeBoundingBox( ( const D3DXVECTOR3* )pVerts,
                                               Mesh.GetMesh()->GetNumVertices(),
                                               Mesh.GetMesh()->GetNumBytesPerVertex(),
                                               &vCurMin, &vCurMax ) ) )
        {
            D3DXMATRIXA16 m;
            D3DXMatrixScaling( &m, ( vNewMax.x - vNewMin.x ) / ( vCurMax.x - vCurMin.x ),
                               ( vNewMax.y - vNewMin.y ) / ( vCurMax.y - vCurMin.y ),
                               ( vNewMax.z - vNewMin.z ) / ( vCurMax.z - vCurMin.z ) );

            vCurCtr = ( vCurMax + vCurMin ) / 2.0f;
            vNewCtr = ( vNewMax + vNewMin ) / 2.0f;

            D3DXMATRIX mShift;
            D3DXMatrixTranslation( &mShift, vNewCtr.x - vCurCtr.x,
                                   vNewCtr.y - vCurCtr.y,
                                   vNewCtr.z - vCurCtr.z );

            D3DXMatrixMultiply( pmScalingCenter, pmScalingCenter, &m );
        }
        Mesh.GetMesh()->UnlockVertexBuffer();
    }
}


//--------------------------------------------------------------------------------------
// Sets the effect
//--------------------------------------------------------------------------------------
void SetEffectTechnique()
{
    if( g_Render.UseFixedFunction )
    {
        g_Render.pEffect->SetTechnique( "FFRenderScene" );
    }
    else
    {
        // For programmable pipeline, check for shader version override (ForceShader).
        switch( g_Render.ForceShader )
        {
            case 9999:
                // 9999 is a code to mean "do not use shader"
                g_Render.hShaderTech = g_Render.pEffect->GetTechniqueByName( "FFRenderScene" );
                break;

            case 9998:
                // 9998 is a code to represent shader 2_a
                g_Render.hShaderTech = g_Render.pEffect->GetTechniqueByName( "RenderScenePS20A" );
                break;

            case 9997:
                // 9997 is a code to represent shader 2_b
                g_Render.hShaderTech = g_Render.pEffect->GetTechniqueByName( "RenderScenePS20B" );
                break;

            case 0:
                // 0 means use the highest valid technique.
                g_Render.pEffect->FindNextValidTechnique( NULL, &g_Render.hShaderTech );
                break;
        }

        g_Render.pEffect->SetTechnique( g_Render.hShaderTech );
    }
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

    V_RETURN( g_Render.DialogResourceManager.OnD3D9ResetDevice() );

    if( g_Render.pFont )
        V_RETURN( g_Render.pFont->OnResetDevice() );
    if( g_Render.pEffect )
        V_RETURN( g_Render.pEffect->OnResetDevice() );

    g_Render.meshCell.RestoreDeviceObjects( pd3dDevice );
    g_Render.meshAmmo.RestoreDeviceObjects( pd3dDevice );
    g_Render.meshDroid.RestoreDeviceObjects( pd3dDevice );
    g_Render.meshDroidExplosion.RestoreDeviceObjects( pd3dDevice );
    g_Render.meshDroidLow.RestoreDeviceObjects( pd3dDevice );
    g_Render.meshDroidExplosionLow.RestoreDeviceObjects( pd3dDevice );

    pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

    // Create a sprite to help batch calls when drawing many lines of text
    V_RETURN( D3DXCreateSprite( pd3dDevice, &g_Render.pTextSprite ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 0.1f, 1000.0f );

    if( DXUTIsWindowed() )
    {
        g_Camera.SetRotateButtons( 0, 0, true );
        g_Camera.SetResetCursorAfterMove( false );
    }
    else
    {
        g_Camera.SetRotateButtons( 0, 0, false, true );
        g_Camera.SetResetCursorAfterMove( true );
    }

    g_Render.MainMenuDlg.SetBackgroundColors( D3DCOLOR_ARGB( 200, 98, 138, 206 ),
                                              D3DCOLOR_ARGB( 200, 54, 105, 192 ),
                                              D3DCOLOR_ARGB( 200, 54, 105, 192 ),
                                              D3DCOLOR_ARGB( 200, 10, 73, 179 ) );
    g_Render.MainMenuDlg.SetLocation( ( pBackBufferSurfaceDesc->Width - 250 ) / 2,
                                      ( pBackBufferSurfaceDesc->Height - 300 ) / 2 );
    g_Render.MainMenuDlg.SetSize( 250, 300 );

    g_Render.AudioMenuDlg.SetBackgroundColors( D3DCOLOR_ARGB( 200, 98, 138, 206 ),
                                               D3DCOLOR_ARGB( 200, 54, 105, 192 ),
                                               D3DCOLOR_ARGB( 200, 54, 105, 192 ),
                                               D3DCOLOR_ARGB( 200, 10, 73, 179 ) );
    g_Render.AudioMenuDlg.SetLocation( ( pBackBufferSurfaceDesc->Width - 250 ) / 2,
                                       ( pBackBufferSurfaceDesc->Height - 300 ) / 2 );
    g_Render.AudioMenuDlg.SetSize( 250, 300 );

    g_Render.VideoMenuDlg.SetBackgroundColors( D3DCOLOR_ARGB( 200, 98, 138, 206 ),
                                               D3DCOLOR_ARGB( 200, 54, 105, 192 ),
                                               D3DCOLOR_ARGB( 200, 54, 105, 192 ),
                                               D3DCOLOR_ARGB( 200, 10, 73, 179 ) );
    g_Render.VideoMenuDlg.SetLocation( ( pBackBufferSurfaceDesc->Width - 250 ) / 2,
                                       ( pBackBufferSurfaceDesc->Height - 300 ) / 2 );
    g_Render.VideoMenuDlg.SetSize( 250, 300 );

    PlayBGMusic();

    DXUTSetCursorSettings( ( g_GameState.gameMode != GAME_RUNNING ), true );

    return S_OK;
}


//--------------------------------------------------------------------------------------
void FireAmmo()
{
    // Check to see if there are already MAX_AMMO balls in the world.
    // Remove the oldest ammo to make room for the newest if necessary.

    double fOldest = g_GameState.AmmoQ[0].fTimeCreated;
    int nOldestIndex = 0;
    int nInactiveIndex = -1;
    for( int iAmmo = 0; iAmmo < MAX_AMMO; iAmmo++ )
    {
        if( !g_GameState.AmmoQ[iAmmo].bActive )
        {
            nInactiveIndex = iAmmo;
            break;
        }
        if( g_GameState.AmmoQ[iAmmo].fTimeCreated < fOldest )
        {
            fOldest = g_GameState.AmmoQ[iAmmo].fTimeCreated;
            nOldestIndex = iAmmo;
        }
    }

    if( nInactiveIndex < 0 )
    {
        g_GameState.AmmoQ[nOldestIndex].bActive = false;
        g_GameState.nAmmoCount--;
        nInactiveIndex = nOldestIndex;
    }

    int nNewAmmoIndex = nInactiveIndex;

    // Get inverse view matrix
    D3DXMATRIXA16 mInvView;
    D3DXMatrixInverse( &mInvView, NULL, g_Camera.GetViewMatrix() );

    // Compute initial velocity in world space from camera space
    D3DXVECTOR4 InitialVelocity( 0.0f, 0.0f, 6.0f, 0.0f );
    D3DXVec4Transform( &InitialVelocity, &InitialVelocity, &mInvView );
    D3DXVECTOR4 InitialPosition( 0.0f, -0.15f, 0.0f, 1.0f );
    D3DXVec4Transform( &InitialPosition, &InitialPosition, &mInvView );

    PlayAudioCue( g_audioState.iAmmoFire );
    CreateAmmo( nNewAmmoIndex, InitialPosition, InitialVelocity );
}


//--------------------------------------------------------------------------------------
float GetDistFromWall( D3DXVECTOR3 P1, D3DXVECTOR3 P2, D3DXVECTOR3 P3, D3DXVECTOR3 N )
{
    // Plane equation: N dot (P-P3) = 0 where P3 and points P are on plane and N is plane normal 
    // Line equation: P = P1 + u(P2-P1) where points P on line pass through points P1 and P2
    // Intersection is when: N dot ((P1+u(P2-P1))-P3)=0 
    //                    or N dot (P1+u(P2-P1))=N dot P3
    //                    or u=[N dot (P3-P1)]/[N dot(P2-P1)]

    D3DXVECTOR3 P3minusP1;
    D3DXVECTOR3 P2minusP1;
    D3DXVec3Subtract( &P3minusP1, &P3, &P1 );
    D3DXVec3Subtract( &P2minusP1, &P2, &P1 );
    float a = D3DXVec3Dot( &N, &P3minusP1 );
    float b = D3DXVec3Dot( &N, &P2minusP1 );
    if( b < 0.01f )
    {
        float u = a / b;
        D3DXVECTOR3 PonLineInterestingPlane = P1 + u * P2minusP1;
        D3DXVECTOR3 P1ToP = P1 - PonLineInterestingPlane;
        float fDist = D3DXVec3Length( &P1ToP );

        return fDist;
    }
    else
    {
        return 9999999.0f;
    }
}


//--------------------------------------------------------------------------------------
void DroidPickNewDirection( int A )
{
    D3DXMATRIXA16 mRot;
    D3DXQUATERNION q;
    float fDistFromWalls = 0.0f;
    int nAngle = ( rand() % 360 );
    float fRotAngle = 0.0f;
    while( fDistFromWalls < 1.0f )
    {
        nAngle += 62; nAngle %= 360;
        fRotAngle = ( nAngle / 360.0f ) * D3DX_PI * 2.0f;
        D3DXQuaternionRotationYawPitchRoll( &q, fRotAngle, 0.0f, 0.0f );
        D3DXMatrixRotationQuaternion( &mRot, &q );
        D3DXVECTOR3 vVel = D3DXVECTOR3( mRot._31, mRot._32, mRot._33 );
        D3DXVECTOR3 vPos = g_GameState.DroidQ[A].vPosition;
        D3DXVECTOR3 vPosPlus = vPos + vVel;

        D3DXVECTOR3 vWallPtA = D3DXVECTOR3( g_MinBound.x, vPos.y, g_MinBound.z );
        D3DXVECTOR3 vWallPtB = D3DXVECTOR3( g_MaxBound.x, vPos.y, g_MinBound.z );
        D3DXVECTOR3 vWallPtC = D3DXVECTOR3( g_MinBound.x, vPos.y, g_MaxBound.z );
        D3DXVECTOR3 vWallPtD = D3DXVECTOR3( g_MaxBound.x, vPos.y, g_MaxBound.z );
        D3DXVECTOR3 vWallABNormal = D3DXVECTOR3( 0, 0, 1 );
        D3DXVECTOR3 vWallACNormal = D3DXVECTOR3( 1, 0, 0 );
        D3DXVECTOR3 vWallBDNormal = D3DXVECTOR3( 0, 0, -1 );
        D3DXVECTOR3 vWallCDNormal = D3DXVECTOR3( -1, 0, 0 );

        float fDistFromWallAB = GetDistFromWall( vPos, vPosPlus, vWallPtA, vWallABNormal );
        float fDistFromWallAC = GetDistFromWall( vPos, vPosPlus, vWallPtA, vWallACNormal );
        float fDistFromWallBD = GetDistFromWall( vPos, vPosPlus, vWallPtD, vWallBDNormal );
        float fDistFromWallCD = GetDistFromWall( vPos, vPosPlus, vWallPtD, vWallCDNormal );
        float fMin1 = min( fDistFromWallAB, fDistFromWallAC );
        float fMin2 = min( fDistFromWallBD, fDistFromWallCD );
        fDistFromWalls = min( fMin1, fMin2 );
    }

    g_GameState.DroidQ[A].fTargetRotation = fRotAngle;
    D3DXQuaternionRotationYawPitchRoll( &g_GameState.DroidQ[A].qTarget, g_GameState.DroidQ[A].fTargetRotation, 0.0f,
                                        0.0f );
    g_GameState.DroidQ[A].qStart = g_GameState.DroidQ[A].qCurrent;
    g_GameState.DroidQ[A].fRotInterp = 0.0f;
    g_GameState.DroidQ[A].aiState = AI_TURNING;
    g_GameState.DroidQ[A].vVelocity = D3DXVECTOR3( 0, 0, 0 );
    g_GameState.DroidQ[A].vNudgeVelocity = D3DXVECTOR3( 0, 0, 0 );
}


//--------------------------------------------------------------------------------------
void DroidChooseNewTask( int A )
{
    int nRand = rand() % 100;
    if( nRand < DROID_TURN_AI_PERCENT && g_GameState.DroidQ[A].aiState != AI_STOPPED )
    {
        DroidPickNewDirection( A );
    }
    else if( nRand < ( DROID_TURN_AI_PERCENT + DROID_TURN_AI_PERCENT ) || g_GameState.DroidQ[A].aiState == AI_STOPPED )
    {
        g_GameState.DroidQ[A].aiState = AI_MOVING;
        D3DXMATRIXA16 mRot;
        D3DXMatrixRotationQuaternion( &mRot, &g_GameState.DroidQ[A].qTarget );
        g_GameState.DroidQ[A].vVelocity = D3DXVECTOR3( mRot._31, mRot._32, mRot._33 ) * DROID_VELOCITY;
        g_GameState.DroidQ[A].fTaskTimer = ( ( rand() % DROID_MOVE_TIME_FLUX ) + DROID_MOVE_TIME_MIN ) / 1000.0f;
    }
    else
    {
        Play3DAudioCue( g_audioState.iDroidScan, &g_GameState.DroidQ[A].vPosition );
        g_GameState.DroidQ[A].aiState = AI_STOPPED;
        g_GameState.DroidQ[A].fTaskTimer = 1.5f;
        g_GameState.DroidQ[A].fRotInterp = 0.0f;
        g_GameState.DroidQ[A].qStart = g_GameState.DroidQ[A].qCurrent;
        g_GameState.DroidQ[A].vVelocity = D3DXVECTOR3( 0, 0, 0 );
    }
}


//--------------------------------------------------------------------------------------
void HandleDroidAI( float fElapsedTime )
{
    for( int A = 0; A < MAX_DROID; A++ )
    {
        if( !g_GameState.DroidQ[A].bActive )
            continue;

        if( g_GameState.DroidQ[A].nHitPoints > 0 )
        {
            if( g_GameState.DroidQ[A].fAlpha < 1.0f )
            {
                g_GameState.DroidQ[A].fAlpha += fElapsedTime;
                g_GameState.DroidQ[A].fAlpha = min( g_GameState.DroidQ[A].fAlpha, 1.0f );
            }

            switch( g_GameState.DroidQ[A].aiState )
            {
                case AI_TURNING:
                {
                    g_GameState.DroidQ[A].fRotInterp += fElapsedTime;
                    if( g_GameState.DroidQ[A].fRotInterp > 1.0f )
                        g_GameState.DroidQ[A].fRotInterp = 1.0f;
                    D3DXQuaternionSlerp( &g_GameState.DroidQ[A].qCurrent, &g_GameState.DroidQ[A].qStart,
                                         &g_GameState.DroidQ[A].qTarget, g_GameState.DroidQ[A].fRotInterp );

                    if( g_GameState.DroidQ[A].fRotInterp == 1.0f )
                    {
                        D3DXMATRIXA16 mRot;
                        D3DXMatrixRotationQuaternion( &mRot, &g_GameState.DroidQ[A].qCurrent );
                        g_GameState.DroidQ[A].vVelocity = D3DXVECTOR3( mRot._31, mRot._32, mRot._33 ) * DROID_VELOCITY;
                        g_GameState.DroidQ[A].aiState = AI_MOVING;
                    }
                    break;
                }

                case AI_MOVING:
                {
                    if( g_GameState.bDroidMove )
                        g_GameState.DroidQ[A].vPosition += g_GameState.DroidQ[A].vVelocity * fElapsedTime;

                    // Check bounce on front and back walls
                    if( g_GameState.DroidQ[A].vPosition.z < g_MinBound.z + ( DROID_SIZE * 0.6f ) )
                    {
                        g_GameState.DroidQ[A].vPosition.z = g_MinBound.z + ( DROID_SIZE * 0.6f );
                        DroidPickNewDirection( A );
                    }
                    if( g_GameState.DroidQ[A].vPosition.z > g_MaxBound.z - ( DROID_SIZE * 0.6f ) )
                    {
                        g_GameState.DroidQ[A].vPosition.z = g_MaxBound.z - ( DROID_SIZE * 0.6f );
                        DroidPickNewDirection( A );
                    }

                    // Check bounce on left and right walls
                    if( g_GameState.DroidQ[A].vPosition.x < g_MinBound.x + ( DROID_SIZE * 0.6f ) )
                    {
                        g_GameState.DroidQ[A].vPosition.x = g_MinBound.x + ( DROID_SIZE * 0.6f );
                        DroidPickNewDirection( A );
                    }
                    if( g_GameState.DroidQ[A].vPosition.x > g_MaxBound.x - ( DROID_SIZE * 0.6f ) )
                    {
                        g_GameState.DroidQ[A].vPosition.x = g_MaxBound.x - ( DROID_SIZE * 0.6f );
                        DroidPickNewDirection( A );
                    }

                    g_GameState.DroidQ[A].fTaskTimer -= fElapsedTime;
                    if( g_GameState.DroidQ[A].fTaskTimer < 0.0f )
                        DroidChooseNewTask( A );
                    break;
                }

                case AI_STOPPED:
                {
                    D3DXQUATERNION q;
                    g_GameState.DroidQ[A].fRotInterp += fElapsedTime * 10.0f;
                    if( g_GameState.DroidQ[A].fRotInterp > 4.0f * D3DX_PI )
                        g_GameState.DroidQ[A].fRotInterp = 4.0f * D3DX_PI;
                    float fRot = sinf( g_GameState.DroidQ[A].fRotInterp ) * 0.3f;
                    D3DXQuaternionRotationYawPitchRoll( &q, 0.0f, fRot, 0.0f );
                    D3DXQuaternionMultiply( &g_GameState.DroidQ[A].qCurrent, &q, &g_GameState.DroidQ[A].qStart );

                    g_GameState.DroidQ[A].fTaskTimer -= fElapsedTime;
                    if( g_GameState.DroidQ[A].fTaskTimer < 0.0f )
                        DroidChooseNewTask( A );
                    break;
                }
            }

            // Nudge droid and keep within bounds
            g_GameState.DroidQ[A].vPosition += g_GameState.DroidQ[A].vNudgeVelocity * fElapsedTime;
            if( g_GameState.DroidQ[A].vPosition.z < g_MinBound.z + ( DROID_SIZE * 0.6f ) )
                g_GameState.DroidQ[A].vPosition.z = g_MinBound.z + ( DROID_SIZE * 0.6f );
            if( g_GameState.DroidQ[A].vPosition.z > g_MaxBound.z - ( DROID_SIZE * 0.6f ) )
                g_GameState.DroidQ[A].vPosition.z = g_MaxBound.z - ( DROID_SIZE * 0.6f );
            if( g_GameState.DroidQ[A].vPosition.x < g_MinBound.x + ( DROID_SIZE * 0.6f ) )
                g_GameState.DroidQ[A].vPosition.x = g_MinBound.x + ( DROID_SIZE * 0.6f );
            if( g_GameState.DroidQ[A].vPosition.x > g_MaxBound.x - ( DROID_SIZE * 0.6f ) )
                g_GameState.DroidQ[A].vPosition.x = g_MaxBound.x - ( DROID_SIZE * 0.6f );
            if( g_GameState.DroidQ[A].vPosition.y < -GROUND_Y + ( DROID_SIZE * 0.6f ) )
                g_GameState.DroidQ[A].vPosition.y = -GROUND_Y + ( DROID_SIZE * 0.6f );
            if( g_GameState.DroidQ[A].vPosition.y > GROUND_Y - ( DROID_SIZE * 0.6f ) )
                g_GameState.DroidQ[A].vPosition.y = GROUND_Y - ( DROID_SIZE * 0.6f );

            // Apply drag to the nudge velocity
            g_GameState.DroidQ[A].vNudgeVelocity -= g_GameState.DroidQ[A].vNudgeVelocity * ( fElapsedTime * 20.0f );
        }
        else
        {
            // Hitpoints are gone, so animate explosion
            g_GameState.DroidQ[A].fDeathAnimation += fElapsedTime * DROID_DEATH_SPEED;
            if( g_GameState.DroidQ[A].fDeathAnimation > 1.0f )
            {
                if( g_GameState.DroidQ[A].fAlpha > 0.0f )
                {
                    g_GameState.DroidQ[A].fAlpha -= fElapsedTime * DROID_DEATH_SPEED;
                    g_GameState.DroidQ[A].fAlpha = max( g_GameState.DroidQ[A].fAlpha, 0.0f );
                }
                else
                {
                    g_GameState.DroidQ[A].bActive = false;
                    g_GameState.nDroidCount--;
                    SetNumDroidsForAudio( g_GameState.nDroidCount );
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------
void CheckForAmmoToDroidCollision( int A )
{
    for( int B = 0; B < MAX_DROID; B++ )
    {
        if( !g_GameState.DroidQ[B].bActive || g_GameState.DroidQ[B].nHitPoints <= 0 )
            continue;

        // Check collision between instances A and B.
        // vAToB is the collision normal vector.
        D3DXVECTOR3 vAToB = - g_GameState.AmmoQ[A].vPosition + g_GameState.DroidQ[B].vPosition;
        float DistSq = D3DXVec3LengthSq( &vAToB );
        if( DistSq < ( ( AMMO_SIZE + DROID_SIZE ) / 2.0f ) * ( ( AMMO_SIZE + DROID_SIZE ) / 2.0f ) )
        {
            D3DXVec3Normalize( &vAToB, &vAToB );

            // Check if the B instances are already moving away from each other.
            // If so, skip collision.  This can happen when a lot of instances are
            // bunched up next to each other.
            float fImpact = D3DXVec3Dot( &vAToB, &g_GameState.AmmoQ[A].vVelocity ) -
                D3DXVec3Dot( &vAToB, &g_GameState.DroidQ[B].vVelocity );
            if( fImpact > 0.0f )
            {
                g_GameState.AmmoQ[A].bActive = false;
                g_GameState.DroidQ[B].vNudgeVelocity += g_GameState.AmmoQ[A].vVelocity * 0.3f;

                g_GameState.DroidQ[B].nHitPoints--;
                if( g_GameState.DroidQ[B].nHitPoints <= 0 )
                {
                    g_GameState.DroidQ[B].fDeathAnimation = 0.0f;
                    Play3DAudioCue( g_audioState.iDroidDestroyed, &g_GameState.DroidQ[B].vPosition );
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------
void CheckForInterAmmoCollision( float fElapsedTime )
{
    if( g_GameState.nAmmoCount <= 1 )
        return;

    for( int A = 0; A < MAX_AMMO - 1; A++ )
    {
        if( !g_GameState.AmmoQ[A].bActive )
            continue;

        for( int B = A + 1; B < MAX_AMMO; B++ )
        {
            if( !g_GameState.AmmoQ[B].bActive )
                continue;

            // Check collision between instances A and B.
            // vAToB is the collision normal vector.
            D3DXVECTOR3 vAToB = g_GameState.AmmoQ[B].vPosition - g_GameState.AmmoQ[A].vPosition;
            float DistSq = D3DXVec3LengthSq( &vAToB );
            if( DistSq < AMMO_SIZE * AMMO_SIZE )
            {
                D3DXVec3Normalize( &vAToB, &vAToB );

                // Check if the B instances are already moving away from each other.
                // If so, skip collision.  This can happen when a lot of instances are
                // bunched up next to each other.
                float fImpact = D3DXVec3Dot( &vAToB, &g_GameState.AmmoQ[A].vVelocity ) -
                    D3DXVec3Dot( &vAToB, &g_GameState.AmmoQ[B].vVelocity );
                if( fImpact > 0.0f )
                {
                    // Compute the normal and tangential compAnts of A's velocity.
                    D3DXVECTOR3 vVelocityAN = ( 1 - BOUNCE_LOST ) *
                        D3DXVec3Dot( &vAToB, &g_GameState.AmmoQ[A].vVelocity ) * vAToB;
                    D3DXVECTOR3 vVelocityAT = ( 1 - BOUNCE_LOST ) * g_GameState.AmmoQ[A].vVelocity - vVelocityAN;

                    // Compute the normal and tangential compAnts of B's velocity.
                    D3DXVECTOR3 vVelocityBN = ( 1 - BOUNCE_LOST ) *
                        D3DXVec3Dot( &vAToB, &g_GameState.AmmoQ[B].vVelocity ) * vAToB;
                    D3DXVECTOR3 vVelocityBT = ( 1 - BOUNCE_LOST ) * g_GameState.AmmoQ[B].vVelocity - vVelocityBN;

                    // Compute post-collision velocity
                    g_GameState.AmmoQ[A].vVelocity = vVelocityAT - vVelocityAN * ( 1 - BOUNCE_TRANSFER ) +
                        vVelocityBN * BOUNCE_TRANSFER;
                    g_GameState.AmmoQ[B].vVelocity = vVelocityBT - vVelocityBN * ( 1 - BOUNCE_TRANSFER ) +
                        vVelocityAN * BOUNCE_TRANSFER;

                    // Fix the positions so that the B balls are exactly AMMO_SIZE apart.
                    float fDistanceToMove = ( AMMO_SIZE - sqrtf( DistSq ) ) * 0.5f;
                    g_GameState.AmmoQ[A].vPosition -= vAToB * fDistanceToMove;
                    g_GameState.AmmoQ[B].vPosition += vAToB * fDistanceToMove;

                    // Flag the B instances so that they are not laying on ground.
                    g_GameState.AmmoQ[A].bGround = g_GameState.AmmoQ[B].bGround = false;

                    // Play sound
                    //                  g_GameState.AmmoQ[A].PlaySound( fImpact );
                    //                  g_GameState.AmmoQ[B].PlaySound( fImpact );
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------
void CheckForAmmoToWallCollision( int A )
{
    // Check bounce on ground
    if( !g_GameState.AmmoQ[A].bGround )
    {
        if( g_GameState.AmmoQ[A].vPosition.y < -GROUND_Y + ( AMMO_SIZE * 0.5f ) )
        {
            g_GameState.AmmoQ[A].vPosition.y = -GROUND_Y + ( AMMO_SIZE * 0.5f );
            g_GameState.AmmoQ[A].vVelocity.y = -g_GameState.AmmoQ[A].vVelocity.y * ( 1 - GROUND_ABSORBANCE );
            //g_GameState.AmmoQ[A].PlaySound( -g_GameState.AmmoQ[A].vVelocity.y );

            // X and Z velocity are reduced because of friction.
            g_GameState.AmmoQ[A].vVelocity.x *= 0.9f;
            g_GameState.AmmoQ[A].vVelocity.z *= 0.9f;
            PlayAudioCue( g_audioState.iAmmoBounce );
        }
    }
    else
    {
        // Ball is resting or rolling on ground.
        // X and Z velocity are reduced because of friction.
        g_GameState.AmmoQ[A].vVelocity.x *= 0.9f;
        g_GameState.AmmoQ[A].vVelocity.z *= 0.9f;
    }

    // Check bounce on ceiling
    if( g_GameState.AmmoQ[A].vPosition.y > GROUND_Y - ( AMMO_SIZE * 0.5f ) )
    {
        g_GameState.AmmoQ[A].vPosition.y = GROUND_Y - ( AMMO_SIZE * 0.5f );
        g_GameState.AmmoQ[A].vVelocity.y = -g_GameState.AmmoQ[A].vVelocity.y * ( 1 - GROUND_ABSORBANCE );
        //g_GameState.AmmoQ[A].PlaySound( g_GameState.AmmoQ[A].vVelocity.y );

        // X and Z velocity are reduced because of friction.
        g_GameState.AmmoQ[A].vVelocity.x *= 0.9f;
        g_GameState.AmmoQ[A].vVelocity.z *= 0.9f;
        PlayAudioCue( g_audioState.iAmmoBounce );
    }

    // If the Y direction motion is below a certain threshold, flag the instance as laying on the ground.
    if( GRAVITY * ( g_GameState.AmmoQ[A].vPosition.y + GROUND_Y - ( AMMO_SIZE * 0.5f ) ) + 0.5f *
        g_GameState.AmmoQ[A].vVelocity.y * g_GameState.AmmoQ[A].vVelocity.y < REST_THRESHOLD )
    {
        g_GameState.AmmoQ[A].vPosition.y = -GROUND_Y + ( AMMO_SIZE * 0.5f );
        g_GameState.AmmoQ[A].vVelocity.y = 0.0f;
        g_GameState.AmmoQ[A].bGround = true;
    }

    // Check bounce on front and back walls
    if( g_GameState.AmmoQ[A].vPosition.z < g_MinBound.z + ( AMMO_SIZE * 0.5f ) )
    {
        g_GameState.AmmoQ[A].vPosition.z = g_MinBound.z + ( AMMO_SIZE * 0.5f );
        g_GameState.AmmoQ[A].vVelocity.z = -g_GameState.AmmoQ[A].vVelocity.z * ( 1 - GROUND_ABSORBANCE );
        PlayAudioCue( g_audioState.iAmmoBounce );
    }
    if( g_GameState.AmmoQ[A].vPosition.z > g_MaxBound.z - ( AMMO_SIZE * 0.5f ) )
    {
        g_GameState.AmmoQ[A].vPosition.z = g_MaxBound.z - ( AMMO_SIZE * 0.5f );
        g_GameState.AmmoQ[A].vVelocity.z = -g_GameState.AmmoQ[A].vVelocity.z * ( 1 - GROUND_ABSORBANCE );
        PlayAudioCue( g_audioState.iAmmoBounce );
    }

    // Check bounce on left and right walls
    if( g_GameState.AmmoQ[A].vPosition.x < g_MinBound.x + ( AMMO_SIZE * 0.5f ) )
    {
        g_GameState.AmmoQ[A].vPosition.x = g_MinBound.x + ( AMMO_SIZE * 0.5f );
        g_GameState.AmmoQ[A].vVelocity.x = -g_GameState.AmmoQ[A].vVelocity.x * ( 1 - GROUND_ABSORBANCE );
        PlayAudioCue( g_audioState.iAmmoBounce );
    }
    if( g_GameState.AmmoQ[A].vPosition.x > g_MaxBound.x - ( AMMO_SIZE * 0.5f ) )
    {
        g_GameState.AmmoQ[A].vPosition.x = g_MaxBound.x - ( AMMO_SIZE * 0.5f );
        g_GameState.AmmoQ[A].vVelocity.x = -g_GameState.AmmoQ[A].vVelocity.x * ( 1 - GROUND_ABSORBANCE );
        PlayAudioCue( g_audioState.iAmmoBounce );
    }
}


//--------------------------------------------------------------------------------------
void HandleAmmoAI( float fElapsedTime )
{
    if( g_GameState.nAmmoCount == 0 )
        return;

    CheckForInterAmmoCollision( fElapsedTime );

    // Apply gravity and check for collision against ground and walls.
    for( int A = 0; A < MAX_AMMO; A++ )
    {
        if( !g_GameState.AmmoQ[A].bActive )
            continue;

        g_GameState.AmmoQ[A].fLifeCountdown -= fElapsedTime;
        if( g_GameState.AmmoQ[A].fLifeCountdown < 1.0f / 0.5f )
        {
            g_GameState.AmmoQ[A].fAlpha = g_GameState.AmmoQ[A].fLifeCountdown * 0.5f;
        }

        if( g_GameState.AmmoQ[A].fLifeCountdown < 0.0f )
        {
            g_GameState.AmmoQ[A].bActive = false;
            g_GameState.nAmmoCount--;
            continue;
        }

        // If the elapsed time is too long, we slice up the time and
        // handle physics over several passes.
        float fTimeLeft = fElapsedTime;
        float fElapsedFrameTime;

        while( fTimeLeft > 0.0f )
        {
            fElapsedFrameTime = min( fTimeLeft, PHYSICS_FRAMELENGTH );
            fTimeLeft -= fElapsedFrameTime;

            g_GameState.AmmoQ[A].vPosition += g_GameState.AmmoQ[A].vVelocity * fElapsedFrameTime;

            g_GameState.AmmoQ[A].vVelocity.x -= g_GameState.AmmoQ[A].vVelocity.x * 0.1f * fElapsedFrameTime;
            g_GameState.AmmoQ[A].vVelocity.z -= g_GameState.AmmoQ[A].vVelocity.z * 0.1f * fElapsedFrameTime;

            // Apply gravity if the ball is not resting on the ground.
            if( !g_GameState.AmmoQ[A].bGround )
                g_GameState.AmmoQ[A].vVelocity.y -= GRAVITY * fElapsedFrameTime;

            CheckForAmmoToWallCollision( A );
            CheckForAmmoToDroidCollision( A );
        }
    }
}


//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    for( DWORD iPort = 0; iPort < DXUT_MAX_CONTROLLERS; iPort++ )
        DXUTGetGamepadState( iPort, &g_GameState.gamePad[iPort], true, true );

    for( DWORD iPort = 0; iPort < DXUT_MAX_CONTROLLERS; iPort++ )
    {
        if( g_GameState.gamePad[iPort].bConnected )
        {
            if( g_GameState.gamePad[iPort].wPressedButtons & XINPUT_GAMEPAD_START )
            {
                ToggleMenu();
                break;
            }
        }
    }

    if( g_GameState.gameMode != GAME_RUNNING )
        return;

    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );

    UpdateAudio();

    // Update the ammo color occassionally
    g_GameState.fAmmoColorLerp += fElapsedTime / 2.0f;
    if( g_GameState.fAmmoColorLerp > 1.0f )
    {
        g_GameState.fAmmoColorLerp = 0.0f;
        g_GameState.BlendFromColor = g_GameState.BlendToColor;
        switch( rand() % 5 )
        {
            case 0:
                g_GameState.BlendToColor = D3DXCOLOR( 0.6f, 0, 0, 1 ); break;
            case 1:
                g_GameState.BlendToColor = D3DXCOLOR( 0, 0.6f, 0, 1 ); break;
            case 2:
                g_GameState.BlendToColor = D3DXCOLOR( 0, 0, 0.6f, 1 ); break;
            case 3:
                g_GameState.BlendToColor = D3DXCOLOR( 0, 0, 0, 1 ); break;
            case 4:
                g_GameState.BlendToColor = D3DXCOLOR( 0.6f, 0.6f, 0, 1 ); break;
        }
    }

    // Check mouse/xinput and shoot ammo if triggered
    bool bFire = DXUTIsMouseButtonDown( VK_LBUTTON );
    for( DWORD iPort = 0; iPort < DXUT_MAX_CONTROLLERS; iPort++ )
    {
        if( g_GameState.gamePad[iPort].bConnected )
        {
            if( ( g_GameState.gamePad[iPort].wButtons & ( XINPUT_GAMEPAD_LEFT_SHOULDER |
                                                          XINPUT_GAMEPAD_RIGHT_SHOULDER | XINPUT_GAMEPAD_A ) ) ||
                g_GameState.gamePad[iPort].bLeftTrigger || g_GameState.gamePad[iPort].bRightTrigger )
            {
                bFire = true;
            }
        }
    }

    if( DXUTIsKeyDown( VK_SPACE ) )
        bFire = true;

    if( bFire )
    {
        static float fLastFired = 0.0f;
        if( fTime - fLastFired >= AUTOFIRE_DELAY )
        {
            FireAmmo();
            fLastFired = ( float )fTime;
        }
    }

    // Check for droid move toggle
    for( DWORD iPort = 0; iPort < DXUT_MAX_CONTROLLERS; iPort++ )
    {
        if( g_GameState.gamePad[iPort].bConnected )
        {
            if( g_GameState.gamePad[iPort].wPressedButtons & XINPUT_GAMEPAD_Y )
            {
                g_GameState.bDroidMove = !g_GameState.bDroidMove;
            }
        }
    }

    // Check for droid add 
    for( DWORD iPort = 0; iPort < DXUT_MAX_CONTROLLERS; iPort++ )
    {
        if( g_GameState.gamePad[iPort].bConnected )
        {
            if( g_GameState.gamePad[iPort].wPressedButtons & XINPUT_GAMEPAD_X )
            {
                if( g_GameState.nDroidCount < MAX_DROID )
                    g_GameState.bDroidCreate = true;
            }
        }
    }

    // Check for droid add 
    for( DWORD iPort = 0; iPort < DXUT_MAX_CONTROLLERS; iPort++ )
    {
        if( g_GameState.gamePad[iPort].bConnected )
        {
            if( g_GameState.gamePad[iPort].wButtons & XINPUT_GAMEPAD_X )
            {
                static float fLastDroidCreateTime = 0.0f;
                if( fTime - fLastDroidCreateTime >= 0.25f )
                {
                    if( g_GameState.nDroidCount < MAX_DROID )
                        g_GameState.bDroidCreate = true;
                    fLastDroidCreateTime = ( float )fTime;
                }
            }
        }
    }

    // Check for mass droid kill
    for( DWORD iPort = 0; iPort < DXUT_MAX_CONTROLLERS; iPort++ )
    {
        if( g_GameState.gamePad[iPort].bConnected )
        {
            if( g_GameState.gamePad[iPort].wPressedButtons & XINPUT_GAMEPAD_B )
            {
                g_GameState.bMassDroidKill = true;
            }
        }
    }

    // Slowly re-add the droids
    if( g_GameState.bAutoAddDroids && g_GameState.nDroidCount < g_GameState.nMaxDroids )
    {
        if( !g_GameState.bDroidCreate )
        {
            g_GameState.fDroidCreateCountdown = ( rand() % DROID_CREATE_DELAY_FLUX ) / 1000.0f;
            g_GameState.bDroidCreate = true;
        }
    }

    if( g_GameState.bDroidCreate )
    {
        g_GameState.fDroidCreateCountdown -= fElapsedTime;
        if( g_GameState.fDroidCreateCountdown < 0.0f )
        {
            CreateDroid();
            g_GameState.bDroidCreate = false;
        }
    }

    if( g_GameState.bMassDroidKill )
    {
        for( int B = 0; B < MAX_DROID; B++ )
        {
            if( !g_GameState.DroidQ[B].bActive || g_GameState.DroidQ[B].nHitPoints <= 0 )
                continue;

            g_GameState.DroidQ[B].nHitPoints = 0;
            g_GameState.DroidQ[B].fDeathAnimation = 0.0f;
            Play3DAudioCue( g_audioState.iDroidDestroyed, &g_GameState.DroidQ[B].vPosition );
        }
        g_GameState.bMassDroidKill = false;
    }

    HandleAmmoAI( fElapsedTime );
    HandleDroidAI( fElapsedTime );
}


//--------------------------------------------------------------------------------------
void CreateAmmo( int nIndex, D3DXVECTOR4 Pos, D3DXVECTOR4 Vel )
{
    D3DXMatrixRotationYawPitchRoll( &g_GameState.AmmoQ[nIndex].mAmmoRot,
                                    rand() % 360 * D3DX_PI / 180.0f,
                                    rand() % 360 * D3DX_PI / 180.0f,
                                    rand() % 360 * D3DX_PI / 180.0f );

    g_GameState.AmmoQ[nIndex].bActive = true;
    g_GameState.AmmoQ[nIndex].fTimeCreated = DXUTGetTime();
    g_GameState.AmmoQ[nIndex].fLifeCountdown = 5.0f;
    g_GameState.AmmoQ[nIndex].fAlpha = 1.0f;
    g_GameState.AmmoQ[nIndex].bGround = false;
    g_GameState.AmmoQ[nIndex].vPosition = ( D3DXVECTOR3 )Pos;
    g_GameState.AmmoQ[nIndex].vVelocity = ( D3DXVECTOR3 )Vel;
    g_GameState.nAmmoCount++;

    D3DXCOLOR cBlendColor;
    D3DXColorLerp( &cBlendColor, &g_GameState.BlendFromColor, &g_GameState.BlendToColor, g_GameState.fAmmoColorLerp );
    g_GameState.AmmoQ[nIndex].Diffuse = ( D3DXVECTOR4 )cBlendColor;
}


//--------------------------------------------------------------------------------------
void RenderAmmo( int A, D3DXMATRIXA16& mView, D3DXMATRIXA16& mProj )
{
    HRESULT hr;
    D3DXMATRIXA16 mWorldView;
    D3DXMATRIXA16 mWorldViewProjection;
    D3DXMATRIXA16 mWorld;
    D3DXMatrixTranslation( &mWorld, g_GameState.AmmoQ[A].vPosition.x,
                           g_GameState.AmmoQ[A].vPosition.y,
                           g_GameState.AmmoQ[A].vPosition.z );
    mWorld = g_GameState.AmmoQ[A].mAmmoRot * g_Render.mAmmoWorld * mWorld;
    mWorldViewProjection = mWorld * mView * mProj;
    mWorldView = mWorld * mView;
    V( g_Render.pEffect->SetMatrix( g_Render.hMatWVP, &mWorldViewProjection ) );
    V( g_Render.pEffect->SetMatrix( g_Render.hMatWV, &mWorldView ) );
    V( g_Render.pEffect->SetMatrix( g_Render.hMatW, &mWorld ) );

    g_Render.pEffect->SetTexture( g_Render.hDiffuseTexture, g_Render.pDefaultTex );
    g_GameState.AmmoQ[A].Diffuse.w = g_GameState.AmmoQ[A].fAlpha;
    g_Render.pEffect->SetVector( g_Render.hDiffuse, ( D3DXVECTOR4* )&g_GameState.AmmoQ[A].Diffuse );
    g_Render.pEffect->SetBool( "g_bUseSpecular", !g_Render.DisableSpecular );

    g_Render.meshAmmo.Render( g_Render.pEffect, 0, NULL, 0, g_Render.hSpecular, 0, g_Render.hPower, true, false );
}


//--------------------------------------------------------------------------------------
void CreateDroid()
{
    int A;
    for( A = 0; A < MAX_DROID; ++A )
    {
        if( !g_GameState.DroidQ[A].bActive )
            break;
    }
    if( A == MAX_DROID )
        return;

    D3DXVECTOR3 Pos;

    for(; ; )
    {
        float fX = ( rand() % 100 ) / 100.0f;
        float fZ = ( rand() % 100 ) / 100.0f;

        Pos = D3DXVECTOR3( g_MinBound.x + fX * ( g_MaxBound.x - g_MinBound.x ),
                           -GROUND_Y + DROID_MIN_HEIGHT + ( ( rand() % 100 ) / 100.0f ) * DROID_HEIGHT_FLUX,
                           g_MinBound.z + fZ * ( g_MaxBound.z - g_MinBound.z ) );

        // Position the droids apart from each other
        bool bInterset = false;
        for( int B = 0; B < MAX_DROID; B++ )
        {
            if( !g_GameState.DroidQ[B].bActive )
                continue;
            D3DXVECTOR3 vAToB = Pos - g_GameState.DroidQ[B].vPosition;
            float Dist = D3DXVec3Length( &vAToB );
            if( Dist < 2.0f * DROID_SIZE )
            {
                bInterset = true;
                break;
            }
        }

        if( !bInterset ) break;
    }

    g_GameState.DroidQ[A].bActive = true;
    g_GameState.DroidQ[A].vPosition = Pos;

    float fStartRotation = ( rand() % 360 ) / 360.0f * D3DX_PI * 2.0f;
    g_GameState.DroidQ[A].fTargetRotation = fStartRotation;
    D3DXQuaternionRotationYawPitchRoll( &g_GameState.DroidQ[A].qTarget, g_GameState.DroidQ[A].fTargetRotation, 0.0f,
                                        0.0f );
    g_GameState.DroidQ[A].qStart = g_GameState.DroidQ[A].qTarget;
    g_GameState.DroidQ[A].qCurrent = g_GameState.DroidQ[A].qTarget;
    g_GameState.DroidQ[A].fRotInterp = 0.0f;
    g_GameState.DroidQ[A].aiState = AI_MOVING;
    g_GameState.DroidQ[A].fTaskTimer = 0.0f;
    g_GameState.DroidQ[A].nHitPoints = DROID_HITPOINTS;
    g_GameState.DroidQ[A].fDeathAnimation = 0.0f;
    g_GameState.DroidQ[A].fAlpha = 0.0f;
    D3DXMATRIXA16 mRot;
    D3DXMatrixRotationQuaternion( &mRot, &g_GameState.DroidQ[A].qTarget );
    g_GameState.DroidQ[A].vVelocity = D3DXVECTOR3( mRot._31, mRot._32, mRot._33 ) * DROID_VELOCITY;

    // Give the droids different specular tints
    float C1 = 0.75f;
    float C2 = 0.35f;
    switch( rand() % 7 )
    {
        case 0:
            g_GameState.DroidQ[A].Specular = D3DXCOLOR( C1, C1, C2, 1.0f ); break;
        case 1:
            g_GameState.DroidQ[A].Specular = D3DXCOLOR( C1, C2, C1, 1.0f ); break;
        case 2:
            g_GameState.DroidQ[A].Specular = D3DXCOLOR( C1, C2, C2, 1.0f ); break;
        case 3:
            g_GameState.DroidQ[A].Specular = D3DXCOLOR( C2, C1, C1, 1.0f ); break;
        case 4:
            g_GameState.DroidQ[A].Specular = D3DXCOLOR( C2, C1, C2, 1.0f ); break;
        case 5:
            g_GameState.DroidQ[A].Specular = D3DXCOLOR( C2, C2, C1, 1.0f ); break;
        case 6:
            g_GameState.DroidQ[A].Specular = D3DXCOLOR( C2, C2, C2, 1.0f ); break;
    }

    g_GameState.nDroidCount++;
    SetNumDroidsForAudio( g_GameState.nDroidCount );
}


//--------------------------------------------------------------------------------------
void RenderDroid( IDirect3DDevice9* pd3dDevice, int A, D3DXMATRIXA16& mView, D3DXMATRIXA16& mProj, bool bExplode )
{
    HRESULT hr;
    D3DXMATRIXA16 mWorldView;
    D3DXMATRIXA16 mWorldViewProjection;
    D3DXMATRIXA16 mWorld, mRot;
    D3DXMatrixTranslation( &mWorld, g_GameState.DroidQ[A].vPosition.x,
                           g_GameState.DroidQ[A].vPosition.y,
                           g_GameState.DroidQ[A].vPosition.z );
    D3DXMatrixRotationQuaternion( &mRot, &g_GameState.DroidQ[A].qCurrent );

    mWorld = g_Render.mDroidWorld * mRot * mWorld;
    mWorldView = mWorld * mView;
    mWorldViewProjection = mWorldView * mProj;
    V( g_Render.pEffect->SetMatrix( g_Render.hMatWVP, &mWorldViewProjection ) );
    V( g_Render.pEffect->SetMatrix( g_Render.hMatWV, &mWorldView ) );
    V( g_Render.pEffect->SetMatrix( g_Render.hMatW, &mWorld ) );

    D3DXVECTOR4 Diffuse = D3DXVECTOR4( 1, 1, 1, 1 );
    g_Render.pEffect->SetVector( g_Render.hDiffuse, ( D3DXVECTOR4* )&Diffuse );
    g_Render.pEffect->SetBool( "g_bUseSpecular", 1 );

    D3DXVECTOR4 vDiffuse = D3DXVECTOR4( 1, 1, 1, g_GameState.DroidQ[A].fAlpha );
    g_Render.pEffect->SetVector( g_Render.hDiffuse, ( D3DXVECTOR4* )&vDiffuse );
    V( g_Render.pEffect->SetFloat( g_Render.hAnimation, g_GameState.DroidQ[A].fDeathAnimation ) );

    V( g_Render.pEffect->SetVector( g_Render.hSpecular, ( D3DXVECTOR4* )&g_GameState.DroidQ[A].Specular ) );
    V( g_Render.pEffect->SetFloat( g_Render.hPower, 16.5f ) );

    if( g_Render.pDroidNormalMap )
        g_Render.pEffect->SetTexture( g_Render.hNormalMap, g_Render.pDroidNormalMap );

    if( g_Render.UseHighResModels )
    {
        if( bExplode )
            g_Render.meshDroidExplosion.Render( g_Render.pEffect, g_Render.hDiffuseTexture, NULL, 0, NULL, 0, NULL,
                                                true, false );
        else
            g_Render.meshDroid.Render( g_Render.pEffect, g_Render.hDiffuseTexture, NULL, 0, NULL, 0, NULL, true,
                                       false );
    }
    else
    {
        if( bExplode )
            g_Render.meshDroidExplosionLow.Render( g_Render.pEffect, g_Render.hDiffuseTexture, NULL, 0, NULL, 0, NULL,
                                                   true, false );
        else
            g_Render.meshDroidLow.Render( g_Render.pEffect, g_Render.hDiffuseTexture, NULL, 0, NULL, 0, NULL, true,
                                          false );
    }

    g_Render.pEffect->SetTexture( g_Render.hNormalMap, g_Render.pDefaultNormalMap );
    V( g_Render.pEffect->SetFloat( g_Render.hAnimation, 0.0f ) );
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

    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;
    D3DXMATRIXA16 mWorldView;
    D3DXMATRIXA16 mWorldViewProjection;

    // Clear the render target and the zbuffer 
    V( pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 45, 50, 170 ), 1.0f, 0 ) );

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
        // Get the projection & view matrix from the camera class
        mProj = *g_Camera.GetProjMatrix();
        mView = *g_Camera.GetViewMatrix();

        mWorldViewProjection = g_Render.mCellWorld * mView * mProj;
        mWorldView = g_Render.mCellWorld * mView;

        // Update the effect's variables.  Instead of using strings, it would 
        // be more efficient to cache a handle to the parameter by calling 
        // ID3DXEffect::GetParameterByName
        V( g_Render.pEffect->SetMatrix( g_Render.hMatV, &mView ) );
        V( g_Render.pEffect->SetMatrix( g_Render.hMatP, &mProj ) );
        V( g_Render.pEffect->SetMatrix( g_Render.hMatWVP, &mWorldViewProjection ) );
        V( g_Render.pEffect->SetMatrix( g_Render.hMatWV, &mWorldView ) );
        V( g_Render.pEffect->SetMatrix( g_Render.hMatW, &g_Render.mCellWorld ) );
        g_Render.pEffect->SetBool( "g_bUseSpecular", false ); // !g_Render.DisableSpecular );

        g_Render.meshCell.Render( g_Render.pEffect, g_Render.hDiffuseTexture, g_Render.hDiffuse, 0, g_Render.hSpecular,
                                  0, g_Render.hPower, true, false );

        // Render opaque objects first
        if( g_GameState.nAmmoCount > 0 )
        {
            for( int A = 0; A < MAX_AMMO; A++ )
            {
                if( !g_GameState.AmmoQ[A].bActive )
                    continue;
                if( g_GameState.AmmoQ[A].fAlpha == 1.0f )
                    RenderAmmo( A, mView, mProj );
            }
        }
        if( g_GameState.nDroidCount > 0 )
        {
            for( int A = 0; A < MAX_DROID; A++ )
            {
                if( !g_GameState.DroidQ[A].bActive )
                    continue;

                if( g_GameState.DroidQ[A].fAlpha == 1.0f )
                {
                    RenderDroid( pd3dDevice, A, mView, mProj, g_GameState.DroidQ[A].nHitPoints <= 0 );
                }
            }
        }

        // Render transparent objects last
        if( g_GameState.nAmmoCount > 0 )
        {
            for( int A = 0; A < MAX_AMMO; A++ )
            {
                if( !g_GameState.AmmoQ[A].bActive )
                    continue;
                if( g_GameState.AmmoQ[A].fAlpha < 1.0f )
                    RenderAmmo( A, mView, mProj );
            }
        }
        if( g_GameState.nDroidCount > 0 )
        {
            for( int A = 0; A < MAX_DROID; A++ )
            {
                if( !g_GameState.DroidQ[A].bActive )
                    continue;

                if( g_GameState.DroidQ[A].fAlpha < 1.0f )
                    RenderDroid( pd3dDevice, A, mView, mProj, g_GameState.DroidQ[A].nHitPoints <= 0 );
            }
        }

        switch( g_GameState.gameMode )
        {
            case GAME_RUNNING:
                RenderText(); break;
            case GAME_MAIN_MENU:
                V( g_Render.MainMenuDlg.OnRender( fElapsedTime ) ); break;
            case GAME_AUDIO_MENU:
                V( g_Render.AudioMenuDlg.OnRender( fElapsedTime ) ); break;
            case GAME_VIDEO_MENU:
                V( g_Render.VideoMenuDlg.OnRender( fElapsedTime ) ); break;
        }

        V( pd3dDevice->EndScene() );
    }
}


//--------------------------------------------------------------------------------------
void RenderText()
{
    CDXUTTextHelper txtHelper( g_Render.pFont, g_Render.pTextSprite, 15 );
    txtHelper.Begin();
    txtHelper.SetInsertionPos( 2, 0 );
    txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    txtHelper.DrawTextLine( DXUTGetFrameStats( true ) );
    txtHelper.DrawTextLine( DXUTGetDeviceStats() );
    txtHelper.DrawTextLine( L"X: Add Droid  Y: Toggle Droid Movement  B: Mass Kill" );
    txtHelper.DrawFormattedTextLine( L"Pos: %0.2f, %0.2f, %0.2f", g_Camera.GetEyePt()->x, g_Camera.GetEyePt()->y,
                                     g_Camera.GetEyePt()->z );
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
    g_Render.DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );

    switch( g_GameState.gameMode )
    {
        case GAME_RUNNING:
            g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam ); break;
        case GAME_MAIN_MENU:
            g_Render.MainMenuDlg.MsgProc( hWnd, uMsg, wParam, lParam ); break;
        case GAME_AUDIO_MENU:
            g_Render.AudioMenuDlg.MsgProc( hWnd, uMsg, wParam, lParam ); break;
        case GAME_VIDEO_MENU:
            g_Render.VideoMenuDlg.MsgProc( hWnd, uMsg, wParam, lParam ); break;
    }

    return 0;
}


//--------------------------------------------------------------------------------------
void UpdateAspectRatioList( DXUTDeviceSettings* pDS )
{
    bool bFullScreenChecked = g_Render.VideoMenuDlg.GetCheckBox( IDC_FULLSCREEN )->GetChecked();

    // If windowed, then display a predefined list.  If fullscreen, then 
    // use the list of mode the HW device supports 
    CD3D9EnumDeviceSettingsCombo* pDeviceSettingsCombo = DXUTGetD3D9Enumeration()->GetDeviceSettingsCombo
        ( &pDS->d3d9 );
    if( pDeviceSettingsCombo == NULL )
        return;
    CGrowableArray <D3DDISPLAYMODE>* pDMList;
    if( !bFullScreenChecked )
        pDMList = &g_Render.aWindowedDMList;
    else
        pDMList = &pDeviceSettingsCombo->pAdapterInfo->displayModeList;

    UINT nAdapterMonitorWidth = g_Render.rcAdapterWork[pDS->d3d9.AdapterOrdinal].right -
        g_Render.rcAdapterWork[pDS->d3d9.AdapterOrdinal].left;
    UINT nAdapterMonitorHeight = g_Render.rcAdapterWork[pDS->d3d9.AdapterOrdinal].bottom -
        g_Render.rcAdapterWork[pDS->d3d9.AdapterOrdinal].top;

    CGrowableArray <float> aspectRatioList;
    for( int i = 0; i < pDMList->GetSize(); i++ )
    {
        D3DDISPLAYMODE dm = pDMList->GetAt( i );

        // Use this display mode only if it meets certain min requirements
        if( ( bFullScreenChecked && dm.Height >= 600 ) ||
            ( !bFullScreenChecked && dm.Width <= nAdapterMonitorWidth && dm.Height <= nAdapterMonitorHeight ) )
        {
            // Calc the aspect ratio of this mode, and create a list of aspect ratios 
            float fAspect = ( float )dm.Width / ( float )dm.Height;
            bool bFound = false;
            for( int iAspect = 0; iAspect < aspectRatioList.GetSize(); iAspect++ )
            {
                if( fabsf( aspectRatioList.GetAt( iAspect ) - fAspect ) < 0.05f )
                {
                    bFound = true;
                    break;
                }
            }
            if( !bFound )
                aspectRatioList.Add( fAspect );
        }
    }

    // Sort aspect ratio list
    if( aspectRatioList.GetData() )
        qsort( aspectRatioList.GetData(), aspectRatioList.GetSize(), sizeof( float ), SortAspectRatios );

    // Store the currently selected aspect ratio so it can be restored later
    float fCurrentlySelectedAspect = 0.0f;
    if( g_Render.VideoMenuDlg.GetComboBox( IDC_ASPECT )->GetNumItems() > 0 )
    {
        void* pD = g_Render.VideoMenuDlg.GetComboBox( IDC_ASPECT )->GetSelectedData();
        fCurrentlySelectedAspect = *( float* )( void* )&pD;
    }

    // Build the UI list of aspect ratios
    g_Render.VideoMenuDlg.GetComboBox( IDC_ASPECT )->RemoveAllItems();
    for( int i = 0; i < aspectRatioList.GetSize(); i++ )
    {
        WCHAR sz[256];
        float fAspect = aspectRatioList.GetAt( i );

        // Make user friendly strings for common aspect ratios
        if( fabsf( fAspect - 1.3333f ) < 0.05f ) swprintf_s( sz, 256, L"4:3" );
        else if( fabsf( fAspect - 1.25f ) < 0.05f ) swprintf_s( sz, 256, L"5:4" );
        else if( fabsf( fAspect - 1.77f ) < 0.05f ) swprintf_s( sz, 256, L"16:9" );
        else if( fabsf( fAspect - 1.6f ) < 0.05f ) swprintf_s( sz, 256, L"16:10" );
        else if( fabsf( fAspect - 1.5f ) < 0.05f ) swprintf_s( sz, 256, L"3:2" );
        else if( fabsf( fAspect - 0.8f ) < 0.05f ) swprintf_s( sz, 256, L"4:5" );
        else if( fabsf( fAspect - 1.66f ) < 0.05f ) swprintf_s( sz, 256, L"5:3" );
        else if( fabsf( fAspect - 0.75f ) < 0.05f ) swprintf_s( sz, 256, L"3:4" );
        else if( fabsf( fAspect - 0.5625f ) < 0.05f ) swprintf_s( sz, 256, L"9:16" );
        else
            swprintf_s( sz, 256, L"%0.2f:1", fAspect );

        g_Render.VideoMenuDlg.GetComboBox( IDC_ASPECT )->AddItem( sz, UintToPtr( *( DWORD* )&fAspect ) );
    }

    // Important: if nothing was selected before, then default to the desktop resolution
    // Defaulting to the aspect ratio of the desktop is the best default aspect ratio
    // since the desktop resolution can be assumed to be setup correctly for the display device
    if( fCurrentlySelectedAspect == 0.0f )
    {
        D3DDISPLAYMODE dmDesktop;
        DXUTGetDesktopResolution( pDS->d3d9.AdapterOrdinal, &dmDesktop.Width, &dmDesktop.Height );
        g_Render.fDesktopAspectRatio = dmDesktop.Width / ( FLOAT )dmDesktop.Height;
        fCurrentlySelectedAspect = g_Render.fDesktopAspectRatio;
    }
    g_Render.VideoMenuDlg.GetComboBox( IDC_ASPECT )->SetSelectedByData( UintToPtr
                                                                        ( *( DWORD* )&fCurrentlySelectedAspect ) );

    // Now update resolution list
    UpdateResolutionList( pDS );
}


//--------------------------------------------------------------------------------------
void UpdateResolutionList( DXUTDeviceSettings* pDS )
{
    bool bFullScreenChecked = g_Render.VideoMenuDlg.GetCheckBox( IDC_FULLSCREEN )->GetChecked();

    // Get the currently selected aspect ratio and show a list of matching aspect ratios
    void* pData = g_Render.VideoMenuDlg.GetComboBox( IDC_ASPECT )->GetSelectedData();
    float fSelectedAspect = *( float* )( void* )&pData;

    // Get the currently selected resolution and restore it later if it still is in the list
    void* pSelectedData = NULL;
    if( g_Render.VideoMenuDlg.GetComboBox( IDC_RESOLUTION )->GetNumItems() > 0 )
        pSelectedData = g_Render.VideoMenuDlg.GetComboBox( IDC_RESOLUTION )->GetSelectedData();

    UINT nAdapterMonitorWidth = g_Render.rcAdapterWork[pDS->d3d9.AdapterOrdinal].right -
        g_Render.rcAdapterWork[pDS->d3d9.AdapterOrdinal].left;
    UINT nAdapterMonitorHeight = g_Render.rcAdapterWork[pDS->d3d9.AdapterOrdinal].bottom -
        g_Render.rcAdapterWork[pDS->d3d9.AdapterOrdinal].top;

    // If windowed, then display a predefined list.  If fullscreen, then 
    // use the list of mode the HW device supports 
    CGrowableArray <D3DDISPLAYMODE>* pDMList;
    CD3D9EnumDeviceSettingsCombo* pDeviceSettingsCombo = DXUTGetD3D9Enumeration()->GetDeviceSettingsCombo
        ( &pDS->d3d9 );
    if( !bFullScreenChecked )
        pDMList = &g_Render.aWindowedDMList;
    else
        pDMList = &pDeviceSettingsCombo->pAdapterInfo->displayModeList;

    // Build a list of resolutions that match the current aspect ratio
    g_Render.VideoMenuDlg.GetComboBox( IDC_RESOLUTION )->RemoveAllItems();
    for( int i = 0; i < pDMList->GetSize(); i++ )
    {
        D3DDISPLAYMODE dm = pDMList->GetAt( i );

        // Use this display mode only if it meets certain min requirements
        if( ( bFullScreenChecked && dm.Height >= 600 ) ||
            ( !bFullScreenChecked && dm.Width <= nAdapterMonitorWidth && dm.Height <= nAdapterMonitorHeight ) )
        {
            // Calc the aspect ratio of this mode and add it if it matchs
            float fAspect = ( float )dm.Width / ( float )dm.Height;
            if( fabsf( fSelectedAspect - fAspect ) < 0.05f )
            {
                WCHAR sz[256];
                swprintf_s( sz, 256, L"%d x %d", dm.Width, dm.Height );
                if( !g_Render.VideoMenuDlg.GetComboBox( IDC_RESOLUTION )->ContainsItem( sz ) )
                    g_Render.VideoMenuDlg.GetComboBox( IDC_RESOLUTION )->AddItem( sz, UIntToPtr( MAKELONG( dm.Width,
                                                                                                           dm.Height
                                                                                                           ) ) );
            }
        }
    }

    bool bSetToDefault = false;
    if( pSelectedData )
    {
        HRESULT hr = g_Render.VideoMenuDlg.GetComboBox( IDC_RESOLUTION )->SetSelectedByData( pSelectedData );
        if( FAILED( hr ) )
            bSetToDefault = true;
    }
    else
    {
        bSetToDefault = true;
    }

    if( bSetToDefault )
    {
        // Important: default to the desktop resolution as that can be assumed to look best on 
        // fixed-pixel size display devices such as LCD, plasma, DLPs.  
        D3DDISPLAYMODE dmDesktop;
        DXUTGetDesktopResolution( pDS->d3d9.AdapterOrdinal, &dmDesktop.Width, &dmDesktop.Height );
        g_Render.VideoMenuDlg.GetComboBox( IDC_RESOLUTION )->SetSelectedByData( UintToPtr( MAKELONG( dmDesktop.Width,
                                                                                                     dmDesktop.Height
                                                                                                     ) ) );
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_RESUME:
            g_GameState.gameMode = GAME_RUNNING;
            DXUTSetCursorSettings( false, true );
            break;
        case IDC_BACK:
            g_GameState.gameMode = GAME_MAIN_MENU;
            break;
        case IDC_AUDIO:
            g_GameState.gameMode = GAME_AUDIO_MENU;
            break;
        case IDC_VIDEO:
        {
            g_GameState.gameMode = GAME_VIDEO_MENU;

            // When display video menu, show the current settings
            g_Render.VideoMenuDlg.EnableNonUserEvents( true );
            g_Render.VideoMenuDlg.GetCheckBox( IDC_FULLSCREEN )->SetChecked( !DXUTIsWindowed() );
            g_Render.VideoMenuDlg.EnableNonUserEvents( false );
            g_Render.VideoMenuDlg.GetCheckBox( IDC_ANTI_ALIASING )->SetChecked( DXUTGetDeviceSettings
                                                                                ().d3d9.pp.MultiSampleType !=
                                                                                D3DMULTISAMPLE_NONE );
            g_Render.VideoMenuDlg.GetComboBox( IDC_RESOLUTION )->SetSelectedByData(
                UintToPtr( MAKELONG( DXUTGetDeviceSettings().d3d9.pp.BackBufferWidth,
                                     DXUTGetDeviceSettings().d3d9.pp.BackBufferHeight ) ) );
            g_Render.VideoMenuDlg.GetCheckBox( IDC_HIGH_MODEL_RES )->SetChecked( g_Render.UseHighResModels );
            g_Render.VideoMenuDlg.GetSlider( IDC_MAX_DROIDS )->SetValue( g_GameState.nMaxDroids );
            break;
        }
        case IDC_QUIT:
            DXUTShutdown( 0 );
            break;
        case IDC_MUSIC_SCALE:
            g_audioState.fMusicVolume = ( float )( g_Render.AudioMenuDlg.GetSlider( IDC_MUSIC_SCALE )->GetValue() /
                                                   100.0f );
            SetMusicVolume( g_audioState.fMusicVolume );
            break;
        case IDC_SOUNDFX_SCALE:
            g_audioState.fSoundFXVolume = ( float )( g_Render.AudioMenuDlg.GetSlider( IDC_SOUNDFX_SCALE )->GetValue() /
                                                     100.0f );
            SetSoundFxVolume( g_audioState.fSoundFXVolume );
            break;
        case IDC_MAX_DROIDS:
        {
            WCHAR sz[256];
            int nMaxObjects = g_Render.VideoMenuDlg.GetSlider( IDC_MAX_DROIDS )->GetValue();
            swprintf_s( sz, 256, L"Max Droids (%d)", nMaxObjects );
            g_Render.VideoMenuDlg.GetStatic( IDC_MAX_DROIDS_TEXT )->SetText( sz );
            break;
        }
        case IDC_FULLSCREEN:
        {
            // Update the list of aspect ratios when the user (un)checks the fullscreen box
            DXUTDeviceSettings ds = DXUTGetDeviceSettings();
            UpdateAspectRatioList( &ds );
            break;
        }
        case IDC_ASPECT:
        {
            // Update the resolution list when the aspect ratio changes
            DXUTDeviceSettings ds = DXUTGetDeviceSettings();
            UpdateResolutionList( &ds );
            break;
        }
        case IDC_APPLY:
        {
            // Get the settings from the dialog
            bool bAA = g_Render.VideoMenuDlg.GetCheckBox( IDC_ANTI_ALIASING )->GetChecked();
            UINT nRes = PtrToInt( g_Render.VideoMenuDlg.GetComboBox( IDC_RESOLUTION )->GetSelectedData() );
            int nWidth = LOWORD( nRes ); int nHeight = HIWORD( nRes );
            bool bFullscreen = g_Render.VideoMenuDlg.GetCheckBox( IDC_FULLSCREEN )->GetChecked();

            DXUTDeviceSettings ds = DXUTGetDeviceSettings();
            ds.d3d9.pp.MultiSampleType = ( bAA ) ? D3DMULTISAMPLE_4_SAMPLES : D3DMULTISAMPLE_NONE;
            ds.d3d9.pp.Windowed = ( BOOL )!bFullscreen;
            ds.d3d9.pp.BackBufferWidth = nWidth; ds.d3d9.pp.BackBufferHeight = nHeight;

            // Change the device settings
            g_Render.bDetectOptimalSettings = false;
            DXUTCreateDeviceFromSettings( &ds );
            g_Render.bDetectOptimalSettings = true;

            g_GameState.nMaxDroids = g_Render.VideoMenuDlg.GetSlider( IDC_MAX_DROIDS )->GetValue();
            g_Render.UseHighResModels = g_Render.VideoMenuDlg.GetCheckBox( IDC_HIGH_MODEL_RES )->GetChecked();

            // Remove droids from scene if needed
            while( g_GameState.nDroidCount > g_GameState.nMaxDroids )
            {
                for( int A = 0; A < MAX_DROID; A++ )
                {
                    if( g_GameState.DroidQ[A].bActive )
                    {
                        g_GameState.DroidQ[A].bActive = false;
                        g_GameState.nDroidCount--;
                        SetNumDroidsForAudio( g_GameState.nDroidCount );
                        break;
                    }
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------
void ToggleMenu()
{
    switch( g_GameState.gameMode )
    {
        case GAME_RUNNING:
            g_GameState.gameMode = GAME_MAIN_MENU; break;
        case GAME_MAIN_MENU:
            g_GameState.gameMode = GAME_RUNNING; break;
        case GAME_AUDIO_MENU:
            g_GameState.gameMode = GAME_MAIN_MENU; break;
        case GAME_VIDEO_MENU:
            g_GameState.gameMode = GAME_MAIN_MENU; break;
    }

    DXUTSetCursorSettings( ( g_GameState.gameMode != GAME_RUNNING ), true );
}


//--------------------------------------------------------------------------------------
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
            case VK_ESCAPE:
                ToggleMenu();
                break;

            case 'Y': // Droid move toggle
                g_GameState.bDroidMove = !g_GameState.bDroidMove;
                break;

            case 'M': // Droid auto-add 
                g_GameState.bAutoAddDroids = !g_GameState.bAutoAddDroids;
                break;

            case 'X': // Droid add
                if( g_GameState.nDroidCount < MAX_DROID )
                    g_GameState.bDroidCreate = true;
                break;

            case 'B': // Mass droid kill
                g_GameState.bMassDroidKill = true;
                break;
        }
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
    g_Render.DialogResourceManager.OnD3D9LostDevice();
    if( g_Render.pFont )
        g_Render.pFont->OnLostDevice();
    if( g_Render.pEffect )
        g_Render.pEffect->OnLostDevice();
    SAFE_RELEASE( g_Render.pTextSprite );
    g_Render.meshCell.InvalidateDeviceObjects();
    g_Render.meshAmmo.InvalidateDeviceObjects();
    g_Render.meshDroid.InvalidateDeviceObjects();
    g_Render.meshDroidExplosion.InvalidateDeviceObjects();
    g_Render.meshDroidLow.InvalidateDeviceObjects();
    g_Render.meshDroidExplosionLow.InvalidateDeviceObjects();
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles. Resources created in the OnCreateDevice callback 
// should be released here, which generally includes all D3DPOOL_MANAGED resources. 
//--------------------------------------------------------------------------------------
void CALLBACK OnDestroyDevice( void* pUserContext )
{
    g_Render.DialogResourceManager.OnD3D9DestroyDevice();
    SAFE_RELEASE( g_Render.pEffect );
    SAFE_RELEASE( g_Render.pFont );
    SAFE_RELEASE( g_Render.pDefaultTex );
    SAFE_RELEASE( g_Render.pDefaultNormalMap );
    SAFE_RELEASE( g_Render.pDroidNormalMap );
    g_Render.meshCell.Destroy();
    g_Render.meshAmmo.Destroy();
    g_Render.meshDroid.Destroy();
    g_Render.meshDroidExplosion.Destroy();
    g_Render.meshDroidLow.Destroy();
    g_Render.meshDroidExplosionLow.Destroy();
    SAFE_RELEASE( g_Render.pMeshDroidExplosion );
}



