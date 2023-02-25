//-------------------------------------------------------------------------------------
//  ParseMesh.h
//
//  Entry points for FBX mesh parsing.
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

using namespace ATG;

VOID ParseMesh( KFbxMesh* pFbxMesh, ExportFrame* pParentFrame, BOOL bSubDProcess = FALSE, const CHAR* strSuffix = NULL );
VOID ParseSubDiv( KFbxSubdiv* pFbxSubD, ExportFrame* pParentFrame );
