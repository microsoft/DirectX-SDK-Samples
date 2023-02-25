//-------------------------------------------------------------------------------------
//  ExportManifest.h
//
//  Data structures to track a list of files that are important to the export process.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

namespace ATG
{
    enum ExportFileType
    {
        EFT_SCENEFILE_XML,
        EFT_SCENEFILE_BINARY,
        EFT_BINARY_RESOURCE,
        EFT_BUNDLED_RESOURCE,
        EFT_TEXTURE2D,
        EFT_TEXTURECUBE,
        EFT_TEXTUREVOLUME,
        EFT_SHADER,
    };

    enum ExportTextureOperation
    {
        ETO_NOTHING = 0,
        ETO_CONVERTFORMAT,
        ETO_BUMPMAP_TO_NORMALMAP
    };

    struct ExportFileRecord
    {
        ExportString            strSourceFileName;
        ExportString            strIntermediateFileName;
        ExportString            strResourceName;
        ExportString            strDevKitFileName;
        ExportFileType          FileType;
        D3DFORMAT               CompressedTextureFormat;
        ExportTextureOperation  TextureOperation;
    };
    typedef std::vector<ExportFileRecord> ExportFileRecordVector;

    class ExportManifest
    {
    public:
        VOID Clear() { m_Files.clear(); }
        VOID ClearFilesOfType( ExportFileType FileType );
        DWORD AddFile( const ExportFileRecord& File );
        DWORD AddFile( ExportString strSourceFileName, ExportString strIntermediateFileName, ExportFileType FileType = EFT_TEXTURE2D );
        DWORD GetFileCount() const { return (DWORD)m_Files.size(); }
        ExportFileRecord& GetFile( DWORD dwIndex ) { return m_Files[dwIndex]; }
        DWORD FindFile( ExportString strFileName ) const;

        static BOOL FileExists( const ExportPath& Path );

    protected:
        ExportFileRecordVector  m_Files;
    };

    class ExportScene;
    class ExportMaterial;
    class ExportMaterialParameter;

    class ExportTextureConverter
    {
    public:
        static VOID ProcessScene( ExportScene* pScene, ExportManifest* pManifest, const ExportPath& TextureSubPath, BOOL bIntermediateDDSFormat );
        static VOID PerformTextureFileOperations( ExportManifest* pManifest );

    protected:
        static VOID ProcessMaterial( ExportMaterial* pMaterial, ExportManifest* pManifest );
        static VOID ProcessTextureParameter( ExportMaterialParameter* pParameter, ExportManifest* pManifest );
    };
}