//-------------------------------------------------------------------------------------
//  ExportMaterialDatabase.cpp
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "exportmaterialdatabase.h"

namespace ATG
{

ExportMaterialDefinitionVector  g_Materials;
CHAR g_strMaterialDBFileName[ MAX_PATH ] = {0};

#define MATCH_ELEMENT_NAME(x) ( _wcsicmp( m_strCurrentElementName, x ) == 0 )

const CHAR* ConvertString( const WCHAR* strData, DWORD dwLength = 0 )
{
    static CHAR strText[256];
    if( strData == NULL )
        strData = L"";
    if( dwLength == 0 )
        dwLength = (DWORD)wcslen( strData );
    assert( dwLength < 255 );
    WideCharToMultiByte( CP_ACP, 0, strData, dwLength, strText, 256, NULL, NULL );
    strText[ dwLength ] = '\0';
    return strText;
}

VOID CopyString( const WCHAR* strData, DWORD dwLength, WCHAR* strDest, DWORD dwDestLength )
{
    wcsncpy_s( strDest, dwDestLength, strData, dwLength );
    strDest[ dwLength ] = L'\0';
}

HRESULT MaterialDatabaseReader::ElementBegin( CONST WCHAR* strName, UINT NameLen, CONST XMLAttribute *pAttributes, UINT NumAttributes )
{
    ProcessElementBeginContent();

    CopyString( strName, NameLen, m_strCurrentElementName, ARRAYSIZE(m_strCurrentElementName) );
    m_bCurrentElementEndTag = FALSE;

    ParseAttributes( pAttributes, NumAttributes );

    return S_OK;
}

HRESULT MaterialDatabaseReader::ElementContent( CONST WCHAR *strData, UINT DataLen, BOOL More )
{
    return S_OK;
}

HRESULT MaterialDatabaseReader::ElementEnd( CONST WCHAR *strName, UINT NameLen )
{
    ProcessElementBeginContent();

    CopyString( strName, NameLen, m_strCurrentElementName, ARRAYSIZE(m_strCurrentElementName) );
    m_bCurrentElementEndTag = TRUE;

    ProcessElementEnd();

    m_strCurrentElementName[0] = L'\0';

    return S_OK;
}

VOID MaterialDatabaseReader::ParseAttributes( const XMLAttribute* pAttributes, DWORD dwAttributeCount )
{
    m_CurrentElementAttributes.clear();
    for( DWORD i = 0; i < dwAttributeCount; ++i )
    {
        ElementAttribute ea;
        CopyString( pAttributes[i].strName, pAttributes[i].NameLen, ea.strName, ARRAYSIZE(ea.strName) );
        CopyString( pAttributes[i].strValue, pAttributes[i].ValueLen, ea.strValue, ARRAYSIZE(ea.strValue) );
        m_CurrentElementAttributes.push_back( ea );
    }
}

const WCHAR* MaterialDatabaseReader::FindAttribute( const WCHAR* strName )
{
    DWORD dwCount = (DWORD)m_CurrentElementAttributes.size();
    for( DWORD i = 0; i < dwCount; ++i )
    {
        if( _wcsicmp( strName, m_CurrentElementAttributes[i].strName ) == 0 )
            return m_CurrentElementAttributes[i].strValue;
    }
    return NULL;
}

VOID MaterialDatabaseReader::Error( HRESULT hError, CONST CHAR *strMessage )
{

}

ExportMaterialParameterType ConvertType( const WCHAR* strType )
{
    if( strType == NULL )
        return MPT_STRING;

    if( _wcsicmp( strType, L"bool" ) == 0 )
        return MPT_BOOL;
    else if( _wcsicmp( strType, L"texture2d" ) == 0 )
        return MPT_TEXTURE2D;
    else if( _wcsicmp( strType, L"float4" ) == 0 )
        return MPT_FLOAT4;
    else if( _wcsicmp( strType, L"float2" ) == 0 )
        return MPT_FLOAT2;
    else if( _wcsicmp( strType, L"float3" ) == 0 )
        return MPT_FLOAT3;
    else if( _wcsicmp( strType, L"float" ) == 0 )
        return MPT_FLOAT;
    else if( _wcsicmp( strType, L"integer" ) == 0 )
        return MPT_INTEGER;
    else if( _wcsicmp( strType, L"texturecube" ) == 0 )
        return MPT_TEXTURECUBE;
    else if( _wcsicmp( strType, L"texturevolume" ) == 0 )
        return MPT_TEXTUREVOLUME;
    else if( _wcsicmp( strType, L"texture" ) == 0 )
        return MPT_TEXTURE2D;

    return MPT_STRING;
}

BOOL ConvertBool( const WCHAR* strBool, BOOL bDefaultValue )
{
    if( strBool == NULL )
        return bDefaultValue;
    WCHAR FirstChar = strBool[0];
    return ( FirstChar == L'T' || FirstChar == L't' || FirstChar == L'1' );
}

VOID MaterialDatabaseReader::ProcessElementBeginContent()
{
    if( MATCH_ELEMENT_NAME( L"Material" ) )
    {
        m_pCurrentMaterial = NULL;

        const WCHAR* strName = FindAttribute( L"Name" );
        if( strName == NULL )
            return;

        m_pCurrentMaterial = new ExportMaterialDefinition();
        m_pCurrentMaterial->strName = ConvertString( strName );

        const WCHAR* strDesc = FindAttribute( L"Description" );
        if( strDesc != NULL )
            m_pCurrentMaterial->strDescription = ConvertString( strDesc );

        g_Materials.push_back( m_pCurrentMaterial );
        return;
    }
    else if( MATCH_ELEMENT_NAME( L"Parameter" ) )
    {
        if( m_pCurrentMaterial == NULL )
            return;

        if( m_pCurrentParam != NULL )
            return;

        const WCHAR* strName = FindAttribute( L"Name" );
        if( strName == NULL )
            return;

        m_pCurrentParam = new ExportMaterialParameterDefinition();
        m_pCurrentParam->strName = ConvertString( strName );

        m_pCurrentMaterial->Parameters.push_back( m_pCurrentParam );

        const WCHAR* strDisplayName = FindAttribute( L"DisplayName" );
        if( strDisplayName != NULL )
            m_pCurrentParam->strDisplayName = ConvertString( strDisplayName );
        else
            m_pCurrentParam->strDisplayName = m_pCurrentParam->strName;

        const WCHAR* strDesc = FindAttribute( L"Description" );
        m_pCurrentParam->strDescription = ConvertString( strDesc );

        const WCHAR* strDisplayHint = FindAttribute( L"DisplayHint" );
        if( strDisplayHint == NULL || wcslen( strDisplayHint ) < 1 )
            m_pCurrentParam->strDisplayHint = " ";
        else
            m_pCurrentParam->strDisplayHint = ConvertString( strDisplayHint );

        const WCHAR* strLoaderHint = FindAttribute( L"LoadHint" );
        m_pCurrentParam->strLoaderHint = ConvertString( strLoaderHint );

        const WCHAR* strType = FindAttribute( L"Type" );
        m_pCurrentParam->ParamType = ConvertType( strType );

        const WCHAR* strVisible = FindAttribute( L"ToolVisible" );
        m_pCurrentParam->bVisibleInTool = ConvertBool( strVisible, FALSE );

        const WCHAR* strExport = FindAttribute( L"Export" );
        m_pCurrentParam->bExportToContentFile = ConvertBool( strExport, TRUE );

        const WCHAR* strDetectAlpha = FindAttribute( L"DetectAlpha" );
        m_pCurrentParam->bDetectAlpha = ConvertBool( strDetectAlpha, FALSE );

        const WCHAR* strDefaultValue = FindAttribute( L"DefaultValue" );
        m_pCurrentParam->strDefaultValue = ConvertString( strDefaultValue );
        return;
    }
}

VOID MaterialDatabaseReader::ProcessElementEnd()
{
    if( MATCH_ELEMENT_NAME( L"Material" ) )
    {
        assert( m_pCurrentMaterial != NULL );
        assert( m_pCurrentParam == NULL );
        m_pCurrentMaterial = NULL;
    }
    else if( MATCH_ELEMENT_NAME( L"Parameter" ) )
    {
        assert( m_pCurrentMaterial != NULL );
        assert( m_pCurrentParam != NULL );
        m_pCurrentParam = NULL;
    }
}

ExportMaterialDefinition::~ExportMaterialDefinition()
{
    DWORD dwCount = (DWORD)Parameters.size();
    for( DWORD i = 0; i < dwCount; ++i )
        delete Parameters[i];
    Parameters.clear();
}

VOID ExportMaterialDatabase::Clear()
{
    for( DWORD i = 0; i < g_Materials.size(); ++i )
    {
        delete g_Materials[i];
    }
    g_Materials.clear();
}

BOOL ExportMaterialDatabase::Initialize( const CHAR* strFileName )
{
    MaterialDatabaseReader mdr;
    XMLParser xp;
    xp.RegisterSAXCallbackInterface( &mdr );
    HRESULT hr = xp.ParseXMLFile( strFileName );
    if( hr == S_OK )
    {
        strcpy_s( g_strMaterialDBFileName, strFileName );
    }
    return ( hr == S_OK );
}

const CHAR* ExportMaterialDatabase::GetDatabaseFileName()
{
    return g_strMaterialDBFileName;
}

DWORD ExportMaterialDatabase::GetMaterialCount()
{
    return (DWORD)g_Materials.size();
}

const ExportMaterialDefinition* ExportMaterialDatabase::GetMaterial( DWORD dwIndex )
{
    assert( dwIndex < GetMaterialCount() );
    return g_Materials[ dwIndex ];
}

const ExportMaterialDefinition* ExportMaterialDatabase::FindMaterial( ExportString strName )
{
    DWORD dwCount = GetMaterialCount();
    for( DWORD i = 0; i < dwCount; ++i )
    {
        if( g_Materials[i]->strName == strName )
            return g_Materials[i];
    }
    return NULL;
}

}
