//-------------------------------------------------------------------------------------
//  ExportPath.h
//
//  The ExportPath class manages a path string, representing a relative or absolute
//  path.  Common operations such as replacing the filename, replacing the extension,
//  and appending paths are supported.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

class ExportPath
{
protected:
    CHAR m_strPath[MAX_PATH];
    CHAR* m_strExtension;  // m_strExtension points to the period before the extension.
    CHAR* m_strFileName;   // m_strFileName points to the first character in the filename, following the slash.

public:
    ExportPath();
    ExportPath( const CHAR* strPath );
    ExportPath( const ExportPath& OtherPath );

    VOID SetPathAndFileName( const CHAR* strPath );
    VOID SetPathOnly( const CHAR* strPath );

    ExportPath& operator=( const ExportPath& OtherPath );
    operator const CHAR*() const { return m_strPath; }

    static ExportPath GetTempPath();
    static ExportPath GetCurrentPath();

    ExportPath GetDirectory() const;
    ExportPath GetFileName() const;
    ExportPath GetFileNameWithoutExtension() const;

    BOOL HasExtension() const { return m_strExtension != NULL; }
    BOOL HasFileName() const;
    const CHAR* GetExtension() const;
    BOOL IsAbsolutePath() const;
	BOOL IsEmpty() const { return m_strPath[0] == '\0'; }

    VOID ChangeExtension( const CHAR* strExtension );
    VOID ChangeFileName( const CHAR* strFileName );
    VOID ChangeFileName( const ExportPath& OtherPath );
	VOID ChangeFileNameWithExtension( const CHAR* strFileName );
	VOID ChangeFileNameWithExtension( const ExportPath& OtherPath );
    VOID Append( const ExportPath& OtherPath );
    VOID AppendToFileName( const CHAR* strText );
    VOID TrimOffFileName();

protected:
    VOID Initialize( const CHAR* strPath );
};
