//-------------------------------------------------------------------------------------
//  ExportLight.h
//
//  Classes to represent various types of lights found in a scene.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

#include "ExportBase.h"

namespace ATG
{
    class ExportLight : public ExportBase
    {
    public:
        ExportLight( ExportString strName );
        enum LightType
        {
            LT_AMBIENT = 0,
            LT_DIRECTIONAL,
            LT_POINT,
            LT_SPOT,
        };
        enum LightFalloff
        {
            LF_NONE = 0,
            LF_LINEAR,
            LF_SQUARED
        };

        LightType           Type;
        D3DXVECTOR3         LocalPosition;
        D3DXCOLOR           Color;
        D3DXVECTOR3         Direction;
        FLOAT               fRange;
        LightFalloff        Falloff;
        LightFalloff        SpotFalloff;
        FLOAT               fInnerAngle;
        FLOAT               fOuterAngle;
    };
}