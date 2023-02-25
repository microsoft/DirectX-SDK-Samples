//-------------------------------------------------------------------------------------
//  ExportScene.cpp
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "exportscene.h"

namespace ATG
{

VOID ExportStatistics::FinalReport()
{
    DWORD ExportTotalTime = EndExportTime - StartExportTime;
    DWORD ExportParseTime = StartSaveTime - StartSceneParseTime;
    DWORD ExportSaveTime = EndExportTime - StartSaveTime;

    ExportLog::LogMsg( 2, "%d poly meshes consisting of %d vertices, %d triangles, and %d materials exported.", MeshesExported, VertsExported, TrisExported, MaterialsExported );
    if( SubDMeshesProcessed > 0 )
    {
        ExportLog::LogMsg( 2, "%d subdivision surface meshes processed, including %d quads and %d triangles.", SubDMeshesProcessed, SubDQuadsProcessed, SubDTrisProcessed );
    }
    ExportLog::LogMsg( 2, "Export complete in %0.2f seconds; %0.2f seconds for scene parse and %0.2f seconds for file writing.", 
        (FLOAT)ExportTotalTime / 1000.0f, (FLOAT)ExportParseTime / 1000.0f, (FLOAT)ExportSaveTime / 1000.0f );
}

ExportScene::ExportScene(void)
: ExportFrame(),
  m_pDCCTransformer( NULL )
{
    m_Information.ExportTime = _time64( NULL );
    CHAR strDomain[50];
    size_t BufferSize = ARRAYSIZE(strDomain);
    getenv_s( &BufferSize, strDomain, ARRAYSIZE(strDomain), "USERDOMAIN" );
    CHAR strUsername[50];
    getenv_s( &BufferSize, strUsername, ARRAYSIZE(strUsername), "USERNAME" );
    CHAR strTemp[256];
    sprintf_s( strTemp, "%s\\%s", strDomain, strUsername );
    m_Information.UserName = strTemp;
    CHAR strComputerName[100];
    BufferSize = ARRAYSIZE(strComputerName);
    getenv_s( &BufferSize, strComputerName, ARRAYSIZE(strComputerName), "COMPUTERNAME" );
    m_Information.MachineName = strComputerName;
    OSVERSIONINFO OSVersion = { 0 };
    OSVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx( &OSVersion );
    sprintf_s( strTemp, "Windows NT %d.%d build %d", OSVersion.dwMajorVersion, OSVersion.dwMinorVersion, OSVersion.dwBuildNumber );
    m_Information.PlatformName = strTemp;
}

ExportScene::~ExportScene(void)
{
    {
        ExportAnimationList::iterator iter = m_vAnimations.begin();
        ExportAnimationList::iterator end = m_vAnimations.end();
        while( iter != end )
        {
            delete *iter;
            ++iter;
        }
        m_vAnimations.clear();
    }
    {
        ExportMeshBaseList::iterator iter = m_vMeshes.begin();
        ExportMeshBaseList::iterator end = m_vMeshes.end();
        while( iter != end )
        {
            delete *iter;
            ++iter;
        }
        m_vMeshes.clear();
    }
    {
        ExportMaterialList::iterator iter = m_vMaterials.begin();
        ExportMaterialList::iterator end = m_vMaterials.end();
        while( iter != end )
        {
            delete *iter;
            ++iter;
        }
        m_vMaterials.clear();
    }
}

BOOL ExportScene::AddAnimation( ExportAnimation* pAnimation )
{
    if( pAnimation == NULL )
        return FALSE;
    if( FindAnimation( pAnimation->GetName() ) != NULL )
        return FALSE;
    m_vAnimations.push_back( pAnimation );
    return TRUE;
}

BOOL ExportScene::AddMaterial( ExportMaterial* pMaterial )
{
    if( pMaterial == NULL )
        return FALSE;
    if( FindMaterial( pMaterial->GetDCCObject() ) != NULL )
        return FALSE;
    m_vMaterials.push_back( pMaterial );
    return TRUE;
}

BOOL ExportScene::AddMesh( ExportMeshBase* pMesh )
{
    if( pMesh == NULL )
        return FALSE;
    if( FindMesh( pMesh->GetName() ) != NULL )
        return FALSE;
    m_vMeshes.push_back( pMesh );
    return TRUE;
}

ExportAnimation* ExportScene::FindAnimation( ExportString name )
{
    for( UINT i = 0; i < m_vAnimations.size(); i++ )
    {
        if( m_vAnimations[i]->GetName() == name )
            return m_vAnimations[i];
    }
    return NULL;
}

ExportMaterial* ExportScene::FindMaterial( ExportString name )
{
    for( UINT i = 0; i < m_vMaterials.size(); i++ )
    {
        if( m_vMaterials[i]->GetName() == name )
            return m_vMaterials[i];
    }
    return NULL;
}

ExportMeshBase* ExportScene::FindMesh( ExportString name )
{
    for( UINT i = 0; i < m_vMeshes.size(); i++ )
    {
        if( m_vMeshes[i]->GetName() == name )
            return m_vMeshes[i];
    }
    return NULL;
}

ExportAnimation* ExportScene::FindAnimation( VOID* pDCCObject )
{
    for( UINT i = 0; i < m_vAnimations.size(); i++ )
    {
        if( m_vAnimations[i]->GetDCCObject() == pDCCObject )
            return m_vAnimations[i];
    }
    return NULL;
}

ExportMaterial* ExportScene::FindMaterial( VOID* pDCCObject )
{
    for( UINT i = 0; i < m_vMaterials.size(); i++ )
    {
        if( m_vMaterials[i]->GetDCCObject() == pDCCObject )
            return m_vMaterials[i];
    }
    return NULL;
}

ExportMeshBase* ExportScene::FindMesh( VOID* pDCCObject )
{
    for( UINT i = 0; i < m_vMeshes.size(); i++ )
    {
        if( m_vMeshes[i]->GetDCCObject() == pDCCObject )
            return m_vMeshes[i];
    }
    return NULL;
}

};
