//--------------------------------------------------------------------------------------
// File: game.h
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "resource.h"
#include "DXUTcamera.h"
#include "DXUTsettingsdlg.h"
#include "SDKmesh.h"

//--------------------------------------------------------------------------------------
// Consts
//--------------------------------------------------------------------------------------
#define MAXANISOTROPY 8 // MAXANISOTROPY is the maximum anisotropy state value used when anisotropic filtering is enabled.
#define GROUND_Y 3.0f // -GROUND_Y is the Y coordinate of the ground.
#define GROUND_ABSORBANCE 0.2f // GROUND_ABSORBANCE is the percentage of the velocity absorbed by ground and walls when an ammo hits.
#define AMMO_ABSORBANCE 0.1f // AMMO_ABSORBANCE is the percentage of the velocity absorbed by ammos when two collide.
#define MAX_AMMO 100  // MAX_AMMO is the maximum number of ammo that can exist in the world.
#define MAX_DROID 50
#define DROID_HITPOINTS 20
#define AMMO_SIZE 0.10f // AMMO_SIZE is the diameter of the ball mesh in the world.
#define DROID_SIZE 0.5f
#define DROID_MIN_HEIGHT 0.5f
#define DROID_HEIGHT_FLUX 0.5f
#define DROID_TURN_AI_PERCENT 40
#define DROID_MOVE_AI_PERCENT 40
#define DROID_MOVE_TIME_MIN  2000
#define DROID_MOVE_TIME_FLUX 3000
#define DROID_CREATE_DELAY_FLUX 2500
#define DROID_DEATH_SPEED 3.0f
#define AUTOFIRE_DELAY 0.1f  // AUTOFIRE_DELAY is the period between two successive ammo firing.
#define CAMERA_SIZE 0.2f // CAMERA_SIZE is used for clipping camera movement
#define GRAVITY 3.0f  // GRAVITY defines the magnitude of the downward force applied to ammos.
#define DROID_VELOCITY 2.0f // MIN_VOL_ADJUST is the minimum volume adjustment based on contact velocity.
#define BOUNCE_TRANSFER 0.8f // BOUNCE_TRANSFER is the proportion of velocity transferred during a collision between 2 ammos.
#define BOUNCE_LOST 0.1f  // BOUNCE_LOST is the proportion of velocity lost during a collision between 2 ammos.
#define REST_THRESHOLD 0.005f // REST_THRESHOLD is the energy below which the ball is flagged as laying on ground.  // It is defined as Gravity * Height_above_ground + 0.5 * Velocity * Velocity
#define PHYSICS_FRAMELENGTH 0.003f // PHYSICS_FRAMELENGTH is the duration of a frame for physics handling when the graphics frame length is too long.
#define MAX_SOUND_VELOCITY 1.0f // MAX_SOUND_VELOCITY is the velocity at which the bouncing sound is played at maximum volume.  Higher velocity uses maximum volume.
#define MIN_SOUND_VELOCITY 0.07f  // MIN_SOUND_VELOCITY is the minimum contact velocity required to make a sound.
#define MIN_VOL_ADJUST 0.8f  // MIN_VOL_ADJUST is the minimum volume adjustment based on contact velocity.

// MinBound and MaxBound are the bounding box representing the cell mesh.
const D3DXVECTOR3           g_MinBound( -6.0f, -GROUND_Y, -6.0f );
const D3DXVECTOR3           g_MaxBound( 6.0f, GROUND_Y, 6.0f );

enum AI_STATE
{
    AI_TURNING = 1,
    AI_MOVING,
    AI_STOPPED,
};

enum GAME_MODE
{
    GAME_RUNNING = 1,
    GAME_MAIN_MENU,
    GAME_AUDIO_MENU,
    GAME_VIDEO_MENU
};


//--------------------------------------------------------------------------------------
// Class definations
//--------------------------------------------------------------------------------------
struct RENDER_STATE
{
    D3DXMATRIXA16 mCellWorld;        // World matrix for cell mesh
    D3DXMATRIXA16 mAmmoWorld;        // World matrix for ball meshes (scaling)
    D3DXMATRIXA16 mDroidWorld;       // World matrix for ball meshes (scaling)

    ID3DXFont* pFont;        // Font for drawing text
    ID3DXSprite* pTextSprite;  // Sprite for batching draw text calls
    CDXUTDialogResourceManager DialogResourceManager; // manager for shared resources of dialogs
    CDXUTDialog MainMenuDlg;  // dialog for main menu
    CDXUTDialog VideoMenuDlg;  // dialog for video menu
    CDXUTDialog AudioMenuDlg;  // dialog for audio menu

    ID3DXEffect* pEffect;      // D3DX effect interface
    IDirect3DTexture9* pDefaultTex;  // Default texture for un-textured geometry.
    IDirect3DTexture9* pDefaultNormalMap; // Default normal map texture for un-normal maped geometry

    IDirect3DTexture9* pDroidNormalMap;

    CDXUTXFileMesh meshCell;          // Cell mesh object
    CDXUTXFileMesh meshAmmo;          // Ammo mesh object
    CDXUTXFileMesh meshDroid;
    CDXUTXFileMesh meshDroidLow;
    CDXUTXFileMesh meshDroidExplosion;
    CDXUTXFileMesh meshDroidExplosionLow;
    ID3DXMesh* pMeshDroidExplosion;

    D3DXHANDLE hShaderTech;          // Technique to use when using programmable rendering path
    D3DXHANDLE hMatW;                // Handles to transformation matrices in effect
    D3DXHANDLE hMatV;
    D3DXHANDLE hMatP;
    D3DXHANDLE hMatWV;
    D3DXHANDLE hMatWVP;
    D3DXHANDLE hDiffuse;             // Handles to material parameters in effect
    D3DXHANDLE hSpecular;
    D3DXHANDLE hDiffuseTexture;
    D3DXHANDLE hPower;
    D3DXHANDLE hAnimation;
    D3DXHANDLE hNormalMap;

    float UseFixedFunction;
    int ForceShader;
    float MaximumResolution;
    float DisableSpecular;
    bool UseAnisotropicFilter;
    bool UseHighResModels;
    bool bDetectOptimalSettings;
    float fDesktopAspectRatio;

    RECT rcAdapterWork[10];
    CGrowableArray <D3DDISPLAYMODE> aWindowedDMList;
};

struct AMMO_STATE
{
    D3DXMATRIXA16 mAmmoRot;
    bool bActive;    // True if ball is visible
    D3DXVECTOR3 vPosition;
    D3DXVECTOR3 vVelocity;

    D3DXVECTOR4 Diffuse;
    double fTimeCreated;
    float fLifeCountdown;
    float fAlpha;
    bool bGround;    // Whether it is laying on the ground (resting or rolling)
};

struct DROID_STATE
{
    bool bActive;
    D3DXVECTOR3 vPosition;
    D3DXVECTOR3 vVelocity;

    D3DXVECTOR3 vNudgeVelocity;
    AI_STATE aiState;
    float fTargetRotation;
    D3DXQUATERNION qTarget;
    D3DXQUATERNION qStart;
    D3DXQUATERNION qCurrent;
    float fRotInterp;
    float fTaskTimer;
    int nHitPoints;
    float fDeathAnimation;
    float fAlpha;
    D3DXCOLOR Specular;
};

struct GAME_STATE
{
    AMMO_STATE      AmmoQ[MAX_AMMO];    // Queue of ammos in the world
    int nAmmoCount;         // Number of ammos that exist in the world
    float fAmmoColorLerp;
    D3DXCOLOR BlendToColor;
    D3DXCOLOR BlendFromColor;

    DROID_STATE     DroidQ[MAX_DROID];
    int nDroidCount;
    int nMaxDroids;

    bool bDroidCreate;
    bool bMassDroidKill;
    float fDroidCreateCountdown;

    bool bDroidMove;
    bool bAutoAddDroids;

    DXUT_GAMEPAD    gamePad[DXUT_MAX_CONTROLLERS]; // XInput controller state

    GAME_MODE gameMode;
};


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
extern CFirstPersonCamera   g_Camera;
extern RENDER_STATE         g_RenderState;
extern GAME_STATE           g_GameState;


//--------------------------------------------------------------------------------------
// External functions
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
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void CALLBACK OnLostDevice( void* pUserContext );
void CALLBACK OnDestroyDevice( void* pUserContext );
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void InitApp();


