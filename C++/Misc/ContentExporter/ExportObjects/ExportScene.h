//-------------------------------------------------------------------------------------
//  ExportScene.h
//
//  The root class of the scene hierarchy.  Exporter settings are also contained here.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

#include "ExportSettings.h"

namespace ATG
{

using namespace std;

class ExportMaterial;
class ExportMeshBase;
class ExportAnimation;

typedef std::vector<ExportMaterial*> ExportMaterialList;
typedef std::vector<ExportAnimation*> ExportAnimationList;
typedef std::vector<ExportMeshBase*> ExportMeshBaseList;

class ExportStatistics
{
public:
    ExportStatistics()
    {
        ZeroMemory( this, sizeof( ExportStatistics ) );
    }
    VOID StartExport() { StartExportTime = GetTickCount(); }
    VOID StartSceneParse() { StartSceneParseTime = GetTickCount(); }
    VOID StartSave() { StartSaveTime = GetTickCount(); }
    VOID EndExport() { EndExportTime = GetTickCount(); }
    DWORD       StartExportTime;
    DWORD       StartSceneParseTime;
    DWORD       StartSaveTime;
    DWORD       EndExportTime;
    UINT        TrisExported;
    UINT        VertsExported;
    UINT        MaterialsExported;
    UINT        VertexBytesExported;
    UINT        IndexBytesExported;
    UINT        MeshesExported;
    UINT        SubDMeshesProcessed;
    UINT        SubDQuadsProcessed;
    UINT        SubDTrisProcessed;
    VOID FinalReport();
};

class ExportInformation
{
public:
    ExportString        ExporterName;
    ExportString        DCCNameAndVersion;
    ExportString        UserName;
    ExportString        MachineName;
    ExportString        PlatformName;
    __time64_t          ExportTime;
};

class IDCCTransformer
{
public:
    virtual VOID TransformMatrix( D3DXMATRIX* pDestMatrix, CONST D3DXMATRIX* pSrcMatrix ) CONST = NULL;
    virtual VOID TransformPosition( D3DXVECTOR3* pDestPosition, CONST D3DXVECTOR3* pSrcPosition ) CONST = NULL;
    virtual VOID TransformDirection( D3DXVECTOR3* pDestDirection, CONST D3DXVECTOR3* pSrcDirection ) CONST = NULL;
    virtual FLOAT TransformLength( FLOAT fInputLength ) CONST = NULL;
};

class ExportScene :
    public ExportFrame
{
public:
    ExportScene();
    virtual ~ExportScene();

    ExportStatistics& Statistics() { return m_Statistics; }
    ExportCoreSettings& Settings() { return g_ExportCoreSettings; }
    ExportInformation& Information() { return m_Information; }

    BOOL AddMaterial( ExportMaterial* pMaterial );
    BOOL AddMesh( ExportMeshBase* pMesh );
    BOOL AddAnimation( ExportAnimation* pAnimation );

    ExportMaterial* FindMaterial( ExportString name );
    ExportMeshBase* FindMesh( ExportString name );
    ExportAnimation* FindAnimation( ExportString name );
    ExportMaterial* FindMaterial( VOID* pDCCObject );
    ExportMeshBase* FindMesh( VOID* pDCCObject );
    ExportAnimation* FindAnimation( VOID* pDCCObject );

    UINT GetMaterialCount() CONST { return (UINT)m_vMaterials.size(); }
    UINT GetMeshCount() CONST { return (UINT)m_vMeshes.size(); }
    UINT GetAnimationCount() CONST { return (UINT)m_vAnimations.size(); }

    ExportMaterial* GetMaterial( UINT uIndex ) { return m_vMaterials[ uIndex ]; }
    ExportMeshBase* GetMesh( UINT uIndex ) { return m_vMeshes[ uIndex ]; }
    ExportAnimation* GetAnimation( UINT uIndex ) { return m_vAnimations[ uIndex ]; }

    IDCCTransformer* GetDCCTransformer() { return m_pDCCTransformer; }
    VOID SetDCCTransformer( IDCCTransformer* pDCCTransformer ) { m_pDCCTransformer = pDCCTransformer; }

protected:    
    ExportMaterialList          m_vMaterials;
    ExportMeshBaseList          m_vMeshes;
    ExportAnimationList         m_vAnimations;
    ExportStatistics            m_Statistics;
    ExportInformation           m_Information;
    IDCCTransformer*            m_pDCCTransformer;
};

};

