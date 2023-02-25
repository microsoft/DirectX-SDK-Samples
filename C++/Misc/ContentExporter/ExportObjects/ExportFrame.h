//-------------------------------------------------------------------------------------
//  ExportFrame.h
//
//  A class representing a node in a scenegraph, containing a local transform, children,
//  and a parent pointer.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

namespace ATG
{

using namespace std;

class ExportModel;
class ExportLight;
class ExportCamera;

class ExportFrame :
    public ExportBase
{
public:
    ExportFrame();
    ExportFrame( ExportString name );
    virtual ~ExportFrame();

    VOID AddChild( ExportFrame* pFrame ) { m_vChildren.push_back( pFrame ); }
    VOID RemoveChild( ExportFrame* pFrame );
    UINT GetChildCount() CONST { return (UINT)m_vChildren.size(); }
    ExportFrame* GetChildByIndex( UINT uIndex ) { return m_vChildren[ uIndex ]; }

    UINT GetModelCount() CONST { return (UINT)m_vModels.size(); }
    VOID AddModel( ExportModel* pModel ) { m_vModels.push_back( pModel ); }
    ExportModel* GetModelByIndex( UINT uIndex ) { return m_vModels[ uIndex ]; }

    UINT GetLightCount() CONST { return (UINT)m_vLights.size(); }
    VOID AddLight( ExportLight* pLight ) { m_vLights.push_back( pLight ); }
    ExportLight* GetLightByIndex( UINT uIndex ) { return m_vLights[ uIndex ]; }

    UINT GetCameraCount() CONST { return (UINT)m_vCameras.size(); }
    VOID AddCamera( ExportCamera* pCamera ) { m_vCameras.push_back( pCamera ); }
    ExportCamera* GetCameraByIndex( UINT uIndex ) { return m_vCameras[ uIndex ]; }

    ExportFrame* FindFrameByName( ExportString name );
    ExportFrame* FindFrameByDCCObject( VOID* pObject );

    ExportTransform& Transform() { return m_Transform; }
    VOID NormalizeTransform();
protected:
    ExportTransform             m_Transform;
    vector< ExportFrame* >      m_vChildren;
    vector< ExportModel* >      m_vModels;
    vector< ExportLight* >      m_vLights;
    vector< ExportCamera* >     m_vCameras;
};

};

