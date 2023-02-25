//--------------------------------------------------------------------------------------
// MonitorAPO.h
//
// XNA Developer Connection
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "SampleAPOBase.h"

#ifndef MONITOR_APO_PIPE_LEN
#define MONITOR_APO_PIPE_LEN 14
#endif

#ifdef _XBOX_VER
#include <AtgLockFreePipe.h>
typedef ATG::LockFreePipe<MONITOR_APO_PIPE_LEN> MonitorAPOPipe;
#else
#include <DXUTLockFreePipe.h>
typedef DXUTLockFreePipe<MONITOR_APO_PIPE_LEN> MonitorAPOPipe;
#endif

struct MonitorAPOParams
{
    MonitorAPOPipe *pipe;
};

class __declspec( uuid("{A4945B8A-EB14-4c96-8067-DF726B528091}")) 
CMonitorAPO
: public CSampleXAPOBase<CMonitorAPO, MonitorAPOParams>
{
public:
    CMonitorAPO();
    ~CMonitorAPO();

    void DoProcess( const MonitorAPOParams&, FLOAT32* __restrict pData, UINT32 cFrames, UINT32 cChannels );
};
