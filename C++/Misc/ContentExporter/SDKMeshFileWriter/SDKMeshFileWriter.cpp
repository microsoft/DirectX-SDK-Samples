//-------------------------------------------------------------------------------------
//  SDKMeshFileWriter.cpp
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "SDKmesh.h"

extern ATG::ExportScene*     g_pScene;

namespace ATG
{
    std::vector<const ExportFrame*>                 g_FrameArray;
    std::vector<SDKMESH_FRAME>                      g_FrameHeaderArray;
    std::vector<const ExportModel*>                 g_ModelArray;
    std::vector<SDKMESH_MESH>                       g_MeshHeaderArray;
    std::vector<const ExportMeshBase*>              g_ModelMeshArray;
    std::vector<const ExportVB*>                    g_VBArray;
    std::vector<SDKMESH_VERTEX_BUFFER_HEADER>       g_VBHeaderArray;
    std::vector<const ExportIB*>                    g_IBArray;
    std::vector<SDKMESH_INDEX_BUFFER_HEADER>        g_IBHeaderArray;
    std::vector<SDKMESH_SUBSET>                     g_SubsetArray;
    std::vector<UINT>                               g_SubsetIndexArray;
    std::vector<UINT>                               g_FrameInfluenceArray;
    std::vector<SDKMESH_MATERIAL>                   g_MaterialArray;

    typedef stdext::hash_map<ExportMaterial*,DWORD> MaterialLookupMap;
    MaterialLookupMap                               g_ExportMaterialToSDKMeshMaterialMap;

    BYTE g_Padding4K[4096] = { 0 };

    BOOL WriteSDKMeshAnimationFile( const CHAR* strFileName, ExportManifest* pManifest );

    VOID ClearSceneArrays()
    {
        g_FrameArray.clear();
        g_FrameHeaderArray.clear();
        g_ModelArray.clear();
        g_MeshHeaderArray.clear();
        g_ModelMeshArray.clear();
        g_VBArray.clear();
        g_VBHeaderArray.clear();
        g_IBArray.clear();
        g_IBHeaderArray.clear();
        g_SubsetArray.clear();
        g_SubsetIndexArray.clear();
        g_FrameInfluenceArray.clear();
        g_MaterialArray.clear();
        g_ExportMaterialToSDKMeshMaterialMap.clear();
    }

	VOID ProcessTexture( CHAR* strDest, const DWORD dwDestLength, const CHAR* strSrc )
	{
        DWORD dwLength = (DWORD)strlen( strSrc ) + 1;
        if( dwLength > dwDestLength )
        {
            ExportLog::LogWarning( "Truncating texture path \"%s\".", strSrc );
            strncpy_s( strDest, dwDestLength, strSrc, dwDestLength - 1 );
            strDest[dwDestLength - 1] = '\0';
        }
        else
        {
            strcpy_s( strDest, dwDestLength, strSrc );
        }
	}

    DWORD CaptureMaterial( ExportMaterial* pMaterial )
    {
        MaterialLookupMap::iterator iter = g_ExportMaterialToSDKMeshMaterialMap.find( pMaterial );
        if( iter != g_ExportMaterialToSDKMeshMaterialMap.end() )
        {
            DWORD dwIndex = iter->second;
            return dwIndex;
        }
		SDKMESH_MATERIAL Material;
        ZeroMemory( &Material, sizeof(SDKMESH_MATERIAL) );
		strcpy_s( Material.Name, pMaterial->GetName() );
        ExportMaterialParameter* pDiffuse = pMaterial->FindParameter( "DiffuseTexture" );
        if( pDiffuse != NULL )
        {
			ProcessTexture( Material.DiffuseTexture, MAX_MATERIAL_NAME, pDiffuse->ValueString.SafeString() );
        }
        ExportMaterialParameter* pNormal = pMaterial->FindParameter( "NormalMapTexture" );
        if( pNormal != NULL )
        {
			ProcessTexture( Material.NormalTexture, MAX_MATERIAL_NAME, pNormal->ValueString.SafeString() );
        }
        ExportMaterialParameter* pSpecular = pMaterial->FindParameter( "SpecularMapTexture" );
        if( pSpecular != NULL )
        {
			ProcessTexture( Material.SpecularTexture, MAX_MATERIAL_NAME, pSpecular->ValueString.SafeString() );
        }
        DWORD dwIndex = (DWORD)g_MaterialArray.size();
        g_MaterialArray.push_back( Material );
        g_ExportMaterialToSDKMeshMaterialMap[pMaterial] = dwIndex;
        return dwIndex;
    }

    VOID CaptureVertexBuffer( const ExportVB* pVB, const D3DVERTEXELEMENT9* pElements, DWORD dwElementCount )
    {
        SDKMESH_VERTEX_BUFFER_HEADER VBHeader;
        ZeroMemory( &VBHeader, sizeof( SDKMESH_VERTEX_BUFFER_HEADER ) );
        VBHeader.DataOffset = 0;
        VBHeader.SizeBytes = pVB->GetVertexDataSize();
        VBHeader.StrideBytes = pVB->GetVertexSize();
        VBHeader.NumVertices = pVB->GetVertexCount();
        memcpy( VBHeader.Decl, pElements, dwElementCount * sizeof( D3DVERTEXELEMENT9 ) );
        static D3DVERTEXELEMENT9 EndElement = D3DDECL_END();
        VBHeader.Decl[ dwElementCount ] = EndElement;
        g_VBArray.push_back( pVB );
        g_VBHeaderArray.push_back( VBHeader );
    }

    VOID CaptureIndexBuffer( const ExportIB* pIB )
    {
        SDKMESH_INDEX_BUFFER_HEADER IBHeader;
        ZeroMemory( &IBHeader, sizeof( SDKMESH_INDEX_BUFFER_HEADER ) );
        IBHeader.DataOffset = 0;
        IBHeader.IndexType = pIB->GetIndexSize() == 2 ? IT_16BIT : IT_32BIT;
        IBHeader.NumIndices = pIB->GetIndexCount();
        IBHeader.SizeBytes = (UINT64)pIB->GetIndexDataSize();
        g_IBArray.push_back( pIB );
        g_IBHeaderArray.push_back( IBHeader );
    }

    VOID CapturePolyMesh( ExportMesh* pMesh )
    {
        ExportSubDProcessMesh* pSubDMesh = pMesh->GetSubDMesh();
        if( pSubDMesh != NULL )
        {
            CaptureIndexBuffer( pSubDMesh->GetQuadPatchIB() );
            CaptureVertexBuffer( pMesh->GetVB(), &pMesh->GetVertexDeclElement( 0 ), pMesh->GetVertexDeclElementCount() );
            CaptureVertexBuffer( pSubDMesh->GetQuadPatchDataVB(), pSubDMesh->GetPatchDataDecl(), pSubDMesh->GetPatchDataDeclElementCount() );
            if( pSubDMesh->GetTrianglePatchDataVB() != NULL )
            {
                ExportLog::LogWarning( "Subdivision surface mesh \"%s\" contains triangle patches, which are not currently written to SDKMESH files.", pMesh->GetName().SafeString() );
            }
        }
        else
        {
            CaptureIndexBuffer( pMesh->GetIB() );
            CaptureVertexBuffer( pMesh->GetVB(), &pMesh->GetVertexDeclElement( 0 ), pMesh->GetVertexDeclElementCount() );
        }
    }

    VOID CaptureSubset( ExportMeshBase* pMeshBase, ExportMaterialSubsetBinding* pBinding, DWORD dwMaxVertexCount )
    {
        ExportIBSubset* pIBSubset = pMeshBase->FindSubset( pBinding->SubsetName );
        assert( pIBSubset != NULL );
        DWORD dwMaterialIndex = CaptureMaterial( pBinding->pMaterial );
        SDKMESH_SUBSET Subset;
        Subset.IndexStart = pIBSubset->GetStartIndex();
        Subset.IndexCount = pIBSubset->GetIndexCount();
        Subset.MaterialID = dwMaterialIndex;
        Subset.VertexStart = 0;
        Subset.VertexCount = dwMaxVertexCount;
        switch( pIBSubset->GetPrimitiveType() )
        {
        case ExportIBSubset::TriangleList:
            Subset.PrimitiveType = PT_TRIANGLE_LIST;
            break;
        case ExportIBSubset::TriangleStrip:
            Subset.PrimitiveType = PT_TRIANGLE_STRIP;
            break;
        default:
            Subset.PrimitiveType = PT_TRIANGLE_LIST;
            break;
        }
        strcpy_s( Subset.Name, pIBSubset->GetName().SafeString() );
        g_SubsetArray.push_back( Subset );
    }

    VOID CaptureSubDSubset( ExportSubDProcessMesh* pSubDMesh, ExportMaterialSubsetBinding* pBinding, DWORD dwMaxVertexCount )
    {
        DWORD dwMaterialIndex = CaptureMaterial( pBinding->pMaterial );
        ExportSubDPatchSubset* pSubset = pSubDMesh->FindSubset( pBinding->SubsetName );
        SDKMESH_SUBSET Subset;
        Subset.IndexStart = pSubset->dwStartPatch;
        Subset.IndexCount = pSubset->dwPatchCount;
        Subset.MaterialID = dwMaterialIndex;
        Subset.VertexStart = 0;
        Subset.VertexCount = dwMaxVertexCount;
        Subset.PrimitiveType = pSubset->bQuadPatches ? PT_QUAD_PATCH_LIST : PT_TRIANGLE_PATCH_LIST;
        strcpy_s( Subset.Name, pSubset->Name.SafeString() );
        g_SubsetArray.push_back( Subset );
    }

    VOID CaptureModel( ExportModel* pModel )
    {
        g_ModelArray.push_back( pModel );
        ExportMeshBase* pMeshBase = pModel->GetMesh();
        g_ModelMeshArray.push_back( pMeshBase );

        SDKMESH_MESH MeshHeader;
        ZeroMemory( &MeshHeader, sizeof( SDKMESH_MESH ) );
        strcpy_s( MeshHeader.Name, pMeshBase->GetName().SafeString() );

        switch( pMeshBase->GetSmallestBound() )
        {
        case ExportMeshBase::SphereBound:
            {
                MeshHeader.BoundingBoxCenter = *(D3DXVECTOR3*)&pMeshBase->GetBoundingSphere().Center;
                FLOAT fSize = pMeshBase->GetBoundingSphere().Radius;
                MeshHeader.BoundingBoxExtents = D3DXVECTOR3( fSize, fSize, fSize );
                break;
            }
        case ExportMeshBase::AxisAlignedBoxBound:
            MeshHeader.BoundingBoxCenter = *(D3DXVECTOR3*)&pMeshBase->GetBoundingAABB().Center;
            MeshHeader.BoundingBoxExtents = *(D3DXVECTOR3*)&pMeshBase->GetBoundingAABB().Extents;
            break;
        case ExportMeshBase::OrientedBoxBound:
            MeshHeader.BoundingBoxCenter = *(D3DXVECTOR3*)&pMeshBase->GetBoundingOBB().Center;
            MeshHeader.BoundingBoxExtents = *(D3DXVECTOR3*)&pMeshBase->GetBoundingOBB().Extents;
            break;
        }

        MeshHeader.NumFrameInfluences = pMeshBase->GetInfluenceCount();

        ExportSubDProcessMesh* pSubDMesh = NULL;
        DWORD dwMaxVertexCount = 0;
        MeshHeader.IndexBuffer = (UINT)g_IBArray.size();
        switch( pMeshBase->GetMeshType() )
        {
        case ExportMeshBase::PolyMesh:
            {
                ExportMesh* pMesh = (ExportMesh*)pMeshBase;
                pSubDMesh = pMesh->GetSubDMesh();
                if( pSubDMesh != NULL )
                {
                    MeshHeader.NumVertexBuffers = 2;
                    MeshHeader.VertexBuffers[0] = (UINT)g_VBArray.size();
                    MeshHeader.VertexBuffers[1] = (UINT)g_VBArray.size() + 1;
                }
                else
                {
                    MeshHeader.NumVertexBuffers = 1;
                    MeshHeader.VertexBuffers[0] = (UINT)g_VBArray.size();
                }
                CapturePolyMesh( pMesh );
                dwMaxVertexCount = pMesh->GetVB()->GetVertexCount();
            }
            break;
        }

        MeshHeader.NumSubsets = pModel->GetBindingCount();
        for( DWORD i = 0; i < MeshHeader.NumSubsets; ++i )
        {
            g_SubsetIndexArray.push_back( (UINT)g_SubsetArray.size() );
            if( pSubDMesh != NULL )
            {
                CaptureSubDSubset( pSubDMesh, pModel->GetBinding( i ), dwMaxVertexCount );
            }
            else
            {
                CaptureSubset( pMeshBase, pModel->GetBinding( i ), dwMaxVertexCount );
            }
        }

        g_MeshHeaderArray.push_back( MeshHeader );
    }

    VOID CaptureScene( ExportFrame* pRootFrame, DWORD dwParentIndex )
    {
        SDKMESH_FRAME Frame;
        ZeroMemory( &Frame, sizeof( SDKMESH_FRAME ) );
        strcpy_s( Frame.Name, pRootFrame->GetName().SafeString() );
        Frame.Matrix = pRootFrame->Transform().Matrix();
        Frame.ParentFrame = dwParentIndex;
        Frame.AnimationDataIndex = INVALID_ANIMATION_DATA;
        DWORD dwCurrentIndex = (DWORD)g_FrameArray.size();

        DWORD dwModelCount = pRootFrame->GetModelCount();
        if ( dwModelCount == 0 )
        {
            Frame.Mesh = INVALID_MESH;
        }
        else
        {
            // Only one mesh per frame is supported in the SDKMesh format.
            if( dwModelCount > 1 )
            {
                ExportLog::LogWarning( "Frame \"%s\" has %d meshes.  Only one mesh per frame is supported in the SDKMesh format.", pRootFrame->GetName(), dwModelCount );
            }
            Frame.Mesh = (UINT)g_MeshHeaderArray.size();
            ExportModel* pModel = pRootFrame->GetModelByIndex( 0 );
            CaptureModel( pModel );
        }

        DWORD dwChildIndex = (DWORD)-1;
        DWORD dwChildCount = pRootFrame->GetChildCount();
        if( dwChildCount > 0 )
            dwChildIndex = (DWORD)g_FrameHeaderArray.size() + 1;
        Frame.ChildFrame = (UINT)dwChildIndex;
        Frame.SiblingFrame = (DWORD)-1;

        g_FrameHeaderArray.push_back( Frame );
        g_FrameArray.push_back( pRootFrame );

        DWORD dwPreviousSiblingIndex = (DWORD)-1;
        DWORD dwCurrentSiblingIndex = (DWORD)-1;
        for( DWORD i = 0; i < dwChildCount; ++i )
        {
            dwPreviousSiblingIndex = dwCurrentSiblingIndex;
            dwCurrentSiblingIndex = (DWORD)g_FrameHeaderArray.size();
            if( dwPreviousSiblingIndex != (DWORD)-1 )
            {
                g_FrameHeaderArray[ dwPreviousSiblingIndex ].SiblingFrame = dwCurrentSiblingIndex;
            }
            CaptureScene( pRootFrame->GetChildByIndex( i ), dwCurrentIndex );
        }
    }

    DWORD FindFrame( ExportString Name )
    {
        DWORD dwFrameCount = (DWORD)g_FrameArray.size();
        for( DWORD i = 0; i < dwFrameCount; ++i )
        {
            if( g_FrameArray[i]->GetName() == Name )
                return i;
        }
        return (DWORD)-1;
    }

    VOID CaptureSecondPass()
    {
        // Create frame influence lists
        assert( g_MeshHeaderArray.size() == g_ModelMeshArray.size() );
        DWORD dwMeshCount = (DWORD)g_MeshHeaderArray.size();
        for( DWORD i = 0; i < dwMeshCount; ++i )
        {
            SDKMESH_MESH& Mesh = g_MeshHeaderArray[i];
            const ExportMeshBase* pMeshBase = g_ModelMeshArray[i];
            for( DWORD j = 0; j < Mesh.NumFrameInfluences; ++j )
            {
                ExportString InfluenceName = pMeshBase->GetInfluence( j );
                DWORD dwFrameIndex = FindFrame( InfluenceName );
                g_FrameInfluenceArray.push_back( (UINT)dwFrameIndex );
            }
        }
    }

    inline DWORD RoundUp4K( DWORD dwValue )
    {
        return ( ( dwValue + 4095 ) / 4096 ) * 4096;
    }

    inline DWORD RoundUp32B( DWORD dwValue )
    {
        return ( ( dwValue + 31 ) / 32 ) * 32;
    }

    DWORD ComputeMeshHeaderIndexDataSize()
    {
        return (DWORD)( ( g_SubsetIndexArray.size() + g_FrameInfluenceArray.size() ) * sizeof( UINT ) ); 
    }

    DWORD ComputeBufferDataSize()
    {
        DWORD dwDataSize = 0;
        DWORD dwVBCount = (DWORD)g_VBHeaderArray.size();
        for( DWORD i = 0; i < dwVBCount; ++i )
        {
            SDKMESH_VERTEX_BUFFER_HEADER& VBHeader = g_VBHeaderArray[i];
            dwDataSize += RoundUp4K( (DWORD)VBHeader.SizeBytes );
        }
        DWORD dwIBCount = (DWORD)g_IBHeaderArray.size();
        for( DWORD i = 0; i < dwIBCount; ++i )
        {
            SDKMESH_INDEX_BUFFER_HEADER& IBHeader = g_IBHeaderArray[i];
            dwDataSize += RoundUp4K( (DWORD)IBHeader.SizeBytes );
        }
        return dwDataSize;
    }

    VOID WriteVertexBufferHeaders( HANDLE hFile, UINT64& DataOffset )
    {
        DWORD dwVBCount = (DWORD)g_VBHeaderArray.size();
        for( DWORD i = 0; i < dwVBCount; ++i )
        {
            SDKMESH_VERTEX_BUFFER_HEADER& VBHeader = g_VBHeaderArray[i];
            VBHeader.DataOffset = DataOffset;
            DataOffset += RoundUp4K( (DWORD)VBHeader.SizeBytes );
            DWORD dwBytesWritten = 0;
            WriteFile( hFile, &VBHeader, sizeof( SDKMESH_VERTEX_BUFFER_HEADER ), &dwBytesWritten, NULL );
        }
    }

    VOID WriteIndexBufferHeaders( HANDLE hFile, UINT64& DataOffset )
    {
        DWORD dwIBCount = (DWORD)g_IBHeaderArray.size();
        for( DWORD i = 0; i < dwIBCount; ++i )
        {
            SDKMESH_INDEX_BUFFER_HEADER& IBHeader = g_IBHeaderArray[i];
            IBHeader.DataOffset = DataOffset;
            DataOffset += RoundUp4K( (DWORD)IBHeader.SizeBytes );
            DWORD dwBytesWritten = 0;
            WriteFile( hFile, &IBHeader, sizeof( SDKMESH_INDEX_BUFFER_HEADER ), &dwBytesWritten, NULL );
        }
    }

    VOID WriteMeshes( HANDLE hFile, UINT64& DataOffset )
    {
        DWORD dwMeshCount = (DWORD)g_MeshHeaderArray.size();
        DWORD dwBytesWritten = 0;
        for( DWORD i = 0; i < dwMeshCount; ++i )
        {
            SDKMESH_MESH& Mesh = g_MeshHeaderArray[i];
            Mesh.SubsetOffset = DataOffset;
            DataOffset += Mesh.NumSubsets * sizeof( UINT );
            Mesh.FrameInfluenceOffset = DataOffset;
            DataOffset += Mesh.NumFrameInfluences * sizeof( UINT );
            WriteFile( hFile, &Mesh, sizeof( SDKMESH_MESH ), &dwBytesWritten, NULL );
        }
    }

    VOID WriteSubsetIndexAndFrameInfluenceData( HANDLE hFile )
    {
        DWORD dwMeshCount = (DWORD)g_MeshHeaderArray.size();
        DWORD dwSubsetIndexCount = 0;
        DWORD dwFrameInfluenceCount = 0;
        DWORD dwBytesWritten = 0;
        for( DWORD i = 0; i < dwMeshCount; ++i )
        {
            SDKMESH_MESH& Mesh = g_MeshHeaderArray[i];
            if( Mesh.NumSubsets > 0 )
            {
                WriteFile( hFile, &g_SubsetIndexArray[ dwSubsetIndexCount ], Mesh.NumSubsets * sizeof( UINT ), &dwBytesWritten, NULL );
                dwSubsetIndexCount += Mesh.NumSubsets;
            }
            if( Mesh.NumFrameInfluences > 0 )
            {
                WriteFile( hFile, &g_FrameInfluenceArray[ dwFrameInfluenceCount ], Mesh.NumFrameInfluences * sizeof( UINT ), &dwBytesWritten, NULL );
                dwFrameInfluenceCount += Mesh.NumFrameInfluences;
            }
        }
    }

    VOID WriteVertexBufferData( HANDLE hFile )
    {
        assert( g_VBHeaderArray.size() == g_VBArray.size() );
        DWORD dwVBCount = (DWORD)g_VBHeaderArray.size();
        DWORD dwBytesWritten = 0;
        for( DWORD i = 0; i < dwVBCount; ++i )
        {
            const ExportVB* pVB = g_VBArray[i];
            DWORD dwDataSize = pVB->GetVertexDataSize();
            WriteFile( hFile, pVB->GetVertexData(), dwDataSize, &dwBytesWritten, NULL );
            DWORD dwPaddingSize = RoundUp4K( dwDataSize ) - dwDataSize;
            assert( dwPaddingSize < 4096 );
            WriteFile( hFile, g_Padding4K, dwPaddingSize, &dwBytesWritten, NULL );
        }
    }

    VOID WriteIndexBufferData( HANDLE hFile )
    {
        assert( g_IBHeaderArray.size() == g_IBArray.size() );
        DWORD dwIBCount = (DWORD)g_IBHeaderArray.size();
        DWORD dwBytesWritten = 0;
        for( DWORD i = 0; i < dwIBCount; ++i )
        {
            const ExportIB* pIB = g_IBArray[i];
            DWORD dwDataSize = pIB->GetIndexDataSize();
            WriteFile( hFile, pIB->GetIndexData(), dwDataSize, &dwBytesWritten, NULL );
            DWORD dwPaddingSize = RoundUp4K( dwDataSize ) - dwDataSize;
            assert( dwPaddingSize < 4096 );
            WriteFile( hFile, g_Padding4K, dwPaddingSize, &dwBytesWritten, NULL );
        }
    }

    BOOL WriteSDKMeshFile( const CHAR* strFileName, ExportManifest* pManifest )
    {
        if( g_pScene == NULL )
            return FALSE;

        ClearSceneArrays();

        CaptureScene( g_pScene, (DWORD)-1 );
        CaptureSecondPass();

        HANDLE hFile = CreateFileA( strFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        if( hFile == INVALID_HANDLE_VALUE )
        {
            ExportLog::LogError( "Could not write to file \"%s\".  Check that the file is not read-only and that the path exists.", strFileName );
            return FALSE;
        }

        ExportLog::LogMsg( 1, "Writing to SDKMESH file \"%s\"", strFileName );

        SDKMESH_HEADER FileHeader;
        ZeroMemory( &FileHeader, sizeof( SDKMESH_HEADER ) );

        FileHeader.Version = SDKMESH_FILE_VERSION;
        FileHeader.IsBigEndian = (BYTE)(!g_pScene->Settings().bLittleEndian);

        FileHeader.NumFrames = (UINT)g_FrameArray.size();
        FileHeader.NumMaterials = (UINT)g_MaterialArray.size();
        FileHeader.NumMeshes = (UINT)g_MeshHeaderArray.size();
        FileHeader.NumTotalSubsets = (UINT)g_SubsetArray.size();
        FileHeader.NumIndexBuffers = (UINT)g_IBHeaderArray.size();
        FileHeader.NumVertexBuffers = (UINT)g_VBHeaderArray.size();

        FileHeader.HeaderSize = sizeof( SDKMESH_HEADER ) +
                                FileHeader.NumVertexBuffers * sizeof( SDKMESH_VERTEX_BUFFER_HEADER ) + 
                                FileHeader.NumIndexBuffers * sizeof( SDKMESH_INDEX_BUFFER_HEADER );

        UINT64 StaticDataSize = FileHeader.NumMeshes * sizeof( SDKMESH_MESH ) +
                                 FileHeader.NumTotalSubsets * sizeof( SDKMESH_SUBSET ) +
                                 FileHeader.NumFrames * sizeof( SDKMESH_FRAME ) + 
                                 FileHeader.NumMaterials * sizeof( SDKMESH_MATERIAL );

        FileHeader.NonBufferDataSize = StaticDataSize + ComputeMeshHeaderIndexDataSize();

        FileHeader.BufferDataSize = ComputeBufferDataSize();


        FileHeader.VertexStreamHeadersOffset = sizeof( SDKMESH_HEADER );
        FileHeader.IndexStreamHeadersOffset = FileHeader.VertexStreamHeadersOffset + FileHeader.NumVertexBuffers * sizeof( SDKMESH_VERTEX_BUFFER_HEADER );
        FileHeader.MeshDataOffset = FileHeader.IndexStreamHeadersOffset + FileHeader.NumIndexBuffers * sizeof( SDKMESH_INDEX_BUFFER_HEADER );
        FileHeader.SubsetDataOffset = FileHeader.MeshDataOffset + FileHeader.NumMeshes * sizeof(SDKMESH_MESH);
        FileHeader.FrameDataOffset = FileHeader.SubsetDataOffset + FileHeader.NumTotalSubsets * sizeof(SDKMESH_SUBSET);
        FileHeader.MaterialDataOffset = FileHeader.FrameDataOffset + FileHeader.NumFrames * sizeof(SDKMESH_FRAME);

        // Write header to file
        DWORD dwBytesWritten = 0;
        WriteFile( hFile, &FileHeader, sizeof( SDKMESH_HEADER ), &dwBytesWritten, NULL );

        UINT64 BufferDataOffset = FileHeader.HeaderSize + FileHeader.NonBufferDataSize;

        // Write VB headers
        WriteVertexBufferHeaders( hFile, BufferDataOffset );

        // Write IB headers
        WriteIndexBufferHeaders( hFile, BufferDataOffset );

        // Write meshes
        UINT64 SubsetListOffset = FileHeader.HeaderSize + StaticDataSize;
        WriteMeshes( hFile, SubsetListOffset );

        // Write subsets
        DWORD dwSubsetCount = (DWORD)g_SubsetArray.size();
        for( DWORD i = 0; i < dwSubsetCount; ++i )
        {
            SDKMESH_SUBSET& Subset = g_SubsetArray[i];
            WriteFile( hFile, &Subset, sizeof( SDKMESH_SUBSET ), &dwBytesWritten, NULL );
        }

        // Write frames
        DWORD dwFrameCount = (DWORD)g_FrameHeaderArray.size();
        for( DWORD i = 0; i < dwFrameCount; ++i )
        {
            SDKMESH_FRAME& Frame = g_FrameHeaderArray[i];
            WriteFile( hFile, &Frame, sizeof( SDKMESH_FRAME ), &dwBytesWritten, NULL );
        }

        // Write materials
        DWORD dwMaterialCount = (DWORD)g_MaterialArray.size();
        for( DWORD i = 0; i < dwMaterialCount; ++i )
        {
            SDKMESH_MATERIAL& Material = g_MaterialArray[i];
            WriteFile( hFile, &Material, sizeof( SDKMESH_MATERIAL ), &dwBytesWritten, NULL );
        }

        // Write subset index lists and frame influence lists
        WriteSubsetIndexAndFrameInfluenceData( hFile );

        // Write VB data
        WriteVertexBufferData( hFile );
        
        // Write IB data
        WriteIndexBufferData( hFile );

        CloseHandle( hFile );

        ClearSceneArrays();

        WriteSDKMeshAnimationFile( strFileName, pManifest );

        return TRUE;
    }

    BOOL SamplePositionData( ExportAnimationPositionKey* pKeys, DWORD dwKeyCount, SDKANIMATION_DATA* pDestKeys, DWORD dwDestKeyCount, FLOAT fKeyInterval )
    {
        if( dwKeyCount == 0 )
            return FALSE;

        DWORD dwCurrentSrcKey = 0;
        BOOL bEndKey = FALSE;
        ExportAnimationPositionKey StartKey = pKeys[dwCurrentSrcKey];
        ExportAnimationPositionKey EndKey;
        if( dwKeyCount > 1 )
        {
            bEndKey = TRUE;
            EndKey = pKeys[dwCurrentSrcKey + 1];
        }

        FLOAT fTime = 0;
        for( DWORD i = 0; i < dwDestKeyCount; ++i )
        {
            while( bEndKey && fTime >= EndKey.fTime )
            {
                StartKey = EndKey;
                ++dwCurrentSrcKey;
                if( dwCurrentSrcKey >= dwKeyCount )
                    bEndKey = FALSE;
                else
                    EndKey = pKeys[dwCurrentSrcKey + 1];
            }
            if( !bEndKey )
            {
                pDestKeys[i].Translation = StartKey.Position;
            }
            else
            {
                assert( fTime <= EndKey.fTime );
                FLOAT fLerpFactor = ( fTime - StartKey.fTime ) / ( EndKey.fTime - StartKey.fTime );
                fLerpFactor = min( max( 0.0f, fLerpFactor ), 1.0f );
                D3DXVec3Lerp( &pDestKeys[i].Translation, &StartKey.Position, &EndKey.Position, fLerpFactor );
            }
            fTime += fKeyInterval;
        }
        return TRUE;
    }

    BOOL SampleOrientationData( ExportAnimationOrientationKey* pKeys, DWORD dwKeyCount, SDKANIMATION_DATA* pDestKeys, DWORD dwDestKeyCount, FLOAT fKeyInterval )
    {
        if( dwKeyCount == 0 )
            return FALSE;

        DWORD dwCurrentSrcKey = 0;
        BOOL bEndKey = FALSE;
        ExportAnimationOrientationKey StartKey = pKeys[dwCurrentSrcKey];
        ExportAnimationOrientationKey EndKey;
        if( dwKeyCount > 1 )
        {
            bEndKey = TRUE;
            EndKey = pKeys[dwCurrentSrcKey + 1];
        }

        FLOAT fTime = 0;
        for( DWORD i = 0; i < dwDestKeyCount; ++i )
        {
            while( bEndKey && fTime >= EndKey.fTime )
            {
                StartKey = EndKey;
                ++dwCurrentSrcKey;
                if( dwCurrentSrcKey >= dwKeyCount )
                    bEndKey = FALSE;
                else
                    EndKey = pKeys[dwCurrentSrcKey + 1];
            }
            if( !bEndKey )
            {
                pDestKeys[i].Orientation = (D3DXVECTOR4)StartKey.Orientation;
            }
            else
            {
                assert( fTime <= EndKey.fTime );
                FLOAT fLerpFactor = ( fTime - StartKey.fTime ) / ( EndKey.fTime - StartKey.fTime );
                fLerpFactor = min( max( 0.0f, fLerpFactor ), 1.0f );
                D3DXVec4Lerp( &pDestKeys[i].Orientation, (D3DXVECTOR4*)&StartKey.Orientation, (D3DXVECTOR4*)&EndKey.Orientation, fLerpFactor );
            }
            fTime += fKeyInterval;
        }
        return TRUE;
    }

    BOOL SampleScaleData( ExportAnimationScaleKey* pKeys, DWORD dwKeyCount, SDKANIMATION_DATA* pDestKeys, DWORD dwDestKeyCount, FLOAT fKeyInterval )
    {
        if( dwKeyCount == 0 )
            return FALSE;

        DWORD dwCurrentSrcKey = 0;
        BOOL bEndKey = FALSE;
        ExportAnimationScaleKey StartKey = pKeys[dwCurrentSrcKey];
        ExportAnimationScaleKey EndKey;
        if( dwKeyCount > 1 )
        {
            bEndKey = TRUE;
            EndKey = pKeys[dwCurrentSrcKey + 1];
        }

        FLOAT fTime = 0;
        for( DWORD i = 0; i < dwDestKeyCount; ++i )
        {
            while( bEndKey && fTime >= EndKey.fTime )
            {
                StartKey = EndKey;
                ++dwCurrentSrcKey;
                if( dwCurrentSrcKey >= dwKeyCount )
                    bEndKey = FALSE;
                else
                    EndKey = pKeys[dwCurrentSrcKey + 1];
            }
            if( !bEndKey )
            {
                pDestKeys[i].Scaling = StartKey.Scale;
            }
            else
            {
                assert( fTime <= EndKey.fTime );
                FLOAT fLerpFactor = ( fTime - StartKey.fTime ) / ( EndKey.fTime - StartKey.fTime );
                fLerpFactor = min( max( 0.0f, fLerpFactor ), 1.0f );
                D3DXVec3Lerp( &pDestKeys[i].Scaling, &StartKey.Scale, &EndKey.Scale, fLerpFactor );
            }
            fTime += fKeyInterval;
        }
        return TRUE;
    }

    BOOL WriteSDKMeshAnimationFile( const CHAR* strFileName, ExportManifest* pManifest )
    {
        if( g_pScene == NULL || g_pScene->GetAnimationCount() == 0 )
            return FALSE;

        if( !g_pScene->Settings().bExportAnimations )
            return FALSE;

        ExportAnimation* pAnim = g_pScene->GetAnimation( 0 );

        CHAR strAnimFileName[MAX_PATH];
        strcpy_s( strAnimFileName, strFileName );
        strcat_s( strAnimFileName, "_anim" );

        HANDLE hFile = CreateFileA( strAnimFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        if( hFile == INVALID_HANDLE_VALUE )
        {
            ExportLog::LogError( "Could not write to file \"%s\".  Check that the file is not read-only and that the path exists." );
            return FALSE;
        }

        ExportLog::LogMsg( 1, "Writing to SDKMESH animation file \"%s\"", strAnimFileName );

        SDKANIMATION_FILE_HEADER AnimHeader;
        ZeroMemory( &AnimHeader, sizeof( SDKANIMATION_FILE_HEADER ) );

        const DWORD dwKeyCount = (DWORD)( pAnim->GetDuration() / pAnim->fSourceFrameInterval );
        const DWORD dwTrackCount = pAnim->GetTrackCount();
        DWORD dwTrackHeadersDataSize = dwTrackCount * sizeof( SDKANIMATION_FRAME_DATA );
        DWORD dwSingleTrackDataSize = dwKeyCount * sizeof( SDKANIMATION_DATA );

        AnimHeader.Version = SDKMESH_FILE_VERSION;
        AnimHeader.IsBigEndian = !g_pScene->Settings().bLittleEndian;
        AnimHeader.FrameTransformType = FTT_RELATIVE;
        AnimHeader.NumAnimationKeys = (UINT)dwKeyCount;
        AnimHeader.AnimationFPS = (UINT)( 1.001f / pAnim->fSourceFrameInterval );
        AnimHeader.NumFrames = dwTrackCount;
        AnimHeader.AnimationDataSize = dwTrackHeadersDataSize + dwTrackCount * dwSingleTrackDataSize;
        AnimHeader.AnimationDataOffset = sizeof( SDKANIMATION_FILE_HEADER );

        DWORD dwBytesWritten = 0;
        WriteFile( hFile, &AnimHeader, sizeof( SDKANIMATION_FILE_HEADER ), &dwBytesWritten, NULL );

        for( DWORD i = 0; i < dwTrackCount; ++i )
        {
            ExportAnimationTrack* pTrack = pAnim->GetTrack( i );
            ExportFrame* pSourceFrame = pTrack->TransformTrack.pSourceFrame;
            SDKANIMATION_FRAME_DATA FrameData;
            ZeroMemory( &FrameData, sizeof( SDKANIMATION_FRAME_DATA ) );
            FrameData.DataOffset = dwTrackHeadersDataSize + i * dwSingleTrackDataSize;
            if( pSourceFrame == NULL )
            {
                strncpy_s( FrameData.FrameName, pTrack->GetName().SafeString(), MAX_FRAME_NAME );
            }
            else
            {
                strncpy_s( FrameData.FrameName, pSourceFrame->GetName().SafeString(), MAX_FRAME_NAME );
            }
            WriteFile( hFile, &FrameData, sizeof( SDKANIMATION_FRAME_DATA ), &dwBytesWritten, NULL );
        }

        SDKANIMATION_DATA* pTrackData = new SDKANIMATION_DATA[ dwKeyCount ];

        for( DWORD i = 0; i < dwTrackCount; ++i )
        {
            ExportAnimationTrack* pTrack = pAnim->GetTrack( i );
            ExportAnimationTransformTrack* pTT = &pTrack->TransformTrack;

            ZeroMemory( pTrackData, dwKeyCount * sizeof( SDKANIMATION_DATA ) );
            SamplePositionData( pTT->GetPositionKeys(), pTT->GetPositionKeyCount(), pTrackData, dwKeyCount, pAnim->fSourceFrameInterval );
            SampleOrientationData( pTT->GetOrientationKeys(), pTT->GetOrientationKeyCount(), pTrackData, dwKeyCount, pAnim->fSourceFrameInterval );
            SampleScaleData( pTT->GetScaleKeys(), pTT->GetScaleKeyCount(), pTrackData, dwKeyCount, pAnim->fSourceFrameInterval );

            WriteFile( hFile, pTrackData, dwKeyCount * sizeof( SDKANIMATION_DATA ), &dwBytesWritten, NULL );
        }

        delete[] pTrackData;

        CloseHandle( hFile );

        return TRUE;
    }
}