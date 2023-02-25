//--------------------------------------------------------------------------------------
// File: MultiDeviceContextDXUTMesh.cpp
//
// Extended implementation of DXUT Mesh for M/T rendering
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DXUT.h"

#include "MultiDeviceContextDXUTMesh.h"

//--------------------------------------------------------------------------------------
HRESULT CMultiDeviceContextDXUTMesh::Create( ID3D11Device* pDev11, LPCTSTR szFileName, bool bCreateAdjacencyIndices, 
                                            MDC_SDKMESH_CALLBACKS11* pCallbacks )
{
    if ( pCallbacks != NULL )
    {
        m_pRenderMeshCallback = pCallbacks->pRenderMesh;
    }
    else
    {
        m_pRenderMeshCallback = NULL;
    }

    return CDXUTSDKMesh::Create( pDev11, szFileName, bCreateAdjacencyIndices, pCallbacks );
}


//--------------------------------------------------------------------------------------
// Name: RenderMesh()
// Calls the RenderMesh of the base class.  We wrap this API because the base class
// version is protected and we need it to be public.
//--------------------------------------------------------------------------------------
void CMultiDeviceContextDXUTMesh::RenderMesh( UINT iMesh,
                                            bool bAdjacent,
                                            ID3D11DeviceContext* pd3dDeviceContext,
                                            UINT iDiffuseSlot,
                                            UINT iNormalSlot,
                                            UINT iSpecularSlot )
{
    CDXUTSDKMesh::RenderMesh( iMesh,
        bAdjacent,
        pd3dDeviceContext,
        iDiffuseSlot,
        iNormalSlot,
        iSpecularSlot );
}


//--------------------------------------------------------------------------------------
void CMultiDeviceContextDXUTMesh::RenderFrame( UINT iFrame,
                                              bool bAdjacent,
                                              ID3D11DeviceContext* pd3dDeviceContext,
                                              UINT iDiffuseSlot,
                                              UINT iNormalSlot,
                                              UINT iSpecularSlot )
{
    if( !m_pStaticMeshData || !m_pFrameArray )
        return;

    if( m_pFrameArray[iFrame].Mesh != INVALID_MESH )
    {
        if ( m_pRenderMeshCallback == NULL )
        {
            RenderMesh( m_pFrameArray[iFrame].Mesh,
                bAdjacent,
                pd3dDeviceContext,
                iDiffuseSlot,
                iNormalSlot,
                iSpecularSlot );
        }
        else
        {
            m_pRenderMeshCallback( this, 
                m_pFrameArray[iFrame].Mesh,
                bAdjacent,
                pd3dDeviceContext,
                iDiffuseSlot,
                iNormalSlot,
                iSpecularSlot );
        }
    }

    // Render our children
    if( m_pFrameArray[iFrame].ChildFrame != INVALID_FRAME )
        RenderFrame( m_pFrameArray[iFrame].ChildFrame, bAdjacent, pd3dDeviceContext, iDiffuseSlot, 
        iNormalSlot, iSpecularSlot );

    // Render our siblings
    if( m_pFrameArray[iFrame].SiblingFrame != INVALID_FRAME )
        RenderFrame( m_pFrameArray[iFrame].SiblingFrame, bAdjacent, pd3dDeviceContext, iDiffuseSlot, 
        iNormalSlot, iSpecularSlot );
}

//--------------------------------------------------------------------------------------
void CMultiDeviceContextDXUTMesh::Render( ID3D11DeviceContext* pd3dDeviceContext,
                                         UINT iDiffuseSlot,
                                         UINT iNormalSlot,
                                         UINT iSpecularSlot )
{
    RenderFrame( 0, false, pd3dDeviceContext, iDiffuseSlot, iNormalSlot, iSpecularSlot );
}

