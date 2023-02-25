//-------------------------------------------------------------------------------------
//  ExportMesh.cpp
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "exportmesh.h"

#define SAFE_RELEASE(x) { if( (x) != NULL ) { (x)->Release(); (x) = NULL; } }

extern ATG::ExportScene* g_pScene;
namespace ATG
{

extern IDirect3DDevice9* g_pd3dDevice;

INT GetElementSizeFromDeclType(DWORD Type);

ExportMeshTriangleAllocator g_MeshTriangleAllocator;

ExportMeshBase::ExportMeshBase( ExportString name )
: ExportBase( name )
{ }

ExportMeshBase::~ExportMeshBase()
{ }

ExportIBSubset* ExportMeshBase::FindSubset( const ExportString Name )
{
    DWORD dwSubsetCount = GetSubsetCount();
    for( DWORD i = 0; i < dwSubsetCount; ++i )
    {
        ExportIBSubset* pSubset = GetSubset( i );
        if( pSubset->GetName() == Name )
            return pSubset;
    }
    return NULL;
}

ExportMesh::ExportMesh( ExportString name )
: ExportMeshBase( name ),
  m_pVB( NULL ),
  m_pIB( NULL ),
  m_uDCCVertexCount( 0 ),
  m_pSubDMesh( NULL )
{
    m_BoundingSphere.Center = XMFLOAT3( 0, 0, 0 );
    m_BoundingSphere.Radius = 0;
}

ExportMesh::~ExportMesh(void)
{
    delete m_pVB;
    delete m_pIB;
    ClearRawTriangles();
}

VOID ExportMesh::ClearRawTriangles()
{
    if( m_RawTriangles.size() > 0 )
    {
        g_MeshTriangleAllocator.ClearAllTriangles();
        m_RawTriangles.clear();
    }
}

VOID ExportMesh::ByteSwap()
{
    if( m_pIB != NULL )
        m_pIB->ByteSwap();
    if( m_pVB != NULL && m_VertexElements.size() > 0 )
        m_pVB->ByteSwap( &m_VertexElements[0], GetVertexDeclElementCount() );
    if( m_pSubDMesh != NULL )
    {
        m_pSubDMesh->ByteSwap();
    }
}

VOID ExportVB::Allocate()
{
    if( m_pVertexData != NULL )
        delete[] m_pVertexData;
    UINT uSize = GetVertexDataSize();
    m_pVertexData = new BYTE[ uSize ];
    ZeroMemory( m_pVertexData, uSize );
}

BYTE* ExportVB::GetVertex( UINT uIndex )
{
    if( m_pVertexData == NULL )
        return NULL;
    if( uIndex >= m_uVertexCount )
        return NULL;
    return m_pVertexData + ( uIndex * m_uVertexSizeBytes );
}

VOID ExportVB::ByteSwap( const D3DVERTEXELEMENT9* pVertexElements, const DWORD dwVertexElementCount )
{
    for( DWORD dwVertexIndex = 0; dwVertexIndex < m_uVertexCount; dwVertexIndex++ )
    {
        BYTE* pVB = GetVertex( dwVertexIndex );
        for( DWORD i = 0; i < dwVertexElementCount; i++ )
        {
            DWORD* pElement = (DWORD*)( pVB + pVertexElements[i].Offset );
            switch( pVertexElements[i].Type )
            {
            case D3DDECLTYPE_FLOAT4:
                *pElement = _byteswap_ulong( *pElement );
                pElement++;
            case D3DDECLTYPE_FLOAT3:
                *pElement = _byteswap_ulong( *pElement );
                pElement++;
            case D3DDECLTYPE_FLOAT2:
                *pElement = _byteswap_ulong( *pElement );
                pElement++;
            case D3DDECLTYPE_FLOAT1:
            case D3DDECLTYPE_D3DCOLOR:
            case D3DDECLTYPE_UBYTE4:
            case D3DDECLTYPE_UBYTE4N:
            case D3DDECLTYPE_DEC3N:
            case D3DDECLTYPE_UDEC3:
                *pElement = _byteswap_ulong( *pElement );
                break;
            case D3DDECLTYPE_SHORT4:
            case D3DDECLTYPE_SHORT4N:
            case D3DDECLTYPE_USHORT4N:
            case D3DDECLTYPE_FLOAT16_4:
                {
                    WORD* pWord = (WORD*)pElement;
                    *pWord = _byteswap_ushort( *pWord );
                    pWord++;
                    *pWord = _byteswap_ushort( *pWord );
                    pElement++;
                }
            case D3DDECLTYPE_SHORT2:
            case D3DDECLTYPE_SHORT2N:
            case D3DDECLTYPE_USHORT2N:
            case D3DDECLTYPE_FLOAT16_2:
                {
                    WORD* pWord = (WORD*)pElement;
                    *pWord = _byteswap_ushort( *pWord );
                    pWord++;
                    *pWord = _byteswap_ushort( *pWord );
                    pElement++;
                    break;
                }
            }
        }
    }
}

VOID ExportIB::ByteSwap()
{
    if( m_dwIndexSize == 2 )
    {
        for( DWORD i = 0; i < m_uIndexCount; i++ )
        {
            WORD wIndex = _byteswap_ushort( m_pIndexData16[ i ] );
            m_pIndexData16[ i ] = wIndex;
        }
    }
    else
    {
        for( DWORD i = 0; i < m_uIndexCount; i++ )
        {
            DWORD dwIndex = _byteswap_ulong( m_pIndexData32[ i ] );
            m_pIndexData32[ i ] = dwIndex;
        }
    }
}

VOID ExportIB::Allocate()
{
    if( m_pIndexData16 != NULL )
        delete[] m_pIndexData16;
    if( m_dwIndexSize == 2 )
    {
        m_pIndexData16 = new WORD[ m_uIndexCount ];
        ZeroMemory( m_pIndexData16, m_uIndexCount * sizeof( WORD ) );
    }
    else
    {
        m_pIndexData32 = new DWORD[ m_uIndexCount ];
        ZeroMemory( m_pIndexData32, m_uIndexCount * sizeof( DWORD ) );
    }
}

VOID ExportMeshTriangleAllocator::Terminate()
{
    AllocationBlockList::iterator iter = m_AllocationBlocks.begin();
    AllocationBlockList::iterator end = m_AllocationBlocks.end();
    while( iter != end )
    {
        AllocationBlock& block = *iter;
        delete[] block.pTriangleArray;
        ++iter;
    }
    m_AllocationBlocks.clear();
    m_uTotalCount = 0;
    m_uAllocatedCount = 0;
}

VOID ExportMeshTriangleAllocator::SetSizeHint( UINT uAnticipatedSize )
{
    if( uAnticipatedSize <= m_uTotalCount )
        return;
    UINT uNewCount = max( uAnticipatedSize - m_uTotalCount, 10000 );
    AllocationBlock NewBlock;
    NewBlock.m_uTriangleCount = uNewCount;
    NewBlock.pTriangleArray = new ExportMeshTriangle[ uNewCount ];
    m_AllocationBlocks.push_back( NewBlock );
    m_uTotalCount += uNewCount;
}

ExportMeshTriangle* ExportMeshTriangleAllocator::GetNewTriangle()
{
    if( m_uAllocatedCount == m_uTotalCount )
        SetSizeHint( m_uTotalCount + 10000 );
    UINT uIndex = m_uAllocatedCount;
    m_uAllocatedCount++;
    AllocationBlockList::iterator iter = m_AllocationBlocks.begin();
    AllocationBlockList::iterator end = m_AllocationBlocks.end();
    while( iter != end )
    {
        AllocationBlock& block = *iter;
        if( uIndex < block.m_uTriangleCount )
        {
            ExportMeshTriangle* pTriangle = &block.pTriangleArray[ uIndex ];
            pTriangle->Initialize();
            return pTriangle;
        }
        uIndex -= block.m_uTriangleCount;
        ++iter;
    }
    assert( FALSE );
    return NULL;
}

VOID ExportMeshTriangleAllocator::ClearAllTriangles()
{
    m_uAllocatedCount = 0;
}

BOOL ExportMeshVertex::Equals( const ExportMeshVertex* pOtherVertex ) const
{
    if( pOtherVertex == this )
        return TRUE;
    if( pOtherVertex->Position != Position )
        return FALSE;
    if( pOtherVertex->Normal != Normal )
        return FALSE;
    for( UINT i = 0; i < 8; i++ )
        if( pOtherVertex->TexCoords[i] != TexCoords[i] )
            return FALSE;
    if( pOtherVertex->Color != Color )
        return FALSE;
    return TRUE;
}

UINT FindOrAddVertex( ExportMeshVertexArray& ExistingVertexArray, ExportMeshVertex* pTestVertex )
{
    for( UINT i = 0; i < ExistingVertexArray.size(); i++ )
    {
        ExportMeshVertex* pVertex = ExistingVertexArray[i];
        if( pVertex->Equals( pTestVertex ) )
            return i;
    }
    UINT index = (UINT)ExistingVertexArray.size();
    ExistingVertexArray.push_back( pTestVertex );
    return index;
}

VOID ExportMesh::AddRawTriangle( ExportMeshTriangle* pTriangle )
{
    m_RawTriangles.push_back( pTriangle );
    m_uDCCVertexCount = max( m_uDCCVertexCount, (UINT)pTriangle->Vertex[0].DCCVertexIndex + 1 );
    m_uDCCVertexCount = max( m_uDCCVertexCount, (UINT)pTriangle->Vertex[1].DCCVertexIndex + 1 );
    m_uDCCVertexCount = max( m_uDCCVertexCount, (UINT)pTriangle->Vertex[2].DCCVertexIndex + 1 );
}

UINT FindOrAddVertexFast( ExportMeshVertexArray& ExistingVertexArray, ExportMeshVertex* pTestVertex )
{
    UINT uIndex = pTestVertex->DCCVertexIndex;
    assert( uIndex < ExistingVertexArray.size() );
    ExportMeshVertex* pVertex = ExistingVertexArray[ uIndex ];
    if( pVertex != NULL )
    {
        ExportMeshVertex* pLastVertex = NULL;
        while( pVertex != NULL )
        {
            uIndex = pVertex->DCCVertexIndex;
            if( pVertex->Equals( pTestVertex ) )
                return uIndex;
            pLastVertex = pVertex;
            pVertex = pVertex->pNextDuplicateVertex;
        }
        assert( pVertex == NULL );
        uIndex = (UINT)ExistingVertexArray.size();
        ExistingVertexArray.push_back( pTestVertex );
        pTestVertex->DCCVertexIndex = uIndex;
        if( pLastVertex != NULL )
        {
            pLastVertex->pNextDuplicateVertex = pTestVertex;
        }
    }
    else
    {
        ExistingVertexArray[ uIndex ] = pTestVertex;
    }
    return uIndex;
}

VOID ExportMesh::SetVertexNormalCount( UINT uCount )
{
    m_VertexFormat.m_bNormal = ( uCount > 0 );
    m_VertexFormat.m_bTangent = ( uCount > 1 );
    m_VertexFormat.m_bBinormal = ( uCount > 2 );
}

BOOL SubsetLess( ExportMeshTriangle* pA, ExportMeshTriangle* pB )
{
    return pA->SubsetIndex < pB->SubsetIndex;
}

VOID ExportMesh::SortRawTrianglesBySubsetIndex()
{
    if( m_RawTriangles.size() == 0 )
        return;

    std::stable_sort( m_RawTriangles.begin(), m_RawTriangles.end(), SubsetLess );
}

VOID ExportMesh::Optimize( DWORD dwFlags )
{
    if( m_RawTriangles.size() == 0 )
        return;
    ExportLog::LogMsg( 4, "Optimizing mesh \"%s\" with %d triangles.", GetName().SafeString(), m_RawTriangles.size() );
    
    SortRawTrianglesBySubsetIndex();

    ExportIBSubset* pCurrentIBSubset = NULL;
    INT iCurrentSubsetIndex = -1;
    std::vector<UINT> IndexData;
    IndexData.reserve( m_RawTriangles.size() * 3 );
    ExportMeshVertexArray VertexData;
    VertexData.resize( m_uDCCVertexCount, (ExportMeshVertex*)NULL );

    BOOL bFlipTriangles = g_pScene->Settings().bFlipTriangles;
    if( dwFlags & FLIP_TRIANGLES )
        bFlipTriangles = !bFlipTriangles;

    // loop through raw triangles
    const DWORD dwTriangleCount = (DWORD)m_RawTriangles.size();
    m_TriangleToPolygonMapping.clear();
    m_TriangleToPolygonMapping.reserve( dwTriangleCount );

    for( UINT i = 0; i < dwTriangleCount; i++ )
    {
        ExportMeshTriangle* pTriangle = m_RawTriangles[i];
        // create a new subset if one is encountered
        // note: subset index will be monotonically increasing
        assert( pTriangle->SubsetIndex >= iCurrentSubsetIndex );
        if( pTriangle->SubsetIndex > iCurrentSubsetIndex )
        {
            pCurrentIBSubset = new ExportIBSubset();
            pCurrentIBSubset->SetName( "Default" );
            pCurrentIBSubset->SetStartIndex( (UINT)IndexData.size() );
            m_vSubsets.push_back( pCurrentIBSubset );
            iCurrentSubsetIndex = pTriangle->SubsetIndex;
        }
        // collapse the triangle verts into the final vertex list
        // this removes unnecessary duplicates, and retains necessary duplicates
        UINT uIndexA = FindOrAddVertexFast( VertexData, &pTriangle->Vertex[0] );
        UINT uIndexB = FindOrAddVertexFast( VertexData, &pTriangle->Vertex[1] );
        UINT uIndexC = FindOrAddVertexFast( VertexData, &pTriangle->Vertex[2] );
        // record final indices into the index list
        IndexData.push_back( uIndexA );
        if( bFlipTriangles )
        {
            IndexData.push_back( uIndexC );
            IndexData.push_back( uIndexB );
        }
        else
        {
            IndexData.push_back( uIndexB );
            IndexData.push_back( uIndexC );
        }
        m_TriangleToPolygonMapping.push_back( pTriangle->PolygonIndex );
        pCurrentIBSubset->IncrementIndexCount( 3 );
    }
    ExportLog::LogMsg( 3, "Triangle list mesh: %d verts, %d indices, %d subsets", VertexData.size(), IndexData.size(), m_vSubsets.size() );
    if( VertexData.size() > 16777215 )
    {
        ExportLog::LogError( "Mesh \"%s\" has more than 16777215 vertices.  Index buffer is invalid.", GetName().SafeString() );
    }

    // Create real index buffer from index list
    m_pIB = new ExportIB();
    m_pIB->SetIndexCount( (UINT)IndexData.size() );
    if( VertexData.size() > 65535 || g_pScene->Settings().bForceIndex32Format )
    {
        m_pIB->SetIndexSize( 4 );
    }
    else
    {
        m_pIB->SetIndexSize( 2 );
    }
    m_pIB->Allocate();
    for( UINT i = 0; i < IndexData.size(); i++ )
    {
        m_pIB->SetIndex( i, IndexData[i] );
    }

    INT iUVAtlasTexCoordIndex = g_pScene->Settings().iGenerateUVAtlasOnTexCoordIndex;
    BOOL bComputeUVAtlas = ( iUVAtlasTexCoordIndex >= 0 );
    if( bComputeUVAtlas )
    {
        if( iUVAtlasTexCoordIndex < (INT)m_VertexFormat.m_uUVSetCount )
        {
            ExportLog::LogWarning( "UV atlas being generated in existing texture coordinate set %d, which will overwrite its contents.", iUVAtlasTexCoordIndex );
        }
        else
        {
            // Save the index of a new UV set
            iUVAtlasTexCoordIndex = (INT)m_VertexFormat.m_uUVSetCount;
            // add another UV set, it will be empty
            ++m_VertexFormat.m_uUVSetCount;
            ExportLog::LogMsg( 4, "Adding a new texture coordinate set for the UV atlas." );
        }
    }

    // Convert vertex data to final format
    BuildVertexBuffer( VertexData, dwFlags );

    // Check if we need to remap the UV atlas texcoord index
    if( bComputeUVAtlas && ( iUVAtlasTexCoordIndex != g_pScene->Settings().iGenerateUVAtlasOnTexCoordIndex ) )
    {
        // Scan the decl elements
        for( DWORD i = 0; i < m_VertexElements.size(); ++i )
        {
            if( m_VertexElements[i].Usage == D3DDECLUSAGE_TEXCOORD && m_VertexElements[i].UsageIndex == iUVAtlasTexCoordIndex )
            {
                // Change the decl usage index to the desired usage index
                m_VertexElements[i].UsageIndex = (BYTE)g_pScene->Settings().iGenerateUVAtlasOnTexCoordIndex;
                iUVAtlasTexCoordIndex = g_pScene->Settings().iGenerateUVAtlasOnTexCoordIndex;
            }
        }
    }

    BOOL bComputeTangentSpace = FALSE;

    // Compute vertex tangent space data
    if( m_VertexFormat.m_bTangent || m_VertexFormat.m_bBinormal )
    {
        bComputeTangentSpace = TRUE;
    }

    if( bComputeTangentSpace || bComputeUVAtlas )
    {
        ID3DXMesh* pMesh = CreateD3DXMesh();
        if( pMesh != NULL )
        {
            if( bComputeTangentSpace )
            {
                ComputeVertexTangentSpacesD3DX( &pMesh );
            }
            if( bComputeUVAtlas )
            {
                ComputeUVAtlas( &pMesh );
            }
            CopyD3DXMeshIntoMesh( pMesh );
            pMesh->Release();
        }
    }

    ClearRawTriangles();
    ComputeBounds();

    ExportLog::LogMsg( 3, "Vertex size: %d bytes; VB size: %d bytes", m_pVB->GetVertexSize(), m_pVB->GetVertexDataSize() );

    if( ExportLog::GetLogLevel() >= 4 )
    {
        DWORD dwDeclSize = GetVertexDeclElementCount();
        
        static const CHAR* strDeclUsages[] = { "Position", "BlendWeight", "BlendIndices", "Normal", "PSize", "TexCoord", "Tangent", "Binormal", "TessFactor", "PositionT", "Color", "Fog", "Depth", "Sample" };
        C_ASSERT( ARRAYSIZE(strDeclUsages) == ( MAXD3DDECLUSAGE + 1 ) );

        static const CHAR* strDeclTypes[] = { "Float1", "Float2", "Float3", "Float4", "D3DColor", "UByte", "Short2", "Short4", "UByte4N", "Short2N", "Short4N", "UShort2N", "UShort4N", "UDec3", "Dec3N", "Float16_2", "Float16_4", "Unused" };
        C_ASSERT( ARRAYSIZE(strDeclTypes) == ( MAXD3DDECLTYPE + 1 ) );

        for( DWORD i = 0; i < dwDeclSize; ++i )
        {
            const D3DVERTEXELEMENT9& Element = GetVertexDeclElement( i );

            ExportLog::LogMsg( 4, "Element %2d Stream %2d Offset %2d: %12s.%-2d Type %s (%d bytes)", i, Element.Stream, Element.Offset, strDeclUsages[Element.Usage], Element.UsageIndex, strDeclTypes[Element.Type], GetElementSizeFromDeclType( Element.Type ) );
        }
    }

    ExportLog::LogMsg( 4, "DCC stored %d verts; final vertex count is %d due to duplication.", m_uDCCVertexCount, m_pVB->GetVertexCount() );

    if( dwFlags & FORCE_SUBD_CONVERSION )
    {
        ExportLog::LogMsg( 2, "Converting mesh \"%s\" to a subdivision surface mesh.", GetName().SafeString() );
        m_pSubDMesh = new ExportSubDProcessMesh();
        m_pSubDMesh->Initialize( this );
    }
}


ID3DXMesh* ExportMesh::CreateD3DXMesh()
{
    // D3DXMesh requires a valid D3D device...
    assert( g_pd3dDevice != NULL );

    D3DXDebugMute( FALSE );

    DWORD dwFaceCount = m_pIB->GetIndexCount() / 3;

    // D3DXMesh requires 32-bit index buffers if the face count is over 65534.
    BOOL bD3DXWorkaround = ( m_pIB->GetIndexSize() == 2 ) && ( dwFaceCount > 65534 );

    DWORD dwMeshOptions = D3DXMESH_SYSTEMMEM;
    if( bD3DXWorkaround || m_pIB->GetIndexSize() == 4 )
        dwMeshOptions |= D3DXMESH_32BIT;

    // Create a decl element list with D3DDECL_END at the end
    vector< D3DVERTEXELEMENT9 > DeclElements = m_VertexElements;
    D3DVERTEXELEMENT9 DeclEnd = D3DDECL_END();
    DeclElements.push_back( DeclEnd );

    D3DVERTEXELEMENT9* pElements = (D3DVERTEXELEMENT9*)&DeclElements.front();

    // Create D3DXMesh
    ID3DXMesh* pInputD3DXMesh = NULL;
    HRESULT hr = D3DXCreateMesh( dwFaceCount, m_pVB->GetVertexCount(), dwMeshOptions, pElements, g_pd3dDevice, &pInputD3DXMesh );
    if( FAILED( hr ) )
    {
        ExportLog::LogError( "Could not create D3DX mesh for mesh \"%s\".", (const CHAR*)GetName() );
        return NULL;
    }

    // copy VB data into D3DXMesh
    {
        BYTE* pVBData = NULL;
        pInputD3DXMesh->LockVertexBuffer( 0, (VOID**)&pVBData );
        memcpy( pVBData, m_pVB->GetVertexData(), m_pVB->GetVertexDataSize() );
        pInputD3DXMesh->UnlockVertexBuffer();
    }

    // copy IB data into D3DXMesh
    {
        BYTE* pIBData = NULL;
        pInputD3DXMesh->LockIndexBuffer( 0, (VOID**)&pIBData );
        if( bD3DXWorkaround )
        {
            DWORD dwIndexCount = m_pIB->GetIndexCount();
            DWORD* pIBDataDW = (DWORD*)pIBData;
            for( DWORD i = 0; i < dwIndexCount; ++i )
            {
                pIBDataDW[i] = m_pIB->GetIndex( i );
            }
        }
        else
        {
            memcpy( pIBData, m_pIB->GetIndexData(), m_pIB->GetIndexDataSize() );
        }
        pInputD3DXMesh->UnlockIndexBuffer();
    }

    // copy subset data into D3DXMesh
    {
        DWORD dwSubsetCount = GetSubsetCount();
        D3DXATTRIBUTERANGE* pMeshAttributeRanges = new D3DXATTRIBUTERANGE[ dwSubsetCount ];
        for( DWORD i = 0; i < dwSubsetCount; ++i )
        {
            pMeshAttributeRanges[i].AttribId = i;
            const ExportIBSubset* pSubset = GetSubset( i );
            pMeshAttributeRanges[i].FaceStart = pSubset->GetStartIndex() / 3;
            pMeshAttributeRanges[i].FaceCount = pSubset->GetIndexCount() / 3;
            pMeshAttributeRanges[i].VertexStart = 0;
            pMeshAttributeRanges[i].VertexCount = m_pVB->GetVertexCount();
        }
        pInputD3DXMesh->SetAttributeTable( pMeshAttributeRanges, dwSubsetCount );
        delete[] pMeshAttributeRanges;
    }

    return pInputD3DXMesh;
}


VOID ExportMesh::CopyD3DXMeshIntoMesh( ID3DXMesh* pMesh )
{
    if( pMesh->GetNumVertices() != m_pVB->GetVertexCount() )
    {
        // check that the D3DXMesh didn't resize the vertex struct
        assert( pMesh->GetNumBytesPerVertex() == m_pVB->GetVertexSize() );
        // resize vertex buffer
        ExportVB* pNewVB = new ExportVB();
        pNewVB->SetVertexCount( pMesh->GetNumVertices() );
        pNewVB->SetVertexSize( pMesh->GetNumBytesPerVertex() );
        pNewVB->Allocate();
        ExportLog::LogMsg( 4, "D3DX mesh operations increased vertex count from %d to %d.", m_pVB->GetVertexCount(), pNewVB->GetVertexCount() );
        delete m_pVB;
        m_pVB = pNewVB;
    }

    // copy D3DXMesh VB data back into m_pVB
    {
        BYTE* pVBData = NULL;
        pMesh->LockVertexBuffer( 0, (VOID**)&pVBData );
        memcpy( m_pVB->GetVertexData(), pVBData, m_pVB->GetVertexDataSize() );
        pMesh->UnlockVertexBuffer();
    }

    // D3DX mesh operations should never change the index count (but the index data may change)
    assert( pMesh->GetNumFaces() == ( m_pIB->GetIndexCount() / 3 ) );

    // Sometimes the destination D3DXMesh will have exceeded 64K verts after computing
    // the tangent space.  In this case, we have to upgrade the index buffer to 32bit.
    BOOL bNeed32BitIB = ( m_pVB->GetVertexCount() > 65535 ) && ( m_pIB->GetIndexSize() == 2 );
    if( bNeed32BitIB )
    {
        ExportIB* pNewIB = new ExportIB();
        pNewIB->SetIndexCount( m_pIB->GetIndexCount() );
        pNewIB->SetIndexSize( 4 );
        pNewIB->Allocate();
        delete m_pIB;
        m_pIB = pNewIB;
    }

    // copy D3DXMesh IB data back into m_pIB
    {
        IDirect3DIndexBuffer9* pMeshIB = NULL;
        pMesh->GetIndexBuffer( &pMeshIB );
        D3DINDEXBUFFER_DESC IBDesc;
        pMeshIB->GetDesc( &IBDesc );
        pMeshIB->Release();
        BOOL bConvert32To16 = FALSE;
        if( IBDesc.Format == D3DFMT_INDEX32 && m_pIB->GetIndexSize() == 2 )
            bConvert32To16 = TRUE;

        BYTE* pIBData = NULL;
        pMesh->LockIndexBuffer( 0, (VOID**)&pIBData );
        if( bConvert32To16 )
        {
            DWORD dwIndexCount = m_pIB->GetIndexCount();
            DWORD* pIBDataDW = (DWORD*)pIBData;
            for( DWORD i = 0; i < dwIndexCount; ++i )
            {
                m_pIB->SetIndex( i, pIBDataDW[i] );
            }
        }
        else
        {
            memcpy( m_pIB->GetIndexData(), pIBData, m_pIB->GetIndexDataSize() );
        }
        pMesh->UnlockIndexBuffer();
    }
}


HRESULT ExportMesh::ComputeVertexTangentSpacesD3DX( ID3DXMesh** ppMesh )
{
    // We must be at least computing a tangent vector here.  Binormal is optional.
    assert( m_VertexFormat.m_bTangent );

    ID3DXMesh* pSrcMesh = *ppMesh;

    // Compute tangent space, results will be in pDestMesh
    DWORD dwTangentCreationOptions = 0;
    ID3DXMesh* pDestMesh = NULL;
    ID3DXBuffer* pMappingBuffer = NULL;
    HRESULT hr = D3DXComputeTangentFrameEx( 
        pSrcMesh, 
        D3DDECLUSAGE_TEXCOORD, 
        0,   
        D3DDECLUSAGE_TANGENT, 
        0, 
        m_VertexFormat.m_bBinormal ? D3DDECLUSAGE_BINORMAL : D3DX_DEFAULT, 
        0, 
        D3DDECLUSAGE_NORMAL, 
        0, 
        dwTangentCreationOptions,
        NULL, 
        0.01f, 0.25f, 0.01f, 
        &pDestMesh, 
        &pMappingBuffer);

    if( SUCCEEDED( hr ) )
    {
        assert( pDestMesh != NULL );
        pSrcMesh->Release();
        pMappingBuffer->Release();
        *ppMesh = pDestMesh;
    }
    else if( pDestMesh != NULL )
    {
        pDestMesh->Release();
    }

    return hr;
}


HRESULT ExportMesh::ComputeUVAtlas( ID3DXMesh** ppMesh )
{
    ID3DXMesh* pSrcMesh = *ppMesh;
    assert( pSrcMesh != NULL );

    ExportLog::LogMsg( 4, "Generating UV atlas..." );

    INT iDestUVIndex = g_pScene->Settings().iGenerateUVAtlasOnTexCoordIndex;
    assert( iDestUVIndex >= 0 && iDestUVIndex < 8 );

    DWORD* pAdjacency = new DWORD[ 3 * pSrcMesh->GetNumFaces() ];
    pSrcMesh->GenerateAdjacency( 0.001f, pAdjacency );

    ID3DXMesh* pDestMesh = NULL;
    DWORD dwNumCharts = 0;
    FLOAT fMaxStretch = 0;
    ID3DXBuffer* pPartitioningBuffer = NULL;
    ID3DXBuffer* pVertexRemapBuffer = NULL;

    HRESULT hr = D3DXUVAtlasCreate(
        pSrcMesh,
        0,
        g_pScene->Settings().fUVAtlasMaxStretch,
        g_pScene->Settings().iUVAtlasTextureSize,
        g_pScene->Settings().iUVAtlasTextureSize,
        g_pScene->Settings().fUVAtlasGutter,
        iDestUVIndex,
        pAdjacency,
        NULL,
        NULL,
        NULL,
        1.0f,
        NULL,
        D3DXUVATLAS_DEFAULT,
        &pDestMesh,
        &pPartitioningBuffer,
        &pVertexRemapBuffer,
        &fMaxStretch,
        (UINT*)&dwNumCharts );

    delete[] pAdjacency;
    SAFE_RELEASE( pPartitioningBuffer );
    SAFE_RELEASE( pVertexRemapBuffer );

    if( SUCCEEDED( hr ) )
    {
        ExportLog::LogMsg( 4, "Created UV atlas with %d charts in texcoord %d.", dwNumCharts, iDestUVIndex );
        assert( pDestMesh != NULL );
        pSrcMesh->Release();
        *ppMesh = pDestMesh;
        return S_OK;
    }
    else if( pDestMesh != NULL )
    {
        pDestMesh->Release();
    }

    ExportLog::LogError( "UV atlas creation failed for mesh \"%s\".", GetName().SafeString() );

    return hr;
}


/*
VOID ExportMesh::ComputeVertexTangentSpacesD3DX()
{
    // D3DXMesh requires a valid D3D device...
    assert( g_pd3dDevice != NULL );

    // We must be at least computing a tangent vector here.  Binormal is optional.
    assert( m_VertexFormat.m_bTangent );

    DWORD dwFaceCount = m_pIB->GetIndexCount() / 3;

    // D3DXMesh requires 32-bit index buffers if the face count is over 65534.
    BOOL bD3DXWorkaround = ( m_pIB->GetIndexSize() == 2 ) && ( dwFaceCount > 65534 );

    DWORD dwMeshOptions = D3DXMESH_SYSTEMMEM;
    if( bD3DXWorkaround || m_pIB->GetIndexSize() == 4 )
        dwMeshOptions |= D3DXMESH_32BIT;

    // Create a decl element list with D3DDECL_END at the end
    vector< D3DVERTEXELEMENT9 > DeclElements = m_VertexElements;
    D3DVERTEXELEMENT9 DeclEnd = D3DDECL_END();
    DeclElements.push_back( DeclEnd );

    D3DVERTEXELEMENT9* pElements = (D3DVERTEXELEMENT9*)&DeclElements.front();

    // Create D3DXMesh
    ID3DXMesh* pInputD3DXMesh = NULL;
    HRESULT hr = D3DXCreateMesh( dwFaceCount, m_pVB->GetVertexCount(), dwMeshOptions, pElements, g_pd3dDevice, &pInputD3DXMesh );
    if( FAILED( hr ) )
    {
        ExportLog::LogError( "Could not create D3DX mesh for mesh \"%s\".", (const CHAR*)GetName() );
        return;
    }

    // copy VB data into D3DXMesh
    {
        BYTE* pVBData = NULL;
        pInputD3DXMesh->LockVertexBuffer( 0, (VOID**)&pVBData );
        memcpy( pVBData, m_pVB->GetVertexData(), m_pVB->GetVertexDataSize() );
        pInputD3DXMesh->UnlockVertexBuffer();
    }

    // copy IB data into D3DXMesh
    {
        BYTE* pIBData = NULL;
        pInputD3DXMesh->LockIndexBuffer( 0, (VOID**)&pIBData );
        if( bD3DXWorkaround )
        {
            DWORD dwIndexCount = m_pIB->GetIndexCount();
            DWORD* pIBDataDW = (DWORD*)pIBData;
            for( DWORD i = 0; i < dwIndexCount; ++i )
            {
                pIBDataDW[i] = m_pIB->GetIndex( i );
            }
        }
        else
        {
            memcpy( pIBData, m_pIB->GetIndexData(), m_pIB->GetIndexDataSize() );
        }
        pInputD3DXMesh->UnlockIndexBuffer();
    }

    // copy subset data into D3DXMesh
    {
        DWORD dwSubsetCount = GetSubsetCount();
        D3DXATTRIBUTERANGE* pMeshAttributeRanges = new D3DXATTRIBUTERANGE[ dwSubsetCount ];
        for( DWORD i = 0; i < dwSubsetCount; ++i )
        {
            pMeshAttributeRanges[i].AttribId = i;
            const ExportIBSubset* pSubset = GetSubset( i );
            pMeshAttributeRanges[i].FaceStart = pSubset->GetStartIndex() / 3;
            pMeshAttributeRanges[i].FaceCount = pSubset->GetIndexCount() / 3;
            pMeshAttributeRanges[i].VertexStart = 0;
            pMeshAttributeRanges[i].VertexCount = m_pVB->GetVertexCount();
        }
        pInputD3DXMesh->SetAttributeTable( pMeshAttributeRanges, dwSubsetCount );
        delete[] pMeshAttributeRanges;
    }

    // Compute tangent space, results will be in pDestMesh
    DWORD dwTangentCreationOptions = 0;
    ID3DXMesh* pDestMesh = NULL;
    ID3DXBuffer* pMappingBuffer = NULL;
    hr = D3DXComputeTangentFrameEx( 
        pInputD3DXMesh, 
        D3DDECLUSAGE_TEXCOORD, 
        0,   
        D3DDECLUSAGE_TANGENT, 
        0, 
        m_VertexFormat.m_bBinormal ? D3DDECLUSAGE_BINORMAL : D3DX_DEFAULT, 
        0, 
        D3DDECLUSAGE_NORMAL, 
        0, 
        dwTangentCreationOptions,
        NULL, 
        0.01f, 0.25f, 0.01f, 
        &pDestMesh, 
        &pMappingBuffer);

    if( FAILED( hr ) )
    {
        pInputD3DXMesh->Release();
        if( pDestMesh != NULL )
            pDestMesh->Release();
        if( pMappingBuffer != NULL )
            pMappingBuffer->Release();
        ExportLog::LogError( "Could not create mesh tangent space for mesh \"%s\".", GetName() );
        return;
    }

    if( pDestMesh->GetNumVertices() != m_pVB->GetVertexCount() )
    {
        // check that the D3DXMesh didn't resize the vertex struct
        assert( pDestMesh->GetNumBytesPerVertex() == m_pVB->GetVertexSize() );
        // resize vertex buffer
        ExportVB* pNewVB = new ExportVB();
        pNewVB->SetVertexCount( pDestMesh->GetNumVertices() );
        pNewVB->SetVertexSize( pDestMesh->GetNumBytesPerVertex() );
        pNewVB->Allocate();
        ExportLog::LogMsg( 4, "Tangent space computation increased vertex count from %d to %d.", m_pVB->GetVertexCount(), pNewVB->GetVertexCount() );
        delete m_pVB;
        m_pVB = pNewVB;
    }

    // copy D3DXMesh VB data back into m_pVB
    {
        BYTE* pVBData = NULL;
        pDestMesh->LockVertexBuffer( 0, (VOID**)&pVBData );
        memcpy( m_pVB->GetVertexData(), pVBData, m_pVB->GetVertexDataSize() );
        pDestMesh->UnlockVertexBuffer();
    }

    // D3DXComputeTangentSpaceEx should never change the index count (but the index data may change)
    assert( pDestMesh->GetNumFaces() == ( m_pIB->GetIndexCount() / 3 ) );

    // Sometimes the destination D3DXMesh will have exceeded 64K verts after computing
    // the tangent space.  In this case, we have to upgrade the index buffer to 32bit.
    BOOL bNeed32BitIB = ( m_pVB->GetVertexCount() > 65535 ) && ( m_pIB->GetIndexSize() == 2 );
    if( bNeed32BitIB )
    {
        ExportIB* pNewIB = new ExportIB();
        pNewIB->SetIndexCount( m_pIB->GetIndexCount() );
        pNewIB->SetIndexSize( 4 );
        pNewIB->Allocate();
        delete m_pIB;
        m_pIB = pNewIB;
    }

    // copy D3DXMesh IB data back into m_pIB
    {
        IDirect3DIndexBuffer9* pMeshIB = NULL;
        pDestMesh->GetIndexBuffer( &pMeshIB );
        D3DINDEXBUFFER_DESC IBDesc;
        pMeshIB->GetDesc( &IBDesc );
        pMeshIB->Release();
        BOOL bConvert32To16 = FALSE;
        if( IBDesc.Format == D3DFMT_INDEX32 && m_pIB->GetIndexSize() == 2 )
            bConvert32To16 = TRUE;

        BYTE* pIBData = NULL;
        pDestMesh->LockIndexBuffer( 0, (VOID**)&pIBData );
        if( bConvert32To16 )
        {
            DWORD dwIndexCount = m_pIB->GetIndexCount();
            DWORD* pIBDataDW = (DWORD*)pIBData;
            for( DWORD i = 0; i < dwIndexCount; ++i )
            {
                m_pIB->SetIndex( i, pIBDataDW[i] );
            }
        }
        else
        {
            memcpy( m_pIB->GetIndexData(), pIBData, m_pIB->GetIndexDataSize() );
        }
        pDestMesh->UnlockIndexBuffer();
    }

    pInputD3DXMesh->Release();
    pDestMesh->Release();
    pMappingBuffer->Release();
}
*/


DWORD MakeCompressedVector( const D3DXVECTOR3& Vec3 )
{
    XMXDECN4 DecN4( Vec3.x, Vec3.y, Vec3.z, 1 );
    return DecN4.v;
}


DWORD MakeCompressedVectorUByte4N( const D3DXVECTOR3& Vec3 )
{
    D3DXVECTOR3 Biased( Vec3 * 0.5f );
    Biased += D3DXVECTOR3( 0.5f, 0.5f, 0.5f );
    XMUBYTEN4 UB4( Biased.x, Biased.y, Biased.z, 1 );
    return UB4.v;
}


VOID NormalizeBoneWeights( BYTE* pWeights )
{
    DWORD dwSum = (DWORD)pWeights[0] + (DWORD)pWeights[1] + (DWORD)pWeights[2] + (DWORD)pWeights[3];
    if( dwSum == 255 )
        return;

    INT iDifference = 255 - (INT)dwSum;
    for( DWORD i = 0; i < 4; ++i )
    {
        if( pWeights[i] == 0 )
            continue;
        INT iValue = (INT)pWeights[i];
        if( iValue + iDifference > 255 )
        {
            iDifference -= ( 255 - iValue );
            iValue = 255;
        }
        else
        {
            iValue += iDifference;
            iDifference = 0;
        }
        pWeights[i] = (BYTE)iValue;
    }

    dwSum = (DWORD)pWeights[0] + (DWORD)pWeights[1] + (DWORD)pWeights[2] + (DWORD)pWeights[3];
    assert( dwSum == 255 );
}


__inline INT GetElementSizeFromDeclType(DWORD Type)
{
    switch (Type)
    {
    case D3DDECLTYPE_FLOAT1:
        return 4;
    case D3DDECLTYPE_FLOAT2:
        return 8;
    case D3DDECLTYPE_FLOAT3:
        return 12;
    case D3DDECLTYPE_FLOAT4:
        return 16;
    case D3DDECLTYPE_D3DCOLOR:
        return 4;
    case D3DDECLTYPE_UBYTE4:
        return 4;
    case D3DDECLTYPE_SHORT2:
        return 4;
    case D3DDECLTYPE_SHORT4:
        return 8;
    case D3DDECLTYPE_UBYTE4N:
        return 4;
    case D3DDECLTYPE_SHORT2N:
        return 4;
    case D3DDECLTYPE_SHORT4N:
        return 8;
    case D3DDECLTYPE_USHORT2N:
        return 4;
    case D3DDECLTYPE_USHORT4N:
        return 8;
    case D3DDECLTYPE_UDEC3:
        return 4;
    case D3DDECLTYPE_DEC3N:
        return 4;
    case D3DDECLTYPE_FLOAT16_2:
        return 4;
    case D3DDECLTYPE_FLOAT16_4:
        return 8;
    case D3DDECLTYPE_UNUSED:
        return 0;
    default:
        assert(false);
        return 0;
    }
}


VOID TransformAndWriteVector( BYTE* pDest, const D3DXVECTOR3& Src, DWORD dwDestFormat )
{
    D3DXVECTOR3 SrcTransformed;
    g_pScene->GetDCCTransformer()->TransformDirection( &SrcTransformed, &Src );
    switch( dwDestFormat )
    {
    case D3DDECLTYPE_FLOAT3:
        {
            *(D3DXVECTOR3*)pDest = SrcTransformed;
            break;
        }
    case D3DDECLTYPE_DEC3N:
        {
            *(DWORD*)pDest = MakeCompressedVector( SrcTransformed );
            break;
        }
    case D3DDECLTYPE_UBYTE4N:
        {
            *(DWORD*)pDest = MakeCompressedVectorUByte4N( SrcTransformed );
            break;
        }
    case D3DDECLTYPE_SHORT4N:
        {
            *(XMSHORTN4*)pDest = XMSHORTN4( SrcTransformed.x, SrcTransformed.y, SrcTransformed.z, 1 );
            break;
        }
    case D3DDECLTYPE_FLOAT16_4:
        {
            *(XMHALF4*)pDest = XMHALF4( SrcTransformed.x, SrcTransformed.y, SrcTransformed.z, 1 );
            break;
        }
    }
}


VOID ExportMesh::BuildVertexBuffer( ExportMeshVertexArray& VertexArray, DWORD dwFlags )
{
    UINT uVertexSize = 0;
    INT iCurrentVertexOffset = 0;
    INT iPositionOffset = -1;
    INT iNormalOffset = -1;
    INT iTangentOffset = -1;
    INT iBinormalOffset = -1;
    INT iSkinDataOffset = -1;
    INT iColorOffset = -1;
    INT iUVOffset = -1;

    // create a vertex element struct and set default values
    D3DVERTEXELEMENT9 VertexElement;
    ZeroMemory( &VertexElement, sizeof( D3DVERTEXELEMENT9 ) );
    VertexElement.Method = D3DDECLMETHOD_DEFAULT;

    BOOL bCompressVertexData = ( dwFlags & COMPRESS_VERTEX_DATA );

    DWORD dwNormalType = D3DDECLTYPE_FLOAT3;
    if( bCompressVertexData )
    {
        dwNormalType = g_pScene->Settings().dwNormalCompressedType;
    }

    m_VertexElements.clear();

    // check each vertex format option, and create a corresponding decl element
    if( m_VertexFormat.m_bPosition )
    {
        iPositionOffset = iCurrentVertexOffset;
        VertexElement.Offset = (WORD)iCurrentVertexOffset;
        VertexElement.Usage = D3DDECLUSAGE_POSITION;
        VertexElement.Type = D3DDECLTYPE_FLOAT3;
        m_VertexElements.push_back( VertexElement );
        iCurrentVertexOffset += GetElementSizeFromDeclType( VertexElement.Type );
    }
    if( ( m_VertexFormat.m_bSkinData && g_pScene->Settings().bExportSkinWeights ) || g_pScene->Settings().bForceExportSkinWeights )
    {
        iSkinDataOffset = iCurrentVertexOffset;
        VertexElement.Offset = (WORD)iCurrentVertexOffset;
        VertexElement.Usage = D3DDECLUSAGE_BLENDWEIGHT;
        VertexElement.Type = D3DDECLTYPE_UBYTE4N;
        m_VertexElements.push_back( VertexElement );
        iCurrentVertexOffset += GetElementSizeFromDeclType( VertexElement.Type );
        VertexElement.Offset = (WORD)iCurrentVertexOffset;
        VertexElement.Usage = D3DDECLUSAGE_BLENDINDICES;
        VertexElement.Type = D3DDECLTYPE_UBYTE4;
        m_VertexElements.push_back( VertexElement );
        iCurrentVertexOffset += GetElementSizeFromDeclType( VertexElement.Type );
    }
    if( m_VertexFormat.m_bNormal )
    {
        iNormalOffset = iCurrentVertexOffset;
        VertexElement.Offset = (WORD)iCurrentVertexOffset;
        VertexElement.Usage = D3DDECLUSAGE_NORMAL;
        VertexElement.Type = (BYTE)dwNormalType;
        m_VertexElements.push_back( VertexElement );
        iCurrentVertexOffset += GetElementSizeFromDeclType( VertexElement.Type );
    }
    if( m_VertexFormat.m_bVertexColor )
    {
        iColorOffset = iCurrentVertexOffset;
        VertexElement.Offset = (WORD)iCurrentVertexOffset;
        VertexElement.Usage = D3DDECLUSAGE_COLOR;
        VertexElement.Type = D3DDECLTYPE_D3DCOLOR;
        m_VertexElements.push_back( VertexElement );
        iCurrentVertexOffset += GetElementSizeFromDeclType( VertexElement.Type );
    }
    if( m_VertexFormat.m_uUVSetCount > 0 )
    {
        iUVOffset = iCurrentVertexOffset;
        for( UINT t = 0; t < m_VertexFormat.m_uUVSetCount; t++ )
        {
            VertexElement.Offset = (WORD)iCurrentVertexOffset;
            VertexElement.Usage = D3DDECLUSAGE_TEXCOORD;
            switch( m_VertexFormat.m_uUVSetSize )
            {
            case 1:
                VertexElement.Type = D3DDECLTYPE_FLOAT1;
                break;
            case 2:
                if( bCompressVertexData )
                {
                    VertexElement.Type = D3DDECLTYPE_FLOAT16_2;
                }
                else
                {
                    VertexElement.Type = D3DDECLTYPE_FLOAT2;
                }
                break;
            case 3:
                if( bCompressVertexData )
                {
                    VertexElement.Type = D3DDECLTYPE_FLOAT16_4;
                }
                else
                {
                    VertexElement.Type = D3DDECLTYPE_FLOAT3;
                }
                break;
            case 4:
                if( bCompressVertexData )
                {
                    VertexElement.Type = D3DDECLTYPE_FLOAT16_4;
                }
                else
                {
                    VertexElement.Type = D3DDECLTYPE_FLOAT4;
                }
            default:
                continue;
            }
            VertexElement.UsageIndex = (BYTE)t;
            m_VertexElements.push_back( VertexElement );
            iCurrentVertexOffset += GetElementSizeFromDeclType( VertexElement.Type );
        }
        VertexElement.UsageIndex = 0;
    }
    if( m_VertexFormat.m_bTangent )
    {
        iTangentOffset = iCurrentVertexOffset;
        VertexElement.Offset = (WORD)iCurrentVertexOffset;
        VertexElement.Usage = D3DDECLUSAGE_TANGENT;
        VertexElement.Type = (BYTE)dwNormalType;
        m_VertexElements.push_back( VertexElement );
        iCurrentVertexOffset += GetElementSizeFromDeclType( VertexElement.Type );
    }
    if( m_VertexFormat.m_bBinormal )
    {
        iBinormalOffset = iCurrentVertexOffset;
        VertexElement.Offset = (WORD)iCurrentVertexOffset;
        VertexElement.Usage = D3DDECLUSAGE_BINORMAL;
        VertexElement.Type = (BYTE)dwNormalType;
        m_VertexElements.push_back( VertexElement );
        iCurrentVertexOffset += GetElementSizeFromDeclType( VertexElement.Type );
    }

    // save vertex size
    uVertexSize = iCurrentVertexOffset;
    if( uVertexSize == 0 )
        return;

    // create vertex buffer and allocate storage
    m_pVB = new ExportVB();
    m_pVB->SetVertexCount( (UINT)VertexArray.size() );
    m_pVB->SetVertexSize( (UINT)uVertexSize );
    m_pVB->Allocate();

    // copy raw vertex data into the packed vertex buffer
    for( UINT i = 0; i < VertexArray.size(); i++ )
    {
        BYTE* pDestVertex = m_pVB->GetVertex( i );
        ExportMeshVertex* pSrcVertex = VertexArray[i];
        if( pSrcVertex == NULL )
        {
            continue;
        }

        if( iPositionOffset != -1 )
        {
            D3DXVECTOR3* pDest = (D3DXVECTOR3*)( pDestVertex + iPositionOffset );
            g_pScene->GetDCCTransformer()->TransformPosition( pDest, &pSrcVertex->Position );
        }
        if( iNormalOffset != -1 )
        {
            TransformAndWriteVector( pDestVertex + iNormalOffset, pSrcVertex->Normal, dwNormalType );
        }
        if( iSkinDataOffset != -1 )
        {
            BYTE* pDest = pDestVertex + iSkinDataOffset;
            BYTE* pBoneWeights = pDest;
            *pDest++ = (BYTE)( pSrcVertex->BoneWeights.x * 255.0f );
            *pDest++ = (BYTE)( pSrcVertex->BoneWeights.y * 255.0f );
            *pDest++ = (BYTE)( pSrcVertex->BoneWeights.z * 255.0f );
            *pDest++ = (BYTE)( pSrcVertex->BoneWeights.w * 255.0f );
            NormalizeBoneWeights( pBoneWeights );
            *pDest++ = pSrcVertex->BoneIndices.x;
            *pDest++ = pSrcVertex->BoneIndices.y;
            *pDest++ = pSrcVertex->BoneIndices.z;
            *pDest++ = pSrcVertex->BoneIndices.w;
        }
        if( iTangentOffset != -1 )
        {
            TransformAndWriteVector( pDestVertex + iTangentOffset, pSrcVertex->Tangent, dwNormalType );
        }
        if( iBinormalOffset != -1 )
        {
            TransformAndWriteVector( pDestVertex + iBinormalOffset, pSrcVertex->Binormal, dwNormalType );
        }
        if( iColorOffset != -1 )
        {
            UINT uColor = 0;
            uColor |= ( (BYTE)( pSrcVertex->Color.w * 255.0f ) ) << 24;
            uColor |= ( (BYTE)( pSrcVertex->Color.x * 255.0f ) ) << 16;
            uColor |= ( (BYTE)( pSrcVertex->Color.y * 255.0f ) ) << 8;
            uColor |= ( (BYTE)( pSrcVertex->Color.z * 255.0f ) );
            memcpy( pDestVertex + iColorOffset, &uColor, 4 );
        }
        if( iUVOffset != -1 )
        {
            if( bCompressVertexData )
            {
                DWORD* pDest = (DWORD*)( pDestVertex + iUVOffset ); 
                for( DWORD t = 0; t < m_VertexFormat.m_uUVSetCount; t++ )
                {
                    switch( m_VertexFormat.m_uUVSetSize )
                    {
                    case 1:
                        {
                            memcpy( pDest, &pSrcVertex->TexCoords[t], 4 );
                            pDest++;
                            break;
                        }
                    case 2:
                        {
                            D3DXFLOAT16* pFloat16 = (D3DXFLOAT16*)pDest;
                            D3DXFloat32To16Array( pFloat16, (FLOAT*)&pSrcVertex->TexCoords[t], 2 );
                            pDest++;
                            break;
                        }
                    case 3:
                        {
                            pDest[1] = 0;
                            D3DXFLOAT16* pFloat16 = (D3DXFLOAT16*)pDest;
                            D3DXFloat32To16Array( pFloat16, (FLOAT*)&pSrcVertex->TexCoords[t], 3 );
                            pDest += 2;
                            break;
                        }
                    case 4:
                        {
                            D3DXFLOAT16* pFloat16 = (D3DXFLOAT16*)pDest;
                            D3DXFloat32To16Array( pFloat16, (FLOAT*)&pSrcVertex->TexCoords[t], 4 );
                            pDest += 2;
                            break;
                        }
                    default:
                        assert( FALSE );
                        break;
                    }
                }
            }
            else
            {
                UINT uStride = m_VertexFormat.m_uUVSetSize * sizeof( FLOAT );
                BYTE* pDest = pDestVertex + iUVOffset;
                for( UINT t = 0; t < m_VertexFormat.m_uUVSetCount; t++ )
                {
                    memcpy( pDest, &pSrcVertex->TexCoords[t], uStride );
                    pDest += uStride;
                }
            }
        }
    }
}

VOID ExportMesh::ComputeBounds()
{
    if( m_pVB == NULL )
        return;

    ComputeBoundingSphereFromPoints( &m_BoundingSphere, m_pVB->GetVertexCount(), 
        (XMFLOAT3*)m_pVB->GetVertexData(), m_pVB->GetVertexSize() );

    ComputeBoundingAxisAlignedBoxFromPoints( &m_BoundingAABB,
                                             m_pVB->GetVertexCount(),
                                             (XMFLOAT3*)m_pVB->GetVertexData(),
                                             m_pVB->GetVertexSize() );

    ComputeBoundingOrientedBoxFromPoints( &m_BoundingOBB,
                                           m_pVB->GetVertexCount(),
                                           (XMFLOAT3*)m_pVB->GetVertexData(),
                                           m_pVB->GetVertexSize() );

    FLOAT fVolumeSphere = XM_PI * ( 4.0f / 3.0f ) * 
                          m_BoundingSphere.Radius * 
                          m_BoundingSphere.Radius * 
                          m_BoundingSphere.Radius;

    FLOAT fVolumeAABB = m_BoundingAABB.Extents.x * 
                        m_BoundingAABB.Extents.y * 
                        m_BoundingAABB.Extents.z * 8.0f;

    FLOAT fVolumeOBB = m_BoundingOBB.Extents.x *
                       m_BoundingOBB.Extents.y *
                       m_BoundingOBB.Extents.z * 8.0f;

    if( fVolumeAABB <= fVolumeSphere && fVolumeAABB <= fVolumeOBB )
        m_SmallestBound = AxisAlignedBoxBound;
    else if( fVolumeOBB <= fVolumeAABB && fVolumeOBB <= fVolumeSphere )
        m_SmallestBound = OrientedBoxBound;
    else
        m_SmallestBound = SphereBound;
}

BOOL ExportModel::SetSubsetBinding( ExportString SubsetName, ExportMaterial* pMaterial, BOOL bSkipValidation )
{
    assert( m_pMesh != NULL );
    if( !bSkipValidation )
    {
        BOOL bResult = FALSE;
        for( UINT i = 0; i < m_pMesh->GetSubsetCount(); i++ )
        {
            ExportIBSubset* pSubset = m_pMesh->GetSubset( i );
            if( pSubset->GetName() == SubsetName )
                bResult = TRUE;
        }
        if( !bResult )
            return FALSE;
    }
    for( UINT i = 0; i < (UINT)m_vBindings.size(); i++ )
    {
        ExportMaterialSubsetBinding* pBinding = m_vBindings[i];
        if( pBinding->SubsetName == SubsetName )
        {
            pBinding->pMaterial = pMaterial;
            return TRUE;
        }
    }
    ExportMaterialSubsetBinding* pBinding = new ExportMaterialSubsetBinding();
    pBinding->SubsetName = SubsetName;
    pBinding->pMaterial = pMaterial;
    m_vBindings.push_back( pBinding );
    return TRUE;
}

ExportModel::~ExportModel()
{
    for( UINT i = 0; i < m_vBindings.size(); i++ )
    {
        delete m_vBindings[i];
    }
    m_vBindings.clear();
    m_pMesh = NULL;
}

};
