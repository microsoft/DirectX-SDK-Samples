//-------------------------------------------------------------------------------------
//  ExportString.h
//
//  A pooled string class that makes string manipulation easier within the export code.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

// Change EXPORTSTRING_COMPARE to strcmp to do case-sensitive string pooling.
#define EXPORTSTRING_COMPARE _stricmp
// EXPORTSTRING_HASHSIZE must be a power of 2, so just change the shift value.
#define EXPORTSTRING_HASHSIZE (1 << 4)

namespace ATG
{

class ExportMutableString
{
public:
    ExportMutableString()
        : m_strValue( NULL )
    { }
    ExportMutableString( const CHAR* strCopy )
    {
        SetAsCopy( strCopy );
    }
    ExportMutableString( const ExportMutableString& strCopy )
    {
        SetAsCopy( strCopy );
    }
    ~ExportMutableString()
    {
        Deallocate();
    }
    operator const CHAR* () const { return m_strValue; }
    ExportMutableString& operator=( const CHAR* strRHS )
    {
        SetAsCopy( strRHS );
        return *this;
    }
    ExportMutableString& operator=( const ExportMutableString& strRHS )
    {
        SetAsCopy( strRHS );
        return *this;
    }
protected:
    VOID SetAsCopy( const CHAR* strCopy )
    {
        if( strCopy == NULL )
        {
            Deallocate();
            return;
        }
        DWORD dwSize = (DWORD)strlen( strCopy ) + 1;
        Allocate( dwSize );
        strcpy_s( m_strValue, dwSize, strCopy );
    }
    VOID Allocate( DWORD dwSize )
    {
        Deallocate();
        m_strValue = new CHAR[dwSize];
    }
    VOID Deallocate()
    {
        if( m_strValue != NULL )
            delete[] m_strValue;
        m_strValue = NULL;
    }
    CHAR*   m_strValue;
};

class ExportString
{
public:
    ExportString() : m_strString( NULL ) {}
    ExportString( const CHAR* strString ) { m_strString = AddString( strString ); }
    ExportString( const ExportString& other ) : m_strString( other.m_strString ) {}

    ExportString& operator= ( const ExportString& RHS ) { m_strString = RHS.m_strString; return *this; }
    ExportString& operator= ( const CHAR* strRHS ) { m_strString = AddString( strRHS ); return *this; }

    BOOL operator== ( const ExportString& RHS ) const { return m_strString == RHS.m_strString; }
    inline BOOL operator== ( const CHAR* strRHS ) const;

    operator const CHAR* () const { return m_strString; }
    inline const CHAR* SafeString() const;
    inline static BYTE HashString( const CHAR* strString );
protected:
    inline static const CHAR* AddString( const CHAR* strString );
protected:
    const CHAR* m_strString;
};

const CHAR* ExportString::AddString( const CHAR* strString )
{
    if( strString == NULL )
        return NULL;
    typedef std::list< const CHAR* > StringList;
    static StringList s_StringLists[ EXPORTSTRING_HASHSIZE ];

    BYTE uBucketIndex = HashString( strString ) & ( EXPORTSTRING_HASHSIZE - 1 );
    StringList& CurrentList = s_StringLists[ uBucketIndex ];

    StringList::iterator iter = CurrentList.begin();
    StringList::iterator end = CurrentList.end();

    while( iter != end )
    {
        const CHAR* strTest = *iter;
        if( EXPORTSTRING_COMPARE( strTest, strString ) == 0 )
            return strTest;
        ++iter;
    }

    DWORD dwSize = (DWORD)strlen( strString ) + 1;
    CHAR* strCopy = new CHAR[ dwSize ];
    strcpy_s( strCopy, dwSize, strString );
    CurrentList.push_back( strCopy );
    return strCopy;
}

BOOL ExportString::operator== ( const CHAR* strRHS ) const
{
    if( strRHS == NULL )
    {
        if( m_strString == NULL )
            return TRUE;
        return FALSE;
    }
    else if( m_strString == NULL )
        return FALSE;

    if( m_strString == strRHS )
        return TRUE;

    return EXPORTSTRING_COMPARE( m_strString, strRHS ) == 0;
}

const CHAR* ExportString::SafeString() const
{
    if( m_strString == NULL )
        return "";
    return m_strString;
}

BYTE ExportString::HashString( const CHAR* strString )
{
    BYTE sum = 0;
    const BYTE* p = (const BYTE*)strString;
    if( *p ) sum += (*p++ & 0x1F); else return sum;
    if( *p ) sum += (*p++ & 0x1F); else return sum;
    if( *p ) sum += (*p++ & 0x1F); else return sum;
    if( *p ) sum += (*p++ & 0x1F); else return sum;
    if( *p ) sum += (*p++ & 0x1F); else return sum;
    if( *p ) sum += (*p++ & 0x1F); else return sum;
    if( *p ) sum += (*p++ & 0x1F); else return sum;
    if( *p ) sum += (*p++ & 0x1F);
    return sum;
}

};
