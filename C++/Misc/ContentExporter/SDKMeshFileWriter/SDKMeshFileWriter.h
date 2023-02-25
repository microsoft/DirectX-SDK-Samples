//-------------------------------------------------------------------------------------
//  SDKMeshFileWriter.h
//
//  Entry point for writing SDKMESH files.  This file writer takes data from the
//  ExportScene stored in a global variable (g_pScene).
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

namespace ATG
{

class ExportManifest;

BOOL WriteSDKMeshFile( const CHAR* strFileName, ExportManifest* pManifest );

}
