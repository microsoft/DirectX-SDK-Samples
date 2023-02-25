//-------------------------------------------------------------------------------------
//  ExportMesh.h
//
//  Classes representing static and skinned meshes.  Also included is code to optimize
//  mesh data and generate export-ready data from a non-indexed triangle list.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

namespace ATG
{

using namespace std;

class ExportVB
{
public:
    ExportVB() 
        : m_pVertexData( NULL ),
          m_uVertexCount( 0 ),
          m_uVertexSizeBytes( 0 )
    {
    }
    ~ExportVB()
    {
        delete[] m_pVertexData;
    }
    VOID SetVertexSize( UINT uByteCount ) { m_uVertexSizeBytes = uByteCount; }
    VOID SetVertexCount( UINT uVertexCount ) { m_uVertexCount = uVertexCount; }
    VOID Allocate();
    UINT GetVertexSize() const { return m_uVertexSizeBytes; }
    UINT GetVertexCount() const { return m_uVertexCount; }
    UINT GetVertexDataSize() const { return m_uVertexSizeBytes * m_uVertexCount; }
    BYTE* GetVertex( UINT uIndex );
    BYTE* GetVertexData() { return m_pVertexData; }
    const BYTE* GetVertexData() const { return m_pVertexData; }

    VOID ByteSwap( const D3DVERTEXELEMENT9* pVertexElements, const DWORD dwVertexElementCount );
protected:
    UINT        m_uVertexSizeBytes;
    UINT        m_uVertexCount;
    BYTE*       m_pVertexData;
};

class ExportIB
{
public:
    ExportIB()
        : m_uIndexCount( 0 ),
          m_dwIndexSize( 2 ),
          m_pIndexData16( NULL )
    {
    }
    ~ExportIB()
    {
        delete[] m_pIndexData16;
    }

    UINT GetIndexCount() const { return m_uIndexCount; }
    VOID SetIndexSize( DWORD dwIndexSize ) { assert( dwIndexSize == 2 || dwIndexSize == 4 ); m_dwIndexSize = dwIndexSize; }
    DWORD GetIndexSize() const { return m_dwIndexSize; }
    VOID SetIndexCount( UINT uIndexCount ) { m_uIndexCount = uIndexCount; }
    VOID Allocate();
    DWORD GetIndex( UINT uIndex ) 
    {
        if( m_dwIndexSize == 2 )
            return m_pIndexData16[ uIndex ];
        else
            return m_pIndexData32[ uIndex ];
    }
    VOID SetIndex( UINT uIndex, DWORD dwData ) 
    {
        if( m_dwIndexSize == 2 )
            m_pIndexData16[ uIndex ] = (WORD)dwData;
        else
            m_pIndexData32[ uIndex ] = dwData;
    }
    BYTE* GetIndexData() { return (BYTE*)m_pIndexData16; }
    const BYTE* GetIndexData() const { return (const BYTE*)m_pIndexData16; }
    DWORD GetIndexDataSize() const { return m_uIndexCount * m_dwIndexSize; }

    VOID ByteSwap();
protected:
    DWORD       m_dwIndexSize;
    UINT        m_uIndexCount;
    union
    {
        WORD*       m_pIndexData16;
        DWORD*      m_pIndexData32;
    };
};

class ExportIBSubset : 
    public ExportBase
{
public:
    enum PrimitiveType
    {
        TriangleList = 0,
        TriangleStrip,
        QuadList
    };
    ExportIBSubset()
        : m_uStartIndex( 0 ),
          m_uIndexCount( 0 ),
          m_PrimitiveType( TriangleList )
    {
    }
    VOID SetStartIndex( UINT uStartIndex ) { m_uStartIndex = uStartIndex; }
    VOID IncrementIndexCount( UINT uSize ) { m_uIndexCount += uSize; }
    VOID SetIndexCount( UINT uIndexCount ) { m_uIndexCount = uIndexCount; }
    UINT GetStartIndex() const { return m_uStartIndex; }
    UINT GetIndexCount() const { return m_uIndexCount; }
    VOID SetPrimitiveType( PrimitiveType NewPT ) { m_PrimitiveType = NewPT; }
    PrimitiveType GetPrimitiveType() const { return m_PrimitiveType; }
protected:
    UINT            m_uStartIndex;
    UINT            m_uIndexCount;
    PrimitiveType   m_PrimitiveType;
};

struct ByteVector4
{
public:
    BYTE    x;
    BYTE    y;
    BYTE    z;
    BYTE    w;
};

class ExportMaterial;

struct ExportMeshVertex
{
public:
    ExportMeshVertex()
    {
        Initialize();
    }
    VOID Initialize()
    {
        ZeroMemory( this, sizeof( ExportMeshVertex ) );
        BoneWeights.x = 1.0f;
    }
    UINT            DCCVertexIndex;
    D3DXVECTOR3     Position;
    D3DXVECTOR3     Normal;
    D3DXVECTOR3     SmoothNormal;
    D3DXVECTOR3     Tangent;
    D3DXVECTOR3     Binormal;
    ByteVector4     BoneIndices;
    D3DXVECTOR4     BoneWeights;
    D3DXVECTOR4     TexCoords[8];
    D3DXVECTOR4     Color;
    ExportMeshVertex* pNextDuplicateVertex;
    BOOL            Equals( const ExportMeshVertex* pOtherVertex ) const;
};

typedef std::vector< ExportMeshVertex* > ExportMeshVertexArray;

struct ExportMeshTriangle
{
public:
    ExportMeshTriangle()
        : SubsetIndex( 0 ),
          PolygonIndex( -1 )
    {
    }
    VOID Initialize()
    {
        SubsetIndex = 0;
        Vertex[0].Initialize();
        Vertex[1].Initialize();
        Vertex[2].Initialize();
    }
    ExportMeshVertex    Vertex[3];
    INT                 SubsetIndex;
    INT                 PolygonIndex;
};

typedef std::vector< ExportMeshTriangle* > ExportMeshTriangleArray;

class ExportMeshTriangleAllocator
{
public:
    ExportMeshTriangleAllocator()
        : m_uAllocatedCount( 0 ),
          m_uTotalCount( 0 )
    {
    }
    ~ExportMeshTriangleAllocator()
    {
        Terminate();
    }
    VOID Initialize() { SetSizeHint( 50000 ); }
    VOID Terminate();
    VOID SetSizeHint( UINT uAnticipatedSize );
    ExportMeshTriangle* GetNewTriangle();
    VOID ClearAllTriangles();
private:
    struct AllocationBlock
    {
        ExportMeshTriangle* pTriangleArray;
        UINT m_uTriangleCount;
    };
    typedef std::list< AllocationBlock > AllocationBlockList;
    AllocationBlockList m_AllocationBlocks;
    UINT m_uTotalCount;
    UINT m_uAllocatedCount;
};

extern ExportMeshTriangleAllocator g_MeshTriangleAllocator;

struct ExportVertexFormat
{
public:
    ExportVertexFormat()
        : m_bPosition( TRUE ),
          m_bNormal( TRUE ),
          m_bSkinData( FALSE ),
          m_bTangent( FALSE ),
          m_bBinormal( FALSE ),
          m_bVertexColor( TRUE ),
          m_uUVSetCount( 1 ),
          m_uUVSetSize( 2 )
    {
    }
    BOOL        m_bPosition;
    BOOL        m_bNormal;
    BOOL        m_bTangent;
    BOOL        m_bBinormal;
    BOOL        m_bSkinData;
    BOOL        m_bVertexColor;
    UINT        m_uUVSetCount;
    UINT        m_uUVSetSize;
};


class ExportMeshBase :
    public ExportBase
{
public:
    ExportMeshBase( ExportString name );
    ~ExportMeshBase();

    enum MeshType
    {
        PolyMesh = 0
    };

    enum BoundsType
    {
        SphereBound = 0,
        AxisAlignedBoxBound = 1,
        OrientedBoxBound = 2
    };

    virtual MeshType GetMeshType() const = NULL;

    VOID AddSubset( ExportIBSubset* pSubset ) { m_vSubsets.push_back( pSubset ); }
    UINT GetSubsetCount() const { return (UINT)m_vSubsets.size(); }
    ExportIBSubset* GetSubset( UINT uIndex ) { return m_vSubsets[ uIndex ]; }
    ExportIBSubset* FindSubset( const ExportString Name );

    Sphere& GetBoundingSphere() { return m_BoundingSphere; }
    AxisAlignedBox& GetBoundingAABB() { return m_BoundingAABB; }
    OrientedBox& GetBoundingOBB() { return m_BoundingOBB; }
    BoundsType GetSmallestBound() const { return m_SmallestBound; }

    virtual VOID AddInfluence( ExportString InfluenceName ) { m_InfluenceNames.push_back( InfluenceName ); }
    UINT GetInfluenceCount() const { return (UINT)m_InfluenceNames.size(); }
    ExportString GetInfluence( UINT uIndex ) const { return m_InfluenceNames[uIndex]; }

protected:
    Sphere                          m_BoundingSphere;
    AxisAlignedBox                  m_BoundingAABB;
    OrientedBox                     m_BoundingOBB;
    BoundsType                      m_SmallestBound;
    vector< ExportIBSubset* >       m_vSubsets;
    vector< ExportString >          m_InfluenceNames;
};


class ExportSubDProcessMesh;

class ExportMesh :
    public ExportMeshBase
{
public:
    enum OptimizationFlags
    {
        COMPRESS_VERTEX_DATA = 1,
        FLIP_TRIANGLES = 2,
        FORCE_SUBD_CONVERSION = 4,
    };

    ExportMesh( ExportString name );
    ~ExportMesh();

    virtual MeshType GetMeshType() const { return ExportMeshBase::PolyMesh; }

    VOID SetVertexUVCount( UINT uCount ) { m_VertexFormat.m_uUVSetCount = uCount; }
    VOID SetVertexUVDimension( UINT uDimension ) { m_VertexFormat.m_uUVSetSize = uDimension; }
    VOID SetVertexNormalCount( UINT uCount );
    VOID SetVertexColorCount( UINT uCount ) { m_VertexFormat.m_bVertexColor = ( uCount > 0 ); }

    UINT GetVertexDeclElementCount() const { return (UINT)m_VertexElements.size(); }
    const D3DVERTEXELEMENT9& GetVertexDeclElement( UINT uIndex ) const { return m_VertexElements[ uIndex ]; }

    VOID AddRawTriangle( ExportMeshTriangle* pTriangle );
    VOID Optimize( DWORD dwFlags );
    VOID ByteSwap();

    ExportVB* GetVB() { return m_pVB; }
    ExportIB* GetIB() { return m_pIB; }

    ExportSubDProcessMesh* GetSubDMesh() { return m_pSubDMesh; }

    DWORD GetTriangleCount() const { return (DWORD)m_TriangleToPolygonMapping.size(); }
    INT GetPolygonForTriangle( DWORD dwTriangleIndex ) const { return m_TriangleToPolygonMapping[dwTriangleIndex]; }

    virtual VOID AddInfluence( ExportString InfluenceName ) { m_InfluenceNames.push_back( InfluenceName ); m_VertexFormat.m_bSkinData = TRUE; }

protected:
    VOID BuildVertexBuffer( ExportMeshVertexArray& VertexArray, DWORD dwFlags );
    VOID ClearRawTriangles();
    ID3DXMesh* CreateD3DXMesh();
    VOID CopyD3DXMeshIntoMesh( ID3DXMesh* pMesh );
    HRESULT ComputeVertexTangentSpacesD3DX( ID3DXMesh** ppMesh );
    HRESULT ComputeUVAtlas( ID3DXMesh** ppMesh );
    VOID ComputeBoneSubsetGroups();
    VOID SortRawTrianglesBySubsetIndex();
    VOID ComputeBounds();

protected:
    ExportVB*                       m_pVB;
    ExportIB*                       m_pIB;
    ExportMeshTriangleArray         m_RawTriangles;
    vector< INT >                   m_TriangleToPolygonMapping;
    ExportVertexFormat              m_VertexFormat;
    vector< D3DVERTEXELEMENT9 >     m_VertexElements;
    UINT                            m_uDCCVertexCount;
    ExportSubDProcessMesh*          m_pSubDMesh;
};

class ExportMaterialSubsetBinding
{
public:
    ExportMaterialSubsetBinding()
    {
        ZeroMemory( this, sizeof( ExportMaterialSubsetBinding ) );
    }
    ExportString        SubsetName;
    ExportMaterial*     pMaterial;
};

typedef std::vector< ExportMaterialSubsetBinding* > ExportMaterialSubsetBindingArray;

class ExportModel
{
public:
    ExportModel( ExportMeshBase* pMesh )
        : m_pMesh( pMesh ),
          m_bCastsShadows( TRUE ),
          m_bReceivesShadows( TRUE )
    {
    }
    ~ExportModel();
    ExportMeshBase* GetMesh() { return m_pMesh; }
    BOOL SetSubsetBinding( ExportString SubsetName, ExportMaterial* pMaterial, BOOL bSkipValidation = FALSE );
    UINT GetBindingCount() const { return (UINT)m_vBindings.size(); }
    ExportMaterialSubsetBinding* GetBinding( UINT i ) { return m_vBindings[i]; }

    BOOL IsShadowCaster() const { return m_bCastsShadows; }
    BOOL IsShadowReceiver() const { return m_bReceivesShadows; }
    VOID SetCastsShadows( BOOL bValue ) { m_bCastsShadows = bValue; }
    VOID SetReceivesShadows( BOOL bValue ) { m_bReceivesShadows = bValue; }

protected:
    ExportMeshBase*                     m_pMesh;
    ExportMaterialSubsetBindingArray    m_vBindings;
    BOOL                                m_bCastsShadows;
    BOOL                                m_bReceivesShadows;
};

DWORD MakeCompressedVector( const D3DXVECTOR3& Vec3 );
VOID NormalizeBoneWeights( BYTE* pWeights );

};

