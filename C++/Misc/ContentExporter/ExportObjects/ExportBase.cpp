//-------------------------------------------------------------------------------------
//  ExportBase.cpp
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "exportbase.h"
#include "ExportScene.h"

extern ATG::ExportScene* g_pScene;

namespace ATG
{

VOID ExportTransform::SetIdentity()
{
    D3DXMatrixIdentity( &m_Matrix );
    m_Position = D3DXVECTOR3( 0, 0, 0 );
    D3DXQuaternionIdentity( &m_Orientation );
    m_Scale = D3DXVECTOR3( 1, 1, 1 );
}

BOOL ExportTransform::InitializeFromFloatsTransposed( FLOAT* pSixteenFloats )
{
    m_Matrix = D3DXMATRIX( pSixteenFloats );
    D3DXMatrixTranspose( &m_Matrix, &m_Matrix );
    g_pScene->GetDCCTransformer()->TransformMatrix( &m_Matrix, &m_Matrix );
    return DecomposeMatrix();
}

BOOL ExportTransform::InitializeFromFloats( FLOAT* pSixteenFloats )
{
    m_Matrix = D3DXMATRIX( pSixteenFloats );
    g_pScene->GetDCCTransformer()->TransformMatrix( &m_Matrix, &m_Matrix );
    return DecomposeMatrix();
}

BOOL ExportTransform::DecomposeMatrix()
{
    D3DXVECTOR3 vAxisX( m_Matrix._11, m_Matrix._12, m_Matrix._13 );
    D3DXVECTOR3 vAxisY( m_Matrix._21, m_Matrix._22, m_Matrix._23 );
    D3DXVECTOR3 vAxisZ( m_Matrix._31, m_Matrix._32, m_Matrix._33 );
    FLOAT fScaleX = D3DXVec3LengthSq( &vAxisX );
    FLOAT fScaleY = D3DXVec3LengthSq( &vAxisY );
    FLOAT fScaleZ = D3DXVec3LengthSq( &vAxisZ );
    FLOAT fDiffXY = fabs( fScaleX - fScaleY );
    FLOAT fDiffYZ = fabs( fScaleY - fScaleZ );
    FLOAT fDiffXZ = fabs( fScaleX - fScaleZ );
    BOOL bUniformScale = TRUE;
    if( fDiffXY > 0.001f || fDiffYZ > 0.001f || fDiffXZ > 0.001f )
        bUniformScale = FALSE;
    D3DXMatrixDecompose( &m_Scale, &m_Orientation, &m_Position, &m_Matrix );
    return bUniformScale;
}

VOID ExportTransform::ComposeMatrix()
{
}

VOID ExportTransform::Multiply( CONST D3DXMATRIX& Matrix )
{
    D3DXMatrixMultiply( &m_Matrix, &m_Matrix, &Matrix );
}

VOID ExportTransform::Normalize()
{
    D3DXVECTOR3 Vec( m_Matrix._11, m_Matrix._21, m_Matrix._31 );
    D3DXVec3Normalize( &Vec, &Vec );
    m_Matrix._11 = Vec.x;
    m_Matrix._21 = Vec.y;
    m_Matrix._31 = Vec.z;

    Vec = D3DXVECTOR3( m_Matrix._12, m_Matrix._22, m_Matrix._32 );
    D3DXVec3Normalize( &Vec, &Vec );
    m_Matrix._12 = Vec.x;
    m_Matrix._22 = Vec.y;
    m_Matrix._32 = Vec.z;

    Vec = D3DXVECTOR3( m_Matrix._13, m_Matrix._23, m_Matrix._33 );
    D3DXVec3Normalize( &Vec, &Vec );
    m_Matrix._13 = Vec.x;
    m_Matrix._23 = Vec.y;
    m_Matrix._33 = Vec.z;
}

ExportBase::~ExportBase(void)
{
    ExportAttributeList::iterator iter = m_Attributes.begin();
    ExportAttributeList::iterator end = m_Attributes.end();

    while( iter != end )
    {
        delete *iter;
        ++iter;
    }
    m_Attributes.clear();
}

BOOL ExportBase::AddAttribute( ExportAttribute* pAttribute )
{
    m_Attributes.push_back( pAttribute );
    return TRUE;
}

};
