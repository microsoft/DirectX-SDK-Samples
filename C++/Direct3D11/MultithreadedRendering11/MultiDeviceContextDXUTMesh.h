//--------------------------------------------------------------------------------------
// File: MultiDeviceContextDXUTMesh.h
//
// Extended implementation of DXUT Mesh for M/T rendering
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "SDKMesh.h"

class CMultiDeviceContextDXUTMesh;
typedef void (*LPRENDERMESH11)( CMultiDeviceContextDXUTMesh* pMesh, 
        UINT iMesh,
        bool bAdjacent,
        ID3D11DeviceContext* pd3dDeviceContext,
        UINT iDiffuseSlot,
        UINT iNormalSlot,
        UINT iSpecularSlot );

struct MDC_SDKMESH_CALLBACKS11 : public SDKMESH_CALLBACKS11
{
    LPRENDERMESH11 pRenderMesh;
};

// Class to override CDXUTSDKMesh in order to allow farming out different Draw
// calls to different DeviceContexts.  Instead of calling RenderMesh directly,
// this class passes the call through a user-supplied callback.
//
// Note it is crucial for the multithreading sample that this class not use
// pd3dDeviceContext in the implementation, other than to pass it through to 
// the callback and to DXUT.  Any other use will not be reflected in the auxiliary
// device contexts used by the sample.
class CMultiDeviceContextDXUTMesh : public CDXUTSDKMesh
{

public:
    virtual HRESULT                 Create( ID3D11Device* pDev11, LPCTSTR szFileName, bool bCreateAdjacencyIndices=
                                            false, MDC_SDKMESH_CALLBACKS11* pCallbacks=NULL );

    void                            RenderMesh( UINT iMesh,
                                                bool bAdjacent,
                                                ID3D11DeviceContext* pd3dDeviceContext,
                                                UINT iDiffuseSlot,
                                                UINT iNormalSlot,
                                                UINT iSpecularSlot );
    void                            RenderFrame( UINT iFrame,
                                             bool bAdjacent,
                                             ID3D11DeviceContext* pd3dDeviceContext,
                                             UINT iDiffuseSlot,
                                             UINT iNormalSlot,
                                             UINT iSpecularSlot );

    virtual void                    Render( ID3D11DeviceContext* pd3dDeviceContext,
                                            UINT iDiffuseSlot = INVALID_SAMPLER_SLOT,
                                            UINT iNormalSlot = INVALID_SAMPLER_SLOT,
                                            UINT iSpecularSlot = INVALID_SAMPLER_SLOT );

protected:
    LPRENDERMESH11                  m_pRenderMeshCallback;
};