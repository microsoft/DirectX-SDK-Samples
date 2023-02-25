//-------------------------------------------------------------------------------------
//  ExportSettings.h
//
//  Generic data structures for export settings, including the settings themselves and
//  data structures for annotating, organizing, and modifying the settings variables.
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

namespace ATG
{
    static const DWORD SETTINGS_STRING_LENGTH = 256;

    class ExportVariant
    {
    public:
        enum VariantType
        {
            VT_NONE,
            VT_BOOL,
            VT_FLOAT,
            VT_INT,
            VT_STRING            
        };
    public:
        ExportVariant()
            : m_Type( VT_NONE )
        { }
        union
        {
            FLOAT           m_fValue;
            INT             m_iValue;
            BOOL            m_bValue;
        };
        ExportMutableString m_strValue;
        VariantType         m_Type;
    };

    struct ExportEnumValue
    {
        const CHAR* strLabel;
        const CHAR* strCommandLine;
        INT         iValue;
    };

    class ExportSettingsEntry
    {
    public:
        enum ControlType
        {
            CT_CATEGORY,
            CT_CHECKBOX,
            CT_BOUNDEDINTSLIDER,
            CT_BOUNDEDFLOATSLIDER,
            CT_STRING,
            CT_ENUM,
        };
    public:
        ExportString        m_DisplayName;
        ExportString        m_SettingName;
        ExportString        m_CommandLineOptionName;
        ControlType         m_Type;

        ExportVariant       m_DefaultValue;
        ExportVariant       m_CurrentValue;
        VOID*               m_pLinkedCurrentValue;

        ExportVariant       m_MinValue;
        ExportVariant       m_MaxValue;

        const ExportEnumValue*  m_pEnumValues;
        DWORD                   m_dwEnumValueCount;

        ExportSettingsEntry*    m_pFirstChild;
        ExportSettingsEntry*    m_pSibling;
    public:
        ExportSettingsEntry();
        ~ExportSettingsEntry();

        VOID SetDefaultValue( BOOL bSetChildren = FALSE, BOOL bSetSiblings = FALSE );
        FLOAT GetValueFloat() const;
        BOOL GetValueBool() const;
        INT GetValueInt() const;
        const CHAR* GetValueString() const;
        VOID SetValue( FLOAT fValue );
        VOID SetValue( INT iValue );
        VOID SetValue( const CHAR* strValue );

        VOID AddChild( ExportSettingsEntry* pNewChild )
        {
            pNewChild->m_pSibling = m_pFirstChild;
            m_pFirstChild = pNewChild;
        }
        VOID ReverseChildOrder();
        VOID CreateSettingName();

    protected:
        VOID* GetCurrentValue()
        {
            return m_pLinkedCurrentValue ? m_pLinkedCurrentValue : &m_CurrentValue;
        }
        const VOID* GetCurrentValue() const
        {
            return m_pLinkedCurrentValue ? m_pLinkedCurrentValue : &m_CurrentValue;
        }
    };

    class ExportSettingsManager
    {
    public:
        ExportSettingsManager() {}
        ~ExportSettingsManager();

        ExportSettingsEntry* AddRootCategory( ExportString Caption );
        ExportSettingsEntry* AddCategory( ExportSettingsEntry* pParentCategory, ExportString Caption );
        BOOL                 DeleteRootCategoryAndChildren( ExportSettingsEntry* pCategory );

        ExportSettingsEntry* AddBool( ExportSettingsEntry* pCategory, ExportString Caption, ExportString CmdLine, BOOL bDefaultValue, BOOL* pLinkedValue = NULL );
        ExportSettingsEntry* AddFloatBounded( ExportSettingsEntry* pCategory, ExportString Caption, ExportString CmdLine, FLOAT fDefaultValue, FLOAT fMin, FLOAT fMax, FLOAT* pLinkedValue = NULL );
        ExportSettingsEntry* AddIntBounded( ExportSettingsEntry* pCategory, ExportString Caption, ExportString CmdLine, INT iDefaultValue, INT iMin, INT iMax, INT* pLinkedValue = NULL );
        ExportSettingsEntry* AddString( ExportSettingsEntry* pCategory, ExportString Caption, ExportString CmdLine, const CHAR* strDefaultValue, CHAR* pLinkedValue = NULL );
        ExportSettingsEntry* AddEnum( ExportSettingsEntry* pCategory, ExportString Caption, ExportString CmdLine, INT iDefaultValue, const ExportEnumValue* pEnumValues, DWORD dwEnumValueCount, INT* pLinkedValue = NULL );

        VOID SetDefaultValues();

        DWORD GetRootCategoryCount() const { return (DWORD)m_RootCategories.size(); }
        ExportSettingsEntry* GetRootCategory( DWORD dwIndex ) { return m_RootCategories[ dwIndex ]; }

        BOOL LoadSettings( const CHAR* strFileName );
        BOOL SaveSettings( const CHAR* strFileName );

        BOOL MarshalAllSettings( CHAR* strDestBuffer, DWORD dwBufferSize, BOOL bNewLines, ExportSettingsEntry* pRoot = NULL );
        BOOL UnMarshalAllSettings( const CHAR* strSrcBuffer );

    protected:
        ExportSettingsEntry* FindSettingsEntry( ExportString SettingName, BOOL bCommandLineName, ExportSettingsEntry* pRoot );

        typedef std::vector<ExportSettingsEntry*> SettingsEntryArray;
        SettingsEntryArray  m_RootCategories;
    };

    extern ExportSettingsManager        g_SettingsManager;

    class ExportCoreSettings
    {
    public:
        ExportCoreSettings();
        VOID SetDefaultSettings();
    public:
        BOOL        bFlipTriangles;
        BOOL        bInvertTexVCoord;
        BOOL        bExportScene;
        BOOL        bExportLights;
        BOOL        bExportCameras;
        BOOL        bExportMaterials;
        BOOL        bExportMeshes;
        BOOL        bExportHiddenObjects;
        BOOL        bExportAnimations;
        BOOL        bLittleEndian;
        BOOL        bExportNormals;
        BOOL        bForceIndex32Format;
        INT         iMaxUVSetCount;
        BOOL        bExportSkinWeights;
        BOOL        bForceExportSkinWeights;
        BOOL        bComputeVertexTangentSpace;
        BOOL        bExportBinormal;
        BOOL        bSetBindPoseBeforeSceneParse;
        INT         iAnimSampleCountPerFrame;
        INT         iAnimPositionExportQuality;
        INT         iAnimOrientationExportQuality;
        BOOL        bRenameAnimationsToFileName;
        CHAR        strDefaultMaterialName[SETTINGS_STRING_LENGTH];
        CHAR        strDefaultDiffuseMapTextureName[SETTINGS_STRING_LENGTH];
        CHAR        strDefaultNormalMapTextureName[SETTINGS_STRING_LENGTH];
        BOOL        bCompressVertexData;
        DWORD       dwNormalCompressedType;
        DWORD       dwTexCoordCompressedType;
        DWORD       dwPositionCompressedType;
        BOOL        bTextureCompression;
        BOOL        bGenerateTextureMipMaps;
        BOOL        bForceTextureOverwrite;
        BOOL        bConvertMeshesToSubD;
        INT         iGenerateUVAtlasOnTexCoordIndex;
        FLOAT       fUVAtlasMaxStretch;
        FLOAT       fUVAtlasGutter;
        INT         iUVAtlasTextureSize;
        FLOAT       fLightRangeScale;
        CHAR        strMeshNameDecoration[SETTINGS_STRING_LENGTH];
        CHAR        strAnimationRootNodeName[SETTINGS_STRING_LENGTH];
        BOOL        bOptimizeAnimations;
    };

    extern ExportCoreSettings       g_ExportCoreSettings;
}