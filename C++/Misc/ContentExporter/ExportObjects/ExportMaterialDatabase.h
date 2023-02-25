//-------------------------------------------------------------------------------------
//  ExportMaterialDatabase.h
//
//  Data structures representing a list of custom material types that can be exposed
//  into the DCC package and assigned to objects.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once
#include "ExportBase.h"

namespace ATG
{

    enum ExportMaterialParameterType
    {
        MPT_STRING = 0,
        MPT_BOOL,
        MPT_INTEGER,
        MPT_FLOAT,
        MPT_FLOAT2,
        MPT_FLOAT3,
        MPT_FLOAT4,
        MPT_TEXTURE2D,
        MPT_TEXTURECUBE,
        MPT_TEXTUREVOLUME
    };

    static const CHAR* ExportMaterialParameterTypeNames[] = 
    {
        "string",
        "bool",
        "integer",
        "float",
        "float2",
        "float3",
        "float4",
        "texture2d",
        "texturecube",
        "texturevolume"
    };

    struct ExportMaterialParameterDefinition
    {
        ExportString                strName;
        ExportString                strDisplayName;
        ExportString                strDescription;
        ExportMaterialParameterType ParamType;
        ExportString                strDisplayHint;
        ExportString                strDefaultValue;
        ExportString                strLoaderHint;
        BOOL                        bVisibleInTool;
        BOOL                        bExportToContentFile;
        BOOL                        bDetectAlpha;
    };
    typedef std::vector<ExportMaterialParameterDefinition*> ExportMaterialParameterDefinitionVector;

    class ExportMaterialDefinition
    {
    public:
        ~ExportMaterialDefinition();
        ExportString        strName;
        ExportString        strDescription;
        ExportMaterialParameterDefinitionVector     Parameters;
    };
    typedef std::vector<ExportMaterialDefinition*> ExportMaterialDefinitionVector;

    class MaterialDatabaseReader : public ISAXCallback
    {
    public:
        MaterialDatabaseReader()
            : m_pCurrentMaterial( NULL ),
            m_pCurrentParam( NULL )
        { }
        virtual HRESULT  StartDocument() { return S_OK; }
        virtual HRESULT  EndDocument() { return S_OK; }

        virtual HRESULT  ElementBegin( CONST WCHAR* strName, UINT NameLen, 
            CONST XMLAttribute *pAttributes, UINT NumAttributes );
        virtual HRESULT  ElementContent( CONST WCHAR *strData, UINT DataLen, BOOL More );
        virtual HRESULT  ElementEnd( CONST WCHAR *strName, UINT NameLen );

        virtual HRESULT  CDATABegin( ) { return S_OK; }
        virtual HRESULT  CDATAData( CONST WCHAR *strCDATA, UINT CDATALen, BOOL bMore ) { return S_OK; }
        virtual HRESULT  CDATAEnd( ) { return S_OK; }

        virtual VOID     Error( HRESULT hError, CONST CHAR *strMessage );
    protected:
        VOID             ParseAttributes( const XMLAttribute* pAttributes, DWORD dwAttributeCount );
        const WCHAR*     FindAttribute( const WCHAR* strName );
        VOID             ProcessElementBeginContent();
        VOID             ProcessElementEnd();
    protected:
        struct ElementAttribute
        {
            WCHAR   strName[256];
            WCHAR   strValue[256];
        };
        typedef std::vector<ElementAttribute> ElementAttributeVector;

        WCHAR                   m_strCurrentElementName[256];
        BOOL                    m_bCurrentElementEndTag;
        ElementAttributeVector  m_CurrentElementAttributes;

        ExportMaterialDefinition*               m_pCurrentMaterial;
        ExportMaterialParameterDefinition*      m_pCurrentParam;
    };

    class ExportMaterialDatabase
    {
    public:
        static VOID Clear();
        static BOOL Initialize( const CHAR* strFileName );
        static const CHAR* GetDatabaseFileName();
        static DWORD GetMaterialCount();
        static const ExportMaterialDefinition* GetMaterial( DWORD dwIndex );
        static const ExportMaterialDefinition* FindMaterial( ExportString strName );
    };

}
