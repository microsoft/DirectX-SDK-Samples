//-------------------------------------------------------------------------------------
//  ParseMaterial.cpp
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "StdAfx.h"
#include <algorithm>
#include "ParseMaterial.h"

extern ATG::ExportScene* g_pScene;

BOOL MaterialParameterSort( ExportMaterialParameter A, ExportMaterialParameter B )
{
    if( A.ParamType == MPT_TEXTURE2D && B.ParamType != MPT_TEXTURE2D )
        return TRUE;
    return FALSE;
}

VOID FixupGenericMaterial( ExportMaterial* pMaterial )
{
    ExportMaterialParameter OutputParam;
    OutputParam.ParamType = MPT_TEXTURE2D;
    OutputParam.bInstanceParam = TRUE;

    ExportMaterialParameter* pParam = pMaterial->FindParameter( "DiffuseTexture" );
    if( pParam == NULL )
    {
        ExportLog::LogWarning( "Material \"%s\" has no diffuse texture.  Assigning a default diffuse texture.", pMaterial->GetName().SafeString() );
        OutputParam.Name = "DiffuseTexture";
        OutputParam.ValueString = ExportMaterial::GetDefaultDiffuseMapTextureName();
        pMaterial->AddParameter( OutputParam );
    }

    pParam = pMaterial->FindParameter( "NormalMapTexture" );
    if( pParam == NULL )
    {
        ExportLog::LogWarning( "Material \"%s\" has no normal map texture.  Assigning a default normal map texture.", pMaterial->GetName().SafeString() );
        OutputParam.Name = "NormalMapTexture";
        OutputParam.ValueString = ExportMaterial::GetDefaultNormalMapTextureName();
        pMaterial->AddParameter( OutputParam );
    }

    MaterialParameterList* pParamList = pMaterial->GetParameterList();
    //std::reverse( pParamList->begin(), pParamList->end() );
    std::stable_sort( pParamList->begin(), pParamList->end(), MaterialParameterSort );
}

/*
KFbxTexture* GetTexture( KFbxLayer* pLayer, KFbxLayerElement::ELayerElementType TextureType, const DWORD dwMaterialIndex, const DWORD dwMaterialCount )
{
    KFbxLayerElementTexture* pTextureSet = pLayer->GetTextures( TextureType );
    if( pTextureSet == NULL )
        return NULL;

    DWORD dwTextureCount = pTextureSet->GetDirectArray().GetCount();
    assert( dwTextureCount <= dwMaterialCount );
    return pTextureSet->GetDirectArray().GetAt( dwMaterialIndex );
}
*/

VOID AddTextureParameter( ExportMaterial* pMaterial, const CHAR* strParamName, DWORD dwIndex, const CHAR* strFileName, DWORD dwFlags )
{
    ExportMaterialParameter OutputParam;
    if( dwIndex == 0 )
    {
        OutputParam.Name = strParamName;
    }
    else
    {
        CHAR strDecoratedName[512];
        sprintf_s( strDecoratedName, "%s%d", strParamName, dwIndex );
        OutputParam.Name = strDecoratedName;
    }
    ExportLog::LogMsg( 4, "Material parameter \"%s\" = \"%s\"", OutputParam.Name.SafeString(), strFileName );
    OutputParam.ValueString = strFileName;
    OutputParam.ParamType = MPT_TEXTURE2D;
    OutputParam.bInstanceParam = TRUE;
    OutputParam.Flags = dwFlags;
    pMaterial->AddParameter( OutputParam );
}

BOOL ExtractTextures( KFbxProperty Property, const CHAR* strParameterName, ExportMaterial* pMaterial, DWORD dwFlags )
{
    BOOL bResult = FALSE;
    DWORD dwLayeredTextureCount = Property.GetSrcObjectCount( KFbxLayeredTexture::ClassId );
    if( dwLayeredTextureCount > 0 )
    {
        DWORD dwTextureIndex = 0;
        for( DWORD i = 0; i < dwLayeredTextureCount; ++i )
        {
            KFbxLayeredTexture* pFbxLayeredTexture = KFbxCast<KFbxLayeredTexture>( Property.GetSrcObject( KFbxLayeredTexture::ClassId, i ) );
            DWORD dwTextureCount = pFbxLayeredTexture->GetSrcObjectCount( KFbxTexture::ClassId );
            for( DWORD j = 0; j < dwTextureCount; ++j )
            {
                KFbxTexture* pFbxTexture = KFbxCast<KFbxTexture>( pFbxLayeredTexture->GetSrcObject( KFbxTexture::ClassId, j ) );
                if( pFbxTexture == NULL )
                    continue;

                AddTextureParameter( pMaterial, strParameterName, dwTextureIndex, pFbxTexture->GetFileName(), dwFlags );
                ++dwTextureIndex;
                bResult = TRUE;
            }
        }
    }
    else
    {
        DWORD dwTextureCount = Property.GetSrcObjectCount( KFbxTexture::ClassId );
        for( DWORD i = 0; i < dwTextureCount; ++i )
        {
            KFbxTexture* pFbxTexture = KFbxCast<KFbxTexture>( Property.GetSrcObject( KFbxTexture::ClassId, i ) );
            if( pFbxTexture == NULL )
                continue;

            AddTextureParameter( pMaterial, strParameterName, i, pFbxTexture->GetFileName(), dwFlags );
            bResult = TRUE;
        }
    }
    return bResult;
}

ExportMaterial* ParseMaterialInLayer( KFbxMesh* pMesh, KFbxLayer* pLayer, DWORD dwMaterialIndex )
{
    KFbxLayerElementMaterial* pMaterials = pLayer->GetMaterials();
    assert( dwMaterialIndex < (DWORD)pMaterials->GetDirectArray().GetCount() );
    UNUSED( pMaterials );

    KFbxSurfaceMaterial* pFbxMaterial = pMesh->GetNode()->GetMaterial( (INT) dwMaterialIndex );
    assert( pFbxMaterial != NULL );

    ExportMaterial* pExistingMaterial = g_pScene->FindMaterial( pFbxMaterial );
    if( pExistingMaterial != NULL )
    {
        ExportLog::LogMsg( 4, "Found existing material \"%s\".", pFbxMaterial->GetName() );
        return pExistingMaterial;
    }

    ExportLog::LogMsg( 2, "Parsing material \"%s\".", pFbxMaterial->GetName() );

    BOOL bRenameMaterial = FALSE;
    ExportString MaterialName( pFbxMaterial->GetName() );
    ExportMaterial* pSameNameMaterial = NULL;
    DWORD dwRenameIndex = 0;
    do 
    {
        pSameNameMaterial = g_pScene->FindMaterial( MaterialName );
        if( pSameNameMaterial != NULL )
        {
            bRenameMaterial = TRUE;
            CHAR strName[200];
            sprintf_s( strName, "%s_%d", pFbxMaterial->GetName(), dwRenameIndex++ );
            MaterialName = strName;
        }
    } while ( pSameNameMaterial != NULL );

    if( bRenameMaterial )
    {
        ExportLog::LogMsg( 2, "Found duplicate material name; renaming material \"%s\" to \"%s\".", pFbxMaterial->GetName(), MaterialName );
    }

    ExportMaterial* pMaterial = new ExportMaterial( MaterialName );
    pMaterial->SetDCCObject( pFbxMaterial );
    pMaterial->SetDefaultMaterialName( g_pScene->Settings().strDefaultMaterialName );

    enum ParameterPostOperations
    {
        PPO_Nothing = 0,
        PPO_TransparentMaterial = 1,
    };

    struct TextureParameterExtraction
    {
        const CHAR* strFbxPropertyName;
        const CHAR* strParameterName;
        DWORD dwPostOperations;
        DWORD dwParameterFlags;
    };

    TextureParameterExtraction ExtractionList[] =
    {
        { KFbxSurfaceMaterial::sTransparentColor,   "AlphaTexture",                 PPO_TransparentMaterial,    ExportMaterialParameter::EMPF_ALPHACHANNEL },
        { KFbxSurfaceMaterial::sDiffuse,            "DiffuseTexture",               PPO_Nothing,                ExportMaterialParameter::EMPF_DIFFUSEMAP },
        { KFbxSurfaceMaterial::sBump,               "NormalMapTexture",             PPO_Nothing,                0 /*ExportMaterialParameter::EMPF_BUMPMAP*/ },
        { KFbxSurfaceMaterial::sNormalMap,          "NormalMapTexture",             PPO_Nothing,                ExportMaterialParameter::EMPF_NORMALMAP },
        { KFbxSurfaceMaterial::sSpecular,           "SpecularMapTexture",           PPO_Nothing,                ExportMaterialParameter::EMPF_SPECULARMAP },
        { KFbxSurfaceMaterial::sEmissive,           "EmissiveMapTexture",           PPO_Nothing,                0 },
    };

    for( DWORD dwExtractionIndex = 0; dwExtractionIndex < ARRAYSIZE(ExtractionList); ++dwExtractionIndex )
    {
        const TextureParameterExtraction& tpe = ExtractionList[dwExtractionIndex];

        KFbxProperty Property = pFbxMaterial->FindProperty( tpe.strFbxPropertyName );
        if( !Property.IsValid() )
            continue;

        BOOL bFound = ExtractTextures( Property, tpe.strParameterName, pMaterial, tpe.dwParameterFlags );
        if( bFound )
        {
            if( tpe.dwPostOperations & PPO_TransparentMaterial )
            {
                ExportLog::LogMsg( 4, "Material \"%s\" is transparent.", pMaterial->GetName().SafeString() );
                pMaterial->SetTransparent( TRUE );
            }
        }
    }

    FixupGenericMaterial( pMaterial );

    BOOL bResult = g_pScene->AddMaterial( pMaterial );
    assert( bResult );
    if( !bResult )
    {
        ExportLog::LogError( "Could not add material \"%s\" to scene.", pMaterial->GetName().SafeString() );
    }
    g_pScene->Statistics().MaterialsExported++;

    return pMaterial;
}
