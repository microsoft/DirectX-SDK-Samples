//-------------------------------------------------------------------------------------
//  ExportSettings.cpp
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "ExportSettings.h"

namespace ATG
{
    ExportSettingsManager   g_SettingsManager;
    ExportCoreSettings      g_ExportCoreSettings;

    ExportSettingsEntry::ExportSettingsEntry()
        : m_pFirstChild( NULL ),
          m_pSibling( NULL ),
          m_pLinkedCurrentValue( NULL ),
          m_pEnumValues( NULL ),
          m_dwEnumValueCount( 0 )
    {
    }

    ExportSettingsEntry::~ExportSettingsEntry()
    {
        if( m_pFirstChild != NULL )
        {
            delete m_pFirstChild;
            m_pFirstChild = NULL;
        }
        if( m_pSibling != NULL )
        {
            delete m_pSibling;
            m_pSibling = NULL;
        }
    }

    VOID ExportSettingsEntry::SetDefaultValue( BOOL bSetChildren, BOOL bSetSiblings )
    {
        if( m_pLinkedCurrentValue != NULL )
        {
            if( m_DefaultValue.m_Type == ExportVariant::VT_STRING )
                strcpy_s( (CHAR*)m_pLinkedCurrentValue, SETTINGS_STRING_LENGTH, m_DefaultValue.m_strValue );
            else
                *(INT*)m_pLinkedCurrentValue = m_DefaultValue.m_iValue;
        }
        else
        {
            m_CurrentValue = m_DefaultValue;
        }

        if( bSetSiblings && m_pSibling != NULL )
        {
            m_pSibling->SetDefaultValue( bSetChildren, TRUE );
        }
        if( bSetChildren && m_pFirstChild != NULL )
        {
            m_pFirstChild->SetDefaultValue( TRUE, TRUE );
        }
    }

    VOID ExportSettingsEntry::ReverseChildOrder()
    {
        ExportSettingsEntry* pHead = NULL;
        ExportSettingsEntry* pCurrent = m_pFirstChild;

        while( pCurrent != NULL )
        {
            ExportSettingsEntry* pNext = pCurrent->m_pSibling;
            pCurrent->m_pSibling = pHead;
            pHead = pCurrent;
            pCurrent = pNext;
        }

        m_pFirstChild = pHead;
    }

    VOID ExportSettingsEntry::CreateSettingName()
    {
        CHAR strSettingName[512];
        ZeroMemory( strSettingName, sizeof( strSettingName ) );
        const CHAR* strDisplayName = m_DisplayName.SafeString();
        assert( strlen( strDisplayName ) < ARRAYSIZE( strSettingName ) );
        CHAR* pDest = strSettingName;
        while( *strDisplayName != '\0' )
        {
            if( *strDisplayName != ' ' )
            {
                *pDest = *strDisplayName;
                ++pDest;
            }
            ++strDisplayName;
        }
        *pDest = '\0';
        m_SettingName = strSettingName;
    }

    BOOL ExportSettingsEntry::GetValueBool() const
    {
        assert( m_Type == CT_CHECKBOX );
        const BOOL* pValue = (const BOOL*)GetCurrentValue();
        return *pValue;
    }

    const CHAR* ExportSettingsEntry::GetValueString() const
    {
        assert( m_Type == CT_STRING );
        if( m_pLinkedCurrentValue != NULL )
            return (const CHAR*)m_pLinkedCurrentValue;
        return m_CurrentValue.m_strValue;
    }

    INT ExportSettingsEntry::GetValueInt() const
    {
        assert( m_Type == CT_BOUNDEDINTSLIDER || m_Type == CT_ENUM );
        const INT* pValue = (const INT*)GetCurrentValue();
        return *pValue;
    }

    FLOAT ExportSettingsEntry::GetValueFloat() const
    {
        assert( m_Type == CT_BOUNDEDFLOATSLIDER );
        const FLOAT* pValue = (const FLOAT*)GetCurrentValue();
        return *pValue;
    }

    VOID ExportSettingsEntry::SetValue( INT iValue )
    {
        assert( m_Type == CT_BOUNDEDINTSLIDER || m_Type == CT_CHECKBOX || m_Type == CT_ENUM );
        INT* pValue = (INT*)GetCurrentValue();
        *pValue = iValue;
    }

    VOID ExportSettingsEntry::SetValue( FLOAT fValue )
    {
        assert( m_Type == CT_BOUNDEDFLOATSLIDER );
        FLOAT* pValue = (FLOAT*)GetCurrentValue();
        *pValue = fValue;
    }

    VOID ExportSettingsEntry::SetValue( const CHAR* strValue )
    {
        assert( m_Type == CT_STRING );
        if( m_pLinkedCurrentValue != NULL )
        {
            strcpy_s( (CHAR*)m_pLinkedCurrentValue, SETTINGS_STRING_LENGTH, strValue );
        }
        else
        {
            m_CurrentValue.m_strValue = strValue;
        }
    }

    ExportSettingsManager::~ExportSettingsManager()
    {
        DWORD dwCount = GetRootCategoryCount();
        for( DWORD i = 0; i < dwCount; ++i )
        {
            delete GetRootCategory( i );
        }
        m_RootCategories.clear();
    }

    ExportSettingsEntry* ExportSettingsManager::AddRootCategory( ExportString Caption )
    {
        return AddCategory( NULL, Caption );
    }

    ExportSettingsEntry* ExportSettingsManager::AddCategory( ExportSettingsEntry* pParentCategory, ExportString Caption )
    {
        ExportSettingsEntry* pNewCategory = new ExportSettingsEntry();
        pNewCategory->m_DisplayName = Caption;
        pNewCategory->m_Type = ExportSettingsEntry::CT_CATEGORY;
        if( pParentCategory != NULL )
        {
            pParentCategory->AddChild( pNewCategory );
        }
        else
        {
            m_RootCategories.push_back( pNewCategory );
        }

        return pNewCategory;
    }

    ExportSettingsEntry* ExportSettingsManager::AddBool( ExportSettingsEntry* pCategory, ExportString Caption, ExportString CmdLine, BOOL bDefaultValue, BOOL* pLinkedValue )
    {
        assert( pCategory != NULL );
        ExportSettingsEntry* pNewEntry = new ExportSettingsEntry();
        pNewEntry->m_DisplayName = Caption;
        pNewEntry->CreateSettingName();
        pNewEntry->m_CommandLineOptionName = CmdLine;
        pNewEntry->m_DefaultValue.m_bValue = bDefaultValue;
        pNewEntry->m_DefaultValue.m_Type = ExportVariant::VT_BOOL;
        pNewEntry->m_pLinkedCurrentValue = pLinkedValue;
        pNewEntry->m_CurrentValue = pNewEntry->m_DefaultValue;
        pNewEntry->m_Type = ExportSettingsEntry::CT_CHECKBOX;

        pCategory->AddChild( pNewEntry );
        return pNewEntry;
    }

    ExportSettingsEntry* ExportSettingsManager::AddIntBounded( ExportSettingsEntry* pCategory, ExportString Caption, ExportString CmdLine, INT iDefaultValue, INT iMin, INT iMax, INT* pLinkedValue )
    {
        assert( pCategory != NULL );
        ExportSettingsEntry* pNewEntry = new ExportSettingsEntry();
        pNewEntry->m_DisplayName = Caption;
        pNewEntry->CreateSettingName();
        pNewEntry->m_CommandLineOptionName = CmdLine;
        pNewEntry->m_DefaultValue.m_iValue = iDefaultValue;
        pNewEntry->m_DefaultValue.m_Type = ExportVariant::VT_INT;
        pNewEntry->m_pLinkedCurrentValue = pLinkedValue;
        pNewEntry->m_CurrentValue = pNewEntry->m_DefaultValue;
        pNewEntry->m_Type = ExportSettingsEntry::CT_BOUNDEDINTSLIDER;

        pNewEntry->m_MinValue.m_iValue = iMin;
        pNewEntry->m_MaxValue.m_iValue = iMax;

        pCategory->AddChild( pNewEntry );
        return pNewEntry;
    }

    ExportSettingsEntry* ExportSettingsManager::AddFloatBounded( ExportSettingsEntry* pCategory, ExportString Caption, ExportString CmdLine, FLOAT fDefaultValue, FLOAT fMin, FLOAT fMax, FLOAT* pLinkedValue )
    {
        assert( pCategory != NULL );
        ExportSettingsEntry* pNewEntry = new ExportSettingsEntry();
        pNewEntry->m_DisplayName = Caption;
        pNewEntry->CreateSettingName();
        pNewEntry->m_CommandLineOptionName = CmdLine;
        pNewEntry->m_DefaultValue.m_fValue = fDefaultValue;
        pNewEntry->m_DefaultValue.m_Type = ExportVariant::VT_FLOAT;
        pNewEntry->m_pLinkedCurrentValue = pLinkedValue;
        pNewEntry->m_CurrentValue = pNewEntry->m_DefaultValue;
        pNewEntry->m_Type = ExportSettingsEntry::CT_BOUNDEDFLOATSLIDER;

        pNewEntry->m_MinValue.m_fValue = fMin;
        pNewEntry->m_MaxValue.m_fValue = fMax;

        pCategory->AddChild( pNewEntry );
        return pNewEntry;
    }

    ExportSettingsEntry* ExportSettingsManager::AddString( ExportSettingsEntry* pCategory, ExportString Caption, ExportString CmdLine, const CHAR* strDefaultValue, CHAR* pLinkedValue )
    {
        assert( pCategory != NULL );
        ExportSettingsEntry* pNewEntry = new ExportSettingsEntry();
        pNewEntry->m_DisplayName = Caption;
        pNewEntry->CreateSettingName();
        pNewEntry->m_CommandLineOptionName = CmdLine;
        pNewEntry->m_DefaultValue.m_strValue = strDefaultValue;
        pNewEntry->m_DefaultValue.m_Type = ExportVariant::VT_STRING;
        pNewEntry->m_pLinkedCurrentValue = pLinkedValue;
        pNewEntry->m_CurrentValue = pNewEntry->m_DefaultValue;
        pNewEntry->m_Type = ExportSettingsEntry::CT_STRING;

        pCategory->AddChild( pNewEntry );
        return pNewEntry;
    }

    ExportSettingsEntry* ExportSettingsManager::AddEnum( ExportSettingsEntry* pCategory, ExportString Caption, ExportString CmdLine, INT iDefaultValue, const ExportEnumValue* pEnumValues, DWORD dwEnumValueCount, INT* pLinkedValue )
    {
        assert( pCategory != NULL );
        ExportSettingsEntry* pNewEntry = new ExportSettingsEntry();
        pNewEntry->m_DisplayName = Caption;
        pNewEntry->CreateSettingName();
        pNewEntry->m_CommandLineOptionName = CmdLine;
        pNewEntry->m_DefaultValue.m_iValue = iDefaultValue;
        pNewEntry->m_DefaultValue.m_Type = ExportVariant::VT_INT;
        pNewEntry->m_pLinkedCurrentValue = pLinkedValue;
        pNewEntry->m_CurrentValue = pNewEntry->m_DefaultValue;
        pNewEntry->m_Type = ExportSettingsEntry::CT_ENUM;
        pNewEntry->m_pEnumValues = pEnumValues;
        pNewEntry->m_dwEnumValueCount = dwEnumValueCount;

        pCategory->AddChild( pNewEntry );
        return pNewEntry;
    }

    BOOL ExportSettingsManager::MarshalAllSettings( CHAR* strDestBuffer, DWORD dwBufferSize, BOOL bNewLines, ExportSettingsEntry* pRoot )
    {
        BOOL bResult = TRUE;
        if( pRoot == NULL )
        {
            DWORD dwRootCount = GetRootCategoryCount();
            for( DWORD i = 0; i < dwRootCount; ++i )
            {
                bResult &= MarshalAllSettings( strDestBuffer, dwBufferSize, bNewLines, GetRootCategory( i ) );
            }
            return bResult;
        }

        if( pRoot->m_Type == ExportSettingsEntry::CT_CATEGORY && pRoot->m_pFirstChild != NULL )
        {
            bResult &= MarshalAllSettings( strDestBuffer, dwBufferSize, bNewLines, pRoot->m_pFirstChild );
        }
        else
        {
            DWORD dwCurrentLen = (DWORD)strlen( strDestBuffer );
            CHAR* strCurrentSpot = strDestBuffer + dwCurrentLen;
            const CHAR* strSettingName = pRoot->m_CommandLineOptionName.SafeString();
            DWORD dwNameLen = (DWORD)strlen( strSettingName );
            if( dwCurrentLen + dwNameLen + 1 > dwBufferSize )
                return FALSE;
            strcat_s( strDestBuffer, dwBufferSize, strSettingName );
            strcat_s( strDestBuffer, dwBufferSize, "=" );
            dwCurrentLen += ( dwNameLen + 1 );

            switch( pRoot->m_CurrentValue.m_Type )
            {
            case ExportVariant::VT_STRING:
                {
                    const CHAR* strValue = pRoot->GetValueString();
                    DWORD dwValueLen = (DWORD)strlen( strValue );
                    if( dwCurrentLen + dwValueLen + 1 > dwBufferSize )
                    {
                        *strCurrentSpot = '\0';
                        return FALSE;
                    }
                    strcat_s( strDestBuffer, dwBufferSize, strValue );
                    break;
                }
            case ExportVariant::VT_INT:
            case ExportVariant::VT_BOOL:
                {
                    INT iValue = 0;
                    if( pRoot->m_CurrentValue.m_Type == ExportVariant::VT_BOOL )
                        iValue = pRoot->GetValueBool();
                    else
                        iValue = pRoot->GetValueInt();
                    CHAR strValue[32];
                    _itoa_s( iValue, strValue, 10 );
                    DWORD dwValueLen = (DWORD)strlen( strValue );
                    if( dwCurrentLen + dwValueLen + 1 > dwBufferSize )
                    {
                        *strCurrentSpot = '\0';
                        return FALSE;
                    }
                    strcat_s( strDestBuffer, dwBufferSize, strValue );
                    break;
                }
            case ExportVariant::VT_FLOAT:
                {
                    FLOAT fValue = pRoot->GetValueFloat();
                    CHAR strValue[32];
                    sprintf_s( strValue, "%0.5f", fValue );
                    DWORD dwValueLen = (DWORD)strlen( strValue );
                    if( dwCurrentLen + dwValueLen + 1 > dwBufferSize )
                    {
                        *strCurrentSpot = '\0';
                        return FALSE;
                    }
                    strcat_s( strDestBuffer, dwBufferSize, strValue );
                    break;
                }
            }
            if( bNewLines )
            {
                strcat_s( strDestBuffer, dwBufferSize, ";\n" );
            }
            else
            {
                strcat_s( strDestBuffer, dwBufferSize, ";" );
            }
        }

        if( pRoot->m_pSibling != NULL )
        {
            bResult &= MarshalAllSettings( strDestBuffer, dwBufferSize, bNewLines, pRoot->m_pSibling );
        }

        return bResult;
    }

    BOOL ExportSettingsManager::UnMarshalAllSettings( const CHAR* strSrcBuffer )
    {
        DWORD dwSrcLen = (DWORD)strlen( strSrcBuffer );
        if( dwSrcLen == 0 )
            return TRUE;

        CHAR* strSrcTokenized = new CHAR[dwSrcLen + 1];
        strcpy_s( strSrcTokenized, dwSrcLen + 1, strSrcBuffer );

        static const CHAR* strDelimiters = ";\n";

        CHAR* strNextToken = NULL;
        CHAR* pToken = strtok_s( strSrcTokenized, strDelimiters, &strNextToken );
        while( pToken != NULL )
        {
            BOOL bProcess = TRUE;
            if( strlen( pToken ) == 0 )
                bProcess = FALSE;
            if( *pToken == '#' )
                bProcess = FALSE;

            if( bProcess )
            {
                CHAR* strSettingName = pToken;
                CHAR* strEquals = strchr( pToken, '=' );
                if( strEquals == NULL )
                    break;
                CHAR* strValue = strEquals + 1;
                *strEquals = '\0';

                ExportSettingsEntry* pEntry = FindSettingsEntry( strSettingName, TRUE, NULL );
                if( pEntry != NULL )
                {
                    ExportLog::LogMsg( 4, "Setting \"%s\" = \"%s\"", strSettingName, strValue );
                    switch( pEntry->m_CurrentValue.m_Type )
                    {
                    case ExportVariant::VT_STRING:
                        pEntry->SetValue( strValue );
                        break;
                    case ExportVariant::VT_INT:
                    case ExportVariant::VT_BOOL:
                        pEntry->SetValue( atoi( strValue ) );
                        break;
                    case ExportVariant::VT_FLOAT:
                        pEntry->SetValue( (FLOAT)atof( strValue ) );
                        break;
                    }
                }
                else
                {
                    ExportLog::LogWarning( "Did not find setting \"%s\"", strSettingName );
                }
            }
            pToken = strtok_s( NULL, strDelimiters, &strNextToken );
        }

        delete[] strSrcTokenized;

        return TRUE;
    }

    ExportSettingsEntry* ExportSettingsManager::FindSettingsEntry( ExportString SettingName, BOOL bCommandLineName, ExportSettingsEntry* pRoot )
    {
        static ExportSettingsEntry* pCachedNextEntry = NULL;
        if( pCachedNextEntry != NULL )
        {
            if( ( bCommandLineName && pCachedNextEntry->m_CommandLineOptionName == SettingName ) ||
                ( !bCommandLineName && pCachedNextEntry->m_SettingName == SettingName ) )
            {
                ExportSettingsEntry* pFound = pCachedNextEntry;
                pCachedNextEntry = pCachedNextEntry->m_pSibling;
                return pFound;
            }
        }

        if( pRoot == NULL )
        {
            DWORD dwRootCount = GetRootCategoryCount();
            for( DWORD i = 0; i < dwRootCount; ++i )
            {
                ExportSettingsEntry* pFound = FindSettingsEntry( SettingName, bCommandLineName, GetRootCategory( i ) );
                if( pFound != NULL )
                {
                    pCachedNextEntry = pFound->m_pSibling;
                    return pFound;
                }
            }
        }
        else
        {
            if( pRoot->m_Type == ExportSettingsEntry::CT_CATEGORY )
            {
                if( pRoot->m_pFirstChild != NULL )
                {
                    ExportSettingsEntry* pEntry = FindSettingsEntry( SettingName, bCommandLineName, pRoot->m_pFirstChild );
                    if( pEntry != NULL )
                    {
                        return pEntry;
                    }
                }
                if( pRoot->m_pSibling != NULL )
                {
                    return FindSettingsEntry( SettingName, bCommandLineName, pRoot->m_pSibling );
                }
            }
            if( ( bCommandLineName && pRoot->m_CommandLineOptionName == SettingName ) ||
                ( !bCommandLineName && pRoot->m_SettingName == SettingName ) )
            {
                return pRoot;
            }
            if( pRoot->m_pSibling != NULL )
            {
                return FindSettingsEntry( SettingName, bCommandLineName, pRoot->m_pSibling );
            }
        }
        return NULL;
    }

    VOID ExportSettingsManager::SetDefaultValues()
    {
        DWORD dwCount = GetRootCategoryCount();
        for( DWORD i = 0; i < dwCount; ++i )
        {
            m_RootCategories[i]->SetDefaultValue( TRUE, TRUE );
        }
    }

    BOOL ExportSettingsManager::SaveSettings( const CHAR* strFileName )
    {
        const DWORD dwBufferSize = 32 * 1024;
        CHAR* strBuffer = new CHAR[dwBufferSize];
        ZeroMemory( strBuffer, dwBufferSize * sizeof( CHAR ) );

        BOOL bSuccess = MarshalAllSettings( strBuffer, dwBufferSize, TRUE, NULL );
        if( !bSuccess )
        {
            return FALSE;
        }

        FILE* fp = NULL;
        fopen_s( &fp, strFileName, "w" );
        if( fp != NULL )
        {
            fputs( strBuffer, fp );
            fclose( fp );
            delete[] strBuffer;
            return TRUE;
        }
        else
        {
            delete[] strBuffer;
            return FALSE;
        }
    }

    BOOL ExportSettingsManager::LoadSettings( const CHAR* strFileName )
    {
        FILE* fp = NULL;
        fopen_s( &fp, strFileName, "r" );
        if( fp == NULL )
        {
            return FALSE;
        }

        const DWORD dwBufferSize = 32 * 1024;
        CHAR* strBuffer = new CHAR[dwBufferSize];
        ZeroMemory( strBuffer, dwBufferSize * sizeof( CHAR ) );

        DWORD dwReadBytes = fread( strBuffer, sizeof(CHAR), dwBufferSize, fp );
        fclose( fp );

        if( dwReadBytes == 0 )
        {
            delete[] strBuffer;
            return FALSE;
        }
        else
        {
            assert( dwReadBytes < dwBufferSize );
            strBuffer[dwReadBytes] = '\0';
            BOOL bSuccess = UnMarshalAllSettings( strBuffer );
            delete[] strBuffer;
            return bSuccess;
        }
    }

    ExportCoreSettings::ExportCoreSettings()
    {
        ExportSettingsEntry* pCategoryPlatform = g_SettingsManager.AddRootCategory( "Target Platform" );
        static const ExportEnumValue EndianEnums[] = {
            { "Big-Endian (PowerPC)", "ppc", 0 },
            { "Little-Endian (Intel)", "intel", 1 },
        };
        g_SettingsManager.AddEnum( pCategoryPlatform, "Data Endianness", "endian", 0, EndianEnums, ARRAYSIZE( EndianEnums ), &bLittleEndian );
        pCategoryPlatform->ReverseChildOrder();
        
        ExportSettingsEntry* pCategoryScene = g_SettingsManager.AddRootCategory( "Scene" );
        g_SettingsManager.AddBool( pCategoryScene, "Export Hidden Objects", "exporthiddenobjects", FALSE, &bExportHiddenObjects );
        g_SettingsManager.AddBool( pCategoryScene, "Export Frames", "exportframes", TRUE, &bExportScene );
        g_SettingsManager.AddBool( pCategoryScene, "Export Lights", "exportlights", TRUE, &bExportLights );
        g_SettingsManager.AddFloatBounded( pCategoryScene, "Light Range Scale", "lightrangescale", 1.0f, 0.0f, 1000.0f, &fLightRangeScale );
        g_SettingsManager.AddBool( pCategoryScene, "Export Cameras", "exportcameras", TRUE, &bExportCameras );
        g_SettingsManager.AddBool( pCategoryScene, "Export in Bind Pose", "exportbindpose", TRUE, &bSetBindPoseBeforeSceneParse );
        pCategoryScene->ReverseChildOrder();

        ExportSettingsEntry* pCategoryMeshes = g_SettingsManager.AddRootCategory( "Meshes" );
        g_SettingsManager.AddBool( pCategoryMeshes, "Export Meshes", "exportmeshes", TRUE, &bExportMeshes );
        g_SettingsManager.AddBool( pCategoryMeshes, "Compress Vertex Data", "compressvertexdata", TRUE, &bCompressVertexData );
        g_SettingsManager.AddBool( pCategoryMeshes, "Compute Vertex Tangent Space", "computevertextangents", TRUE, &bComputeVertexTangentSpace );
        g_SettingsManager.AddBool( pCategoryMeshes, "Export Binormals", "exportbinormals", TRUE, &bExportBinormal );
        static const ExportEnumValue VertexNormalTypes[] = {
            { "FLOAT3 (12 bytes)", "float3", D3DDECLTYPE_FLOAT3 },
            { "DEC3N (4 bytes)", "dec3n", D3DDECLTYPE_DEC3N },
            { "UBYTE4N Biased (4 bytes)", "ubyte4n", D3DDECLTYPE_UBYTE4N },
            { "SHORT4N (8 bytes)", "short4n", D3DDECLTYPE_SHORT4N },
            { "FLOAT16_4 (8 bytes)", "float16_4", D3DDECLTYPE_FLOAT16_4 },
        };
        g_SettingsManager.AddEnum( pCategoryMeshes, "Compressed Type for Normals", "compressednormaltype", D3DDECLTYPE_DEC3N, VertexNormalTypes, ARRAYSIZE( VertexNormalTypes ), (INT*)&dwNormalCompressedType );
        g_SettingsManager.AddBool( pCategoryMeshes, "Export Normals", "exportnormals", TRUE, &bExportNormals );
        g_SettingsManager.AddBool( pCategoryMeshes, "Force 32 Bit Index Buffers", "force32bitindices", FALSE, &bForceIndex32Format );
        g_SettingsManager.AddIntBounded( pCategoryMeshes, "Max UV Set Count", "maxuvsetcount", 8, 0, 8, &iMaxUVSetCount );
        g_SettingsManager.AddBool( pCategoryMeshes, "Export Bone Weights & Indices for Skinned Meshes", "exportboneweights", TRUE, &bExportSkinWeights );
        g_SettingsManager.AddBool( pCategoryMeshes, "Always Export Bone Weights & Indices for Skinned Meshes (even if no data present)", "forceboneweights", FALSE, &bForceExportSkinWeights );
        g_SettingsManager.AddBool( pCategoryMeshes, "Flip Triangle Winding", "fliptriangles", TRUE, &bFlipTriangles );
        g_SettingsManager.AddBool( pCategoryMeshes, "Invert V Texture Coordinates", "invertvtexcoord", TRUE, &bInvertTexVCoord );
        g_SettingsManager.AddString( pCategoryMeshes, "Mesh Name Decoration, applied as a prefix to mesh names", "meshnamedecoration", "Mesh", strMeshNameDecoration );

        ExportSettingsEntry* pCategoryUVAtlas = g_SettingsManager.AddCategory( pCategoryMeshes, "UV Atlas Generation" );
        g_SettingsManager.AddIntBounded( pCategoryUVAtlas, "Generate UV Atlas on Texture Coordinate Index", "generateuvatlas", -1, -1, 7, &iGenerateUVAtlasOnTexCoordIndex );
        g_SettingsManager.AddFloatBounded( pCategoryUVAtlas, "UV Atlas Max Stretch Factor", "uvatlasstretch", 0.75f, 0.0f, 1.0f, &fUVAtlasMaxStretch );
        g_SettingsManager.AddFloatBounded( pCategoryUVAtlas, "UV Atlas Gutter Size", "uvatlasgutter", 2.5f, 0.0f, 10.0f, &fUVAtlasGutter );
        g_SettingsManager.AddIntBounded( pCategoryUVAtlas, "UV Atlas Texture Size", "uvatlastexturesize", 1024, 64, 4096, &iUVAtlasTextureSize );
        pCategoryUVAtlas->ReverseChildOrder();

        ExportSettingsEntry* pCategorySubD = g_SettingsManager.AddCategory( pCategoryMeshes, "Subdivision Surfaces" );
        g_SettingsManager.AddBool( pCategorySubD, "Convert Poly Meshes to Subdivision Surfaces", "convertmeshtosubd", FALSE, &bConvertMeshesToSubD );
        pCategorySubD->ReverseChildOrder();

        pCategoryMeshes->ReverseChildOrder();

        ExportSettingsEntry* pCategoryMaterials = g_SettingsManager.AddRootCategory( "Materials" );
        g_SettingsManager.AddBool( pCategoryMaterials, "Export Materials", "exportmaterials", TRUE, &bExportMaterials );
        g_SettingsManager.AddString( pCategoryMaterials, "Default Material Name", "defaultmaterialname", "Default", strDefaultMaterialName );
        g_SettingsManager.AddBool( pCategoryMaterials, "Use Texture Compression", "texturecompression", TRUE, &bTextureCompression );
        g_SettingsManager.AddBool( pCategoryMaterials, "Generate Texture Mip Maps", "generatetexturemips", FALSE, &bGenerateTextureMipMaps );
        g_SettingsManager.AddBool( pCategoryMaterials, "Force Texture File Overwriting", "forcetextureoverwrite", FALSE, &bForceTextureOverwrite );
        g_SettingsManager.AddString( pCategoryMaterials, "Default Diffuse Map Texture Filename", "defaultdiffusemap", "default.dds", strDefaultDiffuseMapTextureName );
        g_SettingsManager.AddString( pCategoryMaterials, "Default Normal Map Texture Filename", "defaultnormalmap", "default-normalmap.dds", strDefaultNormalMapTextureName );
        pCategoryMaterials->ReverseChildOrder();

        ExportSettingsEntry* pCategoryAnimation = g_SettingsManager.AddRootCategory( "Animation" );
        g_SettingsManager.AddBool( pCategoryAnimation, "Export Animations", "exportanimations", TRUE, &bExportAnimations );
        g_SettingsManager.AddBool( pCategoryAnimation, "Optimize Animations", "optimizeanimations", TRUE, &bOptimizeAnimations );
        g_SettingsManager.AddBool( pCategoryAnimation, "Rename Animations To Match Output File Name", "renameanimations", TRUE, &bRenameAnimationsToFileName );
        g_SettingsManager.AddIntBounded( pCategoryAnimation, "Animation Baking Sample Count Per Frame", "animsamplecount", 1, 1, 10, &iAnimSampleCountPerFrame );
        g_SettingsManager.AddIntBounded( pCategoryAnimation, "Position Curve Quality", "positioncurvequality", 50, 0, 100, &iAnimPositionExportQuality );
        g_SettingsManager.AddIntBounded( pCategoryAnimation, "Orientation Curve Quality", "orientationcurvequality", 50, 0, 100, &iAnimOrientationExportQuality );
        g_SettingsManager.AddString( pCategoryAnimation, "Animation Root Node Name (default includes all nodes)", "animationrootnode", "", strAnimationRootNodeName );
        pCategoryAnimation->ReverseChildOrder();

        SetDefaultSettings();
    }

    VOID ExportCoreSettings::SetDefaultSettings()
    {
        g_SettingsManager.SetDefaultValues();
    }
}