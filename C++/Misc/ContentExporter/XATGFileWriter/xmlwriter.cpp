//-------------------------------------------------------------------------------------
//  xmlwriter.h
//  
//  A simple XML writer.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include "xmlwriter.h"

#define WRITE_BUFFER_SIZE 16384

namespace ATG
{
    //----------------------------------------------------------------------------------
    // Name: XMLWriter
    // Desc: Constructor for the XML writer class.
    //----------------------------------------------------------------------------------
    XMLWriter::XMLWriter()
        : m_strBuffer( NULL ),
          m_uBufferSizeRemaining( 0 ),
          m_hFile( INVALID_HANDLE_VALUE ),
          m_strNameStackTop( m_strNameStack ),
          m_uIndentCount( 0 ),
          m_bValid( FALSE )
    {
    }


    //----------------------------------------------------------------------------------
    // Name: XMLWriter
    // Desc: Constructor for the XML writer class.
    //----------------------------------------------------------------------------------
    XMLWriter::XMLWriter( CONST CHAR* strFileName )
    {
        Initialize( strFileName );
    }


    //----------------------------------------------------------------------------------
    // Name: XMLWriter
    // Desc: Constructor for the XML writer class.
    //----------------------------------------------------------------------------------
    XMLWriter::XMLWriter( CHAR* strBuffer, UINT uBufferSize )
    {
        Initialize( strBuffer, uBufferSize );
    }


    //----------------------------------------------------------------------------------
    // Name: ~XMLWriter
    // Desc: Destructor for the XML writer class.
    //----------------------------------------------------------------------------------
    XMLWriter::~XMLWriter()
    {
        Close();
    }


    //----------------------------------------------------------------------------------
    // Name: Initialize
    // Desc: Sets up the XML writer to write to a file.
    //----------------------------------------------------------------------------------
    VOID XMLWriter::Initialize( CONST CHAR* strFileName )
    {
        m_strBuffer = new CHAR[WRITE_BUFFER_SIZE];
        m_strBufferStart = m_strBuffer;
        m_uBufferSizeRemaining = WRITE_BUFFER_SIZE;
        m_bOpenTagFinished = TRUE;
        m_bWriteCloseTagIndent = FALSE;
        m_hFile = CreateFile( strFileName, FILE_WRITE_DATA, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
        m_strNameStack[0] = '\0';
        m_strNameStackTop = m_strNameStack;
        m_NameStackPositions.clear();
        SetIndentCount( 4 );
        m_bWriteNewlines = TRUE;
        if( m_hFile == INVALID_HANDLE_VALUE )
        {
            m_bValid = FALSE;
        }
        else
        {
            m_bValid = TRUE;
        }
    }


    //----------------------------------------------------------------------------------
    // Name: Initialize
    // Desc: Sets up the XML writer to write to a string buffer.
    //----------------------------------------------------------------------------------
    VOID XMLWriter::Initialize( CHAR* strBuffer, UINT uBufferSize )
    {
        m_strBuffer = strBuffer;
        m_strBufferStart = m_strBuffer;
        m_uBufferSizeRemaining = uBufferSize;
        m_hFile = INVALID_HANDLE_VALUE;
        m_bOpenTagFinished = TRUE;
        m_bWriteCloseTagIndent = FALSE;
        m_strNameStack[0] = '\0';
        m_strNameStackTop = m_strNameStack;
        m_NameStackPositions.clear();
        SetIndentCount( 0 );
        m_bWriteNewlines = FALSE;
        m_bValid = TRUE;
    }


    //----------------------------------------------------------------------------------
    // Name: Close
    // Desc: Cleans up the output of the XML writing operation.
    //----------------------------------------------------------------------------------
    VOID XMLWriter::Close()
    {
        if( m_hFile != INVALID_HANDLE_VALUE )
        {
            FlushBufferToFile();
            CloseHandle( m_hFile );
            delete[] m_strBufferStart;
            m_strBufferStart = NULL;
            m_strBuffer = NULL;
            m_hFile = INVALID_HANDLE_VALUE;
        }
        if( m_strBuffer != NULL )
        {
            m_strBuffer = NULL;
            m_strBufferStart = NULL;
            m_uBufferSizeRemaining = 0;
        }
    }


    VOID XMLWriter::FlushBufferToFile()
    {
        if( m_uBufferSizeRemaining >= WRITE_BUFFER_SIZE || m_hFile == INVALID_HANDLE_VALUE )
            return;
        DWORD dwBytesWritten = 0;
        WriteFile( m_hFile, m_strBufferStart, WRITE_BUFFER_SIZE - m_uBufferSizeRemaining, &dwBytesWritten, NULL );
        m_uBufferSizeRemaining = WRITE_BUFFER_SIZE;
        m_strBuffer = m_strBufferStart;
    }


    //----------------------------------------------------------------------------------
    // Name: SetIndentCount
    // Desc: Builds a string with the correct amount of indentation spaces.
    //----------------------------------------------------------------------------------
    VOID XMLWriter::SetIndentCount( UINT uSpaces )
    {
        m_uIndentCount = ( uSpaces > 8 ) ? 8 : uSpaces;
        if( m_uIndentCount > 0 )
            memset( m_strIndent, ' ', m_uIndentCount );
        m_strIndent[ m_uIndentCount ] = '\0';
    }


    //----------------------------------------------------------------------------------
    // Name: StartElement
    // Desc: Writes the beginning of an XML open tag.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::StartElement( CONST CHAR* strName )
    {
        if( !m_bOpenTagFinished )
        {
            if( !EndOpenTag() ) 
                return FALSE;
            if( !WriteNewline() )
                return FALSE;
        }
        BOOL result = TRUE;
        result &= WriteIndent();
        PushName( strName );
        result &= OutputStringFast( "<", 1 );
        result &= OutputString( strName );
        m_bOpenTagFinished = FALSE;
        m_bWriteCloseTagIndent = FALSE;
        return result;
    }


    //----------------------------------------------------------------------------------
    // Name: EndElement
    // Desc: Writes an element close tag corresponding with the most recent open tag.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::EndElement()
    {
        CONST CHAR* strName = PopName();
        if( strName == NULL )
            return FALSE;
        BOOL result = TRUE;
        if( !m_bOpenTagFinished )
        {
            m_bOpenTagFinished = TRUE;
            result &= OutputStringFast( " />", 3 );
            result &= WriteNewline();
            m_bWriteCloseTagIndent = TRUE;
            return result;
        }
        if( m_bWriteCloseTagIndent )
            result &= WriteIndent();
        result &= OutputStringFast( "</", 2 );
        result &= OutputString( strName );
        result &= OutputStringFast( ">", 1 );
        result &= WriteNewline();
        m_bWriteCloseTagIndent = TRUE;
        return result;
    }


    //----------------------------------------------------------------------------------
    // Name: WriteElement
    // Desc: Convenience function to write an XML element with a body and no attributes.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::WriteElement( CONST CHAR* strName, CONST CHAR* strBody )
    {
        BOOL result = TRUE;
        result &= StartElement( strName );
        result &= WriteString( strBody );
        result &= EndElement();
        return result;
    }


    //----------------------------------------------------------------------------------
    // Name: WriteElement
    // Desc: Convenience function to write an XML element with a body and no attributes.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::WriteElement( CONST CHAR* strName, INT iBody )
    {
        BOOL result = TRUE;
        result &= StartElement( strName );
        CHAR strTemp[32];
        _itoa_s( iBody, strTemp, 10 );
        result &= WriteString( strTemp );
        result &= EndElement();
        return result;
    }


    //----------------------------------------------------------------------------------
    // Name: WriteElement
    // Desc: Convenience function to write an XML element with a body and no attributes.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::WriteElement( CONST CHAR* strName, FLOAT fBody )
    {
        BOOL result = TRUE;
        result &= StartElement( strName );
        CHAR strTemp[32];
        sprintf_s( strTemp, "%f", fBody );
        result &= WriteString( strTemp );
        result &= EndElement();
        return result;
    }


    //----------------------------------------------------------------------------------
    // Name: WriteElementFormat
    // Desc: Convenience function to write an XML element with a body and no attributes.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::WriteElementFormat( CONST CHAR* strName, CONST CHAR* strFormat, ... )
    {
        BOOL result = TRUE;
        result &= StartElement( strName );
        CHAR strTemp[512];
        va_list args;
        va_start( args, strFormat );
        vsprintf_s( strTemp, strFormat, args );
        result &= WriteString( strTemp );
        result &= EndElement();
        return result;
    }


    //----------------------------------------------------------------------------------
    // Name: StartCDATA
    // Desc: Starts a CDATA block.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::StartCDATA()
    {
        BOOL bResult = TRUE;
        if( !m_bOpenTagFinished )
        {
            if( !EndOpenTag() ) 
                return FALSE;
            if( !WriteNewline() )
                return FALSE;
        }
        bResult &= WriteIndent();
        bResult &= OutputStringFast( "<![CDATA[", 9 );
        return bResult;
    }


    //----------------------------------------------------------------------------------
    // Name: EndCDATA
    // Desc: Ends a CDATA block.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::EndCDATA()
    {
        BOOL bResult = TRUE;
        bResult &= OutputStringFast( "]]>", 3 );
        bResult &= WriteNewline();
        m_bWriteCloseTagIndent = TRUE;
        return bResult;
    }


    //----------------------------------------------------------------------------------
    // Name: WriteCDATA
    // Desc: Writes a CDATA block.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::WriteCDATA( CONST CHAR* strData, DWORD dwDataLength )
    {
        BOOL bResult = StartCDATA();
        bResult &= OutputStringFast( strData, dwDataLength );
        bResult &= EndCDATA();
        return bResult;
    }


    //----------------------------------------------------------------------------------
    // Name: StartComment
    // Desc: Writes the beginning of an XML comment tag.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::StartComment( BOOL bInline )
    {
        if( !m_bOpenTagFinished )
        {
            if( !EndOpenTag() ) 
                return FALSE;
            if( !bInline && !WriteNewline() )
                return FALSE;
        }
        BOOL result = TRUE;
        if( !bInline )
            result &= WriteIndent();
        result &= OutputStringFast( "<!-- ", 5 );
        m_bOpenTagFinished = TRUE;
        m_bWriteCloseTagIndent = FALSE;
        m_bInlineComment = bInline;
        return result;
    }


    //----------------------------------------------------------------------------------
    // Name: EndComment
    // Desc: Writes a comment close tag.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::EndComment()
    {
        BOOL result = TRUE;
        result &= OutputStringFast( " -->", 4 );
        if( !m_bInlineComment )
            result &= WriteNewline();
        m_bWriteCloseTagIndent = !m_bInlineComment;
        return result;
    }


    //----------------------------------------------------------------------------------
    // Name: WriteComment
    // Desc: Convenience function to write an entire comment.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::WriteComment( CONST CHAR* strComment, BOOL bInline )
    {
        BOOL result = TRUE;
        result &= StartComment( bInline );
        result &= WriteString( strComment );
        result &= EndComment();
        return result;
    }


    //----------------------------------------------------------------------------------
    // Name: AddAttributeFormat
    // Desc: Adds a key-value attribute pair to an XML open tag.  This must be called
    //       after calling StartElement(), but before calling WriteString() or
    //       EndElement().
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::AddAttributeFormat( CONST CHAR* strName, CONST CHAR* strFormat, ... )
    {
        if( m_bOpenTagFinished )
            return FALSE;
        BOOL result = TRUE;
        result &= OutputStringFast( " ", 1 );
        result &= OutputString( strName );
        result &= OutputStringFast( "=\"", 2 );
        CHAR strTemp[256];
        va_list args;
        va_start( args, strFormat );
        vsprintf_s( strTemp, strFormat, args );
        result &= OutputString( strTemp );
        result &= OutputStringFast( "\"", 1 );
        return result;
    }


    //----------------------------------------------------------------------------------
    // Name: AddAttribute
    // Desc: Adds a key-value attribute pair to an XML open tag.  This must be called
    //       after calling StartElement(), but before calling WriteString() or
    //       EndElement().
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::AddAttribute( CONST CHAR* strName, CONST CHAR* strValue )
    {
        if( m_bOpenTagFinished )
            return FALSE;
        BOOL result = TRUE;
        result &= OutputStringFast( " ", 1 );
        result &= OutputString( strName );
        result &= OutputStringFast( "=\"", 2 );
        result &= OutputString( strValue );
        result &= OutputStringFast( "\"", 1 );
        return result;
    }


    //----------------------------------------------------------------------------------
    // Name: AddAttribute
    // Desc: Adds a key-value attribute pair to an XML open tag.  This must be called
    //       after calling StartElement(), but before calling WriteString() or
    //       EndElement().
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::AddAttribute( CONST CHAR* strName, CONST WCHAR* wstrValue )
    {
        CHAR strTemp[256];
        WideCharToMultiByte( CP_ACP, 0, wstrValue, (INT)wcslen( wstrValue ) + 1, strTemp, 256, NULL, NULL );
        return AddAttribute( strName, strTemp );
    }


    //----------------------------------------------------------------------------------
    // Name: AddAttribute
    // Desc: Adds a key-value attribute pair to an XML open tag.  This must be called
    //       after calling StartElement(), but before calling WriteString() or
    //       EndElement().
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::AddAttribute( CONST CHAR* strName, INT iValue )
    {
        CHAR strTemp[20];
        _itoa_s( iValue, strTemp, 10 );
        return AddAttribute( strName, strTemp );
    }


    //----------------------------------------------------------------------------------
    // Name: AddAttribute
    // Desc: Adds a key-value attribute pair to an XML open tag.  This must be called
    //       after calling StartElement(), but before calling WriteString() or
    //       EndElement().
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::AddAttribute( CONST CHAR* strName, FLOAT fValue )
    {
        CHAR strTemp[20];
        sprintf_s( strTemp, "%f", fValue );
        return AddAttribute( strName, strTemp );
    }


    //----------------------------------------------------------------------------------
    // Name: WriteString
    // Desc: Writes a string after an XML open tag.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::WriteString( CONST CHAR* strText )
    {
        if( strText == NULL )
            strText = "";
        if( !m_bOpenTagFinished )
        {
            if( !EndOpenTag() )
                return FALSE;
        }
        return OutputString( strText );
    }


    //----------------------------------------------------------------------------------
    // Name: WriteStringFormat
    // Desc: Writes a formatted string after an XML open tag.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::WriteStringFormat( CONST CHAR* strFormat, ... )
    {
        if( !m_bOpenTagFinished )
        {
            if( !EndOpenTag() )
                return FALSE;
        }
        CHAR strTemp[1024];
        va_list args;
        va_start( args, strFormat );
        vsprintf_s( strTemp, strFormat, args );
        return OutputString( strTemp );
    }


    //----------------------------------------------------------------------------------
    // Name: EndOpenTag
    // Desc: Writes the closing angle bracket of an XML open tag, and sets the proper
    //       state.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::EndOpenTag()
    {
        assert( !m_bOpenTagFinished );
        OutputStringFast( ">", 1 );
        m_bOpenTagFinished = TRUE;
        return TRUE;
    }


    //----------------------------------------------------------------------------------
    // Name: WriteNewline
    // Desc: Writes a new line, if that option is enabled.  Writes a hard return to
    //       files, and a soft return to buffers.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::WriteNewline()
    {
        if( !m_bWriteNewlines )
            return TRUE;
        if( m_hFile != INVALID_HANDLE_VALUE )
            return OutputStringFast( "\r\n", 2 );
        return OutputStringFast( "\n", 1 );
    }


    //----------------------------------------------------------------------------------
    // Name: WriteIndent
    // Desc: Writes an indentation using spaces if indentation is enabled.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::WriteIndent()
    {
        if( m_uIndentCount == 0 )
            return TRUE;
        for( UINT i = 0; i < m_NameStackPositions.size(); i++ )
        {
            if( !OutputStringFast( m_strIndent, m_uIndentCount ) )
                return FALSE;
        }
        return TRUE;
    }


    //----------------------------------------------------------------------------------
    // Name: PushName
    // Desc: Pushes an XML tag name onto the stack.  This is used to write an open tag.
    //----------------------------------------------------------------------------------
    VOID XMLWriter::PushName( CONST CHAR* strName )
    {
        UINT uLen = (UINT)strlen( strName );
        if( ( m_strNameStackTop - m_strNameStack + uLen ) >= XMLWRITER_NAME_STACK_SIZE )
        {
            assert( false );
            return;
        }
        m_NameStackPositions.push_back( (UINT)( m_strNameStackTop - m_strNameStack ) );
        *m_strNameStackTop = '\0';
        strcat_s( m_strNameStack, strName );
        m_strNameStackTop += uLen;
    }


    //----------------------------------------------------------------------------------
    // Name: PopName
    // Desc: Pops an XML tag name off the stack.  This is used to write a close tag.
    //----------------------------------------------------------------------------------
    CONST CHAR* XMLWriter::PopName()
    {
        if( m_NameStackPositions.size() == 0 )
            return NULL;
        UINT uPos = m_NameStackPositions.back();
        m_NameStackPositions.pop_back();
        *m_strNameStackTop = '\0';
        m_strNameStackTop = m_strNameStack + uPos;
        return m_strNameStackTop;
    }


    //----------------------------------------------------------------------------------
    // Name: OutputString
    // Desc: Sends a null-terminated string to the output.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::OutputString( CONST CHAR* strText )
    {
        assert( strText != NULL );
        return OutputStringFast( strText, (UINT)strlen( strText ) );
    }


    //----------------------------------------------------------------------------------
    // Name: OutputStringFast
    // Desc: Sends a string with a supplied length to the output.
    //----------------------------------------------------------------------------------
    BOOL XMLWriter::OutputStringFast( CONST CHAR* strText, UINT uLength )
    {
        if( m_hFile != INVALID_HANDLE_VALUE )
        {
            while( uLength >= m_uBufferSizeRemaining )
            {
                memcpy( m_strBuffer, strText, m_uBufferSizeRemaining );
                m_strBuffer += m_uBufferSizeRemaining;
                strText += m_uBufferSizeRemaining;
                uLength -= m_uBufferSizeRemaining;
                m_uBufferSizeRemaining = 0;
                FlushBufferToFile();
            }
            memcpy( m_strBuffer, strText, uLength );
            m_uBufferSizeRemaining -= uLength;
            m_strBuffer += uLength;
            return TRUE;
        }
        else if( m_strBuffer != NULL )
        {
            if( ( uLength + 1 ) > m_uBufferSizeRemaining )
                return FALSE;
            memcpy( m_strBuffer, strText, uLength + 1 );
            m_uBufferSizeRemaining -= uLength;
            m_strBuffer += uLength;
            return TRUE;
        }
        return FALSE;
    }
}