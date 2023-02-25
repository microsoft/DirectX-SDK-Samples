//-------------------------------------------------------------------------------------
//  ExportAnimation.cpp
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "exportanimation.h"

namespace ATG
{
    FLOAT g_fPositionExportTolerance = 0.99f;
    FLOAT g_fOrientationExportTolerance = 0.99f;
    FLOAT g_fScaleExportTolerance = 0.99f;

    FLOAT ConvertToleranceValue( INT iValue )
    {
        FLOAT fExponent = (FLOAT)iValue / 15.0f;
        FLOAT fValue = 1.0f - powf( 10.0f, -fExponent );
        return max( min( fValue, 1.0f ), 0.9f );
    }

    VOID ExportAnimation::SetAnimationExportQuality( INT iPos, INT iOrientation, INT iScale )
    {
        g_fPositionExportTolerance = ConvertToleranceValue( iPos );
        g_fOrientationExportTolerance = ConvertToleranceValue( iOrientation );
        g_fScaleExportTolerance = ConvertToleranceValue( iScale );
    }

    VOID ExportAnimationTransformTrack::AddKey( FLOAT fTime, const D3DXVECTOR3& Position, const D3DXQUATERNION& Orientation, const D3DXVECTOR3& Scale )
    {
        ExportAnimationPositionKey PositionKey;
        ExportAnimationOrientationKey OrientationKey;
        ExportAnimationScaleKey ScaleKey;
        PositionKey.fTime = fTime;
        OrientationKey.fTime = fTime;
        ScaleKey.fTime = fTime;
        PositionKey.Position = Position;
        OrientationKey.Orientation = Orientation;
        ScaleKey.Scale = Scale;

        const BOOL bForceNoConstantOptimization = !g_ExportCoreSettings.bOptimizeAnimations;

        if( bForceNoConstantOptimization || PositionChangedFromLastTwoKeys( PositionKey ) )
        {
            PositionKeys.push_back( PositionKey );
        }
        else
        {
            ExportAnimationPositionKey& LastKey = PositionKeys.back();
            LastKey.fTime = fTime;
        }

        if( bForceNoConstantOptimization || OrientationChangedFromLastTwoKeys( OrientationKey ) )
        {
            OrientationKeys.push_back( OrientationKey );
        }
        else
        {
            ExportAnimationOrientationKey& LastKey = OrientationKeys.back();
            LastKey.fTime = fTime;
        }

        if( bForceNoConstantOptimization || ScaleChangedFromLastTwoKeys( ScaleKey ) )
        {
            ScaleKeys.push_back( ScaleKey );
        }
        else
        {
            ExportAnimationScaleKey& LastKey = ScaleKeys.back();
            LastKey.fTime = fTime;
        }
    }

    VOID ExportAnimationTransformTrack::AddKey( FLOAT fTime, const ExportTransform& Transform )
    {
        AddKey( fTime, Transform.Position(), Transform.Orientation(), Transform.Scale() );
    }

    VOID ExportAnimationTransformTrack::AddKey( FLOAT fTime, const D3DXMATRIX& matTransform )
    {
        ExportTransform Transform;
        Transform.InitializeFromFloats( (FLOAT*)&matTransform );
        AddKey( fTime, Transform );
    }

    BOOL ExportAnimationTransformTrack::PositionChangedFromLastTwoKeys( const ExportAnimationPositionKey& pk )
    {
        if( PositionKeys.size() < 2 )
            return TRUE;
        const ExportAnimationPositionKey& pk1 = PositionKeys[ PositionKeys.size() - 1 ];
        const ExportAnimationPositionKey& pk2 = PositionKeys[ PositionKeys.size() - 2 ];
        return ( pk1.Position != pk.Position || pk2.Position != pk.Position );
    }

    BOOL ExportAnimationTransformTrack::OrientationChangedFromLastTwoKeys( const ExportAnimationOrientationKey& pk )
    {
        if( OrientationKeys.size() < 2 )
            return TRUE;
        const ExportAnimationOrientationKey& pk1 = OrientationKeys[ OrientationKeys.size() - 1 ];
        const ExportAnimationOrientationKey& pk2 = OrientationKeys[ OrientationKeys.size() - 2 ];
        return ( pk1.Orientation != pk.Orientation || pk2.Orientation != pk.Orientation );
    }

    BOOL ExportAnimationTransformTrack::ScaleChangedFromLastTwoKeys( const ExportAnimationScaleKey& pk )
    {
        if( ScaleKeys.size() < 2 )
            return TRUE;
        const ExportAnimationScaleKey& pk1 = ScaleKeys[ ScaleKeys.size() - 1 ];
        const ExportAnimationScaleKey& pk2 = ScaleKeys[ ScaleKeys.size() - 2 ];
        return ( pk1.Scale != pk.Scale || pk2.Scale != pk.Scale );
    }

    VOID ExportAnimationTransformTrack::OptimizeKeys()
    {
        SortKeys();

        OptimizePositionKeys();
        OptimizeOrientationKeys();
        OptimizeScaleKeys();
    }

    D3DXVECTOR3 ComputeSlope( const ExportAnimationPositionKey& KeyA, const ExportAnimationPositionKey& KeyB )
    {
        D3DXVECTOR3 vDelta = KeyB.Position - KeyA.Position;
        FLOAT fTimeDelta = KeyB.fTime - KeyA.fTime;
        assert( fTimeDelta > 0.0f );
        return vDelta / fTimeDelta;
    }

    D3DXVECTOR4 ComputeSlope( const ExportAnimationOrientationKey& KeyA, const ExportAnimationOrientationKey& KeyB )
    {
        D3DXVECTOR4 vDelta = (D3DXVECTOR4)KeyB.Orientation - (D3DXVECTOR4)KeyA.Orientation;
        FLOAT fTimeDelta = KeyB.fTime - KeyA.fTime;
        assert( fTimeDelta > 0.0f );
        return vDelta / fTimeDelta;
    }

    D3DXVECTOR3 ComputeSlope( const ExportAnimationScaleKey& KeyA, const ExportAnimationScaleKey& KeyB )
    {
        D3DXVECTOR3 vDelta = KeyB.Scale - KeyA.Scale;
        FLOAT fTimeDelta = KeyB.fTime - KeyA.fTime;
        assert( fTimeDelta > 0.0f );
        return vDelta / fTimeDelta;
    }

    BOOL NewSlopeEncountered( const D3DXVECTOR3& vSlopeRef, const D3DXVECTOR3& vSlopeNew, const FLOAT fThreshold )
    {
        D3DXVECTOR3 vRefNormalized;
        if( D3DXVec3LengthSq( &vSlopeRef ) < 1e-4f )
        {
            vRefNormalized = D3DXVECTOR3( 1, 0, 0 );
        }
        else
        {
            D3DXVec3Normalize( &vRefNormalized, &vSlopeRef );
        }
        D3DXVECTOR3 vNewNormalized;
        if( D3DXVec3LengthSq( &vSlopeNew ) < 1e-4f )
        {
            vNewNormalized = D3DXVECTOR3( 1, 0, 0 );
        }
        else
        {
            D3DXVec3Normalize( &vNewNormalized, &vSlopeNew );
        }

        FLOAT fDot = D3DXVec3Dot( &vRefNormalized, &vNewNormalized );
        return( fDot <= fThreshold );
    }

    BOOL NewSlopeEncountered( const D3DXVECTOR4& vSlopeRef, const D3DXVECTOR4& vSlopeNew, const FLOAT fThreshold )
    {
        D3DXVECTOR4 vRefNormalized;
        if( D3DXVec4LengthSq( &vSlopeRef ) < 1e-4f )
        {
            vRefNormalized = D3DXVECTOR4( 1, 0, 0, 0 );
        }
        else
        {
            D3DXVec4Normalize( &vRefNormalized, &vSlopeRef );
        }
        D3DXVECTOR4 vNewNormalized;
        if( D3DXVec4LengthSq( &vSlopeNew ) < 1e-4f )
        {
            vNewNormalized = D3DXVECTOR4( 1, 0, 0, 0 );
        }
        else
        {
            D3DXVec4Normalize( &vNewNormalized, &vSlopeNew );
        }

        FLOAT fDot = D3DXVec4Dot( &vRefNormalized, &vNewNormalized );
        return( fDot <= fThreshold );
    }

    VOID ExportAnimationTransformTrack::OptimizePositionKeys()
    {
        if( PositionKeys.size() < 2 )
            return;

        D3DXVECTOR3 vCurrentSlope( 0, 0, 0 );
        PositionKeyList NewKeyList;
        NewKeyList.push_back( PositionKeys[0] );

        for( DWORD i = 1; i < PositionKeys.size(); ++i )
        {
            D3DXVECTOR3 vSlope = ComputeSlope( PositionKeys[i - 1], PositionKeys[i] );
            if( NewSlopeEncountered( vCurrentSlope, vSlope, g_fPositionExportTolerance ) )
            {
                if( i > 1 )
                {
                    NewKeyList.push_back( PositionKeys[i - 1] );
                }
                vCurrentSlope = vSlope;
            }
        }

        D3DXVECTOR3 vFinalSlope = ComputeSlope( NewKeyList.back(), PositionKeys.back() );
        if( D3DXVec3LengthSq( &vFinalSlope ) > 1e-4f )
        {
            NewKeyList.push_back( PositionKeys.back() );
        }

        PositionKeys = NewKeyList;
    }

    VOID ExportAnimationTransformTrack::OptimizeOrientationKeys()
    {
        if( OrientationKeys.size() < 2 )
            return;

        D3DXVECTOR4 vCurrentSlope( 0, 0, 0, 0 );
        OrientationKeyList NewKeyList;
        NewKeyList.push_back( OrientationKeys[0] );

        for( DWORD i = 1; i < OrientationKeys.size(); ++i )
        {
            D3DXVECTOR4 vSlope = ComputeSlope( OrientationKeys[i - 1], OrientationKeys[i] );
            if( NewSlopeEncountered( vCurrentSlope, vSlope, g_fOrientationExportTolerance ) )
            {
                if( i > 1 )
                {
                    NewKeyList.push_back( OrientationKeys[i - 1] );
                }
                vCurrentSlope = vSlope;
            }
        }

        D3DXVECTOR4 vFinalSlope = ComputeSlope( NewKeyList.back(), OrientationKeys.back() );
        if( D3DXVec4LengthSq( &vFinalSlope ) > 1e-4f )
        {
            NewKeyList.push_back( OrientationKeys.back() );
        }

        OrientationKeys = NewKeyList;
    }

    VOID ExportAnimationTransformTrack::OptimizeScaleKeys()
    {
        if( ScaleKeys.size() < 2 )
            return;

        D3DXVECTOR3 vCurrentSlope( 0, 0, 0 );
        ScaleKeyList NewKeyList;
        NewKeyList.push_back( ScaleKeys[0] );

        for( DWORD i = 1; i < ScaleKeys.size(); ++i )
        {
            D3DXVECTOR3 vSlope = ComputeSlope( ScaleKeys[i - 1], ScaleKeys[i] );
            if( NewSlopeEncountered( vCurrentSlope, vSlope, g_fScaleExportTolerance ) )
            {
                if( i > 1 )
                {
                    NewKeyList.push_back( ScaleKeys[i - 1] );
                }
                vCurrentSlope = vSlope;
            }
        }

        D3DXVECTOR3 vFinalSlope = ComputeSlope( NewKeyList.back(), ScaleKeys.back() );
        if( D3DXVec3LengthSq( &vFinalSlope ) > 1e-4f )
        {
            NewKeyList.push_back( ScaleKeys.back() );
        }

        ScaleKeys = NewKeyList;
    }

    BOOL PositionLessEqual( ExportAnimationPositionKey& A, ExportAnimationPositionKey& B )
    {
        return A.fTime <= B.fTime;
    }

    BOOL OrientationLessEqual( ExportAnimationOrientationKey& A, ExportAnimationOrientationKey& B )
    {
        return A.fTime <= B.fTime;
    }

    BOOL ScaleLessEqual( ExportAnimationScaleKey& A, ExportAnimationScaleKey& B )
    {
        return A.fTime <= B.fTime;
    }

    VOID ExportAnimationTransformTrack::SortKeys()
    {
        std::sort( PositionKeys.begin(), PositionKeys.end(), PositionLessEqual );
        std::sort( OrientationKeys.begin(), OrientationKeys.end(), OrientationLessEqual );
        std::sort( ScaleKeys.begin(), ScaleKeys.end(), ScaleLessEqual );
    }

    VOID ExportAnimationTransformTrack::EndianSwap()
    {
        DWORD dwPositionFloatCount = GetPositionDataSize() / sizeof( FLOAT );
        DWORD dwOrientationFloatCount = GetOrientationDataSize() / sizeof( FLOAT );
        DWORD dwScaleFloatCount = GetScaleDataSize() / sizeof( FLOAT );

        DWORD* pData = (DWORD*)GetPositionData();
        for( DWORD i = 0; i < dwPositionFloatCount; ++i )
        {
            *pData = _byteswap_ulong( *pData );
            ++pData;
        }
        pData = (DWORD*)GetOrientationData();
        for( DWORD i = 0; i < dwOrientationFloatCount; ++i )
        {
            *pData = _byteswap_ulong( *pData );
            ++pData;
        }
        pData = (DWORD*)GetScaleData();
        for( DWORD i = 0; i < dwScaleFloatCount; ++i )
        {
            *pData = _byteswap_ulong( *pData );
            ++pData;
        }
    }

    BOOL ExportAnimationTransformTrack::IsTrackEmpty()
    {
        // If any key array has more than 1 key, the track is not empty
        if( PositionKeys.size() > 1 ||
            OrientationKeys.size() > 1 ||
            ScaleKeys.size() > 1 )
        {
            return FALSE;
        }

        BOOL bResult = TRUE;

        if( pSourceFrame != NULL )
        {
            // Check for an orientation that is different from the scene position
            if( OrientationKeys.size() == 1 )
            {
                if( OrientationKeys[0].Orientation != pSourceFrame->Transform().Orientation() )
                {
                    bResult = FALSE;
                }
            }
            // Check for a position that is different from the scene position
            if( PositionKeys.size() == 1 )
            {
                if( PositionKeys[0].Position != pSourceFrame->Transform().Position() )
                {
                    bResult = FALSE;
                }
            }
            // Check for a scale that is different from the scene scale
            if( ScaleKeys.size() == 1 )
            {
                if( ScaleKeys[0].Scale != pSourceFrame->Transform().Scale() )
                {
                    bResult = FALSE;
                }
            }
        }
        else
        {
            bResult = FALSE;
        }

        return bResult;
    }

ExportAnimation::ExportAnimation(void)
{
}

ExportAnimation::~ExportAnimation(void)
{
}

VOID ExportAnimation::Optimize()
{
    if( !g_ExportCoreSettings.bOptimizeAnimations )
    {
        return;
    }

    ExportLog::LogMsg( 4, "Optimizing animation with %d tracks.", m_vTracks.size() );
    vector< ExportAnimationTrack* > NewTrackList;
    for( DWORD i = 0; i < m_vTracks.size(); i++ )
    {
        ExportAnimationTrack* pTrack = m_vTracks[i];
        pTrack->TransformTrack.OptimizeKeys();
        if( pTrack->TransformTrack.IsTrackEmpty() )
        {
            delete pTrack;
        }
        else
        {
            NewTrackList.push_back( pTrack );
        }
    }
    ExportLog::LogMsg( 4, "Animation has %d tracks after optimization.", NewTrackList.size() );
    m_vTracks = NewTrackList;
}

VOID ExportAnimation::EndianSwap()
{
    for( DWORD i = 0; i < m_vTracks.size(); i++ )
    {
        ExportAnimationTrack* pTrack = m_vTracks[i];
        pTrack->TransformTrack.EndianSwap();
    }
}

};

