//-------------------------------------------------------------------------------------
//  ExportLight.cpp
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "ExportLight.h"

namespace ATG
{

    ExportLight::ExportLight( ExportString strName )
      : ExportBase( strName ),
        Type( LT_AMBIENT ),
        LocalPosition( 0, 0, 0 ),
        Color( 1, 1, 1, 1 ),
        Direction( 0, 0, -1 ),
        fRange( 0 ),
        Falloff( LF_NONE ),
        fInnerAngle( 0 ),
        fOuterAngle( 0 )
    {
    }

}

