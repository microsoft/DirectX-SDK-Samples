//----------------------------------------------------------------------------
// File: dxdiag.h
//
// Desc: 
//
// Copyright (c) Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef DXDIAG_H
#define DXDIAG_H

// Headers for dxdiagn.dll
#include <dxdiag.h>

// Headers for structs to hold info
#include "fileinfo.h"
#include "sysinfo.h"
#include "dispinfo.h"
#include "sndinfo.h"
#include "musinfo.h"
#include "inptinfo.h"
#include "netinfo.h"
#include "showinfo.h"

//-----------------------------------------------------------------------------
// Defines, and constants
//-----------------------------------------------------------------------------
class CDxDiagInfo
{
public:
            CDxDiagInfo();
            ~CDxDiagInfo();

    HRESULT Init( BOOL bAllowWHQLChecks );
    HRESULT QueryDxDiagViaDll();

protected:
    HRESULT GetSystemInfo( SysInfo** ppSysInfo );
    HRESULT GetSystemDevices( vector <SystemDevice*>& vSystemDevices );
    HRESULT GatherSystemDeviceDriverList( IDxDiagContainer* pParent, vector <FileNode*>& vDriverList );
    HRESULT GatherFileNodeInst( FileNode* pFileNode, IDxDiagContainer* pObject );
    HRESULT GetDirectXFilesInfo( FileInfo** ppFileInfo );
    HRESULT GetDisplayInfo( vector <DisplayInfo*>& vDisplayInfo );
    HRESULT GatherDXVA_DeinterlaceCaps( IDxDiagContainer* pParent, vector <DxDiag_DXVA_DeinterlaceCaps*>& vDXVACaps );
    HRESULT GetSoundInfo( vector <SoundInfo*>& vSoundInfos, vector <SoundCaptureInfo*>& vSoundCaptureInfos );
    HRESULT GetMusicInfo( MusicInfo** ppMusicInfo );
    HRESULT GetInputInfo( InputInfo** ppInputInfo );
    HRESULT GatherInputRelatedDeviceInst( InputRelatedDeviceInfo* pInputRelatedDevice, IDxDiagContainer* pContainer );
    HRESULT GatherInputRelatedDeviceInstDrivers( InputRelatedDeviceInfo* pInputRelatedDevice,
                                                 IDxDiagContainer* pChild );
    HRESULT GetNetworkInfo( NetInfo** ppNetInfo );
    HRESULT GetShowInfo( ShowInfo** ppShowInfo );
    HRESULT GetLogicalDiskInfo( vector <LogicalDisk*>& vLogicalDisks );

    HRESULT GetStringValue( IDxDiagContainer* pObject, WCHAR* wstrName, TCHAR* strValue, int nStrLen );
    HRESULT GetUIntValue( IDxDiagContainer* pObject, WCHAR* wstrName, DWORD* pdwValue );
    HRESULT GetIntValue( IDxDiagContainer* pObject, WCHAR* wstrName, LONG* pnValue );
    HRESULT GetBoolValue( IDxDiagContainer* pObject, WCHAR* wstrName, BOOL* pbValue );
    HRESULT GetInt64Value( IDxDiagContainer* pObject, WCHAR* wstrName, ULONGLONG* pullValue );

    VOID    DestroyFileList( FileInfo* pFileInfo );
    VOID    DestroySystemDevice( vector <SystemDevice*>& vSystemDevices );
    VOID    DestroyDisplayInfo( vector <DisplayInfo*>& vDisplayInfo );
    VOID    DeleteInputTree( vector <InputRelatedDeviceInfo*>& vDeviceList );
    VOID    DeleteFileList( vector <FileNode*>& vDriverList );
    VOID    DestroyInputInfo( InputInfo* pInputInfo );
    VOID    DestroyMusicInfo( MusicInfo* pMusicInfo );
    VOID    DestroyNetworkInfo( NetInfo* pNetInfo );
    VOID    DestroySoundInfo( vector <SoundInfo*>& vSoundInfos );
    VOID    DestroySoundCaptureInfo( vector <SoundCaptureInfo*>& vSoundCaptureInfos );
    VOID    DestroyShowInfo( ShowInfo* pShowInfo );

    IDxDiagProvider* m_pDxDiagProvider;
    IDxDiagContainer* m_pDxDiagRoot;
    BOOL m_bCleanupCOM;

public:
    SysInfo* m_pSysInfo;
    vector <SystemDevice*> m_vSystemDevices;
    FileInfo* m_pFileInfo;
    vector <DisplayInfo*> m_vDisplayInfo;
    vector <SoundInfo*> m_vSoundInfos;
    vector <SoundCaptureInfo*> m_vSoundCaptureInfos;
    MusicInfo* m_pMusicInfo;
    InputInfo* m_pInputInfo;
    NetInfo* m_pNetInfo;
    ShowInfo* m_pShowInfo;
    vector <LogicalDisk*> m_vLogicalDiskList;
};


#endif // DXDIAG_H
