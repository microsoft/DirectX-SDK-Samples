//-------------------------------------------------------------------------------------
//  ExportSubD.h
//
//  Classes representing Catmull-Clark subdivision surfaces.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

#include <deque>
using namespace stdext;

namespace ATG
{
    struct ExportSubDPatchSubset
    {
        ExportString    Name;
        INT             iOriginalMeshSubset;
        BOOL            bQuadPatches;
        DWORD           dwStartPatch;
        DWORD           dwPatchCount;
    };

    class ExportSubDProcessMesh
    {
    public:
        enum 
        {
            MAX_POINT_COUNT = 32,
            MAX_QUAD_NEIGHBOR_COUNT = (MAX_POINT_COUNT - 4),
            MAX_TRIANGLE_NEIGHBOR_COUNT = (MAX_POINT_COUNT - 3)
        };
        struct PatchData
        {
            //UINT iMeshIndices[MAX_POINT_COUNT];
            BYTE bValence[4];
            BYTE bPrefix[4];
        };
        struct Triangle
        {
            INT iIndices[3];
            INT iMeshIndices[3];
            INT iNeighbors[MAX_TRIANGLE_NEIGHBOR_COUNT];
            BYTE bValence[3];
            BYTE bPrefix[3];
            INT iPolyIndex;
            INT iMeshSubsetIndex;
        };
        struct Quad
        {
            INT iIndices[4];
            INT iMeshIndices[4];
            INT iNeighbors[MAX_QUAD_NEIGHBOR_COUNT];
            BYTE bValence[4];
            BYTE bPrefix[4];
            INT iPolyIndex;
            INT iMeshSubsetIndex;
            BOOL bDegenerate;
        };
    protected:
        struct Edge
        {
            INT iTriangleIndex;
            INT iQuadIndex;
            INT iLocalIndex;
        };
        typedef hash_map< UINT64, Edge > EdgeMap;

        vector< Triangle >          m_Triangles;
        vector< Quad >              m_Quads;
        vector< D3DXVECTOR3 >       m_Positions;
        vector< INT >               m_MeshVertexToPositionMapping;
        vector< INT >               m_PositionToMeshVertexMapping;
        vector< INT >               m_PositionToDegeneratePositionMapping;
        vector< INT >               m_IncidentBoundaryEdgesPerPosition;
        EdgeMap                     m_BoundaryEdges;

        ExportMesh*             m_pPolyMesh;
        ExportIB*               m_pQuadPatchIB;
        ExportIB*               m_pTrianglePatchIB;
        ExportVB*               m_pQuadPatchDataVB;
        ExportVB*               m_pTrianglePatchDataVB;

        vector< ExportSubDPatchSubset > m_Subsets;
    public:
        ExportSubDProcessMesh();
        VOID Initialize( ExportMesh* pMesh );

        VOID ByteSwap();

        DWORD GetQuadPatchCount() const { return ( m_pQuadPatchDataVB != NULL ) ? m_pQuadPatchDataVB->GetVertexCount() : 0; }
        ExportVB* GetQuadPatchDataVB() const { return m_pQuadPatchDataVB; }
        ExportIB* GetQuadPatchIB() const { return m_pQuadPatchIB; }
        DWORD GetTrianglePatchCount() const { return ( m_pTrianglePatchDataVB != NULL ) ? m_pTrianglePatchDataVB->GetVertexCount() : 0; }
        ExportVB* GetTrianglePatchDataVB() const { return m_pTrianglePatchDataVB; }
        ExportIB* GetTriPatchIB() const { return m_pTrianglePatchIB; }

        DWORD GetSubsetCount() const { return (DWORD)m_Subsets.size(); }
        ExportSubDPatchSubset* GetSubset( DWORD dwIndex ) { return &m_Subsets[dwIndex]; }
        ExportSubDPatchSubset* FindSubset( ExportString strName );

        static const D3DVERTEXELEMENT9* GetPatchDataDecl();
        static DWORD GetPatchDataDeclElementCount();

    protected:
        VOID BuildMesh();
        INT CreateOrAddPosition( const D3DXVECTOR3& vPosition, INT iMeshVertexIndex );
        VOID AddTriangle( INT iPolyIndex, INT iSubsetIndex, const INT* pIndices, const INT* pMeshIndices );
        VOID AddQuad( INT iPolyIndex,  INT iSubsetIndex, const INT* pIndices, const INT* pMeshIndices );

        VOID BuildBoundaryEdgeTable();
        INT AddOrRemoveEdge( INT iPositionIndexA, INT iPositionIndexB, INT iTriangleIndex, INT iQuadIndex, INT iLocalIndex );

        VOID CreateDegenerateGeometry();
        INT CreateOrAddDegeneratePosition( INT iPositionIndex );

        VOID ComputeAdjacency();
        VOID ComputeTriangleAdjacency( const INT iTriangleIndex );
        VOID ComputeQuadAdjacency( const INT iQuadIndex );
        INT ExecuteSweep( INT iPivotPositionIndex, INT iSweepPositionIndex, INT iStopPositionIndex, INT iStartQuadIndex, INT iStartTriangleIndex, vector<INT>& Neighbors );
        INT FindTriangleWithEdge( INT iStartPositionIndex, INT iEndPositionIndex, INT iExcludeThisTriangle );
        INT FindQuadWithEdge( INT iStartPositionIndex, INT iEndPositionIndex, INT iExcludeThisQuad );
        INT FindLocalIndexInTriangle( INT iTriangleIndex, INT iPositionIndex );
        INT FindLocalIndexInQuad( INT iQuadIndex, INT iPositionIndex );
        INT GetNextPositionIndexInTriangle( INT iTriangleIndex, INT iPivotPositionIndex, INT iSweepPositionIndex );
        INT GetNextPositionIndexInQuad( INT iQuadIndex, INT iPivotPositionIndex, INT iSweepPositionIndex );
        INT GetOppositePositionIndexInQuad( INT iQuadIndex, INT iPositionIndex );

        VOID RemoveValenceTwoQuads( deque<INT>& BadQuads );

        D3DXVECTOR3 GetQuadCenter( INT iQuadIndex );

        VOID SortPatches();

        VOID BuildQuadPatchBuffer();
        VOID BuildTriPatchBuffer();

        VOID ConvertSubsets();

        VOID ClearIntermediateBuffers();
    };
}