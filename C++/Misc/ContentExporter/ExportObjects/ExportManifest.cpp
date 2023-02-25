//-------------------------------------------------------------------------------------
//  ExportManifest.cpp
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "ExportManifest.h"
#include "ExportObjects.h"

extern ExportPath g_CurrentInputFileName;
extern ExportPath g_CurrentOutputFileName;

ExportPath g_TextureSubPath;
BOOL g_bIntermediateDDSFormat = TRUE;
extern ATG::ExportScene* g_pScene;

namespace ATG
{
    DWORD ExportManifest::AddFile( ExportString strSourceFileName, ExportString strIntermediateFileName, ExportFileType FileType )
    {
        ExportFileRecord Record;
        Record.strSourceFileName = strSourceFileName;
        Record.strIntermediateFileName = strIntermediateFileName;
        Record.FileType = FileType;
        return AddFile( Record );
    }

    DWORD ExportManifest::AddFile( const ExportFileRecord& File )
    {
        DWORD dwIndex = FindFile( File.strIntermediateFileName );
        if( dwIndex != (DWORD)-1 )
            return dwIndex;
        dwIndex = (DWORD)m_Files.size();
        m_Files.push_back( File );
        return dwIndex;
    }

    DWORD ExportManifest::FindFile( ExportString strIntermediateFileName ) const
    {
        for( DWORD i = 0; i < m_Files.size(); i++ )
        {
            if( m_Files[i].strIntermediateFileName == strIntermediateFileName )
                return i;
        }
        return (DWORD)-1;
    }

    VOID ExportManifest::ClearFilesOfType( ExportFileType FileType )
    {
        ExportFileRecordVector NewFileList;
        for( DWORD i = 0; i < m_Files.size(); i++ )
        {
            if( m_Files[i].FileType != FileType )
                NewFileList.push_back( m_Files[i] );
        }
        m_Files.clear();
        m_Files = NewFileList;
    }

    VOID ExportTextureConverter::ProcessScene( ExportScene* pScene, ExportManifest* pManifest, const ExportPath& TextureSubPath, BOOL bIntermediateDDSFormat )
    {
        g_TextureSubPath = TextureSubPath;
        g_bIntermediateDDSFormat = bIntermediateDDSFormat;

        DWORD dwMaterialCount = pScene->GetMaterialCount();
        for( DWORD i = 0; i < dwMaterialCount; ++i )
        {
            ProcessMaterial( pScene->GetMaterial( i ), pManifest );
        }
    }

    VOID ExportTextureConverter::ProcessMaterial( ExportMaterial* pMaterial, ExportManifest* pManifest )
    {
        MaterialParameterList* pParameters = pMaterial->GetParameterList();
        MaterialParameterList::iterator iter = pParameters->begin();
        MaterialParameterList::iterator end = pParameters->end();
        while( iter != end )
        {
            ExportMaterialParameter* pParameter = &(*iter);
            if( pMaterial->IsTransparent() && pParameter->Flags & ExportMaterialParameter::EMPF_DIFFUSEMAP )
            {
                pParameter->Flags |= ExportMaterialParameter::EMPF_ALPHACHANNEL;
            }
            ProcessTextureParameter( pParameter, pManifest );
            ++iter;
        }
    }

    BOOL ExportManifest::FileExists( const ExportPath& Path )
    {
        HANDLE hFile = CreateFile( (const CHAR*)Path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if( hFile != INVALID_HANDLE_VALUE )
        {
            CloseHandle( hFile );
            return TRUE;
        }
        return FALSE;
    }

    VOID ExportTextureConverter::ProcessTextureParameter( ExportMaterialParameter* pParameter, ExportManifest* pManifest )
    {
        switch( pParameter->ParamType )
        {
        case MPT_TEXTURE2D:
            break;
        case MPT_TEXTURECUBE:
            break;
        case MPT_TEXTUREVOLUME:
            break;
        default:
            return;
        }

        // Skip processing if it's the default texture.
        if( pParameter->ValueString == ExportMaterial::GetDefaultDiffuseMapTextureName() )
            return;
        if( pParameter->ValueString == ExportMaterial::GetDefaultNormalMapTextureName() )
            return;

        // Look for our texture in the location specified by the content file.
        ExportPath SourceFileName( pParameter->ValueString.SafeString() );
        if( !ExportManifest::FileExists( SourceFileName ) )
        {
            ExportPath DifferentSourceFileName( g_CurrentInputFileName );
            DifferentSourceFileName.ChangeFileNameWithExtension( SourceFileName );

            if( !ExportManifest::FileExists( DifferentSourceFileName ) )
            {
                ExportLog::LogWarning( "Source texture file \"%s\" could not be found.", (const CHAR*)SourceFileName );
                return;
            }
            else
            {
                SourceFileName = DifferentSourceFileName;
            }
        }

        // At this point we have a valid file in SourceFileName.
        // Next we build an intermediate file name, which is the content file copied to a location parallel to the output scene file.
        ExportPath IntermediateFileName( g_CurrentOutputFileName );
        IntermediateFileName.Append( g_TextureSubPath );
        IntermediateFileName.ChangeFileNameWithExtension( SourceFileName );

        // IntermediateFileName is now complete, but we may need to perform a texture format conversion.
        const CHAR* strSourceExtension = IntermediateFileName.GetExtension();

        // Check if the texture is already in the desired file format; otherwise, we will need to convert.
        ExportTextureOperation TexOperation = ETO_NOTHING;
        const CHAR* strDesiredTextureFileFormat = "dds";
        if( !g_bIntermediateDDSFormat )
        {
            strDesiredTextureFileFormat = "tga";
        }
        if( _stricmp( strSourceExtension, strDesiredTextureFileFormat ) != 0 )
        {
            TexOperation = ETO_CONVERTFORMAT;
            IntermediateFileName.ChangeExtension( strDesiredTextureFileFormat );
            DWORD dwFoundFile = pManifest->FindFile( (const CHAR*)IntermediateFileName );
            if( dwFoundFile != (DWORD)-1 )
            {
                const ExportFileRecord& fr = pManifest->GetFile( dwFoundFile );
                if( fr.strSourceFileName != ExportString( (const CHAR*)SourceFileName ) )
                {
                    // File collision detected.
                    // For example, texture.jpg and texture.bmp are both being converted to texture.dds.
                    // But they are different textures.
                    // Therefore, give a unique label to this conversion, to avoid the name collision.
                    CHAR strUniqueLabel[10];
                    sprintf_s( strUniqueLabel, "_%s", strSourceExtension );
                    IntermediateFileName.AppendToFileName( strUniqueLabel );
                }
            }
        }

        // If the file requires processing, that will change the name of the intermediate
        // file and possibly the extension too.
        if( pParameter->Flags & ExportMaterialParameter::EMPF_BUMPMAP )
        {
            IntermediateFileName.AppendToFileName( "_normal" );
            TexOperation = ETO_BUMPMAP_TO_NORMALMAP;
        }

        // IntermediateFileName is now ready.
        // Next we build the resource file name, which is the filename used by the title to load this texture.
        // We will use the intermediate file name without its path.

        ExportPath ResourceFileName( IntermediateFileName.GetFileName() );
        pParameter->ValueString = (const CHAR*)ResourceFileName;

        // Determine the proper texture compression format.
        D3DFORMAT CompressedTextureFormat = D3DFMT_DXT1;
        if( pParameter->Flags & ExportMaterialParameter::EMPF_ALPHACHANNEL )
        {
            CompressedTextureFormat = D3DFMT_DXT5;
        }
        if( !g_pScene->Settings().bTextureCompression )
        {
            CompressedTextureFormat = D3DFMT_A8R8G8B8;
        }

        // Build the export file record for the manifest.
        ExportFileRecord fr;
        fr.strSourceFileName = SourceFileName;
        fr.strIntermediateFileName = IntermediateFileName;
        fr.strResourceName = ResourceFileName;
        fr.TextureOperation = TexOperation;
        fr.CompressedTextureFormat = CompressedTextureFormat;
        ExportPath DevKitFileName( g_TextureSubPath );
        DevKitFileName.ChangeFileNameWithExtension( ResourceFileName );
        fr.strDevKitFileName = DevKitFileName;

        switch( pParameter->ParamType )
        {
        case MPT_TEXTURE2D:
            fr.FileType = EFT_TEXTURE2D;
            break;
        case MPT_TEXTURECUBE:
            fr.FileType = EFT_TEXTURECUBE;
            break;
        case MPT_TEXTUREVOLUME:
            fr.FileType = EFT_TEXTUREVOLUME;
            break;
        }

        pManifest->AddFile( fr );
    }

    VOID ConvertImageFormat( LPDIRECT3DDEVICE9 pd3dDevice, const CHAR* strSourceFileName, const CHAR* strDestFileName, D3DFORMAT CompressedFormat )
    {
        assert( pd3dDevice != NULL );

        if( CompressedFormat != D3DFMT_A8R8G8B8 )
        {
            ExportLog::LogMsg( 4, "Compressing and converting file \"%s\" to file \"%s\".", strSourceFileName, strDestFileName );
        }
        else
        {
            ExportLog::LogMsg( 4, "Converting file \"%s\" to file \"%s\".", strSourceFileName, strDestFileName );
        }

        DWORD dwMipCount = 1;
        if( g_pScene->Settings().bGenerateTextureMipMaps )
        {
            dwMipCount = 0;
        }

        // Load texture from source file.
        LPDIRECT3DTEXTURE9 pTexture = NULL;
        HRESULT hr = D3DXCreateTextureFromFileEx( pd3dDevice, 
            strSourceFileName, 
            D3DX_DEFAULT_NONPOW2, 
            D3DX_DEFAULT_NONPOW2,
            dwMipCount,
            0,
            CompressedFormat,
            D3DPOOL_MANAGED,
            D3DX_FILTER_NONE,
            D3DX_DEFAULT,
            0,
            NULL,
            NULL,
            &pTexture );

        if( FAILED( hr ) || pTexture == NULL )
        {
            ExportLog::LogError( "Could not load texture \"%s\".", strSourceFileName );
            return;
        }

        D3DXIMAGE_FILEFORMAT FileFormat = D3DXIFF_DDS;
        if( strstr( strDestFileName, ".tga" ) != NULL )
        {
            FileFormat = D3DXIFF_TGA;
        }

        // Save texture to destination file.
        hr = D3DXSaveTextureToFile( strDestFileName, FileFormat, pTexture, NULL );
        if( FAILED( hr ) )
        {
            ExportLog::LogError( "Could not write texture to file \"%s\".", strDestFileName );
        }
        pTexture->Release();
    }


    VOID CreateNormalMapFromBumpMap( LPDIRECT3DDEVICE9 pd3dDevice, const CHAR* strSourceFileName, const CHAR* strDestFileName, D3DFORMAT CompressedFormat )
    {
        assert( pd3dDevice != NULL );

        ExportLog::LogMsg( 4, "Converting bump map file \"%s\" to normal map file %s.", strSourceFileName, strDestFileName );

        DWORD dwMipCount = 1;
        if( g_pScene->Settings().bGenerateTextureMipMaps )
        {
            dwMipCount = 0;
        }

        // Load texture from source file.
        LPDIRECT3DTEXTURE9 pTexture = NULL;
        D3DXCreateTextureFromFileEx( pd3dDevice, 
            strSourceFileName, 
            D3DX_DEFAULT_NONPOW2, 
            D3DX_DEFAULT_NONPOW2,
            1,
            0,
            D3DFMT_A8R8G8B8,
            D3DPOOL_MANAGED,
            D3DX_FILTER_NONE,
            D3DX_DEFAULT,
            0,
            NULL,
            NULL,
            &pTexture );

        if( pTexture == NULL )
        {
            ExportLog::LogError( "Could not load texture \"%s\".", strSourceFileName );
        }

        D3DSURFACE_DESC SurfDesc;
        pTexture->GetLevelDesc( 0, &SurfDesc );
        LPDIRECT3DTEXTURE9 pDestTexture = NULL;
        D3DXCreateTexture( pd3dDevice, SurfDesc.Width, SurfDesc.Height, dwMipCount, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pDestTexture );

        HRESULT hr = D3DXComputeNormalMap( pDestTexture, pTexture, NULL, 0, D3DX_CHANNEL_LUMINANCE, 10.0f );
        if( FAILED( hr ) )
        {
            ExportLog::LogError( "Could not compute normal map." );
        }

        if( g_pScene->Settings().bGenerateTextureMipMaps )
        {
            hr = D3DXFilterTexture( pDestTexture, NULL, 0, D3DX_DEFAULT );
            if( FAILED(hr) )
            {
                ExportLog::LogError( "Could not create normal map mip maps." );
            }
        }

        // Save texture to destination file.
        D3DXSaveTextureToFile( strDestFileName, D3DXIFF_DDS, pDestTexture, NULL );
        pTexture->Release();
        pDestTexture->Release();
    }


    VOID ExportTextureConverter::PerformTextureFileOperations( ExportManifest* pManifest )
    {
        LPDIRECT3DDEVICE9 pd3dDevice = ExportMaterial::GetDirect3DDevice();

        if( g_pScene->Settings().bForceTextureOverwrite )
        {
            ExportLog::LogMsg( 4, "Reprocessing and overwriting all destination textures." );
        }

        for( DWORD i = 0; i < pManifest->GetFileCount(); i++ )
        {
            ExportFileRecord& File = pManifest->GetFile( i );
            if( File.FileType != EFT_TEXTURE2D &&
                File.FileType != EFT_TEXTURECUBE &&
                File.FileType != EFT_TEXTUREVOLUME )
                continue;

            if( File.strSourceFileName == File.strIntermediateFileName )
                continue;

            if( ExportManifest::FileExists( File.strIntermediateFileName.SafeString() ) && !g_pScene->Settings().bForceTextureOverwrite )
            {
                ExportLog::LogMsg( 4, "Destination texture file \"%s\" already exists.", File.strIntermediateFileName );
                continue;
            }

            switch( File.TextureOperation )
            {
            case ETO_NOTHING:
                // Copy file to intermediate location.
                ExportLog::LogMsg( 4, "Copying texture \"%s\" to \"%s\"...", File.strSourceFileName, File.strIntermediateFileName );
                CopyFile( File.strSourceFileName, File.strIntermediateFileName, FALSE );
                ExportLog::LogMsg( 4, "Texture copy complete." );
                break;
            case ETO_CONVERTFORMAT:
                // Convert source file to intermediate location.
                ConvertImageFormat( pd3dDevice, File.strSourceFileName, File.strIntermediateFileName, File.CompressedTextureFormat );
                break;
            case ETO_BUMPMAP_TO_NORMALMAP:
                // Convert source file to a normal map, copy to intermediate file location.
                CreateNormalMapFromBumpMap( pd3dDevice, File.strSourceFileName, File.strIntermediateFileName, File.CompressedTextureFormat );
                break;
            }
        }

        ExportMaterial::ReleaseDirect3DDevice();
    }
}

