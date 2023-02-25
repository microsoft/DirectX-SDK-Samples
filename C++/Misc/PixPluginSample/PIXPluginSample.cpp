//==================================================================================================
// PIXPluginSample.cpp
//
// Copyright (c) Microsoft Corporation, All rights Reserved
//==================================================================================================

#include <Windows.h>
#include <stdio.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
#include <PIXPlugin.h>
#include "Resource.h"

// Indicate the version of this plugin
#define SAMPLE_PLUGIN_VERSION 0x0102


// Macro for determining the number of elements in an array
#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(x)  (sizeof(x)/sizeof((x)[0]))
#endif


// Unique identifiers for each counter implemented in this plugin
enum MYCOUNTERID
{
    CTR_FIRST,
    CTR_MOUSEPOSXY = CTR_FIRST,
    CTR_MOUSEPOSX,
    CTR_MOUSEPOSY,
    CTR_KEYBOARDCAPTURE,
    CTR_COUNT
};

//==================================================================================================
// pixCounterSet - Lists all counters exposed by this plugin.  It is used by PIXGetCounterInfo.  In
//                 this example we provide a fixed set of counters, but a plugin could use a 
//                 variable set instead.
//==================================================================================================
PIXCOUNTERINFO pixCounterSet[] =
{
    { CTR_MOUSEPOSXY,       L"MousePosXY",      PCDT_STRING },
    { CTR_MOUSEPOSX,        L"MousePosX",       PCDT_INT },
    { CTR_MOUSEPOSY,        L"MousePosY",       PCDT_INT },
    { CTR_KEYBOARDCAPTURE,  L"KeyboardCapture", PCDT_STRING },
};


//==================================================================================================
// pixCounterDesc - Description strings for all counters.  It is used by PIXGetCounterDesc.  This
//                  example shows how to use constant strings embedded in the code.  Another 
//                  approach would be to read the description strings from resources.
//==================================================================================================
WCHAR*  pixCounterDesc[] =
{
    L"A string indicating the X and Y positions of the mouse at this frame.",
    L"X coordinate of the mouse position at this frame.",
    L"Y coordinate of the mouse position at this frame.",
    L"A string indicating all keys pressed at this frame."
};


//==================================================================================================
// MousePosXYBegin - Called at the beginning of the experiment if the CTR_MOUSEPOSXY counter is 
//                   active.
//==================================================================================================
BOOL MousePosXYBegin( const WCHAR* pApplication )
{
    WCHAR ctemp[1024];

    StringCchPrintf( ctemp, ARRAY_LENGTH(ctemp), L"Exp MousePosXY begin %s\r\n", pApplication );

    OutputDebugString( ctemp );

    return TRUE;
}


//==================================================================================================
// MousePosXYFrame - Get the mouse position (x,y) and return it as a string to PIX.
//==================================================================================================
WCHAR g_szPosXY[MAX_PATH] = {0};
BOOL MousePosXYFrame( DWORD* pdwReturnBytes, BYTE** ppReturnData )
{
    //get the mouse position
    POINT Point;
    if( !GetCursorPos( &Point ) )
    {
        ( *pdwReturnBytes ) = 0;  //don't return any data if we fail
        return FALSE;
    }

    StringCchPrintf( g_szPosXY, ARRAY_LENGTH(g_szPosXY), L"Mouse: %d,%d", Point.x, Point.y ); //this will never be larger than MAX_PATH chars

    *ppReturnData = ( BYTE* )g_szPosXY;
    *pdwReturnBytes = ( ( DWORD )wcslen( g_szPosXY ) + 1 ) * sizeof( WCHAR );

    return TRUE;
}


//==================================================================================================
// MousePosXYEnd
//==================================================================================================
BOOL MousePosXYEnd()
{
    return TRUE;
}


//==================================================================================================
// MousePosXBegin
//==================================================================================================
BOOL MousePosXBegin( const WCHAR* pszApplication )
{
    WCHAR ctemp[1024];

    StringCchPrintf( ctemp, ARRAY_LENGTH(ctemp), L"Exp MousePosX begin %s\r\n", pszApplication );

    OutputDebugString( ctemp );

    return TRUE;
}


//==================================================================================================
// MousePosXFrame - Get the mouse position (x) and return it as an INT to PIX.
//==================================================================================================
INT     g_iPosX = 0;
BOOL MousePosXFrame( DWORD* pdwReturnBytes, BYTE** ppReturnData )
{
    //get the mouse position
    POINT Point;
    if( !GetCursorPos( &Point ) )
    {
        ( *pdwReturnBytes ) = 0;  //don't return any data if we fail
        return FALSE;
    }

    g_iPosX = Point.x;

    *ppReturnData = ( BYTE* )&g_iPosX;
    *pdwReturnBytes = sizeof( INT );

    return TRUE;
}


//==================================================================================================
// MousePosXEnd
//==================================================================================================
BOOL MousePosXEnd()
{
    return TRUE;
}


//==================================================================================================
// MousePosYBegin
//==================================================================================================
BOOL MousePosYBegin( const WCHAR* pszApplication )
{
    WCHAR ctemp[1024];

    StringCchPrintf( ctemp, ARRAY_LENGTH(ctemp), L"Exp MousePosY begin %s\r\n", pszApplication );

    OutputDebugString( ctemp );

    return TRUE;
}


//==================================================================================================
// MousePosYFrame - get the mouse position (y) and return it as an INT to PIX
//==================================================================================================
INT     g_iPosY = 0;
BOOL MousePosYFrame( DWORD* pdwReturnBytes, BYTE** ppReturnData )
{
    //get the mouse position
    POINT Point;
    if( !GetCursorPos( &Point ) )
    {
        ( *pdwReturnBytes ) = 0;  //don't return any data if we fail
        return FALSE;
    }

    g_iPosY = Point.y;

    *ppReturnData = ( BYTE* )&g_iPosY;
    *pdwReturnBytes = sizeof( INT );

    return TRUE;
}


//==================================================================================================
// MousePosYEnd
//==================================================================================================
BOOL MousePosYEnd()
{
    return TRUE;
}


//==================================================================================================
// KeyboardCaptureBegin
//==================================================================================================
BOOL KeyboardCaptureBegin( const WCHAR* pszApplication )
{
    WCHAR ctemp[1024];

    StringCchPrintf( ctemp, ARRAY_LENGTH(ctemp), L"Exp Keyboard Capture begin %s\r\n", pszApplication );

    OutputDebugString( ctemp );

    return TRUE;
}


//==================================================================================================
// KeyboardCaptureFrame - get the keys currently being pressed
//==================================================================================================
WCHAR g_szKeys[513] = {0};
BOOL KeyboardCaptureFrame( DWORD* pdwReturnBytes, BYTE** ppReturnData )
{
    BOOL bKeyFound = FALSE;
    ZeroMemory( g_szKeys, 513 * sizeof( WCHAR ) );
    int iStringIndex = 0;
    for( int i = 0x41; i < 0x5b; i++ ) // Just look at A-Z
    {
        if( GetAsyncKeyState( i ) )
        {
            if( iStringIndex < 512 )
            {
                g_szKeys[iStringIndex] = ( WCHAR )i;
                iStringIndex ++;
                g_szKeys[iStringIndex] = ' ';
                iStringIndex ++;

                bKeyFound = TRUE;
            }
        }
    }

    *ppReturnData = ( BYTE* )g_szKeys;
    *pdwReturnBytes = ( ( DWORD )wcslen( g_szKeys ) + 1 ) * sizeof( WCHAR );

    return TRUE;
}


//==================================================================================================
// KeyboardCaptureEnd
//==================================================================================================
BOOL KeyboardCaptureEnd()
{
    return TRUE;
}


//==================================================================================================
// PIXGetPluginInfo
//==================================================================================================
BOOL WINAPI PIXGetPluginInfo( PIXPLUGININFO* pPIXPluginInfo )
{
    // If we want the hInst of the app, we can get it from pPIXPluginInfo->hinst

    //
    // Fill in our information and give it to PIX
    //
    pPIXPluginInfo->iPluginVersion = SAMPLE_PLUGIN_VERSION; //version of this plugin

    pPIXPluginInfo->iPluginSystemVersion = PIX_PLUGIN_SYSTEM_VERSION; //version of PIX we are designed to work with

    pPIXPluginInfo->pstrPluginName = L"PIX Sample Plugin"; // name of this plugin

    return TRUE;
}


//==================================================================================================
// PIXGetCounterInfo
//==================================================================================================
BOOL WINAPI PIXGetCounterInfo( DWORD* pdwReturnCounters, PIXCOUNTERINFO** ppCounterInfoList )
{
    *pdwReturnCounters = ARRAY_LENGTH(pixCounterSet);
    *ppCounterInfoList = &pixCounterSet[0];
    return TRUE;
}


//==================================================================================================
// PIXGetCounterDesc
//==================================================================================================
BOOL WINAPI PIXGetCounterDesc( PIXCOUNTERID id, WCHAR** ppstrCounterDesc )
{
    if( id < CTR_FIRST || id >= CTR_COUNT )
        return FALSE;
    *ppstrCounterDesc = pixCounterDesc[id];
    return TRUE;
}


//==================================================================================================
// PIXBeginExperiment
//==================================================================================================
BOOL WINAPI PIXBeginExperiment( PIXCOUNTERID id, const WCHAR* pstrApplication )
{
    switch( id )
    {
        case CTR_MOUSEPOSXY:
            return MousePosXYBegin( pstrApplication );
        case CTR_MOUSEPOSX:
            return MousePosXBegin( pstrApplication );
        case CTR_MOUSEPOSY:
            return MousePosYBegin( pstrApplication );
        case CTR_KEYBOARDCAPTURE:
            return KeyboardCaptureBegin( pstrApplication );
        default:
            return FALSE;
    }
}


//==================================================================================================
// PIXEndFrame
//==================================================================================================
BOOL WINAPI PIXEndFrame( PIXCOUNTERID id, UINT iFrame, DWORD* pdwReturnBytes, BYTE** ppReturnData )
{
    switch( id )
    {
        case CTR_MOUSEPOSXY:
            return MousePosXYFrame( pdwReturnBytes, ppReturnData );
        case CTR_MOUSEPOSX:
            return MousePosXFrame( pdwReturnBytes, ppReturnData );
        case CTR_MOUSEPOSY:
            return MousePosYFrame( pdwReturnBytes, ppReturnData );
        case CTR_KEYBOARDCAPTURE:
            return KeyboardCaptureFrame( pdwReturnBytes, ppReturnData );
        default:
            return FALSE;
    }
}


//==================================================================================================
// PIXEndExperiment
//==================================================================================================
BOOL WINAPI PIXEndExperiment( PIXCOUNTERID id )
{
    switch( id )
    {
        case CTR_MOUSEPOSXY:
            return MousePosXYEnd();
        case CTR_MOUSEPOSX:
            return MousePosXEnd();
        case CTR_MOUSEPOSY:
            return MousePosYEnd();
        case CTR_KEYBOARDCAPTURE:
            return KeyboardCaptureEnd();
        default:
            return FALSE;
    }
}


//==================================================================================================
// eof: PIXPluginSample.cpp
//==================================================================================================
