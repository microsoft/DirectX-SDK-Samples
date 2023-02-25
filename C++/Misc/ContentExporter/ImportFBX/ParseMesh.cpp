//-------------------------------------------------------------------------------------
//  ParseMesh.cpp
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "StdAfx.h"
#include "ParseMesh.h"
#include "ParseMaterial.h"
#include "ParseMisc.h"

extern ATG::ExportScene* g_pScene;

class SkinData
{
public:
    vector<KFbxNode*> InfluenceNodes;
    DWORD dwVertexCount;
    DWORD dwVertexStride;
    BYTE* pBoneIndices;
    FLOAT* pBoneWeights;

    SkinData()
        : pBoneWeights( NULL ),
          pBoneIndices( NULL ),
          dwVertexCount( 0 ),
          dwVertexStride( 0 )
    {
    }

    ~SkinData()
    {
        if( pBoneIndices != NULL ) delete[] pBoneIndices;
        if( pBoneWeights != NULL ) delete[] pBoneWeights;
    }

    VOID Alloc( DWORD dwCount, DWORD dwStride )
    {
        dwVertexCount = dwCount;
        dwVertexStride = dwStride;

        DWORD dwBufferSize = dwVertexCount * dwVertexStride;
        pBoneIndices = new BYTE[ dwBufferSize ];
        ZeroMemory( pBoneIndices, sizeof(BYTE) * dwBufferSize );
        pBoneWeights = new FLOAT[ dwBufferSize ];
        ZeroMemory( pBoneWeights, sizeof(FLOAT) * dwBufferSize );
    }


    BYTE* GetIndices( DWORD dwIndex ) 
    { 
        assert( dwIndex < dwVertexCount ); 
        return pBoneIndices + ( dwIndex * dwVertexStride ); 
    }
    FLOAT* GetWeights( DWORD dwIndex ) 
    { 
        assert( dwIndex < dwVertexCount ); 
        return pBoneWeights + ( dwIndex * dwVertexStride ); 
    }

    DWORD GetBoneCount() const { return (DWORD)InfluenceNodes.size(); }

    VOID InsertWeight( DWORD dwIndex, DWORD dwBoneIndex, FLOAT fBoneWeight )
    {
        assert( dwBoneIndex < 256 );

        BYTE* pIndices = GetIndices( dwIndex );
        FLOAT* pWeights = GetWeights( dwIndex );

        for( DWORD i = 0; i < dwVertexStride; ++i )
        {
            if( fBoneWeight > pWeights[i] )
            {
                for( DWORD j = (dwVertexStride - 1); j > i; --j )
                {
                    pIndices[j] = pIndices[j - 1];
                    pWeights[j] = pWeights[j - 1];
                }
                pIndices[i] = (BYTE)dwBoneIndex;
                pWeights[i] = fBoneWeight;
                break;
            }
        }
    }
};

VOID CaptureBindPoseMatrix( KFbxNode* pNode, const KFbxMatrix& matBindPose )
{
    PoseMap::iterator iter = g_BindPoseMap.find( pNode );
    if( iter != g_BindPoseMap.end() )
    {
        KFbxMatrix matExisting = iter->second;
        if( matExisting != matBindPose )
        {
            // found the bind pose matrix, but it is different than what we prevoiusly encountered
            g_BindPoseMap[pNode] = matBindPose;
            g_bBindPoseFixupRequired = TRUE;
            ExportLog::LogMsg( 4, "Updating bind pose matrix for frame \"%s\"", pNode->GetName() );
        }
    }
    else
    {
        // have not encountered this frame in the bind pose yet
        g_BindPoseMap[pNode] = matBindPose;
        g_bBindPoseFixupRequired = TRUE;
        ExportLog::LogMsg( 4, "Adding bind pose matrix for frame \"%s\"", pNode->GetName() );
    }
}

BOOL ParseMeshSkinning( KFbxMesh* pMesh, SkinData* pSkinData )
{
    DWORD dwDeformerCount = pMesh->GetDeformerCount( KFbxDeformer::eSKIN );
    if( dwDeformerCount == 0 )
        return FALSE;

    ExportLog::LogMsg( 4, "Parsing skin weights on mesh %s", pMesh->GetName() );

    const DWORD dwVertexCount = pMesh->GetControlPointsCount();
    const DWORD dwStride = 4;
    pSkinData->Alloc( dwVertexCount, dwStride );

    for( DWORD dwDeformerIndex = 0; dwDeformerIndex < dwDeformerCount; ++dwDeformerIndex )
    {
        KFbxSkin* pSkin = (KFbxSkin*)pMesh->GetDeformer( dwDeformerIndex, KFbxDeformer::eSKIN );
        DWORD dwClusterCount = pSkin->GetClusterCount();

        for( DWORD dwClusterIndex = 0; dwClusterIndex < dwClusterCount; ++dwClusterIndex )
        {
            KFbxCluster* pCluster = pSkin->GetCluster( dwClusterIndex );
            DWORD dwClusterSize = pCluster->GetControlPointIndicesCount();
            if( dwClusterSize == 0 )
                continue;

            KFbxNode* pLink = pCluster->GetLink();

            DWORD dwBoneIndex = pSkinData->GetBoneCount();
            pSkinData->InfluenceNodes.push_back( pLink );
            ExportLog::LogMsg( 4, "Influence %d: %s", dwBoneIndex, pLink->GetName() );
            
            KFbxXMatrix matXBindPose;
            pCluster->GetTransformLinkMatrix( matXBindPose );
            KFbxMatrix matBindPose = matXBindPose;
            CaptureBindPoseMatrix( pLink, matBindPose );

            INT* pIndices = pCluster->GetControlPointIndices();
            DOUBLE* pWeights = pCluster->GetControlPointWeights();

            for( DWORD i = 0; i < dwClusterSize; ++i )
            {
                pSkinData->InsertWeight( pIndices[i], dwBoneIndex, (FLOAT)pWeights[i] );
            }
        }
    }

    return TRUE;
}

VOID ParseMesh( KFbxMesh* pFbxMesh, ExportFrame* pParentFrame, BOOL bSubDProcess, const CHAR* strSuffix )
{
    if( !g_pScene->Settings().bExportMeshes )
        return;

    if( pFbxMesh == NULL )
        return;

	const CHAR* strName = pFbxMesh->GetName();
	if( strName == NULL || strName[0] == '\0' )
		strName = pParentFrame->GetName().SafeString();

    if( strSuffix == NULL )
    {
        strSuffix = "";
    }
	CHAR strDecoratedName[512];
	sprintf_s( strDecoratedName, "%s_%s%s", g_pScene->Settings().strMeshNameDecoration, strName, strSuffix );
	ExportMesh* pMesh = new ExportMesh( strDecoratedName );
	pMesh->SetDCCObject( pFbxMesh );

    BOOL bSmoothMesh = FALSE;

    KFbxMesh::MeshSmoothness Smoothness = pFbxMesh->GetMeshSmoothness();
    if( Smoothness != KFbxMesh::HULL && g_pScene->Settings().bConvertMeshesToSubD )
    {
        bSubDProcess = TRUE;
        bSmoothMesh = TRUE;
    }

    ExportLog::LogMsg( 2, "Parsing %s mesh \"%s\", renamed to \"%s\"", bSmoothMesh ? "smooth" : "poly", strName, strDecoratedName );

    SkinData skindata;
    BOOL bSkinnedMesh = ParseMeshSkinning( pFbxMesh, &skindata );
    if( bSkinnedMesh )
    {
        DWORD dwBoneCount = skindata.GetBoneCount();
        for( DWORD i = 0; i < dwBoneCount; ++i )
        {
            pMesh->AddInfluence( skindata.InfluenceNodes[i]->GetName() );
        }
    }

    pMesh->SetVertexColorCount( 0 );

    KFbxLayerElementArrayTemplate<KFbxVector4> *pNormals = NULL;
    pFbxMesh->GetNormals( &pNormals );
    if( pNormals == NULL )
    {
        pFbxMesh->InitNormals();
        pFbxMesh->ComputeVertexNormals();
    }
    // Vertex normals and tangent spaces
    if( !g_pScene->Settings().bExportNormals )
    {
        pMesh->SetVertexNormalCount( 0 );
    }
    else if( g_pScene->Settings().bComputeVertexTangentSpace )
    {
        if( g_pScene->Settings().bExportBinormal )
            pMesh->SetVertexNormalCount( 3 );
        else
            pMesh->SetVertexNormalCount( 2 );
    }
    else
    {
        pMesh->SetVertexNormalCount( 1 );
    }

    DWORD dwLayerCount = pFbxMesh->GetLayerCount();
    ExportLog::LogMsg( 4, "%d layers in FBX mesh", dwLayerCount );

    DWORD dwVertexColorCount = 0;
    KFbxLayerElementVertexColor* pVertexColorSet = NULL;
    DWORD dwUVSetCount = 0;
    std::vector<KFbxLayerElementUV*> VertexUVSets;
    KFbxLayerElementMaterial* pMaterialSet = NULL;
    std::vector<ExportMaterial*> MaterialList;
    for( DWORD dwLayerIndex = 0; dwLayerIndex < dwLayerCount; ++dwLayerIndex )
    {
        if( pFbxMesh->GetLayer(dwLayerIndex)->GetVertexColors() != NULL )
        {
            if( dwVertexColorCount == 0 )
            {
                dwVertexColorCount++;
                pVertexColorSet = pFbxMesh->GetLayer(dwLayerIndex)->GetVertexColors();
            }
            else
            {
                ExportLog::LogWarning( "Only one vertex color set is allowed; ignoring additional vertex color sets." );
            }
        }
        if( pFbxMesh->GetLayer(dwLayerIndex)->GetUVs() != NULL )
        {
            dwUVSetCount++;
            VertexUVSets.push_back( pFbxMesh->GetLayer(dwLayerIndex)->GetUVs() );
        }
        if( pFbxMesh->GetLayer(dwLayerIndex)->GetMaterials() != NULL )
        {
            if( pMaterialSet != NULL )
            {
                ExportLog::LogWarning( "Multiple material layers detected on mesh %s.  Some will be ignored.", pMesh->GetName().SafeString() );
            }
            pMaterialSet = pFbxMesh->GetLayer(dwLayerIndex)->GetMaterials();
            DWORD dwMaterialCount = pMaterialSet->GetDirectArray().GetCount();
            for( DWORD i = 0; i < dwMaterialCount; ++i )
            {
                ExportMaterial* pMaterial = ParseMaterialInLayer( pFbxMesh, pFbxMesh->GetLayer( dwLayerIndex ), i );
                MaterialList.push_back( pMaterial );
            }
        }
    }

    ExportLog::LogMsg( 4, "Found %d UV sets", dwUVSetCount );
    dwUVSetCount = min( dwUVSetCount, (DWORD)g_pScene->Settings().iMaxUVSetCount );
    ExportLog::LogMsg( 4, "Using %d UV sets", dwUVSetCount );

    pMesh->SetVertexColorCount( dwVertexColorCount );
    pMesh->SetVertexUVCount( dwUVSetCount );
    // TODO: Does FBX only support 2D texture coordinates?
    pMesh->SetVertexUVDimension( 2 );

    DWORD dwMeshOptimizationFlags = 0;
    if( g_pScene->Settings().bCompressVertexData )
        dwMeshOptimizationFlags |= ExportMesh::COMPRESS_VERTEX_DATA;

    DWORD dwPolyCount = pFbxMesh->GetPolygonCount();
    // Assume that polys are usually quads.
    g_MeshTriangleAllocator.SetSizeHint( dwPolyCount * 2 );

    DWORD dwVertexCount = pFbxMesh->GetControlPointsCount();
    KFbxVector4* pVertexPositions = pFbxMesh->GetControlPoints();

    if( bSkinnedMesh )
    {
        assert( skindata.dwVertexCount == dwVertexCount );
    }
    
    ExportLog::LogMsg( 4, "%d vertices, %d polygons", dwVertexCount, dwPolyCount );

    DWORD dwNonConformingSubDPolys = 0;

    // Loop over polygons.
    for( DWORD dwPolyIndex = 0; dwPolyIndex < dwPolyCount; ++dwPolyIndex )
    {
        // Triangulate each polygon into one or more triangles.
        DWORD dwPolySize = pFbxMesh->GetPolygonSize( dwPolyIndex );
        assert( dwPolySize >= 3 );
        DWORD dwTriangleCount = dwPolySize - 2;
        assert( dwTriangleCount > 0 );

        if( dwPolySize > 4 )
        {
            ++dwNonConformingSubDPolys;
        }

        DWORD dwMaterialIndex = 0;
        if( pMaterialSet != NULL )
        {
            switch( pMaterialSet->GetMappingMode() )
            {
            case KFbxLayerElement::eBY_POLYGON:
                switch( pMaterialSet->GetReferenceMode() )
                {
                case KFbxLayerElement::eDIRECT:
                    dwMaterialIndex = dwPolyIndex;
                    break;
                case KFbxLayerElement::eINDEX:
                case KFbxLayerElement::eINDEX_TO_DIRECT:
                    dwMaterialIndex = pMaterialSet->GetIndexArray().GetAt( dwPolyIndex );
                    break;
                }
            case KFbxLayerElement::eALL_SAME:
                break;
            }
        }

        const BOOL bInvertTexVCoord = g_pScene->Settings().bInvertTexVCoord;

        DWORD dwCornerIndices[3];
        // Loop over triangles in the polygon.
        for( DWORD dwTriangleIndex = 0; dwTriangleIndex < dwTriangleCount; ++dwTriangleIndex )
        {
            dwCornerIndices[0] = pFbxMesh->GetPolygonVertex( dwPolyIndex, 0 );
            dwCornerIndices[1] = pFbxMesh->GetPolygonVertex( dwPolyIndex, dwTriangleIndex + 1 );
            dwCornerIndices[2] = pFbxMesh->GetPolygonVertex( dwPolyIndex, dwTriangleIndex + 2 );

            //ExportLog::LogMsg( 4, "Poly %d Triangle %d: %d %d %d", dwPolyIndex, dwTriangleIndex, dwCornerIndices[0], dwCornerIndices[1], dwCornerIndices[2] );

            KFbxVector4 vNormals[3];
            ZeroMemory( vNormals, 3 * sizeof(KFbxVector4) );
            INT iPolyIndex = (INT)dwPolyIndex;
            INT iVertIndex[3] = { 0, (INT)dwTriangleIndex + 1, (INT)dwTriangleIndex + 2 };
            //INT iVertIndexUV[3] = { (INT)dwTriangleIndex, (INT)dwTriangleIndex + 1, (INT)dwTriangleIndex + 2 };
            pFbxMesh->GetPolygonVertexNormal( iPolyIndex, iVertIndex[0], vNormals[0] );
            pFbxMesh->GetPolygonVertexNormal( iPolyIndex, iVertIndex[1], vNormals[1] );
            pFbxMesh->GetPolygonVertexNormal( iPolyIndex, iVertIndex[2], vNormals[2] );

            // Build the raw triangle.
            ExportMeshTriangle* pTriangle = g_MeshTriangleAllocator.GetNewTriangle();

            // Store polygon index
            pTriangle->PolygonIndex = (INT)dwPolyIndex;

            // Store material subset index
            pTriangle->SubsetIndex = dwMaterialIndex;

            for( DWORD dwCornerIndex = 0; dwCornerIndex < 3; ++dwCornerIndex )
            {
                const DWORD& dwDCCIndex = dwCornerIndices[dwCornerIndex];
                // Store DCC vertex index (this helps the mesh reduction/VB generation code)
                pTriangle->Vertex[dwCornerIndex].DCCVertexIndex = dwDCCIndex;

                // Store vertex position
                pTriangle->Vertex[dwCornerIndex].Position.x = (FLOAT)pVertexPositions[dwDCCIndex].mData[0];
                pTriangle->Vertex[dwCornerIndex].Position.y = (FLOAT)pVertexPositions[dwDCCIndex].mData[1];
                pTriangle->Vertex[dwCornerIndex].Position.z = (FLOAT)pVertexPositions[dwDCCIndex].mData[2];

                // Store vertex normal
                pTriangle->Vertex[dwCornerIndex].Normal.x = (FLOAT)vNormals[dwCornerIndex].mData[0];
                pTriangle->Vertex[dwCornerIndex].Normal.y = (FLOAT)vNormals[dwCornerIndex].mData[1];
                pTriangle->Vertex[dwCornerIndex].Normal.z = (FLOAT)vNormals[dwCornerIndex].mData[2];

                // Store UV sets
                for( DWORD dwUVIndex = 0; dwUVIndex < dwUVSetCount; ++dwUVIndex )
                {
                    // Crack apart the FBX dereferencing system for UV coordinates
                    KFbxLayerElementUV* pUVSet = VertexUVSets[dwUVIndex];
                    KFbxVector2 Value( 0, 0 );
                    INT iUVIndex = 0;
                    switch( pUVSet->GetMappingMode() )
                    {
                    case KFbxLayerElement::eBY_CONTROL_POINT:
                        iUVIndex = pFbxMesh->GetPolygonVertex( iPolyIndex, iVertIndex[dwCornerIndex] );
                        break;
                    case KFbxLayerElement::eBY_POLYGON_VERTEX:
                        iUVIndex = pFbxMesh->GetTextureUVIndex( iPolyIndex, iVertIndex[dwCornerIndex] );
                        break;
                    }
                    Value = pUVSet->GetDirectArray().GetAt( iUVIndex );

                    // Store a single UV set
                    pTriangle->Vertex[dwCornerIndex].TexCoords[dwUVIndex].x = (FLOAT)Value.mData[0];
                    if( bInvertTexVCoord )
                    {
                        pTriangle->Vertex[dwCornerIndex].TexCoords[dwUVIndex].y = 1.0f - (FLOAT)Value.mData[1];
                    }
                    else
                    {
                        pTriangle->Vertex[dwCornerIndex].TexCoords[dwUVIndex].y = (FLOAT)Value.mData[1];
                    }
                }

                // Store vertex color set
                if( dwVertexColorCount > 0 && pVertexColorSet != NULL )
                {
                    // Crack apart the FBX dereferencing system for Color coordinates
                    KFbxColor Value( 0, 0, 0, 1 );
                    switch( pVertexColorSet->GetMappingMode() )
                    {
                    case KFbxLayerElement::eBY_CONTROL_POINT:
                        switch( pVertexColorSet->GetReferenceMode() )
                        {
                        case KFbxLayerElement::eDIRECT:
                            Value = pVertexColorSet->GetDirectArray().GetAt( dwDCCIndex );
                            break;
                        case KFbxLayerElement::eINDEX_TO_DIRECT:
                            Value = pVertexColorSet->GetDirectArray().GetAt( pVertexColorSet->GetIndexArray().GetAt( dwDCCIndex ) );
                            break;
                        }
                        break;
                    case KFbxLayerElement::eBY_POLYGON_VERTEX:
                        switch( pVertexColorSet->GetReferenceMode() )
                        {
                        case KFbxLayerElement::eDIRECT:
                            Value = pVertexColorSet->GetDirectArray().GetAt( iVertIndex[dwCornerIndex] );
                            break;
                        case KFbxLayerElement::eINDEX_TO_DIRECT:
                            Value = pVertexColorSet->GetDirectArray().GetAt( pVertexColorSet->GetIndexArray().GetAt( iVertIndex[dwCornerIndex] ) );
                            break;
                        }
                        break;
                    }

                    // Store a single vertex color set
                    pTriangle->Vertex[dwCornerIndex].Color.x = (FLOAT)Value.mRed;
                    pTriangle->Vertex[dwCornerIndex].Color.y = (FLOAT)Value.mGreen;
                    pTriangle->Vertex[dwCornerIndex].Color.z = (FLOAT)Value.mBlue;
                    pTriangle->Vertex[dwCornerIndex].Color.w = (FLOAT)Value.mAlpha;
                }

                // Store skin weights
                if( bSkinnedMesh )
                {
                    memcpy( &pTriangle->Vertex[dwCornerIndex].BoneIndices, skindata.GetIndices( dwDCCIndex ), sizeof(ByteVector4) );
                    memcpy( &pTriangle->Vertex[dwCornerIndex].BoneWeights, skindata.GetWeights( dwDCCIndex ), sizeof(D3DXVECTOR4) );
                }
            }

            // Add raw triangle to the mesh.
            pMesh->AddRawTriangle( pTriangle );
        }
    }

    if( bSubDProcess )
    {
        dwMeshOptimizationFlags |= ExportMesh::FORCE_SUBD_CONVERSION;
    }

    pMesh->Optimize( dwMeshOptimizationFlags );

    ExportModel* pModel = new ExportModel( pMesh );
    DWORD dwMaterialCount = (DWORD)MaterialList.size();
    if( pMesh->GetSubDMesh() == NULL )
    {
        for( DWORD dwSubset = 0; dwSubset < dwMaterialCount; ++dwSubset )
        {
            ExportMaterial* pMaterial = MaterialList[dwSubset];
            ExportIBSubset* pSubset = pMesh->GetSubset( dwSubset );
            CHAR strUniqueSubsetName[100];
            sprintf_s( strUniqueSubsetName, "subset%d_%s", dwSubset, pMaterial->GetName().SafeString() );
            pSubset->SetName( strUniqueSubsetName );
            pModel->SetSubsetBinding( pSubset->GetName(), pMaterial );
        }
    }
    else
    {
        ExportSubDProcessMesh* pSubDMesh = pMesh->GetSubDMesh();
        DWORD dwSubsetCount = pSubDMesh->GetSubsetCount();
        for( DWORD dwSubset = 0; dwSubset < dwSubsetCount; ++dwSubset )
        {
            ExportSubDPatchSubset* pSubset = pSubDMesh->GetSubset( dwSubset );
            assert( pSubset != NULL );
            assert( pSubset->iOriginalMeshSubset < (INT)dwMaterialCount );
            ExportMaterial* pMaterial = MaterialList[pSubset->iOriginalMeshSubset];
            CHAR strUniqueSubsetName[100];
            sprintf_s( strUniqueSubsetName, "subset%d_%s", dwSubset, pMaterial->GetName().SafeString() );
            pSubset->Name = strUniqueSubsetName;
            pModel->SetSubsetBinding( pSubset->Name, pMaterial, TRUE );
        }
    }

    if( bSubDProcess && ( dwNonConformingSubDPolys > 0 ) )
    {
        ExportLog::LogWarning( "Encountered %d polygons with 5 or more sides in mesh \"%s\", which were subdivided into quad and triangle patches.  Mesh appearance may have been affected.", dwNonConformingSubDPolys, pMesh->GetName().SafeString() );
    }

    // update statistics
    if( pMesh->GetSubDMesh() != NULL )
    {
        g_pScene->Statistics().SubDMeshesProcessed++;
        g_pScene->Statistics().SubDQuadsProcessed += pMesh->GetSubDMesh()->GetQuadPatchCount();
        g_pScene->Statistics().SubDTrisProcessed += pMesh->GetSubDMesh()->GetTrianglePatchCount();
    }
    else
    {
        g_pScene->Statistics().TrisExported += pMesh->GetIB()->GetIndexCount() / 3;
        g_pScene->Statistics().VertsExported += pMesh->GetVB()->GetVertexCount();
        g_pScene->Statistics().MeshesExported++;
    }

    pParentFrame->AddModel( pModel );
    g_pScene->AddMesh( pMesh );
}

VOID ParseSubDiv( KFbxSubdiv* pFbxSubD, ExportFrame* pParentFrame )
{
    if( !g_pScene->Settings().bExportMeshes )
        return;

    if( pFbxSubD == NULL )
    {
        return;
    }

    const CHAR* strName = pFbxSubD->GetName();
    if( strName == NULL || strName[0] == '\0' )
        strName = pParentFrame->GetName().SafeString();

    DWORD dwLevelCount = (DWORD)pFbxSubD->GetLevelCount();
    ExportLog::LogMsg( 2, "Parsing subdivision surface \"%s\" with %d levels", strName, dwLevelCount );
    if( dwLevelCount == 0 )
    {
        ExportLog::LogWarning( "Subdivision surface \"%s\" has no levels.", strName );
        return;
    }

    KFbxMesh* pLevelMesh = NULL;
    DWORD dwCurrentLevel = dwLevelCount - 1;
    while( pLevelMesh == NULL && dwCurrentLevel > 0 )
    {
        pLevelMesh = pFbxSubD->GetMesh( dwCurrentLevel );
        if( pLevelMesh == NULL )
        {
            --dwCurrentLevel;
        }
    }
    if( pLevelMesh == NULL )
    {
        pLevelMesh = pFbxSubD->GetBaseMesh();
    }

    assert( pLevelMesh != NULL );

    ExportLog::LogMsg( 3, "Parsing level %d", dwCurrentLevel );
    CHAR strSuffix[32];
    sprintf_s( strSuffix, "_level%d", dwCurrentLevel );
    ParseMesh( pLevelMesh, pParentFrame, TRUE, strSuffix );
}