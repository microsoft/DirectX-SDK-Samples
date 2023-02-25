//-------------------------------------------------------------------------------------
//  xmlwriter.h
//  
//  A simple XML writer.
//  
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

#define XMLWRITER_NAME_STACK_SIZE 255

namespace ATG
{
    class XMLWriter
    {
    public:
        XMLWriter();
        XMLWriter( CHAR* strBuffer, UINT uBufferSize );
        XMLWriter( CONST CHAR* strFileName );
        ~XMLWriter();

        VOID Initialize( CHAR* strBuffer, UINT uBufferSize );
        VOID Initialize( CONST CHAR* strFileName );
        VOID Close();

        BOOL IsValid() const { return m_bValid; }

        VOID SetIndentCount( UINT uSpaces );
        VOID EnableNewlines( BOOL bWriteNewLines ) { m_bWriteNewlines = bWriteNewLines; }

        BOOL StartElement( CONST CHAR* strName );
        BOOL EndElement();
        BOOL WriteElement( CONST CHAR* strName, CONST CHAR* strBody );
        BOOL WriteElement( CONST CHAR* strName, INT iBody );
        BOOL WriteElement( CONST CHAR* strName, FLOAT fBody );
        BOOL WriteElementFormat( CONST CHAR* strName, CONST CHAR* strFormat, ... );

        BOOL StartCDATA();
        BOOL EndCDATA();
        BOOL WriteCDATA( CONST CHAR* strData, DWORD dwDataLength );

        BOOL StartComment( BOOL bInline = FALSE );
        BOOL EndComment();
        BOOL WriteComment( CONST CHAR* strComment, BOOL bInline = FALSE );

        BOOL AddAttributeFormat( CONST CHAR* strName, CONST CHAR* strFormat, ... );
        BOOL AddAttribute( CONST CHAR* strName, CONST CHAR* strValue );
        BOOL AddAttribute( CONST CHAR* strName, CONST WCHAR* wstrValue );
        BOOL AddAttribute( CONST CHAR* strName, INT iValue );
        BOOL AddAttribute( CONST CHAR* strName, FLOAT fValue );

        BOOL WriteString( CONST CHAR* strText );
        BOOL WriteStringFormat( CONST CHAR* strFormat, ... );

    private:

        VOID PushName( CONST CHAR* strName );
        CONST CHAR* PopName();

        inline BOOL EndOpenTag();
        inline BOOL WriteNewline();
        inline BOOL WriteIndent();

        inline BOOL OutputString( CONST CHAR* strText );
        inline BOOL OutputStringFast( CONST CHAR* strText, UINT uLength );
        VOID FlushBufferToFile();

        HANDLE          m_hFile;
        CHAR*           m_strBuffer;
        CHAR*           m_strBufferStart;
        UINT            m_uBufferSizeRemaining;

        CHAR            m_strNameStack[XMLWRITER_NAME_STACK_SIZE];
        CHAR*           m_strNameStackTop;
        UINT            m_uNameStackSize;
        std::vector<UINT>    m_NameStackPositions;
        UINT            m_uIndentCount;
        CHAR            m_strIndent[9];
        BOOL            m_bWriteNewlines;

        BOOL            m_bOpenTagFinished;
        BOOL            m_bWriteCloseTagIndent;
        BOOL            m_bInlineComment;

        BOOL            m_bValid;
    };

    class IXMLSerializable
    {
    public:
        virtual BOOL Serialize( XMLWriter* pXMLWriter ) = NULL;
    };
}