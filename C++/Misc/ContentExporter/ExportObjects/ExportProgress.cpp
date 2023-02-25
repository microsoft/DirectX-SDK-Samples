//-------------------------------------------------------------------------------------
//  ExportProgress.cpp
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "exportprogress.h"

namespace ATG
{
    VOID ExportProgress::Initialize( CONST CHAR* strTitle )
    {
    }

    VOID ExportProgress::Terminate()
    {
    }

    VOID ExportProgress::StartNewTask( CONST CHAR* strCaption, FLOAT fTaskPercentOfWhole )
    {
    }

    VOID ExportProgress::SetCaption( CONST CHAR* strCaption )
    {
    }

    VOID ExportProgress::SetProgress( FLOAT fTaskRelativeProgress )
    {
    }

    ExportProgress g_NullProgressBar;
    ExportProgress* g_pProgress = &g_NullProgressBar;
}

