//-------------------------------------------------------------------------------------
//  ExportCamera.h
//
//  A data structure representing a camera.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

namespace ATG
{
    class ExportCamera : public ExportBase
    {
    public:
        ExportCamera( ExportString strName );

        D3DXVECTOR3         LocalPosition;
        D3DXVECTOR3         Direction;
        D3DXVECTOR3         Up;
        FLOAT               fFocalLength;
        FLOAT               fFieldOfView;
        FLOAT               fNearClip;
        FLOAT               fFarClip;
    };
}