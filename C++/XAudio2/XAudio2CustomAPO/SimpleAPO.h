//--------------------------------------------------------------------------------------
// SimpleAPO.h
//
// XNA Developer Connection
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "SampleAPOBase.h"

struct SimpleAPOParams
{
    float gain;
};

class __declspec( uuid("{5EB8D611-FF96-429d-8365-2DDF89A7C1CD}")) 
CSimpleAPO 
: public CSampleXAPOBase<CSimpleAPO, SimpleAPOParams>
{
public:
    CSimpleAPO();
    ~CSimpleAPO();

    void DoProcess( const SimpleAPOParams&, FLOAT32* __restrict pData, UINT32 cFrames, UINT32 cChannels );
};

