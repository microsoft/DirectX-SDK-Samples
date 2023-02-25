//-------------------------------------------------------------------------------------
//  ExportAnimation.h
//
//  Data structures for keyframed animation curves, and code to manipulate and optimize
//  the keyframes.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

namespace ATG
{

using namespace std;

class ExportAnimationAnnotationTrack
{
public:
protected:
};

struct ExportAnimationPositionKey
{
    FLOAT           fTime;
    D3DXVECTOR3     Position;
};
typedef std::vector<ExportAnimationPositionKey> PositionKeyList;

struct ExportAnimationOrientationKey
{
    FLOAT           fTime;
    D3DXQUATERNION  Orientation;
};
typedef std::vector<ExportAnimationOrientationKey> OrientationKeyList;

struct ExportAnimationScaleKey
{
    FLOAT           fTime;
    D3DXVECTOR3     Scale;
};
typedef std::vector<ExportAnimationScaleKey> ScaleKeyList;

class ExportFrame;

class ExportAnimationTransformTrack
{
public:
    ExportAnimationTransformTrack()
        : pSourceFrame( NULL )
    {
    }
    VOID AddKey( FLOAT fTime, const ExportTransform& Transform );
    VOID AddKey( FLOAT fTime, const D3DXMATRIX& matTransform );
    VOID AddKey( FLOAT fTime, const D3DXVECTOR3& Position, const D3DXQUATERNION& Orientation, const D3DXVECTOR3& Scale );
    VOID OptimizeKeys();
    VOID SortKeys();
    VOID EndianSwap();
    FLOAT* GetPositionData() const { return (FLOAT*)&PositionKeys.front(); }
    FLOAT* GetOrientationData() const { return (FLOAT*)&OrientationKeys.front(); }
    FLOAT* GetScaleData() const { return (FLOAT*)&ScaleKeys.front(); }
    DWORD GetPositionDataSize() const { return (DWORD)PositionKeys.size() * 4 * sizeof( FLOAT ); }
    DWORD GetOrientationDataSize() const { return (DWORD)OrientationKeys.size() * 5 * sizeof( FLOAT ); }
    DWORD GetScaleDataSize() const { return (DWORD)ScaleKeys.size() * 4 * sizeof( FLOAT ); }
    ExportAnimationPositionKey* GetPositionKeys() { return &PositionKeys.front(); }
    DWORD GetPositionKeyCount() const { return (DWORD)PositionKeys.size(); }
    ExportAnimationOrientationKey* GetOrientationKeys() { return &OrientationKeys.front(); }
    DWORD GetOrientationKeyCount() const { return (DWORD)OrientationKeys.size(); }
    ExportAnimationScaleKey* GetScaleKeys() { return &ScaleKeys.front(); }
    DWORD GetScaleKeyCount() const { return (DWORD)ScaleKeys.size(); }
    BOOL IsTrackEmpty();
protected:
    VOID OptimizePositionKeys();
    VOID OptimizeOrientationKeys();
    VOID OptimizeScaleKeys();

    BOOL PositionChangedFromLastTwoKeys( const ExportAnimationPositionKey& pk );
    BOOL OrientationChangedFromLastTwoKeys( const ExportAnimationOrientationKey& ok );
    BOOL ScaleChangedFromLastTwoKeys( const ExportAnimationScaleKey& sk );
public:
    PositionKeyList         PositionKeys;
    OrientationKeyList      OrientationKeys;
    ScaleKeyList            ScaleKeys;
    ExportFrame*            pSourceFrame;
};

class ExportAnimationTrack :
    public ExportBase
{
public:
    ExportAnimationTransformTrack       TransformTrack;
    ExportAnimationAnnotationTrack      AnnotationTrack;
};

class ExportAnimation :
    public ExportBase
{
public:
    ExportAnimation(void);
    ~ExportAnimation(void);
    VOID AddTrack( ExportAnimationTrack* pTrack ) { m_vTracks.push_back( pTrack ); }
    DWORD GetTrackCount() const { return (DWORD)m_vTracks.size(); }
    ExportAnimationTrack* GetTrack( DWORD dwIndex ) { return m_vTracks[ dwIndex ]; }
    FLOAT GetDuration() const { return fEndTime - fStartTime; }
    VOID Optimize();
    VOID EndianSwap();
    static VOID SetAnimationExportQuality( INT iPos, INT iOrientation, INT iScale );
public:
    FLOAT                               fStartTime;
    FLOAT                               fEndTime;
    FLOAT                               fSourceFrameInterval;
    FLOAT                               fSourceSamplingInterval;
protected:
    vector< ExportAnimationTrack* >     m_vTracks;
};

};
