//-------------------------------------------------------------------------------------
//  ExportXmlParser.h
//
//  A simple non-validating XML parser.
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once
#ifndef ATGXMLPARSER_H
#define ATGXMLPARSER_H

namespace ATG
{

//-----------------------------------------------------------------------------
// error returns from XMLParse
//-----------------------------------------------------------------------------
#define _ATGFAC 0x61B
#define E_COULD_NOT_OPEN_FILE   MAKE_HRESULT(1, _ATGFAC, 0x0001 )
#define E_INVALID_XML_SYNTAX    MAKE_HRESULT(1, _ATGFAC, 0x0002 )


CONST UINT XML_MAX_ATTRIBUTES_PER_ELEMENT  =   32;
CONST UINT XML_MAX_NAME_LENGTH             =   128;
CONST UINT XML_READ_BUFFER_SIZE            =   2048;
CONST UINT XML_WRITE_BUFFER_SIZE           =   2048;   

// No tag can be longer than XML_WRITE_BUFFER_SIZE - an error will be returned if 
// it is

//-------------------------------------------------------------------------------------
struct XMLAttribute
{
    WCHAR*  strName;
    UINT    NameLen;
    WCHAR*  strValue;
    UINT    ValueLen;       
};

//-------------------------------------------------------------------------------------
class ISAXCallback
{
friend class XMLParser;
public:
    ISAXCallback() {};
    ~ISAXCallback() {};

    virtual HRESULT  StartDocument() = 0;
    virtual HRESULT  EndDocument() = 0;

    virtual HRESULT  ElementBegin( CONST WCHAR* strName, UINT NameLen, 
                                   CONST XMLAttribute *pAttributes, UINT NumAttributes ) = 0;
    virtual HRESULT  ElementContent( CONST WCHAR *strData, UINT DataLen, BOOL More ) = 0;
    virtual HRESULT  ElementEnd( CONST WCHAR *strName, UINT NameLen ) = 0;

    virtual HRESULT  CDATABegin( ) = 0;
    virtual HRESULT  CDATAData( CONST WCHAR *strCDATA, UINT CDATALen, BOOL bMore ) = 0;
    virtual HRESULT  CDATAEnd( ) = 0;

    virtual VOID     Error( HRESULT hError, CONST CHAR *strMessage ) = 0;

    virtual VOID     SetParseProgress( DWORD dwProgress ) { }

    const CHAR*      GetFilename() { return m_strFilename; }
    UINT             GetLineNumber() { return m_LineNum; }
    UINT             GetLinePosition() { return m_LinePos; }

private:
    CONST CHAR *m_strFilename;
    UINT        m_LineNum;
    UINT        m_LinePos;
};


//-------------------------------------------------------------------------------------
class XMLParser
{
public:    
    XMLParser();
    ~XMLParser();
   
    //      Register an interface inheiriting from ISAXCallback
    VOID            RegisterSAXCallbackInterface( ISAXCallback *pISAXCallback );
    
    //      Get the registered interface
    ISAXCallback*   GetSAXCallbackInterface();    

    //      ParseXMLFile returns one of the following:
    //         E_COULD_NOT_OPEN_FILE - couldn't open the file    
    //         E_INVALID_XML_SYNTAX - bad XML syntax according to this parser
    //         E_NOINTERFACE - RegisterSAXCallbackInterface not called
    //         E_ABORT - callback returned a fail code
    //         S_OK - file parsed and completed   

    HRESULT    ParseXMLFile( CONST CHAR *strFilename );                              
    
    //      Parses from a buffer- if you pass a WCHAR buffer (and cast it), it will 
    //         correctly detect it and use unicode instead.  Return codes are the
    //         same as for ParseXMLFile

    HRESULT    ParseXMLBuffer( CONST CHAR* strBuffer, UINT uBufferSize );    

private:      
    HRESULT    MainParseLoop();

    HRESULT    AdvanceCharacter( BOOL bOkToFail = FALSE ); 
    VOID       SkipNextAdvance();           

    HRESULT    ConsumeSpace();            
    HRESULT    ConvertEscape();           
    HRESULT    AdvanceElement();           
    HRESULT    AdvanceName();            
    HRESULT    AdvanceAttrVal();           
    HRESULT    AdvanceCDATA();           
    HRESULT    AdvanceComment();          

    VOID    FillBuffer();
    
    VOID    Error( HRESULT hRet, CONST CHAR* strFormat, ... );

    ISAXCallback*   m_pISAXCallback;    

    HANDLE          m_hFile;    
    CONST CHAR*     m_pInXMLBuffer; 
    UINT            m_uInXMLBufferCharsLeft;
    DWORD           m_dwCharsTotal;
    DWORD           m_dwCharsConsumed;

    BYTE            m_pReadBuf[ XML_READ_BUFFER_SIZE + 2 ]; // room for a trailing NULL
    WCHAR           m_pWriteBuf[ XML_WRITE_BUFFER_SIZE ];    

    BYTE*           m_pReadPtr;
    WCHAR*          m_pWritePtr;        // write pointer within m_pBuf      

    BOOL            m_bUnicode;         // TRUE = 16-bits, FALSE = 8-bits
    BOOL            m_bReverseBytes;    // TRUE = reverse bytes, FALSE = don't reverse
    
    BOOL            m_bSkipNextAdvance;
    WCHAR           m_Ch;               // Current character being parsed
};

}  // namespace ATG

#endif
