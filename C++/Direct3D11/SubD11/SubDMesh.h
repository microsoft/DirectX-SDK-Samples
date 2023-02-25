//--------------------------------------------------------------------------------------
// File: SubDMesh.h
//
// This class encapsulates the mesh loading and housekeeping functions for a SubDMesh.
// The mesh loads preprocessed SDKMESH files from disk and stages them for rendering.
//
// To view the mesh preprocessing code, please find the ExportSubDMesh.cpp file in the 
// samples content exporter.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "SDKmesh.h"

// Maximum number of points that can be part of a subd quad.
// This includes the 4 interior points of the quad, plus the 1-ring neighborhood.
#define MAX_EXTRAORDINARY_POINTS 32

// Maximum valence we expect to encounter for extraordinary vertices
#define MAX_VALENCE 16

// Control point for a sub-d patch
struct SUBD_CONTROL_POINT
{
	D3DXVECTOR3 m_vPosition;
    BYTE		m_Weights[4];
    BYTE		m_Bones[4];
    D3DXVECTOR3 m_vNormal;      // Normal is not used for patch computation.
	D3DXVECTOR2 m_vUV;
	D3DXVECTOR3 m_vTanU;
};

//--------------------------------------------------------------------------------------
// This class handles most of the loading and conversion for a subd mesh.  It also
// creates and tracks buffers used by the mesh.
//--------------------------------------------------------------------------------------
class CSubDMesh
{
private:
    struct PatchPiece
    {
        SDKMESH_MESH*   m_pMesh;
        UINT            m_MeshIndex;

        ID3D11Buffer*   m_pExtraordinaryPatchIB;        // Index buffer for patches
        ID3D11Buffer*   m_pControlPointVB;              // Stores the control points for mesh
        ID3D11Buffer*   m_pPerPatchDataVB;              // Stores valences and prefixes on a per-patch basis
        ID3D11ShaderResourceView* m_pPerPatchDataSRV;   // Per-patch SRV
        INT             m_iPatchCount;
        INT             m_iRegularExtraodinarySplitPoint;

        ID3D11Buffer* m_pMyRegularPatchIB;
        ID3D11Buffer* m_pMyExtraordinaryPatchIB;
        ID3D11Buffer* m_pMyRegularPatchData;
        ID3D11Buffer* m_pMyExtraordinaryPatchData;

        ID3D11ShaderResourceView* m_pMyRegularPatchDataSRV;
        ID3D11ShaderResourceView* m_pMyExtraordinaryPatchDataSRV;

        CGrowableArray<UINT> RegularPatchStart;
        CGrowableArray<UINT> ExtraordinaryPatchStart;
        CGrowableArray<UINT> RegularPatchCount;
        CGrowableArray<UINT> ExtraordinaryPatchCount;

        D3DXVECTOR3     m_vCenter;
        D3DXVECTOR3     m_vExtents;
        INT             m_iFrameIndex;
    };

    struct PolyMeshPiece
    {
        SDKMESH_MESH*   m_pMesh;
        UINT            m_MeshIndex;
        INT             m_iFrameIndex;
        D3DXVECTOR3     m_vCenter;
        D3DXVECTOR3     m_vExtents;

        ID3D11Buffer*   m_pIndexBuffer;
        ID3D11Buffer*   m_pVertexBuffer;
    };

    CGrowableArray<PatchPiece*>  m_PatchPieces;
    CGrowableArray<PolyMeshPiece*>  m_PolyMeshPieces;

    CDXUTSDKMesh*   m_pMeshFile;

    ID3D11Buffer*   m_pPerSubsetCB;

    UINT            m_iCameraFrameIndex;

public:
                ~CSubDMesh();

    // Loading
    HRESULT     LoadSubDFromSDKMesh( ID3D11Device* pd3dDevice, const WCHAR* strFileName, const WCHAR* strAnimationFileName, const CHAR* strCameraName = "" );
    void        Destroy();

    void        RenderPatchPiece_OnlyRegular( ID3D11DeviceContext* pd3dDeviceContext, int PieceIndex );
    void        RenderPatchPiece_OnlyExtraordinary( ID3D11DeviceContext* pd3dDeviceContext, int PieceIndex );
    void        RenderPolyMeshPiece( ID3D11DeviceContext* pd3dDeviceContext, int PieceIndex );

    // Per-frame update
    void        Update( D3DXMATRIX* pWorld, double fTime );

    bool        GetCameraViewMatrix( D3DXMATRIX* pViewMatrix, D3DXVECTOR3* pCameraPosWorld );

    FLOAT       GetAnimationDuration();

    void        GetBounds( D3DXVECTOR3* pvCenter, D3DXVECTOR3* pvExtents ) const;

    // Accessors
    int         GetNumInfluences( UINT iMeshIndex )
    {
        return m_pMeshFile->GetNumInfluences( iMeshIndex );
    }
    const D3DXMATRIX* GetInfluenceMatrix( UINT iMeshIndex, UINT iInfluence )
    {
        return m_pMeshFile->GetMeshInfluenceMatrix( iMeshIndex, iInfluence );
    }

    int         GetPatchMeshIndex( UINT iPatchPiece ) const
    {
        return m_PatchPieces[iPatchPiece]->m_MeshIndex;
    }
    int         GetNumPatchPieces() const
    {
        return m_PatchPieces.GetSize();
    }
    bool        GetPatchPieceTransform( UINT iPatchPiece, D3DXMATRIX* pTransform ) const
    {
        INT iFrameIndex = m_PatchPieces[iPatchPiece]->m_iFrameIndex;
        if( iFrameIndex == -1 )
        {
            D3DXMatrixIdentity( pTransform );
        }
        else
        {
            *pTransform = *( m_pMeshFile->GetWorldMatrix( iFrameIndex ) );
        }
        return true;
    }

    int         GetPolyMeshIndex( UINT iPolyMeshPiece ) const
    {
        return m_PolyMeshPieces[iPolyMeshPiece]->m_MeshIndex;
    }
    int         GetNumPolyMeshPieces() const
    {
        return m_PolyMeshPieces.GetSize();
    }
    bool        GetPolyMeshPieceTransform( UINT iPolyMeshPiece, D3DXMATRIX* pTransform ) const
    {
        INT iFrameIndex = m_PolyMeshPieces[iPolyMeshPiece]->m_iFrameIndex;
        if( iFrameIndex == -1 )
        {
            D3DXMatrixIdentity( pTransform );
        }
        else
        {
            *pTransform = *( m_pMeshFile->GetWorldMatrix( iFrameIndex ) );
        }
        return true;
    }

protected:
    void SetupMaterial( ID3D11DeviceContext* pd3dDeviceContext, int MaterialID );
};
