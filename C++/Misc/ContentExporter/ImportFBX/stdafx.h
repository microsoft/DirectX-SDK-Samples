//-------------------------------------------------------------------------------------
//  stdafx.h
//
//  Precompiled header for the ImportFBX project.
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

#pragma warning( disable: 4512 )
#pragma warning( disable: 4505 )
#pragma warning( disable: 4100 )
#pragma warning( disable: 4996 )

#ifndef _SECURE_SCL
#define _SECURE_SCL 0
#endif

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <WindowsX.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <list>
#include <vector>
#include <hash_map>
#include <algorithm>
#include <commctrl.h>
#include <richedit.h>
#include <process.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <shellapi.h>

#include <xnamath.h>

#include "..\ExporterGlobals.h"
#include "..\ExportObjects\collision.h"
#include "..\ExportObjects\ExportXmlParser.h"
#include "..\ExportObjects\ExportPath.h"
#include "..\ExportObjects\ExportMaterial.h"
#include "..\ExportObjects\ExportObjects.h"

#include <fbxsdk.h>

#include "..\XATGFileWriter\XATGFileWriter.h"
#include "..\SDKMeshFileWriter\SDKMeshFileWriter.h"

#define CONTENT_EXPORTER_TITLE CONTENT_EXPORTER_GLOBAL_TITLE " for FBX"
extern CHAR g_strExporterName[100];

#ifndef UNUSED
#define UNUSED(x) (x)
#endif
