//-----------------------------------------------------------------------------
// File: Common.h
//
// Common header for project.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef _COMMON_H_
#define _COMMON_H_

#ifndef WINVER
#define WINVER          0x0500
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT    0x0500
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <shellapi.h>

#define NO_SHLWAPI_STRFCNS
#include <shlwapi.h>

#endif /* _COMMON_H_ */
