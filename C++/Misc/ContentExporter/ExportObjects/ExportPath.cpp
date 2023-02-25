//-------------------------------------------------------------------------------------
//  ExportPath.cpp
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "ExportPath.h"

const CHAR g_VolumeSeparator = ':';
const CHAR g_DirectorySeparator = '\\';
const CHAR g_AltDirectorySeparator = '/';

ExportPath::ExportPath()
{
    Initialize( "" );
}

ExportPath::ExportPath( const CHAR* strPath )
{
    Initialize( strPath );
}

ExportPath::ExportPath( const ExportPath& OtherPath )
{
    Initialize( (const CHAR*)OtherPath );
}

ExportPath& ExportPath::operator=( const ExportPath& OtherPath )
{
	Initialize( (const CHAR*)OtherPath );
	return *this;
}

VOID ExportPath::SetPathAndFileName( const CHAR* strPath )
{
    Initialize( strPath );
}

VOID ExportPath::SetPathOnly( const CHAR* strPath )
{
    const CHAR* strLastSlash = strrchr( strPath, g_DirectorySeparator );
    if( strLastSlash == NULL )
    {
        strLastSlash = strrchr( strPath, g_AltDirectorySeparator );
    }
    if( strLastSlash == NULL || strLastSlash[1] != '\0' )
    {
        CHAR strModifiedPath[MAX_PATH];
        strcpy_s( strModifiedPath, strPath );
        strcat_s( strModifiedPath, "\\" );
        Initialize( strModifiedPath );
    }
    else
    {
        Initialize( strPath );
    }
}

VOID ExportPath::Initialize( const CHAR* strPath )
{
    // Copy in our new path if one is provided
    if( strPath != NULL )
    {
        strcpy_s( m_strPath, strPath );
    }

    // Convert all slashes to the same slash
    CHAR* strSrc = m_strPath;
    while( *strSrc != '\0' )
    {
        if( *strSrc == g_AltDirectorySeparator )
        {
            *strSrc = g_DirectorySeparator;
        }
        ++strSrc;
    }

    // Look for the file extension
    m_strExtension = strrchr( m_strPath, '.' );

    // Look for the file name
    m_strFileName = strrchr( m_strPath, g_DirectorySeparator );
    if( m_strFileName != NULL )
    {
        ++m_strFileName;
    }
    else
    {
        m_strFileName = m_strPath;
    }
}

VOID ExportPath::ChangeExtension( const CHAR* strExtension )
{
    // NULL extension means empty string
    if( strExtension == NULL )
    {
        strExtension = "";
    }

    // make sure extension does not include a leading period
    while( *strExtension == '.' && *strExtension != '\0' )
    {
        ++strExtension;
    }

    // make sure we have a valid filename
    assert( m_strFileName != NULL );

    if( m_strExtension == NULL )
    {
        // append a trailing period on the path
        strcat_s( m_strPath, "." );
    }
    else
    {
        // ensure that the path has a trailing period
        assert( *m_strExtension == '.' );
        m_strExtension[1] = '\0';
    }

    // concatenate extension onto path
    strcat_s( m_strPath, strExtension );

    // re-initialize all of our pointers
    Initialize( NULL );
}

VOID ExportPath::ChangeFileName( const CHAR* strFileName )
{
    // Ensure that incoming filename has no separators
    const CHAR* strSep = strchr( strFileName, g_DirectorySeparator );
    assert( strSep == NULL );
    strSep = strchr( strFileName, g_AltDirectorySeparator );
    assert( strSep == NULL );

    // save current extension
    CHAR strExtension[MAX_PATH] = "";
    if( m_strExtension != NULL )
    {
        strcpy_s( strExtension, m_strExtension );
    }

    // trim off current file name
    assert( m_strFileName != NULL );
    *m_strFileName = '\0';

    // concatenate filename onto path
    strcat_s( m_strPath, strFileName );

    // concatenate original extension
    strcat_s( m_strPath, strExtension );

    // re-initialize all of our pointers
    Initialize( NULL );
}

VOID ExportPath::ChangeFileName( const ExportPath& OtherPath )
{
    ChangeFileName( (const CHAR*)OtherPath.GetFileNameWithoutExtension() );
}

VOID ExportPath::ChangeFileNameWithExtension( const CHAR* strFileName )
{
	// Ensure that incoming filename has no separators
	const CHAR* strSep = strchr( strFileName, g_DirectorySeparator );
	assert( strSep == NULL );
	strSep = strchr( strFileName, g_AltDirectorySeparator );
	assert( strSep == NULL );

	// trim off current file name
	assert( m_strFileName != NULL );
	*m_strFileName = '\0';

	// concatenate filename onto path
	strcat_s( m_strPath, strFileName );

	// re-initialize all of our pointers
	Initialize( NULL );
}

VOID ExportPath::ChangeFileNameWithExtension( const ExportPath& OtherPath )
{
	ChangeFileNameWithExtension( (const CHAR*)OtherPath.GetFileName() );
}

ExportPath ExportPath::GetFileName() const
{
    return ExportPath( m_strFileName );
}

BOOL ExportPath::HasFileName() const
{
    // If there is no filename, the filename pointer will point to the end of the string
    return ( *m_strFileName != '\0' );
}

ExportPath ExportPath::GetFileNameWithoutExtension() const
{
    // if we have no extension, just return the whole filename
    if( m_strExtension == NULL )
    {
        return GetFileName();
    }

    CHAR strFileName[MAX_PATH];

    // copy only the chars between the filename and the extension period
    DWORD dwCount = (DWORD)( m_strExtension - m_strFileName );
    assert( dwCount < MAX_PATH );
    strncpy_s( strFileName, m_strFileName, dwCount );
    strFileName[dwCount] = '\0';

    return ExportPath( strFileName );
}

ExportPath ExportPath::GetDirectory() const
{
    CHAR strDirectory[MAX_PATH];

    // copy only the chars between the path and the start of the filename
    DWORD dwCount = (DWORD)( m_strFileName - m_strPath );
    assert( dwCount < MAX_PATH );
    strncpy_s( strDirectory, m_strPath, dwCount );
    strDirectory[dwCount] = '\0';

    return ExportPath( strDirectory );
}

const CHAR* ExportPath::GetExtension() const
{
    if( m_strExtension == NULL )
        return NULL;

    // return the extension without the period
    return m_strExtension + 1;
}

BOOL ExportPath::IsAbsolutePath() const
{
    // look for volume separator in the path
    const CHAR* strVolumeSep = strchr( m_strPath, g_VolumeSeparator );
    if( strVolumeSep != NULL )
    {
        return TRUE;
    }

    // look for two leading slashes
    return ( m_strPath[0] == g_DirectorySeparator && m_strPath[1] == g_DirectorySeparator );
}

VOID ExportPath::Append( const ExportPath& OtherPath )
{
    // the other path can't be an absolute path
    assert( !OtherPath.IsAbsolutePath() );

    // if we don't have a filename, just concatenate the strings
    if( !HasFileName() )
    {
        strcat_s( m_strPath, (const CHAR*)OtherPath );

        Initialize( NULL );
        return;
    }

    // we have a filename, and we're appending a path.  assume a directory append.
    // save the existing filename
    CHAR strFileName[MAX_PATH];
    strcpy_s( strFileName, m_strFileName );

    // trim off the filename
    *m_strFileName = '\0';

    // append the other path
    strcat_s( m_strPath, (const CHAR*)OtherPath );

    // if the other path has a filename, we'll use that
    if( OtherPath.HasFileName() )
    {
        Initialize( NULL );
        return;
    }

    // otherwise, use our filename
    strcat_s( m_strPath, strFileName );

    // re-initialize all of our pointers
    Initialize( NULL );
}

VOID ExportPath::AppendToFileName( const CHAR* strText )
{
    if( strText == NULL )
        return;

    if( !HasFileName() )
    {
        strcat_s( m_strPath, strText );
        Initialize( NULL );
        return;
    }

    // save current extension and then trim it off
    CHAR strExtension[MAX_PATH] = "";
    if( m_strExtension != NULL )
    {
        strcpy_s( strExtension, m_strExtension );
        *m_strExtension = '\0';
    }

    // concatenate filename onto path
    strcat_s( m_strPath, strText );

    // concatenate original extension
    strcat_s( m_strPath, strExtension );

    // re-initialize all of our pointers
    Initialize( NULL );
}

VOID ExportPath::TrimOffFileName()
{
    // trim off the filename
    *m_strFileName = '\0';

    // re-initialize all of our pointers
    Initialize( NULL );
}

ExportPath ExportPath::GetTempPath()
{
    CHAR strPath[MAX_PATH];
    ::GetTempPathA( MAX_PATH, strPath );
    return ExportPath( strPath );
}

ExportPath ExportPath::GetCurrentPath()
{
    CHAR strPath[MAX_PATH];
    ::GetCurrentDirectoryA( MAX_PATH, strPath );
	strcat_s( strPath, "\\" );
    return ExportPath( strPath );
}
