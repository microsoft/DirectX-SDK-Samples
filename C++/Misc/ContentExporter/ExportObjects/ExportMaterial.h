//-------------------------------------------------------------------------------------
//  ExportMaterial.h
//
//  Data structures representing parameterized materials (surface shaders).
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

#include "ExportMaterialDatabase.h"

namespace ATG
{

class ExportMaterialParameter
{
public:
    enum ExportMaterialParameterFlags
    {
        EMPF_NONE = 0,
        EMPF_BUMPMAP = 1,
        EMPF_DIFFUSEMAP = 2,
        EMPF_NORMALMAP = 4,
        EMPF_SPECULARMAP = 8,
        EMPF_ALPHACHANNEL = 16,
    };
    ExportMaterialParameter()
    {
        ZeroMemory( this, sizeof( ExportMaterialParameter ) );
    }
    ExportString                    Name;
    ExportMaterialParameterType     ParamType;
    ExportString                    Hint;
    BOOL                            bInstanceParam;
    ExportString                    ValueString;
    FLOAT                           ValueFloat[16];
    INT                             ValueInt;
    DWORD                           Flags;
};
typedef std::list<ExportMaterialParameter> MaterialParameterList;

class ExportMaterial :
    public ExportBase
{
public:
    ExportMaterial();
    ExportMaterial( ExportString name );
    ~ExportMaterial();

    VOID SetMaterialDefinition( const ExportMaterialDefinition* pDef ) { m_pMaterialDefinition = pDef; }
    const ExportMaterialDefinition* GetMaterialDefinition() const { return m_pMaterialDefinition; }

    VOID SetDefaultMaterialName( const ExportString strDefaultName ) { m_DefaultMaterialName = strDefaultName; }
    ExportString GetDefaultMaterialName() const { return m_DefaultMaterialName; }

    VOID AddParameter( CONST ExportMaterialParameter& Param ) { m_Parameters.push_back( Param ); }
    MaterialParameterList* GetParameterList() { return &m_Parameters; }
    DWORD GetParameterCount() const { return (DWORD)m_Parameters.size(); }
    ExportMaterialParameter* FindParameter( const ExportString strName );

    VOID SetTransparent( BOOL bTransparent ) { m_bTransparent = bTransparent; }
    BOOL IsTransparent() const { return m_bTransparent; }

    static LPDIRECT3DDEVICE9 GetDirect3DDevice();
    static VOID ReleaseDirect3DDevice();

    static ExportString GetDefaultDiffuseMapTextureName();
    static ExportString GetDefaultNormalMapTextureName();

protected:
    const ExportMaterialDefinition*     m_pMaterialDefinition;
    ExportString                        m_DefaultMaterialName;
    MaterialParameterList               m_Parameters;
    BOOL                                m_bTransparent;
};

};

