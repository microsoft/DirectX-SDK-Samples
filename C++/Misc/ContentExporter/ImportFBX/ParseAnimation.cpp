//-------------------------------------------------------------------------------------
//  ParseAnimation.cpp
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "StdAfx.h"
#include "ParseAnimation.h"

extern ATG::ExportScene* g_pScene;

struct AnimationScanNode
{
    INT iParentIndex;
    KFbxNode* pNode;
    ExportAnimationTrack* pTrack;
    DWORD dwFlags;
    D3DXMATRIX matGlobal;
};

typedef vector<AnimationScanNode> ScanList;

VOID ParseNode( KFbxNode* pNode, ScanList& scanlist, DWORD dwFlags, INT iParentIndex, BOOL bIncludeNode )
{
    INT iCurrentIndex = iParentIndex;

    if( !bIncludeNode )
    {
        const CHAR* strNodeName = pNode->GetName();
        if( _stricmp( strNodeName, g_pScene->Settings().strAnimationRootNodeName ) == 0 )
        {
            bIncludeNode = TRUE;
        }
    }

    if( bIncludeNode )
    {
        iCurrentIndex = (INT)scanlist.size();

        // add node to anim list
        AnimationScanNode asn = { 0 };
        asn.iParentIndex = iParentIndex;
        asn.pNode = pNode;
        asn.dwFlags = dwFlags;
        scanlist.push_back( asn );
    }

    DWORD dwChildCount = pNode->GetChildCount();
    for( DWORD i = 0; i < dwChildCount; ++i )
    {
        ParseNode( pNode->GetChild( i ), scanlist, dwFlags, iCurrentIndex, bIncludeNode );
    }
}

D3DXMATRIX ConvertMatrix( const KFbxXMatrix& matrix )
{
    D3DXMATRIX matResult;
    FLOAT* fData = (FLOAT*)&matResult;
    DOUBLE* pSrcData = (DOUBLE*)&matrix;
    for( DWORD i = 0; i < 16; ++i )
    {
        fData[i] = (FLOAT)pSrcData[i];
    }
    return matResult;
}


VOID AddKey( AnimationScanNode& asn, const AnimationScanNode* pParent, KFbxXMatrix& matFBXGlobal, FLOAT fTime )
{
    D3DXMATRIX matGlobal = ConvertMatrix( matFBXGlobal );
    asn.matGlobal = matGlobal;
    D3DXMATRIX matLocal = matGlobal;
    if( pParent != NULL )
    {
        D3DXMATRIX matInvParentGlobal;
        D3DXMatrixInverse( &matInvParentGlobal, NULL, &pParent->matGlobal );

        D3DXMatrixMultiply( &matLocal, &matGlobal, &matInvParentGlobal );
    }

    D3DXMATRIX matLocalFinal;
    g_pScene->GetDCCTransformer()->TransformMatrix( &matLocalFinal, &matLocal );

    XMVECTOR vScale;
    XMVECTOR qRotation;
    XMVECTOR vTranslation;

    XMMatrixDecompose( &vScale, &qRotation, &vTranslation, (XMMATRIX)matLocalFinal );

    asn.pTrack->TransformTrack.AddKey( fTime, *(D3DXVECTOR3*)&vTranslation, *(D3DXQUATERNION*)&qRotation, *(D3DXVECTOR3*)&vScale );
}

VOID CaptureAnimation( ScanList& scanlist, ExportAnimation* pAnim )
{
    const FLOAT fDeltaTime = pAnim->fSourceSamplingInterval;
    const FLOAT fStartTime = pAnim->fStartTime;
    const FLOAT fEndTime = pAnim->fEndTime;
    FLOAT fCurrentTime = fStartTime;

    DWORD dwNodeCount = (DWORD)scanlist.size();

    ExportLog::LogMsg( 2, "Capturing animation data from %d nodes, from time %0.3f to %0.3f, at an interval of %0.3f seconds.", dwNodeCount, fStartTime, fEndTime, fDeltaTime );

    while( fCurrentTime <= fEndTime )
    {
        KTime CurrentTime;
        CurrentTime.SetSecondDouble( fCurrentTime );
        for( DWORD i = 0; i < dwNodeCount; ++i )
        {
            AnimationScanNode& asn = scanlist[i];
            KFbxXMatrix& matGlobal = asn.pNode->GetGlobalFromCurrentTake( CurrentTime );
            AnimationScanNode* pParent = NULL;
            if( asn.iParentIndex >= 0 )
                pParent = &scanlist[asn.iParentIndex];
            AddKey( asn, pParent, matGlobal, fCurrentTime - fStartTime );
        }
        fCurrentTime += fDeltaTime;
    }
}

VOID ParseTake( KFbxScene* pFbxScene, KString* strTakeName )
{
    if( strTakeName->Compare( KFBXTAKENODE_DEFAULT_NAME ) == 0 )
    {
        return;
    }

    pFbxScene->SetCurrentTake( strTakeName->Buffer() );
    KFbxTakeInfo* pTakeInfo = pFbxScene->GetTakeInfo( *strTakeName );

    ExportLog::LogMsg( 2, "Parsing animation \"%s\"", strTakeName->Buffer() );

    ExportAnimation* pAnim = new ExportAnimation();
    pAnim->SetName( strTakeName->Buffer() );
    pAnim->SetDCCObject( pTakeInfo );
    g_pScene->AddAnimation( pAnim );

    KTime FrameTime;
    FrameTime.SetTime( 0, 0, 0, 1, 0, pFbxScene->GetGlobalTimeSettings().GetTimeMode() );

    //KTime::ETimeMode TimeMode = pFbxScene->GetGlobalTimeSettings().GetTimeMode();
    FLOAT fStartTime = 0.0f;
    FLOAT fEndTime = 1.0f;
    //FLOAT fFrameTime = 1.0f / (FLOAT)KTime::GetFrameRate( TimeMode );
    FLOAT fFrameTime = (FLOAT)FrameTime.GetSecondDouble();
    FLOAT fSampleTime = fFrameTime / (FLOAT)g_pScene->Settings().iAnimSampleCountPerFrame;
    assert( fSampleTime > 0 );

    if( pTakeInfo != NULL )
    {
        fStartTime = (FLOAT)pTakeInfo->mLocalTimeSpan.GetStart().GetSecondDouble();
        fEndTime = (FLOAT)pTakeInfo->mLocalTimeSpan.GetStop().GetSecondDouble();
    }
    else
    {
        ExportLog::LogWarning( "Animation take \"%s\" has no takeinfo; using defaults.", pAnim->GetName().SafeString() );
    }
    pAnim->fStartTime = fStartTime;
    pAnim->fEndTime = fEndTime;
    pAnim->fSourceFrameInterval = fFrameTime;
    pAnim->fSourceSamplingInterval = fSampleTime;

    BOOL bIncludeAllNodes = TRUE;
    if( strlen( g_pScene->Settings().strAnimationRootNodeName ) > 0 )
    {
        bIncludeAllNodes = FALSE;
    }

    ScanList scanlist;
    ParseNode( pFbxScene->GetRootNode(), scanlist, 0, -1, bIncludeAllNodes );

    DWORD dwTrackCount = (DWORD)scanlist.size();
    for( DWORD i = 0; i < dwTrackCount; ++i )
    {
        const CHAR* strTrackName = scanlist[i].pNode->GetName();
        ExportLog::LogMsg( 4, "Track: %s", strTrackName );
        ExportAnimationTrack* pTrack = new ExportAnimationTrack();
        pTrack->SetName( strTrackName );
        pTrack->TransformTrack.pSourceFrame = g_pScene->FindFrameByDCCObject( scanlist[i].pNode );
        pAnim->AddTrack( pTrack );
        scanlist[i].pTrack = pTrack;
    }

    CaptureAnimation( scanlist, pAnim );

    pAnim->Optimize();
}

VOID ParseAnimation( KFbxScene* pFbxScene )
{
    assert( pFbxScene != NULL );

    // set animation quality settings
    ExportAnimation::SetAnimationExportQuality( g_pScene->Settings().iAnimPositionExportQuality, g_pScene->Settings().iAnimOrientationExportQuality, 50 );

    KArrayTemplate<KString*> TakeNameArray;
    pFbxScene->FillTakeNameArray( TakeNameArray );

    DWORD dwTakeCount = (DWORD)TakeNameArray.GetCount();
    for( DWORD i = 0; i < dwTakeCount; ++i )
    {
        ParseTake( pFbxScene, TakeNameArray.GetAt(i) );
    }
}
