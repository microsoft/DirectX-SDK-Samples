//-------------------------------------------------------------------------------------
//  XATGFileWriter.h
//  
//  Writes files in the XATG file format, from data stored in the export objects hierarchy.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

namespace ATG
{

class ExportManifest;

BOOL WriteXATGFile( const CHAR* strFileName, ExportManifest* pManifest );

VOID BundleTextures();

struct XATGExportSettings
{
    BOOL    bBundleTextures;
    BOOL    bUseExistingBundle;
    BOOL    bBinaryBlobExport;
};

VOID XATGInitializeSettings();

}
