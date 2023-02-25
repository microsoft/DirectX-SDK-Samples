//--------------------------------------------------------------------------------------
// File: Collision9.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "resource.h"
#include "xnacollision.h"

//#define DEBUG_VS   // Uncomment this line to debug D3D9 vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug D3D9 pixel shaders 

//--------------------------------------------------------------------------------------
// Types
//--------------------------------------------------------------------------------------

// Collision objects
struct CollisionSphere
{
    XNA::Sphere sphere;
    int collision;
};

struct CollisionBox
{
    XNA::OrientedBox obox;
    int collision;
};

struct CollisionAABox
{
    XNA::AxisAlignedBox aabox;
    int collision;
};

struct CollisionFrustum
{
    XNA::Frustum frustum;
    int collision;
};

struct CollisionTriangle
{
    XMVECTOR pointa;
    XMVECTOR pointb;
    XMVECTOR pointc;
    int collision;
};

struct CollisionRay
{
    XMVECTOR origin;
    XMVECTOR direction;
};

struct XMPosVertex
{
    XMFLOAT3 p;
};

//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------
const int GROUP_COUNT = 4;
const int CAMERA_COUNT = 4;

const float CAMERA_SPACING = 50.f;

const int MAX_VERTS = 100;
const int MAX_INDEX = 24;

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CModelViewerCamera          g_Camera;               // A model viewing camera
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = NULL;
CDXUTDialog                 g_HUD;                  // dialog for standard controls
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls

// Direct3D 9 resources
ID3DXFont*                      g_pFont9 = NULL;
ID3DXSprite*                    g_pSprite9 = NULL;
ID3DXEffect*                    g_pEffect9 = NULL;
IDirect3DVertexDeclaration9*    g_pVertexDecl = NULL;
LPDIRECT3DVERTEXBUFFER9         g_pVB = NULL;
LPDIRECT3DINDEXBUFFER9          g_pIB = NULL;
D3DXHANDLE                      g_pmWorldViewProj = NULL;
D3DXHANDLE                      g_Color = NULL;

// Primary collision objects
XNA::Frustum g_PrimaryFrustum;
XNA::OrientedBox g_PrimaryOrientedBox;
XNA::AxisAlignedBox g_PrimaryAABox;
CollisionRay g_PrimaryRay;

// Secondary collision objects
CollisionSphere     g_SecondarySpheres[GROUP_COUNT];
CollisionBox        g_SecondaryOrientedBoxes[GROUP_COUNT];
CollisionAABox      g_SecondaryAABoxes[GROUP_COUNT];
CollisionTriangle   g_SecondaryTriangles[GROUP_COUNT];

// Ray testing results display object
CollisionAABox g_RayHitResultBox;

// Camera preset locations
XMVECTOR g_CameraOrigins[CAMERA_COUNT];


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_STATIC              -1
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3
#define IDC_GROUP               4


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

void InitializeObjects();
void Animate( double fTime );
void Collide();
void RenderObjects( IDirect3DDevice9* pd3dDevice );
void SetViewForGroup( int group );

void DrawGrid( IDirect3DDevice9* pd3dDevice, const XMFLOAT3& XAxis, const XMFLOAT3& YAxis,
               const XMFLOAT3& Origin, int iXDivisions, int iYDivisions, D3DCOLOR Color );
void DrawFrustum( IDirect3DDevice9* pd3dDevice, const XNA::Frustum& frustum, D3DCOLOR Color );
void DrawAabb( IDirect3DDevice9* pd3dDevice, const XNA::AxisAlignedBox& box, D3DCOLOR Color );
void DrawObb( IDirect3DDevice9* pd3dDevice, const XNA::OrientedBox& obb, D3DCOLOR Color );
void DrawSphere( IDirect3DDevice9* pd3dDevice, const XNA::Sphere& sphere, D3DCOLOR Color );
void DrawRay( IDirect3DDevice9* pd3dDevice, const XMFLOAT3& Origin, const XMFLOAT3& Direction,
              BOOL bNormalize, D3DCOLOR Color );
void DrawTriangle( IDirect3DDevice9* pd3dDevice, const XMFLOAT3& PointA, const XMFLOAT3& PointB,
                   const XMFLOAT3& PointC, D3DCOLOR Color );


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

    // XNAMath uses SSE/SSE2 instructions on Windows. We should verify the CPU supports these instructions
    // as early in the program as possible
    if ( !XMVerifyCPUSupport() )
    {
        MessageBox( NULL, TEXT("This application requires the processor support SSE2 instructions."),
                    TEXT("Collision"), MB_OK | MB_ICONEXCLAMATION);
        return -1;
    }

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
    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"Collision" );
    DXUTCreateDevice( true, 640, 480 );
    DXUTMainLoop(); // Enter into the DXUT render loop

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

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 10;

    CDXUTComboBox* pComboBox = NULL;
    g_SampleUI.AddStatic( IDC_STATIC, L"(G)roup", 10, 0, 105, 25 );
    g_SampleUI.AddComboBox( IDC_GROUP, 10, 25, 140, 24, 'G', false, &pComboBox );
    if( pComboBox )
        pComboBox->SetDropHeight( 50 );

    pComboBox->AddItem( L"Frustum", IntToPtr( 0 ) );
    pComboBox->AddItem( L"Axis-aligned Box", IntToPtr( 1 ) );
    pComboBox->AddItem( L"Oriented Box", IntToPtr( 2 ) );
    pComboBox->AddItem( L"Ray", IntToPtr( 3 ) );

    InitializeObjects();
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
    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Initialize the starting positions of the collision objects
//--------------------------------------------------------------------------------------
void InitializeObjects()
{
    const XMVECTOR XMZero = XMVectorZero();

    // Set up the primary frustum object from a D3D projection matrix
    // NOTE: This can also be done on your camera's projection matrix.  The projection
    // matrix built here is somewhat contrived so it renders well.
    XMMATRIX xmProj = XMMatrixPerspectiveFovLH( XM_PIDIV4, 1.77778f, 0.5f, 10.0f );
    ComputeFrustumFromProjection( &g_PrimaryFrustum, &xmProj );
    g_PrimaryFrustum.Origin.z = -7.0f;
    g_CameraOrigins[0] = XMVectorSet( 0, 0, 0, 0 );

    // Set up the primary axis aligned box
    g_PrimaryAABox.Center = XMFLOAT3( CAMERA_SPACING, 0, 0 );
    g_PrimaryAABox.Extents = XMFLOAT3( 5, 5, 5 );
    g_CameraOrigins[1] = XMVectorSet( CAMERA_SPACING, 0, 0, 0 );

    // Set up the primary oriented box with some rotation
    g_PrimaryOrientedBox.Center = XMFLOAT3( -CAMERA_SPACING, 0, 0 );
    g_PrimaryOrientedBox.Extents = XMFLOAT3( 5, 5, 5 );
    XMStoreFloat4( &g_PrimaryOrientedBox.Orientation, XMQuaternionRotationRollPitchYaw( XM_PIDIV4, XM_PIDIV4, 0 ) );
    g_CameraOrigins[2] = XMVectorSet( -CAMERA_SPACING, 0, 0, 0 );

    // Set up the primary ray
    g_PrimaryRay.origin = XMVectorSet( 0, 0, CAMERA_SPACING, 0 );
    g_PrimaryRay.direction = XMVectorSet( 0, 0, 1, 0 );
    g_CameraOrigins[3] = XMVectorSet( 0, 0, CAMERA_SPACING, 0 );

    // Initialize all of the secondary objects with default values
    for( UINT i = 0; i < GROUP_COUNT; i++ )
    {
        g_SecondarySpheres[i].sphere.Radius = 1.0f;
        g_SecondarySpheres[i].sphere.Center = XMFLOAT3( 0, 0, 0 );
        g_SecondarySpheres[i].collision = FALSE;

        g_SecondaryOrientedBoxes[i].obox.Center = XMFLOAT3( 0, 0, 0 );
        g_SecondaryOrientedBoxes[i].obox.Extents = XMFLOAT3( 0.5f, 0.5f, 0.5f );
        g_SecondaryOrientedBoxes[i].obox.Orientation = XMFLOAT4( 0, 0, 0, 1 );
        g_SecondaryOrientedBoxes[i].collision = FALSE;

        g_SecondaryAABoxes[i].aabox.Center = XMFLOAT3( 0, 0, 0 );
        g_SecondaryAABoxes[i].aabox.Extents = XMFLOAT3( 0.5f, 0.5f, 0.5f );
        g_SecondaryAABoxes[i].collision = FALSE;

        g_SecondaryTriangles[i].pointa = XMZero;
        g_SecondaryTriangles[i].pointb = XMZero;
        g_SecondaryTriangles[i].pointc = XMZero;
        g_SecondaryTriangles[i].collision = FALSE;
    }

    // Set up ray hit result box
    g_RayHitResultBox.aabox.Center = XMFLOAT3( 0, 0, 0 );
    g_RayHitResultBox.aabox.Extents = XMFLOAT3( 0.05f, 0.05f, 0.05f );
}


//--------------------------------------------------------------------------------------
// Move objects around over time
//--------------------------------------------------------------------------------------
void Animate( double fTime )
{
    float t = ( FLOAT )(fTime * 0.2);

    const float camera0OriginX = XMVectorGetX( g_CameraOrigins[0] );
    const float camera1OriginX = XMVectorGetX( g_CameraOrigins[1] );
    const float camera2OriginX = XMVectorGetX( g_CameraOrigins[2] );
    const float camera3OriginX = XMVectorGetX( g_CameraOrigins[3] );
    const float camera3OriginZ = XMVectorGetZ( g_CameraOrigins[3] );

    // animate sphere 0 around the frustum
    g_SecondarySpheres[0].sphere.Center.x = 10 * sinf( 3 * t );
    g_SecondarySpheres[0].sphere.Center.y = 7 * cosf( 5 * t );

    // animate oriented box 0 around the frustum
    g_SecondaryOrientedBoxes[0].obox.Center.x = 8 * sinf( 3.5f * t );
    g_SecondaryOrientedBoxes[0].obox.Center.y = 5 * cosf( 5.1f * t );
    XMStoreFloat4( &( g_SecondaryOrientedBoxes[0].obox.Orientation ), XMQuaternionRotationRollPitchYaw( t * 1.4f,
                                                                                                          t * 0.2f,
                                                                                                          t ) );

    // animate aligned box 0 around the frustum
    g_SecondaryAABoxes[0].aabox.Center.x = 10 * sinf( 2.1f * t );
    g_SecondaryAABoxes[0].aabox.Center.y = 7 * cosf( 3.8f * t );

    // animate sphere 1 around the aligned box
    g_SecondarySpheres[1].sphere.Center.x = 8 * sinf( 2.9f * t ) + camera1OriginX;
    g_SecondarySpheres[1].sphere.Center.y = 8 * cosf( 4.6f * t );
    g_SecondarySpheres[1].sphere.Center.z = 8 * cosf( 1.6f * t );
  
    // animate oriented box 1 around the aligned box
    g_SecondaryOrientedBoxes[1].obox.Center.x = 8 * sinf( 3.2f * t ) + camera1OriginX;
    g_SecondaryOrientedBoxes[1].obox.Center.y = 8 * cosf( 2.1f * t );
    g_SecondaryOrientedBoxes[1].obox.Center.z = 8 * sinf( 1.6f * t );
    XMStoreFloat4( &( g_SecondaryOrientedBoxes[1].obox.Orientation ), XMQuaternionRotationRollPitchYaw( t * 0.7f,
                                                                                                          t * 1.3f,
                                                                                                          t ) );

    // animate aligned box 1 around the aligned box
    g_SecondaryAABoxes[1].aabox.Center.x = 8 * sinf( 1.1f * t ) + camera1OriginX;
    g_SecondaryAABoxes[1].aabox.Center.y = 8 * cosf( 5.8f * t );
    g_SecondaryAABoxes[1].aabox.Center.z = 8 * cosf( 3.0f * t );

    // animate sphere 2 around the oriented box
    g_SecondarySpheres[2].sphere.Center.x = 8 * sinf( 2.2f * t ) + camera2OriginX;
    g_SecondarySpheres[2].sphere.Center.y = 8 * cosf( 4.3f * t );
    g_SecondarySpheres[2].sphere.Center.z = 8 * cosf( 1.8f * t );

    // animate oriented box 2 around the oriented box
    g_SecondaryOrientedBoxes[2].obox.Center.x = 8 * sinf( 3.7f * t ) + camera2OriginX;
    g_SecondaryOrientedBoxes[2].obox.Center.y = 8 * cosf( 2.5f * t );
    g_SecondaryOrientedBoxes[2].obox.Center.z = 8 * sinf( 1.1f * t );
    XMStoreFloat4( &( g_SecondaryOrientedBoxes[2].obox.Orientation ), XMQuaternionRotationRollPitchYaw( t * 0.9f,
                                                                                                          t * 1.8f,
                                                                                                          t ) );

    // animate aligned box 2 around the oriented box
    g_SecondaryAABoxes[2].aabox.Center.x = 8 * sinf( 1.3f * t ) + camera2OriginX;
    g_SecondaryAABoxes[2].aabox.Center.y = 8 * cosf( 5.2f * t );
    g_SecondaryAABoxes[2].aabox.Center.z = 8 * cosf( 3.5f * t );

    // triangle points in local space - equilateral triangle with radius of 2
    const XMVECTOR TrianglePointA = { 0, 2, 0, 0 };
    const XMVECTOR TrianglePointB = { 1.732f, -1, 0, 0 };
    const XMVECTOR TrianglePointC = { -1.732f, -1, 0, 0 };

    // animate triangle 0 around the frustum
    XMMATRIX TriangleCoords = XMMatrixRotationRollPitchYaw( t * 1.4f, t * 2.5f, t );
    XMMATRIX Translation = XMMatrixTranslation( 5 * sinf( 5.3f * t ) + camera0OriginX,
                                                5 * cosf( 2.3f * t ),
                                                5 * sinf( 3.4f * t ) );
    TriangleCoords = XMMatrixMultiply( TriangleCoords, Translation );
    g_SecondaryTriangles[0].pointa = XMVector3Transform( TrianglePointA, TriangleCoords );
    g_SecondaryTriangles[0].pointb = XMVector3Transform( TrianglePointB, TriangleCoords );
    g_SecondaryTriangles[0].pointc = XMVector3Transform( TrianglePointC, TriangleCoords );

    // animate triangle 1 around the aligned box
    TriangleCoords = XMMatrixRotationRollPitchYaw( t * 1.4f, t * 2.5f, t );
    Translation = XMMatrixTranslation( 8 * sinf( 5.3f * t ) + camera1OriginX,
                                       8 * cosf( 2.3f * t ),
                                       8 * sinf( 3.4f * t ) );
    TriangleCoords = XMMatrixMultiply( TriangleCoords, Translation );
    g_SecondaryTriangles[1].pointa = XMVector3Transform( TrianglePointA, TriangleCoords );
    g_SecondaryTriangles[1].pointb = XMVector3Transform( TrianglePointB, TriangleCoords );
    g_SecondaryTriangles[1].pointc = XMVector3Transform( TrianglePointC, TriangleCoords );

    // animate triangle 2 around the oriented box
    TriangleCoords = XMMatrixRotationRollPitchYaw( t * 1.4f, t * 2.5f, t );
    Translation = XMMatrixTranslation( 8 * sinf( 5.3f * t ) + camera2OriginX,
                                       8 * cosf( 2.3f * t ),
                                       8 * sinf( 3.4f * t ) );
    TriangleCoords = XMMatrixMultiply( TriangleCoords, Translation );
    g_SecondaryTriangles[2].pointa = XMVector3Transform( TrianglePointA, TriangleCoords );
    g_SecondaryTriangles[2].pointb = XMVector3Transform( TrianglePointB, TriangleCoords );
    g_SecondaryTriangles[2].pointc = XMVector3Transform( TrianglePointC, TriangleCoords );

    // animate primary ray (this is the only animated primary object)
    g_PrimaryRay.direction = XMVectorSet( sinf( t * 3 ), 0, cosf( t * 3 ), 0 );

    // animate sphere 3 around the ray
    g_SecondarySpheres[3].sphere.Center = XMFLOAT3( camera3OriginX - 3,
                                                      0.5f * sinf( t * 5 ),
                                                      camera3OriginZ );

    // animate aligned box 3 around the ray
    g_SecondaryAABoxes[3].aabox.Center = XMFLOAT3( camera3OriginX + 3,
                                                     0.5f * sinf( t * 4 ),
                                                     camera3OriginZ );

    // animate oriented box 3 around the ray
    g_SecondaryOrientedBoxes[3].obox.Center = XMFLOAT3( camera3OriginX,
                                                          0.5f * sinf( t * 4.5f ),
                                                          camera3OriginZ + 3 );
    XMStoreFloat4( &( g_SecondaryOrientedBoxes[3].obox.Orientation ), XMQuaternionRotationRollPitchYaw( t * 0.9f,
                                                                                                          t * 1.8f,
                                                                                                          t ) );

    // animate triangle 3 around the ray
    TriangleCoords = XMMatrixRotationRollPitchYaw( t * 1.4f, t * 2.5f, t );
    Translation = XMMatrixTranslation( camera3OriginX,
                                       0.5f * cosf( 4.3f * t ),
                                       camera3OriginZ - 3 );
    TriangleCoords = XMMatrixMultiply( TriangleCoords, Translation );
    g_SecondaryTriangles[3].pointa = XMVector3Transform( TrianglePointA, TriangleCoords );
    g_SecondaryTriangles[3].pointb = XMVector3Transform( TrianglePointB, TriangleCoords );
    g_SecondaryTriangles[3].pointc = XMVector3Transform( TrianglePointC, TriangleCoords );
}


//--------------------------------------------------------------------------------------
// Test collisions between pairs of collision objects using XNACollision functions
//--------------------------------------------------------------------------------------
void Collide()
{
    // test collisions between objects and frustum
    g_SecondarySpheres[0].collision = XNA::IntersectSphereFrustum( &g_SecondarySpheres[0].sphere,
                                                                   &g_PrimaryFrustum );
    g_SecondaryOrientedBoxes[0].collision = XNA::IntersectOrientedBoxFrustum( &g_SecondaryOrientedBoxes[0].obox,
                                                                              &g_PrimaryFrustum );
    g_SecondaryAABoxes[0].collision = XNA::IntersectAxisAlignedBoxFrustum( &g_SecondaryAABoxes[0].aabox,
                                                                           &g_PrimaryFrustum );
    g_SecondaryTriangles[0].collision = XNA::IntersectTriangleFrustum( g_SecondaryTriangles[0].pointa,
                                                                       g_SecondaryTriangles[0].pointb,
                                                                       g_SecondaryTriangles[0].pointc,
                                                                       &g_PrimaryFrustum );

    // test collisions between objects and aligned box
    g_SecondarySpheres[1].collision = XNA::IntersectSphereAxisAlignedBox( &g_SecondarySpheres[1].sphere,
                                                                          &g_PrimaryAABox );
    g_SecondaryOrientedBoxes[1].collision = XNA::IntersectAxisAlignedBoxOrientedBox( &g_PrimaryAABox,
                                                                        &g_SecondaryOrientedBoxes[1].obox );
    g_SecondaryAABoxes[1].collision = XNA::IntersectAxisAlignedBoxAxisAlignedBox( &g_SecondaryAABoxes[1].aabox,
                                                                                  &g_PrimaryAABox );
    g_SecondaryTriangles[1].collision = XNA::IntersectTriangleAxisAlignedBox( g_SecondaryTriangles[1].pointa,
                                                                              g_SecondaryTriangles[1].pointb,
                                                                              g_SecondaryTriangles[1].pointc,
                                                                              &g_PrimaryAABox );

    // test collisions between objects and oriented box
    g_SecondarySpheres[2].collision = XNA::IntersectSphereOrientedBox( &g_SecondarySpheres[2].sphere,
                                                                       &g_PrimaryOrientedBox );
    g_SecondaryOrientedBoxes[2].collision = XNA::IntersectOrientedBoxOrientedBox( &g_SecondaryOrientedBoxes[2].obox,
                                                                                  &g_PrimaryOrientedBox );
    g_SecondaryAABoxes[2].collision = XNA::IntersectAxisAlignedBoxOrientedBox( &g_SecondaryAABoxes[2].aabox,
                                                                               &g_PrimaryOrientedBox );
    g_SecondaryTriangles[2].collision = XNA::IntersectTriangleOrientedBox( g_SecondaryTriangles[2].pointa,
                                                                           g_SecondaryTriangles[2].pointb,
                                                                           g_SecondaryTriangles[2].pointc,
                                                                           &g_PrimaryOrientedBox );

    // test collisions between objects and ray
    float fDistance = -1.0f;
    g_SecondarySpheres[3].collision = XNA::IntersectRaySphere( g_PrimaryRay.origin,
                                                               g_PrimaryRay.direction,
                                                               &g_SecondarySpheres[3].sphere,
                                                               &fDistance );
    g_SecondaryOrientedBoxes[3].collision = XNA::IntersectRayOrientedBox( g_PrimaryRay.origin,
                                                                          g_PrimaryRay.direction,
                                                                          &g_SecondaryOrientedBoxes[3].obox,
                                                                          &fDistance );
    g_SecondaryAABoxes[3].collision = XNA::IntersectRayAxisAlignedBox( g_PrimaryRay.origin,
                                                                       g_PrimaryRay.direction,
                                                                       &g_SecondaryAABoxes[3].aabox,
                                                                       &fDistance );
    g_SecondaryTriangles[3].collision = XNA::IntersectRayTriangle( g_PrimaryRay.origin,
                                                                   g_PrimaryRay.direction,
                                                                   g_SecondaryTriangles[3].pointa,
                                                                   g_SecondaryTriangles[3].pointb,
                                                                   g_SecondaryTriangles[3].pointc,
                                                                   &fDistance );

    // If one of the ray intersection tests was successful, fDistance will be positive.
    // If so, compute the intersection location and store it in g_RayHitResultBox.
    if( fDistance > 0 )
    {
        // The primary ray's direction is assumed to be normalized.
        XMVECTOR HitLocation = XMVectorMultiplyAdd( g_PrimaryRay.direction, XMVectorReplicate( fDistance ),
                                                    g_PrimaryRay.origin );
        XMStoreFloat3( &g_RayHitResultBox.aabox.Center, HitLocation );
        g_RayHitResultBox.collision = TRUE;
    }
    else
    {
        g_RayHitResultBox.collision = FALSE;
    }
}


//--------------------------------------------------------------------------------------
// Returns the color based on the collision result and the gruop number.
// Frustum tests (group 0) return 0, 1, or 2 for outside, partially inside, and fully inside;
// all other tests return 0 or 1 for no collision or collision.
//--------------------------------------------------------------------------------------
inline D3DCOLOR GetCollisionColor( int collision, int groupnumber )
{
    const D3DCOLOR ColorCollide = 0xFFFF0000;
    const D3DCOLOR ColorPartialCollide = 0xFFFFFF00;
    const D3DCOLOR ColorNoCollide = 0xFF80C080;

    // special case: a value of 1 for groups 1 and higher needs to register as a full collision
    if( groupnumber > 0 && collision > 0 )
        collision = 2;

    switch( collision )
    {
        case 0:
            return ColorNoCollide;
        case 1:
            return ColorPartialCollide;
        case 2:
        default:
            return ColorCollide;
    }
}


//--------------------------------------------------------------------------------------
// Renders collision objects
//--------------------------------------------------------------------------------------
void RenderObjects( IDirect3DDevice9* pd3dDevice )
{
    // Set up some color constants
    const D3DCOLOR ColorWhite = 0xFFFFFFFF;
    const D3DCOLOR ColorGround = 0xFF000000;
    const D3DCOLOR ColorYellow = 0xFFFFFF00;

    // Draw ground planes
    for( int i = 0; i < CAMERA_COUNT; ++i )
    {
        XMFLOAT3 vXAxis( 20, 0,  0 );
        XMFLOAT3 vYAxis( 0, 0, 20 );
        XMFLOAT3 vOrigin( XMVectorGetX( g_CameraOrigins[i] ),
                          XMVectorGetY( g_CameraOrigins[i] ) - 10.f,
                          XMVectorGetZ( g_CameraOrigins[i] ) );
        const int iXDivisions = 20;
        const int iYDivisions = 20;
        DrawGrid( pd3dDevice, vXAxis, vYAxis, vOrigin, iXDivisions, iYDivisions, ColorGround );
    }

    // Draw primary collision objects in white
    DrawFrustum( pd3dDevice, g_PrimaryFrustum, ColorWhite );
    DrawAabb( pd3dDevice, g_PrimaryAABox, ColorWhite );
    DrawObb( pd3dDevice, g_PrimaryOrientedBox, ColorWhite );

    {
        XMFLOAT3 Origin;
        XMFLOAT3 Direction;
        XMStoreFloat3( &Origin, g_PrimaryRay.origin );
        XMStoreFloat3( &Direction, XMVectorScale( g_PrimaryRay.direction, 10.0f ) );
        DrawRay( pd3dDevice, Origin, Direction, FALSE, 0xFF505050 );
        XMStoreFloat3( &Direction, g_PrimaryRay.direction );
        DrawRay( pd3dDevice, Origin, Direction, FALSE, ColorWhite );
    }

    // Draw secondary collision objects in colors based on collision results
    for( int i = 0; i < GROUP_COUNT; ++i )
    {
        const CollisionSphere& sphere = g_SecondarySpheres[i];
        D3DCOLOR c = GetCollisionColor( sphere.collision, i );
        DrawSphere( pd3dDevice, sphere.sphere, c );

        const CollisionBox& obox = g_SecondaryOrientedBoxes[i];
        c = GetCollisionColor( obox.collision, i );
        DrawObb( pd3dDevice, obox.obox, c );

        const CollisionAABox& aabox = g_SecondaryAABoxes[i];
        c = GetCollisionColor( aabox.collision, i );
        DrawAabb( pd3dDevice, aabox.aabox, c );

        const CollisionTriangle& tri = g_SecondaryTriangles[i];
        c = GetCollisionColor( tri.collision, i );
        XMFLOAT3 Verts[3];
        XMStoreFloat3( &Verts[0], tri.pointa );
        XMStoreFloat3( &Verts[1], tri.pointb );
        XMStoreFloat3( &Verts[2], tri.pointc );
        DrawTriangle( pd3dDevice, Verts[0], Verts[1], Verts[2], c );
    }

    // Draw results of ray-object intersection, if there was a hit this frame
    if( g_RayHitResultBox.collision )
        DrawAabb( pd3dDevice, g_RayHitResultBox.aabox, ColorYellow );
}


//--------------------------------------------------------------------------------------
// Sets the camera to view a particular group of objects
//--------------------------------------------------------------------------------------
void SetViewForGroup( int group )
{
    assert( group < GROUP_COUNT );

    g_Camera.Reset();

    XMFLOAT3 vecAt;
    XMStoreFloat3( &vecAt, g_CameraOrigins[group] );

    XMFLOAT3 vecEye( vecAt.x, vecAt.y + 20.0f, (group == 0) ? (vecAt.z + 20.f) : (vecAt.z - 20.f) );
    g_Camera.SetViewParams( (D3DXVECTOR3*)&vecEye, (D3DXVECTOR3*)&vecAt );

    g_Camera.SetModelCenter( *((D3DXVECTOR3*)&vecAt) );
}


//--------------------------------------------------------------------------------------
// Simple draw operations
//--------------------------------------------------------------------------------------
void DrawGrid( IDirect3DDevice9* pd3dDevice, const XMFLOAT3& XAxis, const XMFLOAT3& YAxis,
               const XMFLOAT3& Origin, int iXDivisions, int iYDivisions, D3DCOLOR Color )
{
    HRESULT hr;

    iXDivisions = max( 1, iXDivisions );
    iYDivisions = max( 1, iYDivisions );

    // build grid geometry
    INT iLineCount = iXDivisions + iYDivisions + 2;
    assert( (2*iLineCount) <= MAX_VERTS );

    XMFLOAT3* pLines = 0;
    V( g_pVB->Lock( 0, 0, (void**)&pLines, D3DLOCK_DISCARD ) )

    XMVECTOR vX = XMLoadFloat3( &XAxis );
    XMVECTOR vY = XMLoadFloat3( &YAxis );
    XMVECTOR vOrigin = XMLoadFloat3( &Origin );

    for( INT i = 0; i <= iXDivisions; i++ )
    {
        FLOAT fPercent = ( FLOAT )i / ( FLOAT )iXDivisions;
        fPercent = ( fPercent * 2.0f ) - 1.0f;
        XMVECTOR vScale = XMVectorScale( vX, fPercent );
        vScale = XMVectorAdd( vScale, vOrigin );
        XMStoreFloat3( &pLines[ ( i * 2 ) ], XMVectorSubtract( vScale, vY ) );
        XMStoreFloat3( &pLines[ ( i * 2 ) + 1 ], XMVectorAdd( vScale, vY ) );
    }

    INT iStartIndex = ( iXDivisions + 1 ) * 2;
    for( INT i = 0; i <= iYDivisions; i++ )
    {
        FLOAT fPercent = ( FLOAT )i / ( FLOAT )iYDivisions;
        fPercent = ( fPercent * 2.0f ) - 1.0f;
        XMVECTOR vScale = XMVectorScale( vY, fPercent );
        vScale = XMVectorAdd( vScale, vOrigin );
        XMStoreFloat3( &pLines[ ( i * 2 ) + iStartIndex ], XMVectorSubtract( vScale, vX ) );
        XMStoreFloat3( &pLines[ ( i * 2 ) + 1 + iStartIndex ], XMVectorAdd( vScale, vX ) );
    }

    V( g_pVB->Unlock() )

    // draw grid
    D3DXCOLOR clr = Color;
    g_pEffect9->SetFloatArray( g_Color, clr, 4 );
    g_pEffect9->CommitChanges();
    pd3dDevice->DrawPrimitive( D3DPT_LINELIST, 0, iLineCount );
}

void DrawFrustum( IDirect3DDevice9* pd3dDevice, const XNA::Frustum& frustum, D3DCOLOR Color )
{
    // compute corner points
    XMVECTOR Origin = XMVectorSet( frustum.Origin.x, frustum.Origin.y, frustum.Origin.z, 0 );
    FLOAT Near = frustum.Near;
    FLOAT Far = frustum.Far;
    FLOAT RightSlope = frustum.RightSlope;
    FLOAT LeftSlope = frustum.LeftSlope;
    FLOAT TopSlope = frustum.TopSlope;
    FLOAT BottomSlope = frustum.BottomSlope;

    XMFLOAT3 CornerPoints[8];
    CornerPoints[0] = XMFLOAT3( RightSlope * Near, TopSlope * Near, Near );
    CornerPoints[1] = XMFLOAT3( LeftSlope * Near, TopSlope * Near, Near );
    CornerPoints[2] = XMFLOAT3( LeftSlope * Near, BottomSlope * Near, Near );
    CornerPoints[3] = XMFLOAT3( RightSlope * Near, BottomSlope * Near, Near );

    CornerPoints[4] = XMFLOAT3( RightSlope * Far, TopSlope * Far, Far );
    CornerPoints[5] = XMFLOAT3( LeftSlope * Far, TopSlope * Far, Far );
    CornerPoints[6] = XMFLOAT3( LeftSlope * Far, BottomSlope * Far, Far );
    CornerPoints[7] = XMFLOAT3( RightSlope * Far, BottomSlope * Far, Far );

    XMVECTOR Orientation = XMLoadFloat4( &frustum.Orientation );
    XMMATRIX Mat = XMMatrixRotationQuaternion( Orientation );
    for( UINT i = 0; i < 8; i++ )
    {
        XMVECTOR Result = XMVector3Transform( XMLoadFloat3( &CornerPoints[i] ), Mat );
        Result = XMVectorAdd( Result, Origin );
        XMStoreFloat3( &CornerPoints[i], Result );
    }

    // copy to vertex buffer
    assert( (12 * 2) <= MAX_VERTS );

    XMFLOAT3* pLines = 0;
    HRESULT hr;
    V( g_pVB->Lock( 0, 0, (void**)&pLines, D3DLOCK_DISCARD ) )

    pLines[0] = CornerPoints[0];
    pLines[1] = CornerPoints[1];
    pLines[2] = CornerPoints[1];
    pLines[3] = CornerPoints[2];
    pLines[4] = CornerPoints[2];
    pLines[5] = CornerPoints[3];
    pLines[6] = CornerPoints[3];
    pLines[7] = CornerPoints[0];

    pLines[8] = CornerPoints[0];
    pLines[9] = CornerPoints[4];
    pLines[10] = CornerPoints[1];
    pLines[11] = CornerPoints[5];
    pLines[12] = CornerPoints[2];
    pLines[13] = CornerPoints[6];
    pLines[14] = CornerPoints[3];
    pLines[15] = CornerPoints[7];

    pLines[16] = CornerPoints[4];
    pLines[17] = CornerPoints[5];
    pLines[18] = CornerPoints[5];
    pLines[19] = CornerPoints[6];
    pLines[20] = CornerPoints[6];
    pLines[21] = CornerPoints[7];
    pLines[22] = CornerPoints[7];
    pLines[23] = CornerPoints[4];

    V( g_pVB->Unlock() )

    // draw frustum
    D3DXCOLOR clr = Color;
    g_pEffect9->SetFloatArray( g_Color, clr, 4 );
    g_pEffect9->CommitChanges();
    pd3dDevice->DrawPrimitive( D3DPT_LINELIST, 0, 12 );
}

void DrawCube( IDirect3DDevice9* pd3dDevice, const XMMATRIX& matWorld, D3DCOLOR Color )
{
    static XMVECTOR verts[8] =
    {
        { -1, -1, -1, 0 },
        { 1, -1, -1, 0 },
        { 1, -1, 1, 0 },
        { -1, -1, 1, 0 },
        { -1, 1, -1, 0 },
        { 1, 1, -1, 0 },
        { 1, 1, 1, 0 },
        { -1, 1, 1, 0 }
    };
    static const WORD indices[] =
    {
        0, 1,
        1, 2,
        2, 3,
        3, 0,
        4, 5,
        5, 6,
        6, 7,
        7, 4,
        0, 4,
        1, 5,
        2, 6,
        3, 7
    };

    // copy to vertex buffer
    assert( 8 <= MAX_VERTS );

    XMFLOAT3* pVerts = NULL;
    HRESULT hr;
    V( g_pVB->Lock( 0, 0, (void**)&pVerts, D3DLOCK_DISCARD ) )

    for( int i=0; i < 8; ++i )
    {
        XMVECTOR v = XMVector3Transform( verts[i], matWorld );
        XMStoreFloat3( &pVerts[i], v );
    }

    V( g_pVB->Unlock() )

    // copy to index buffer
    assert( 24 <= MAX_INDEX );

    WORD *pIndices = NULL;
    V( g_pIB->Lock( 0, 0, (void**)&pIndices, D3DLOCK_DISCARD ) )

    memcpy( pIndices, indices, sizeof(indices) );

    V( g_pIB->Unlock() )

    // Draw box
    D3DXCOLOR clr = Color;
    g_pEffect9->SetFloatArray( g_Color, clr, 4 );
    g_pEffect9->CommitChanges();
    pd3dDevice->SetIndices( g_pIB );
    pd3dDevice->DrawIndexedPrimitive( D3DPT_LINELIST, 0, 0, 8, 0, 12 );
}

void DrawAabb( IDirect3DDevice9* pd3dDevice, const XNA::AxisAlignedBox& box, D3DCOLOR Color )
{
    XMMATRIX matWorld = XMMatrixScaling( box.Extents.x, box.Extents.y, box.Extents.z );
    XMVECTOR position = XMLoadFloat3( &box.Center );
    matWorld.r[3] = XMVectorSelect( matWorld.r[3], position, XMVectorSelectControl( 1, 1, 1, 0 ) );

    DrawCube( pd3dDevice, matWorld, Color );
}

void DrawObb( IDirect3DDevice9* pd3dDevice, const XNA::OrientedBox& obb, D3DCOLOR Color )
{
    XMMATRIX matWorld = XMMatrixRotationQuaternion( XMLoadFloat4( &obb.Orientation ) );
    XMMATRIX matScale = XMMatrixScaling( obb.Extents.x, obb.Extents.y, obb.Extents.z );
    matWorld = XMMatrixMultiply( matScale, matWorld );
    XMVECTOR position = XMLoadFloat3( &obb.Center );
    matWorld.r[3] = XMVectorSelect( matWorld.r[3], position, XMVectorSelectControl( 1, 1, 1, 0 ) );

    DrawCube( pd3dDevice, matWorld, Color );
}

void DrawRing( IDirect3DDevice9* pd3dDevice, const XMFLOAT3& Origin, const XMFLOAT3& MajorAxis,
               const XMFLOAT3& MinorAxis, D3DCOLOR Color )
{
    static const DWORD dwRingSegments = 32;

    XMFLOAT3 verts[ dwRingSegments + 1 ];

    XMVECTOR vOrigin = XMLoadFloat3( &Origin );
    XMVECTOR vMajor = XMLoadFloat3( &MajorAxis );
    XMVECTOR vMinor = XMLoadFloat3( &MinorAxis );

    FLOAT fAngleDelta = XM_2PI / ( float )dwRingSegments;
    // Instead of calling cos/sin for each segment we calculate
    // the sign of the angle delta and then incrementally calculate sin
    // and cosine from then on.
    XMVECTOR cosDelta = XMVectorReplicate( cosf( fAngleDelta ) );
    XMVECTOR sinDelta = XMVectorReplicate( sinf( fAngleDelta ) );
    XMVECTOR incrementalSin = XMVectorZero();
    static const XMVECTOR initialCos =
    {
        1.0f, 1.0f, 1.0f, 1.0f
    };
    XMVECTOR incrementalCos = initialCos;
    for( DWORD i = 0; i < dwRingSegments; i++ )
    {
        XMVECTOR Pos;
        Pos = XMVectorMultiplyAdd( vMajor, incrementalCos, vOrigin );
        Pos = XMVectorMultiplyAdd( vMinor, incrementalSin, Pos );
        XMStoreFloat3( ( XMFLOAT3* )&verts[i], Pos );
        // Standard formula to rotate a vector.
        XMVECTOR newCos = incrementalCos * cosDelta - incrementalSin * sinDelta;
        XMVECTOR newSin = incrementalCos * sinDelta + incrementalSin * cosDelta;
        incrementalCos = newCos;
        incrementalSin = newSin;
    }
    verts[ dwRingSegments ] = verts[0];

    // Copy to vertex buffer
    assert( (dwRingSegments+1) <= MAX_VERTS );

    XMFLOAT3* pVerts = NULL;
    HRESULT hr;
    V( g_pVB->Lock( 0, 0, (void**)&pVerts, D3DLOCK_DISCARD ) )
    memcpy( pVerts, verts, sizeof(verts) );
    V( g_pVB->Unlock() )

    // Draw ring
    D3DXCOLOR clr = Color;
    g_pEffect9->SetFloatArray( g_Color, clr, 4 );
    g_pEffect9->CommitChanges();
    pd3dDevice->DrawPrimitive( D3DPT_LINESTRIP, 0, dwRingSegments );
}

void DrawSphere( IDirect3DDevice9* pd3dDevice, const XNA::Sphere& sphere, D3DCOLOR Color )
{
    const XMFLOAT3 Origin = sphere.Center;
    const float fRadius = sphere.Radius;

    DrawRing( pd3dDevice, Origin, XMFLOAT3( fRadius, 0, 0 ), XMFLOAT3( 0, 0, fRadius ), Color );
    DrawRing( pd3dDevice, Origin, XMFLOAT3( fRadius, 0, 0 ), XMFLOAT3( 0, fRadius, 0 ), Color );
    DrawRing( pd3dDevice, Origin, XMFLOAT3( 0, fRadius, 0 ), XMFLOAT3( 0, 0, fRadius ), Color );
}

void DrawRay( IDirect3DDevice9* pd3dDevice, const XMFLOAT3& Origin, const XMFLOAT3& Direction,
              BOOL bNormalize, D3DCOLOR Color )
{
    XMFLOAT3 verts[3];
    memcpy( &verts[0], &Origin, 3 * sizeof( FLOAT ) );

    XMVECTOR RayOrigin = XMLoadFloat3( &Origin );
    XMVECTOR RayDirection = XMLoadFloat3( &Direction );
    XMVECTOR NormDirection = XMVector3Normalize( RayDirection );
    if( bNormalize )
        RayDirection = NormDirection;

    XMVECTOR PerpVector;
    XMVECTOR CrossVector = XMVectorSet( 0, 1, 0, 0 );
    PerpVector = XMVector3Cross( NormDirection, CrossVector );

    if( XMVector3Equal( XMVector3LengthSq( PerpVector ), XMVectorSet( 0, 0, 0, 0 ) ) )
    {
        CrossVector = XMVectorSet( 0, 0, 1, 0 );
        PerpVector = XMVector3Cross( NormDirection, CrossVector );
    }
    PerpVector = XMVector3Normalize( PerpVector );

    XMStoreFloat3( ( XMFLOAT3* )&verts[1], XMVectorAdd( RayDirection, RayOrigin ) );
    PerpVector = XMVectorScale( PerpVector, 0.0625f );
    NormDirection = XMVectorScale( NormDirection, -0.25f );
    RayDirection = XMVectorAdd( PerpVector, RayDirection );
    RayDirection = XMVectorAdd( NormDirection, RayDirection );
    XMStoreFloat3( ( XMFLOAT3* )&verts[2], XMVectorAdd( RayDirection, RayOrigin ) );
    
    // Copy to vertex buffer
    assert( 3 <= MAX_VERTS );
    XMFLOAT3* pVerts = NULL;
    HRESULT hr;
    V( g_pVB->Lock( 0, 0, (void**)&pVerts, D3DLOCK_DISCARD ) )
    memcpy( pVerts, verts, sizeof(verts) );
    V( g_pVB->Unlock() )

    // Draw ray
    D3DXCOLOR clr = Color;
    g_pEffect9->SetFloatArray( g_Color, clr, 4 );
    g_pEffect9->CommitChanges();
    pd3dDevice->DrawPrimitive( D3DPT_LINESTRIP, 0, 2 );
}

void DrawTriangle( IDirect3DDevice9* pd3dDevice, const XMFLOAT3& PointA, const XMFLOAT3& PointB,
                   const XMFLOAT3& PointC, D3DCOLOR Color )
{
    // Copy to vertex buffer
    assert( 4 <= MAX_VERTS );
    XMFLOAT3* pVerts = NULL;
    HRESULT hr;
    V( g_pVB->Lock( 0, 0, (void**)&pVerts, D3DLOCK_DISCARD ) )
    pVerts[0] = PointA;
    pVerts[1] = PointB;
    pVerts[2] = PointC;
    pVerts[3] = PointA;
    V( g_pVB->Unlock() )

    // Draw triangle
    D3DXCOLOR clr = Color;
    g_pEffect9->SetFloatArray( g_Color, clr, 4 );
    g_pEffect9->CommitChanges();
    pd3dDevice->DrawPrimitive( D3DPT_LINESTRIP, 0, 3 );
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
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"Collision.fx" ) );
    V_RETURN( D3DXCreateEffectFromFile( pd3dDevice, str, NULL, NULL, dwShaderFlags | D3DXFX_LARGEADDRESSAWARE,
                                        NULL, &g_pEffect9, NULL ) );

    // Get handles to the effect's parameters (required since we are using D3DXFX_LARGEADDRESS_HANDLE)
    g_pmWorldViewProj = g_pEffect9->GetParameterByName( NULL, "g_mWorldViewProjection" );
    g_Color = g_pEffect9->GetParameterByName( NULL, "g_Color" );

    if ( !g_pmWorldViewProj || !g_Color )
        return E_FAIL;

    // Setup the camera's view parameters
    CDXUTComboBox* pComboBox = g_SampleUI. GetComboBox( IDC_GROUP );
    SetViewForGroup( (pComboBox) ? (int)PtrToInt( pComboBox->GetSelectedData() ) : 0 );

    // Create drawing resources
    static const D3DVERTEXELEMENT9 elements[ 2 ] =
    {
        { 0,     0, D3DDECLTYPE_FLOAT3,     0,  D3DDECLUSAGE_POSITION,  0 },
        D3DDECL_END()
    };

    V_RETURN( pd3dDevice->CreateVertexDeclaration( elements, &g_pVertexDecl ) );

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

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 0.1f, 1000.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 350 );
    g_SampleUI.SetSize( 170, 300 );

    // Create our draw buffers
    V_RETURN( pd3dDevice->CreateVertexBuffer( MAX_VERTS * sizeof( XMPosVertex ), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                              0, D3DPOOL_DEFAULT, &g_pVB, NULL ) );

    V_RETURN( pd3dDevice->CreateIndexBuffer( MAX_INDEX * sizeof(WORD), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
                                             D3DPOOL_DEFAULT, &g_pIB, NULL ) );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update position of collision objects
    Animate( fTime );

    // Compute collisions
    Collide();

    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;
    D3DXMATRIXA16 mWorld;
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;
    D3DXMATRIXA16 mWorldViewProjection;

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
        D3DXHANDLE htech = g_pEffect9->GetTechniqueByName( "RenderScene" );
        V( g_pEffect9->SetTechnique( htech ) );

        // Get the projection & view matrix from the camera class
        mWorld = *g_Camera.GetWorldMatrix();
        mProj = *g_Camera.GetProjMatrix();
        mView = *g_Camera.GetViewMatrix();

        mWorldViewProjection = mWorld * mView * mProj;

        // Update the effect's variables
        V( g_pEffect9->SetMatrix( g_pmWorldViewProj, &mWorldViewProjection ) );

        DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"World" ); // These events are to help PIX identify what the code is doing
        UINT cPasses;
        V( g_pEffect9->Begin( &cPasses, 0 ) );

        V( pd3dDevice->SetStreamSource( 0, g_pVB, 0, sizeof(XMPosVertex) ) );
        V( pd3dDevice->SetVertexDeclaration( g_pVertexDecl ) );

        for( UINT iPass = 0; iPass < cPasses; ++iPass )
        {
            V( g_pEffect9->BeginPass( iPass ) );
            RenderObjects( pd3dDevice );
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

        pd3dDevice->SetStreamSource( 0, NULL, 0, 0 );
        pd3dDevice->SetIndices( NULL );
    }
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

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    switch (nChar)
    {
    case '1':
    case '2':
    case '3':
    case '4':
        {
            int group = (nChar - '1');
            CDXUTComboBox* pComboBox = g_SampleUI. GetComboBox( IDC_GROUP );
            assert(pComboBox != NULL);
            pComboBox->SetSelectedByData( IntToPtr( group ) );
            SetViewForGroup( group );
        }
        break;
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
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;
        case IDC_GROUP:
            {
                CDXUTComboBox* pComboBox = ( CDXUTComboBox* )pControl;
                SetViewForGroup( (int)PtrToInt( pComboBox->GetSelectedData() ) );
            }
            break;
    }
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9LostDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D9LostDevice();
    g_SettingsDlg.OnD3D9LostDevice();
    if( g_pFont9 ) g_pFont9->OnLostDevice();

    SAFE_RELEASE( g_pVB );
    SAFE_RELEASE( g_pIB );

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
}


