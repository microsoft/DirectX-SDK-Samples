//-------------------------------------------------------------------------------------
//  ParseMisc.cpp
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "StdAfx.h"
#include "ParseMisc.h"
#include "ParseMesh.h"

extern ATG::ExportScene* g_pScene;

D3DXMATRIX ConvertMatrix( const KFbxMatrix& matFbx )
{
	D3DXMATRIX matConverted;
	FLOAT* pFloats = (FLOAT*)&matConverted;
	const DOUBLE* pDoubles = (const DOUBLE*)matFbx.mData;
	for( DWORD i = 0; i < 16; ++i )
	{
		pFloats[i] = (FLOAT)pDoubles[i];
	}
	return matConverted;
}

inline BOOL IsEqual( FLOAT A, FLOAT B )
{
    return fabs( A - B ) <= 1e-5f;
}

D3DXMATRIX ParseTransform( KFbxNode* pNode, ExportFrame* pFrame, const D3DXMATRIX& matParentWorld, const BOOL bWarnings = TRUE )
{
	D3DXMATRIX matWorld;
	D3DXMATRIX matLocal;
	BOOL bProcessDefaultTransform = TRUE;

	if( !g_BindPoseMap.empty() )
	{
        PoseMap::iterator iter = g_BindPoseMap.find( pNode );
        if( iter != g_BindPoseMap.end() )
        {
            KFbxMatrix PoseMatrix = iter->second;
            matWorld = ConvertMatrix( PoseMatrix );
            D3DXMATRIX matInvParentWorld;
            D3DXMatrixInverse( &matInvParentWorld, NULL, &matParentWorld );
            D3DXMatrixMultiply( &matLocal, &matWorld, &matInvParentWorld );
            bProcessDefaultTransform = FALSE;
        }
	}

	if( bProcessDefaultTransform )
	{
		KFbxVector4 Translation, Rotation, Scale;
		pNode->GetDefaultT( Translation );
		pNode->GetDefaultR( Rotation );
		pNode->GetDefaultS( Scale );

		KFbxMatrix matTransform( Translation, Rotation, Scale );
		matLocal = ConvertMatrix( matTransform );
		D3DXMatrixMultiply( &matWorld, &matParentWorld, &matLocal );
	}

    pFrame->Transform().InitializeFromFloats( (FLOAT*)&matLocal );

    const D3DXVECTOR3& Scale = pFrame->Transform().Scale();
    if( bWarnings && 
        ( !IsEqual( Scale.x, Scale.y ) ||
          !IsEqual( Scale.y, Scale.z ) ||
          !IsEqual( Scale.x, Scale.z ) ) )
    {
        ExportLog::LogWarning( "Non-uniform scale found on node \"%s\".", pFrame->GetName().SafeString() );
    }

    const ExportTransform& Transform = pFrame->Transform();
    ExportLog::LogMsg( 5, "Node transform for \"%s\": Translation <%0.3f %0.3f %0.3f> Rotation <%0.3f %0.3f %0.3f %0.3f> Scale <%0.3f %0.3f %0.3f>",
        pFrame->GetName().SafeString(),
        Transform.Position().x,
        Transform.Position().y,
        Transform.Position().z,
        Transform.Orientation().x,
        Transform.Orientation().y,
        Transform.Orientation().z,
        Transform.Orientation().w,
        Transform.Scale().x,
        Transform.Scale().y,
        Transform.Scale().z );

	return matWorld;
}


VOID ParseNode( KFbxNode* pNode, ExportFrame* pParentFrame, const D3DXMATRIX& matParentWorld )
{
    ExportLog::LogMsg( 2, "Parsing node \"%s\".", pNode->GetName() );

    ExportFrame* pFrame = new ExportFrame( pNode->GetName() );
    pFrame->SetDCCObject( pNode );
    D3DXMATRIX matWorld = ParseTransform( pNode, pFrame, matParentWorld );
    pParentFrame->AddChild( pFrame );

    if( pNode->GetSubdiv() != NULL )
    {
        ParseSubDiv( pNode->GetSubdiv(), pFrame );
    }
    else if( pNode->GetMesh() != NULL )
    {
        ParseMesh( pNode->GetMesh(), pFrame, FALSE );
    }
    ParseCamera( pNode->GetCamera(), pFrame );
    ParseLight( pNode->GetLight(), pFrame );

    DWORD dwChildCount = pNode->GetChildCount();
    for( DWORD i = 0; i < dwChildCount; ++i )
    {
        ParseNode( pNode->GetChild( i ), pFrame, matWorld );
    }
}

VOID FixupNode( ExportFrame* pFrame, const D3DXMATRIX& matParentWorld )
{
    KFbxNode* pNode = (KFbxNode*)pFrame->GetDCCObject();

    D3DXMATRIX matWorld;
    if( pNode != NULL )
    {
        ExportLog::LogMsg( 4, "Fixing up frame \"%s\".", pFrame->GetName().SafeString() );
        matWorld = ParseTransform( pNode, pFrame, matParentWorld, FALSE );
    }
    else
    {
        matWorld = matParentWorld;
    }

    DWORD dwChildCount = pFrame->GetChildCount();
    for( DWORD i = 0; i < dwChildCount; ++i )
    {
        FixupNode( pFrame->GetChildByIndex( i ), matWorld );
    }
}

VOID ParseCamera( KFbxCamera* pFbxCamera, ExportFrame* pParentFrame )
{
    if( pFbxCamera == NULL || !g_pScene->Settings().bExportCameras )
        return;

    ExportLog::LogMsg( 2, "Parsing camera \"%s\".", pFbxCamera->GetName() );

    ExportCamera* pCamera = new ExportCamera( pFbxCamera->GetName() );
    pCamera->SetDCCObject( pFbxCamera );

    pCamera->fNearClip = (FLOAT)pFbxCamera->NearPlane.Get();
    pCamera->fFarClip = (FLOAT)pFbxCamera->FarPlane.Get();
    pCamera->fFieldOfView = (FLOAT)pFbxCamera->FieldOfView.Get();
    pCamera->fFocalLength = (FLOAT)pFbxCamera->FocalLength.Get();

    pParentFrame->AddCamera( pCamera );
}

VOID ParseLight( KFbxLight* pFbxLight, ExportFrame* pParentFrame )
{
    if( pFbxLight == NULL || !g_pScene->Settings().bExportLights )
        return;

    ExportLog::LogMsg( 2, "Parsing light \"%s\".", pFbxLight->GetName() );

    ExportLight* pLight = new ExportLight( pFbxLight->GetName() );
    pLight->SetDCCObject( pFbxLight );
    pParentFrame->AddLight( pLight );

    DOUBLE* pColorRGB = (DOUBLE*)pFbxLight->Color.Get();
    FLOAT fIntensity = (FLOAT)pFbxLight->Intensity.Get();
    fIntensity *= 0.01f;

    D3DXCOLOR Color( (FLOAT)pColorRGB[0], (FLOAT)pColorRGB[1], (FLOAT)pColorRGB[2], 1.0f );
    Color *= fIntensity;
    pLight->Color = Color;

    switch( pFbxLight->DecayType.Get() )
    {
    case KFbxLight::eNONE:
        pLight->Falloff = ExportLight::LF_NONE;
        pLight->fRange = 20.0f;
        break;
    case KFbxLight::eLINEAR:
        pLight->Falloff = ExportLight::LF_LINEAR;
        pLight->fRange = 4.0f * fIntensity;
        break;
    case KFbxLight::eQUADRATIC:
    case KFbxLight::eCUBIC:
        pLight->Falloff = ExportLight::LF_SQUARED;
        pLight->fRange = 2.0f * sqrtf( fIntensity );
        break;
    }

    pLight->fRange *= g_pScene->Settings().fLightRangeScale;

    ExportLog::LogMsg( 4, "Light color (multiplied by intensity): <%0.2f %0.2f %0.2f> intensity: %0.2f falloff: %0.2f", Color.r, Color.g, Color.b, fIntensity, pLight->fRange );

    switch( pFbxLight->LightType.Get() )
    {
    case KFbxLight::ePOINT:
        pLight->Type = ExportLight::LT_POINT;
        break;
    case KFbxLight::eSPOT:
        pLight->Type = ExportLight::LT_SPOT;
        pLight->fOuterAngle = (FLOAT)pFbxLight->ConeAngle.Get();
        // BUG: FBX doesn't keep track of the inner angle (HotSpot) properly, the value is always 0
        //pLight->fInnerAngle = (FLOAT)pFbxLight->HotSpot.Get();
        pLight->fInnerAngle = pLight->fOuterAngle;
        pLight->SpotFalloff = pLight->Falloff;
        break;
    case KFbxLight::eDIRECTIONAL:
        pLight->Type = ExportLight::LT_DIRECTIONAL;
        break;
    default:
        ExportLog::LogWarning( "Could not determine light type." );
        return;
    }
}