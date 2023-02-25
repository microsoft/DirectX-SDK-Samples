//-------------------------------------------------------------------------------------
//  ExportBase.h
//
//  Base functionality for all of the export objects.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once
#ifndef EXPORTBASE_H
#define EXPORTBASE_H

#if _SECURE_SCL != 0
#error SECURE_SCL is not properly set.
#endif

#include "ExportString.h"

namespace ATG
{

class ExportTransform
{
public:
    VOID SetIdentity();
    BOOL InitializeFromFloatsTransposed( FLOAT* pSixteenFloats );
    BOOL InitializeFromFloats( FLOAT* pSixteenFloats );
    CONST D3DXMATRIX& Matrix() CONST { return m_Matrix; }
    D3DXMATRIX& Matrix() { return m_Matrix; }
    VOID Multiply( CONST D3DXMATRIX& Matrix );
    VOID Normalize();
    const D3DXVECTOR3& Position() const { return m_Position; }
    const D3DXQUATERNION& Orientation() const { return m_Orientation; }
    const D3DXVECTOR3& Scale() const { return m_Scale; }
protected:
    BOOL DecomposeMatrix();
    VOID ComposeMatrix();
protected:
    D3DXMATRIX      m_Matrix;
    D3DXVECTOR3     m_Position;
    D3DXQUATERNION  m_Orientation;
    D3DXVECTOR3     m_Scale;
};

struct ExportAttribute
{
    enum AttributeType
    {
        AT_UNKNOWN = 0,
        AT_STRING,
        AT_INT,
        AT_FLOAT,
        AT_VEC4
    };
    ExportAttribute()
    {
        ZeroMemory( this, sizeof( ExportAttribute ) );
    }
    ExportString    Name;
    AttributeType   Type;
    ExportString    strValue;
    INT             iValue;
    FLOAT           fValue;
    D3DXVECTOR4     vecValue;
};

typedef std::list< ExportAttribute* > ExportAttributeList;


class ExportBase
{
public:
    ExportBase() : m_pDCCObject(NULL) {}
    ExportBase( ExportString name ) : m_Name( name ), m_pDCCObject(NULL) {}
    virtual ~ExportBase();

    ExportString GetName() CONST { return m_Name; }
    VOID SetName( ExportString newName ) { m_Name = newName; }

    VOID ClearAttributes();
    virtual BOOL AddAttribute( ExportAttribute* pAttribute );
    BOOL DeleteAttribute( CONST CHAR* strName );
    UINT GetAttributeCount() CONST;
    ExportAttribute* GetAttributeByIndex( UINT uIndex ) CONST;
    ExportAttribute* GetAttribute( CONST CHAR* strName ) CONST;

    VOID SetDCCObject( VOID* pDCCObject ) { m_pDCCObject = pDCCObject; }
    VOID* GetDCCObject() CONST { return m_pDCCObject; }
protected:
    ExportString        m_Name;
    ExportAttributeList m_Attributes;
    VOID*               m_pDCCObject;
};

};

#endif
