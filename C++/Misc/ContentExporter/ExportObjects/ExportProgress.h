//-------------------------------------------------------------------------------------
//  ExportProgress.h
//
//  A DCC-agnostic system for reporting progress of the export operation.
//  The export front-end implements a subclass of ExportProgress that manipulates the
//  DCC user interface, and sets up the g_pProgress global pointer so the DCC-agnostic
//  code can report progress updates.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

namespace ATG
{
    class ExportProgress
    {
    public:
        virtual void Initialize( CONST CHAR* strTitle );
        virtual void Terminate();

        virtual void SetCaption( CONST CHAR* strCaption );
        virtual void StartNewTask( CONST CHAR* strCaption, FLOAT fTaskPercentOfWhole );
        virtual void SetProgress( FLOAT fTaskRelativeProgress );
    };

    extern ExportProgress* g_pProgress;
}

