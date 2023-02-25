//-------------------------------------------------------------------------------------
//  ExportFrame.cpp
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "ExportFrame.h"

namespace ATG
{

ExportFrame::ExportFrame()
: ExportBase( NULL )
{
    m_Transform.SetIdentity();
}

ExportFrame::ExportFrame( ExportString name )
: ExportBase( name )
{
    m_Transform.SetIdentity();
}

ExportFrame::~ExportFrame(void)
{
    for( UINT i = 0; i < m_vModels.size(); i++ )
    {
        delete m_vModels[i];
    }
    m_vModels.clear();
    for( UINT i = 0; i < m_vLights.size(); i++ )
    {
        delete m_vLights[i];
    }
    m_vLights.clear();
    for( UINT i = 0; i < m_vCameras.size(); i++ )
    {
        delete m_vCameras[i];
    }
    m_vCameras.clear();
    for( UINT i = 0; i < m_vChildren.size(); i++ )
    {
        delete m_vChildren[i];
    }
    m_vChildren.clear();
}

ExportFrame* ExportFrame::FindFrameByDCCObject( VOID* pObject )
{
    if( pObject == NULL )
        return NULL;
    if( pObject == GetDCCObject() )
        return this;

    for( UINT i = 0; i < m_vChildren.size(); i++ )
    {
        ExportFrame* pFrame = m_vChildren[i]->FindFrameByDCCObject( pObject );
        if( pFrame != NULL )
            return pFrame;
    }
    return NULL;
}

VOID ExportFrame::NormalizeTransform()
{
    m_Transform.Normalize();
}

};
