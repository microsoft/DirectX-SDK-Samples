//-------------------------------------------------------------------------------------
//  ConsoleMain.cpp
//
//  Entry point for the content exporter application.  Also contains all of the command
//  line parsing code.
//
//  Microsoft XNA Developer Connection
//  Copyright © Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "stdafx.h"
#include <conio.h>

#include "FBXImportMain.h"

CHAR g_strExporterName[100];

namespace ATG
{
    extern XATGExportSettings g_XATGSettings;
}

using namespace ATG;

ExportScene* g_pScene = NULL;
FBXTransformer g_FBXTransformer;

class ConsoleOutListener : public ILogListener
{
protected:
    HANDLE  m_hOut;
    WORD    m_wDefaultConsoleTextAttributes;
    WORD    m_wBackgroundAttributes;
public:
    ConsoleOutListener()
    {
        m_hOut = GetStdHandle( STD_OUTPUT_HANDLE );
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo( m_hOut, &csbi );
        m_wDefaultConsoleTextAttributes = csbi.wAttributes;
        m_wBackgroundAttributes = m_wDefaultConsoleTextAttributes & 0x00F0;
    }
    virtual VOID LogMessage( const CHAR* strMessage )
    {
        puts( strMessage );
    }
    virtual VOID LogWarning( const CHAR* strMessage )
    {
        SetConsoleTextAttribute( m_hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | m_wBackgroundAttributes );
        LogMessage( strMessage );
        SetConsoleTextAttribute( m_hOut, m_wDefaultConsoleTextAttributes );
    }
    virtual VOID LogError( const CHAR* strMessage )
    {
        SetConsoleTextAttribute( m_hOut, FOREGROUND_RED | FOREGROUND_INTENSITY | m_wBackgroundAttributes );
        LogMessage( strMessage );
        SetConsoleTextAttribute( m_hOut, m_wDefaultConsoleTextAttributes );
    }
};

BOOL g_bHelpPrinted = FALSE;
VOID PrintHelp();

ConsoleOutListener g_ConsoleOutListener;
DebugSpewListener g_DebugSpewListener;

ExportManifest g_Manifest;

typedef std::vector<ExportPath> FileNameVector;
FileNameVector g_InputFileNames;

ExportPath g_WorkingPath;
ExportPath g_OutputFilePath;
ExportPath g_CurrentOutputFileName;
ExportPath g_CurrentInputFileName;

enum ExportFileFormat
{
    FILEFORMAT_XATG = 0,
    FILEFORMAT_SDKMESH = 1,
};
INT g_ExportFileFormat = FILEFORMAT_XATG;

typedef BOOL MacroCommandCallback( const CHAR* strArgument );

struct MacroCommand
{
    const CHAR* strCommandLine;
    const CHAR* strAnnotation;
    const CHAR* strDescription;
    MacroCommandCallback* pCallback;
};

BOOL MacroDisplayHelp( const CHAR* strArgument )
{
    PrintHelp();
    return FALSE;
}

BOOL MacroSetOutputPath( const CHAR* strArgument )
{
    g_OutputFilePath.SetPathOnly( strArgument );
    return TRUE;
}

BOOL MacroCollisionMesh( const CHAR* strArgument )
{
    g_pScene->Settings().bExportAnimations = FALSE;
    g_pScene->Settings().bCompressVertexData = FALSE;
    g_pScene->Settings().bComputeVertexTangentSpace = FALSE;
    g_pScene->Settings().bExportBinormal = FALSE;
    g_pScene->Settings().bExportCameras = FALSE;
    g_pScene->Settings().bExportLights = FALSE;
    g_pScene->Settings().bExportMaterials = FALSE;
    g_pScene->Settings().bExportNormals = FALSE;
    g_pScene->Settings().bExportSkinWeights = FALSE;
    g_pScene->Settings().bForceIndex32Format = TRUE;
    g_pScene->Settings().iMaxUVSetCount = 0;
    g_XATGSettings.bBundleTextures = FALSE;
    g_XATGSettings.bUseExistingBundle = FALSE;
    strcpy_s( g_pScene->Settings().strMeshNameDecoration, "CollisionMesh" );
    return FALSE;
}

BOOL MacroAnimation( const CHAR* strArgument )
{
    g_pScene->Settings().bExportAnimations = TRUE;
    g_pScene->Settings().bCompressVertexData = FALSE;
    g_pScene->Settings().bComputeVertexTangentSpace = FALSE;
    g_pScene->Settings().bExportBinormal = FALSE;
    g_pScene->Settings().bExportCameras = FALSE;
    g_pScene->Settings().bExportLights = FALSE;
    g_pScene->Settings().bExportMaterials = FALSE;
    g_pScene->Settings().bExportNormals = FALSE;
    g_pScene->Settings().bExportSkinWeights = FALSE;
    g_pScene->Settings().bForceIndex32Format = FALSE;
    g_pScene->Settings().bExportMeshes = FALSE;
    g_pScene->Settings().bExportScene = FALSE;
    g_pScene->Settings().iMaxUVSetCount = 0;
    g_XATGSettings.bBinaryBlobExport = FALSE;
    g_XATGSettings.bBundleTextures = FALSE;
    g_XATGSettings.bUseExistingBundle = FALSE;
    g_pScene->Settings().bRenameAnimationsToFileName = TRUE;
    return FALSE;
}

BOOL MacroCharacter( const CHAR* strArgument )
{
    g_pScene->Settings().bExportAnimations = FALSE;
    g_pScene->Settings().bExportCameras = FALSE;
    g_pScene->Settings().bExportLights = FALSE;
    g_pScene->Settings().bExportMaterials = TRUE;
    g_pScene->Settings().bExportMeshes = TRUE;
    g_pScene->Settings().bExportScene = TRUE;
    return FALSE;
}

BOOL MacroWindowsD3D9( const CHAR* strArgument )
{
    g_pScene->Settings().bLittleEndian = TRUE;
    g_XATGSettings.bBundleTextures = FALSE;
    g_XATGSettings.bUseExistingBundle = FALSE;
    g_pScene->Settings().bCompressVertexData = FALSE;
    g_pScene->Settings().dwNormalCompressedType = D3DDECLTYPE_DEC3N;
    return FALSE;
}

BOOL MacroWindowsD3D10( const CHAR* strArgument )
{
    g_pScene->Settings().bLittleEndian = TRUE;
    g_XATGSettings.bBundleTextures = FALSE;
    g_XATGSettings.bUseExistingBundle = FALSE;
    g_pScene->Settings().bCompressVertexData = TRUE;
    g_pScene->Settings().dwNormalCompressedType = D3DDECLTYPE_DEC3N;
    g_ExportFileFormat = FILEFORMAT_SDKMESH;
    return FALSE;
}

BOOL MacroXbox360( const CHAR* strArgument )
{
    g_pScene->Settings().bLittleEndian = FALSE;
    g_XATGSettings.bBinaryBlobExport = TRUE;
    g_XATGSettings.bBundleTextures = TRUE;
    g_XATGSettings.bUseExistingBundle = TRUE;
    g_pScene->Settings().bCompressVertexData = TRUE;
    g_pScene->Settings().dwNormalCompressedType = D3DDECLTYPE_DEC3N;
    g_ExportFileFormat = FILEFORMAT_XATG;
    return FALSE;
}

BOOL MacroSDKMesh( const CHAR* strArgument )
{
    g_ExportFileFormat = FILEFORMAT_SDKMESH;
    return FALSE;
}

BOOL MacroXATG( const CHAR* strArgument )
{
    g_ExportFileFormat = FILEFORMAT_XATG;
    return FALSE;
}

BOOL MacroSubD11( const CHAR* strArgument )
{
    MacroWindowsD3D10( NULL );
    MacroSDKMesh( NULL );
    g_pScene->Settings().bCompressVertexData = FALSE;
    g_pScene->Settings().bConvertMeshesToSubD = TRUE;
    g_pScene->Settings().bExportBinormal = FALSE;
    g_pScene->Settings().bForceExportSkinWeights = TRUE;
    g_pScene->Settings().iMaxUVSetCount = 1;
    g_pScene->Settings().bOptimizeAnimations = FALSE;
    return FALSE;
}

BOOL MacroSubDXbox( const CHAR* strArgument )
{
    MacroXbox360( NULL );
    g_pScene->Settings().bCompressVertexData = TRUE;
    g_pScene->Settings().bConvertMeshesToSubD = TRUE;
    g_pScene->Settings().bExportBinormal = FALSE;
    g_pScene->Settings().bForceExportSkinWeights = TRUE;
    g_pScene->Settings().iMaxUVSetCount = 1;
    return FALSE;
}

BOOL MacroSetLogLevel( const CHAR* strArgument )
{
    INT iValue = atoi( strArgument );
    iValue = min( 10, max( 0, iValue ) );
    ExportLog::SetLogLevel( (UINT)iValue );
    return TRUE;
}

BOOL MacroSetVerbose( const CHAR* strArgument )
{
    ExportLog::SetLogLevel( 4 );
    return FALSE;
}

BOOL MacroAttach( const CHAR* strArgument )
{
#ifdef _DEBUG
	ExportLog::LogMsg( 0, "!!! Attach debugger NOW and then press any key..." );
	_getch();
#endif
    return FALSE;
}

BOOL MacroSaveSettings( const CHAR* strArgument )
{
    BOOL bResult = g_SettingsManager.SaveSettings( strArgument );
    if( !bResult )
    {
        ExportLog::LogError( "Could not save settings to file \"%s\".", strArgument );
    }
    else
    {
        ExportLog::LogMsg( 1, "Saved settings to file \"%s\".", strArgument );
    }
    return TRUE;
}

BOOL MacroLoadSettings( const CHAR* strArgument )
{
    BOOL bResult = g_SettingsManager.LoadSettings( strArgument );
    if( !bResult )
    {
        ExportLog::LogError( "Could not load settings from file \"%s\".", strArgument );
    }
    else
    {
        ExportLog::LogMsg( 1, "Loaded settings from file \"%s\".", strArgument );
    }
    return TRUE;
}

BOOL MacroLoadFileList( const CHAR* strArgument )
{
    FILE* fp = NULL;
    fopen_s( &fp, strArgument, "r" );
    if( fp == NULL )
    {
        ExportLog::LogError( "Could not load file list from file \"%s\".", strArgument );
        return TRUE;
    }

    DWORD dwCount = 0;
    while( !feof( fp ) )
    {
        CHAR strFileName[MAX_PATH];
        fgets( strFileName, ARRAYSIZE( strFileName ), fp );
        CHAR* strNewline = strchr( strFileName, '\n' );
        if( strNewline != NULL )
        {
            *strNewline = '\0';
        }

        g_InputFileNames.push_back( strFileName );
        ++dwCount;
    }

    fclose( fp );

    ExportLog::LogMsg( 1, "Loaded %d input filenames from file \"%s\".", dwCount, strArgument );

    return TRUE;
}

MacroCommand g_MacroCommands[] = {
#ifdef _DEBUG
	{ "attach", "", "Wait for debugger attach", MacroAttach },
#endif
    { "help", "", "Display help", MacroDisplayHelp },
    { "?", "", "Display help", MacroDisplayHelp },
    { "outputpath", " <path>", "Sets the output root path; files will appear in scenes/ and textures/ subdirectories", MacroSetOutputPath },
    { "verbose", "", "Displays more detailed output, equivalent to -loglevel 4", MacroSetVerbose },
    { "xatg", "", "Use the XATG output file format (default), equivalent to -fileformat xatg", MacroXATG },
    { "sdkmesh", "", "Use the SDKMESH output file format, equivalent to -fileformat sdkmesh", MacroSDKMesh },
    { "xbox360", "", "Sets export options for an Xbox 360 target", MacroXbox360 },
    { "windowsd3d9", "", "Sets export options for a Windows Direct3D 9 target", MacroWindowsD3D9 },
    { "windowsd3d10", "", "Sets export options for a Windows Direct3D 10/11 target", MacroWindowsD3D10 },
    { "collisionmesh", "", "Sets export options for collision mesh export", MacroCollisionMesh },
    { "animation", "", "Sets export options for animation track export", MacroAnimation },
    { "character", "", "Sets export options for character (mesh & skeleton) export", MacroCharacter },
    { "subd11", "", "Sets export options for subdivision surface processing for SubD11 sample", MacroSubD11 },
    { "subdxbox", "", "Sets export options for subdivision surface processing for Xbox SubD sample", MacroSubDXbox },
    { "savesettings", " <filename>", "Saves all settings to the specified filename", MacroSaveSettings },
    { "loadsettings", " <filename>", "Loads settings from the specified filename", MacroLoadSettings },
    { "filelist", " <filename>", "Loads a list of input filenames from the specified filename", MacroLoadFileList },
    { "loglevel", " <ranged value 1 - 10>", "Sets the message logging level, higher values show more messages", MacroSetLogLevel },
};

ExportSettingsEntry* FindCommandHelper( ExportSettingsEntry* pRoot, const CHAR* strCommand )
{
    if( pRoot == NULL )
        return NULL;

    if( pRoot->m_Type == ExportSettingsEntry::CT_CATEGORY )
    {
        ExportSettingsEntry* pResult = FindCommandHelper( pRoot->m_pFirstChild, strCommand );
        if( pResult != NULL )
            return pResult;
        return FindCommandHelper( pRoot->m_pSibling, strCommand );
    }

    const CHAR* strMatch = pRoot->m_CommandLineOptionName.SafeString();
    if( strstr( strCommand, strMatch ) == strCommand )
    {
        return pRoot;
    }
    return FindCommandHelper( pRoot->m_pSibling, strCommand );
}

ExportSettingsEntry* FindCommand( const CHAR* strCommand )
{
    ExportSettingsEntry* pResult = NULL;
    DWORD dwCount = g_SettingsManager.GetRootCategoryCount();
    for( DWORD i = 0; i < dwCount; ++i )
    {
        ExportSettingsEntry* pEntry = g_SettingsManager.GetRootCategory( i );
        pResult = FindCommandHelper( pEntry, strCommand );
        if( pResult != NULL )
            return pResult;
    }
    return NULL;
}

BOOL ParseMacroCommand( const CHAR* strCommand, const CHAR* strArgument, BOOL* pMacroFound )
{
    *pMacroFound = FALSE;
    DWORD dwCount = ARRAYSIZE( g_MacroCommands );
    for( DWORD i = 0; i < dwCount; ++i )
    {
        MacroCommand& Command = g_MacroCommands[i];
        if( _stricmp( strCommand, Command.strCommandLine ) == 0 )
        {
            *pMacroFound = TRUE;
            return (Command.pCallback)( strArgument );
        }
    }
    return FALSE;
}

BOOL ParseEnum( ExportSettingsEntry* pEntry, const CHAR* strArgument )
{
    DWORD dwEnumCount = pEntry->m_dwEnumValueCount;
    for( DWORD i = 0; i < dwEnumCount; ++i )
    {
        if( _stricmp( strArgument, pEntry->m_pEnumValues[i].strCommandLine ) == 0 )
        {
            pEntry->SetValue( pEntry->m_pEnumValues[i].iValue );
            return TRUE;
        }
    }
    return FALSE;
}

BOOL ParseCommand( const CHAR* strOriginalCommand, const CHAR* strArgument )
{
    CHAR strCommand[128];
    strcpy_s( strCommand, strOriginalCommand );
    _strlwr_s( strCommand );

    BOOL bUsedArgument = FALSE;

    ExportSettingsEntry* pEntry = FindCommand( strCommand );
    if( pEntry == NULL )
    {
        BOOL bMacro = FALSE;
        bUsedArgument = ParseMacroCommand( strCommand, strArgument, &bMacro );
        if( !bMacro )
        {
            ExportLog::LogWarning( "Unknown command line option \"%s\"", strCommand );
            return FALSE;
        }
        return bUsedArgument;
    }

    const CHAR* strCommandName = pEntry->m_CommandLineOptionName.SafeString();
    switch( pEntry->m_Type )
    {
    case ExportSettingsEntry::CT_CHECKBOX:
        {
            DWORD dwLength = (DWORD)strlen( strCommandName );
            CHAR Argument = strCommand[dwLength];
            if( Argument == '+' )
            {
                pEntry->SetValue( TRUE );
            }
            else if( Argument == '-' )
            {
                pEntry->SetValue( FALSE );
            }
            else
            {
                ExportLog::LogWarning( "Could not parse command \"%s\".", strCommandName );
            }
            break;
        }
    case ExportSettingsEntry::CT_BOUNDEDINTSLIDER:
        {
            if( strArgument == NULL )
            {
                ExportLog::LogWarning( "No parameter passed for command \"%s\".", strCommandName );
                break;
            }
            INT iValue = atoi( strArgument );
            pEntry->SetValue( iValue );
            bUsedArgument = TRUE;
            break;
        }
    case ExportSettingsEntry::CT_BOUNDEDFLOATSLIDER:
        {
            if( strArgument == NULL )
            {
                ExportLog::LogWarning( "No parameter passed for command \"%s\".", strCommandName );
                break;
            }
            FLOAT fValue = (FLOAT)atof( strArgument );
            pEntry->SetValue( fValue );
            bUsedArgument = TRUE;
            break;
        }
    case ExportSettingsEntry::CT_ENUM:
        {
            if( strArgument == NULL )
            {
                ExportLog::LogWarning( "No parameter passed for command \"%s\".", strCommandName );
                break;
            }
            BOOL bValid = ParseEnum( pEntry, strArgument );
            if( !bValid )
            {
                ExportLog::LogWarning( "Invalid value given for command \"%s\".", strCommand );
            }
            bUsedArgument = TRUE;
            break;
        }
    case ExportSettingsEntry::CT_STRING:
        {
            if( strArgument == NULL )
            {
                ExportLog::LogWarning( "No parameter passed for command \"%s\".", strCommandName );
                break;
            }
            pEntry->SetValue( strArgument );
            bUsedArgument = TRUE;
            break;
        }
    }
    return bUsedArgument;
}

VOID PrintEntryHelp( ExportSettingsEntry* pEntry )
{
    if( pEntry == NULL )
        return;

    static CHAR strAnnotation[256];
    switch( pEntry->m_Type )
    {
    case ExportSettingsEntry::CT_CATEGORY:
        ExportLog::LogMsg( 0, "\n%s Options:", pEntry->m_DisplayName );
        PrintEntryHelp( pEntry->m_pFirstChild );
        PrintEntryHelp( pEntry->m_pSibling );
        return;
    case ExportSettingsEntry::CT_CHECKBOX:
        sprintf_s( strAnnotation, "[+|-] (default %s)", pEntry->m_DefaultValue.m_bValue ? "+" : "-" );
        break;
    case ExportSettingsEntry::CT_BOUNDEDFLOATSLIDER:
        sprintf_s( strAnnotation, " <ranged value %0.3f - %0.3f, default %0.3f>", pEntry->m_MinValue.m_fValue, pEntry->m_MaxValue.m_fValue, pEntry->m_DefaultValue.m_fValue );
        break;
    case ExportSettingsEntry::CT_BOUNDEDINTSLIDER:
        sprintf_s( strAnnotation, " <ranged value %d - %d, default %d>", pEntry->m_MinValue.m_iValue, pEntry->m_MaxValue.m_iValue, pEntry->m_DefaultValue.m_iValue );
        break;
    case ExportSettingsEntry::CT_ENUM:
        {
            strcpy_s( strAnnotation, " [ " );
            DWORD dwCount = pEntry->m_dwEnumValueCount;
            for( DWORD i = 0; i < dwCount; ++i )
            {
                if( i > 0 )
                {
                    strcat_s( strAnnotation, " | " );
                }
                strcat_s( strAnnotation, pEntry->m_pEnumValues[i].strCommandLine );
            }
            strcat_s( strAnnotation, " ]" );
            break;
        }
    case ExportSettingsEntry::CT_STRING:
        sprintf_s( strAnnotation, " <string default: \"%s\">", pEntry->m_DefaultValue.m_strValue );
        break;
    }

    if( pEntry->m_CommandLineOptionName != NULL )
    {
        ExportLog::LogMsg( 0, "    -%s%s : %s", pEntry->m_CommandLineOptionName, strAnnotation, pEntry->m_DisplayName );
    }
    PrintEntryHelp( pEntry->m_pSibling );
}

VOID PrintHelp()
{
    if( g_bHelpPrinted )
    {
        return;
    }
    g_bHelpPrinted = TRUE;

    ExportLog::LogMsg( 0, "\nUsage: ContentExporter [options] \"filename1\" \"filename2\" ... \"filenameN\"" );
    ExportLog::LogMsg( 0, "\nThe following command line options can be used to modify scene parse and file output behavior." );

    ExportLog::LogMsg( 0, "\nGeneral & Macro Options:" );
    DWORD dwMacroCount = ARRAYSIZE( g_MacroCommands );
    for( DWORD i = 0; i < dwMacroCount; ++i )
    {
        ExportLog::LogMsg( 0, "    -%s%s : %s", g_MacroCommands[i].strCommandLine, g_MacroCommands[i].strAnnotation, g_MacroCommands[i].strDescription );
    }

    DWORD dwRootCategoryCount = g_SettingsManager.GetRootCategoryCount();
    for( DWORD i = 0; i < dwRootCategoryCount; ++i )
    {
        ExportSettingsEntry* pCategory = g_SettingsManager.GetRootCategory( i );
        PrintEntryHelp( pCategory );
    }
    ExportLog::LogMsg( 0, "" );
}

VOID ParseInputFileName( const CHAR* strFileName )
{
	ExportPath InputFileName( strFileName );
	if( !InputFileName.IsAbsolutePath() )
	{
		InputFileName = g_WorkingPath;
		InputFileName.Append( strFileName );
	}

    g_InputFileNames.push_back( InputFileName );
}

std::vector<const CHAR*> g_CommandStrings;

VOID ParseCommandLine( INT argc, CHAR* argv[] )
{
    assert( argc >= 1 );

    for( INT i = 1; i < argc; ++i )
    {
        const CHAR* strToken = argv[i];
        g_CommandStrings.push_back( strToken );
    }

    DWORD dwCommandCount = (DWORD)g_CommandStrings.size();
    for( DWORD i = 0; i < dwCommandCount; ++i )
    {
        const CHAR* strCommand = g_CommandStrings[i];
        if( strCommand[0] == '-' || strCommand[0] == '/' )
        {
            const CHAR* strArgument = NULL;
            if( i < ( dwCommandCount - 1 ) )
            {
                strArgument = g_CommandStrings[i + 1];
            }
            BOOL bCommandWithParameter = ParseCommand( strCommand + 1, strArgument );
            if( bCommandWithParameter )
            {
                ++i;
            }
        }
        else
        {
            ParseInputFileName( strCommand );
        }
    }
}

VOID BuildOutputFileName( const ExportPath& InputFileName )
{
	if( !g_OutputFilePath.IsEmpty() )
	{
		g_CurrentOutputFileName = g_OutputFilePath;
	}
	else
	{
		g_CurrentOutputFileName = InputFileName;
		g_CurrentOutputFileName.TrimOffFileName();
	}

	g_CurrentOutputFileName.ChangeFileName( InputFileName );

    if( g_ExportFileFormat == FILEFORMAT_SDKMESH )
	{
		g_CurrentOutputFileName.ChangeExtension( CONTENT_EXPORTER_BINARYFILE_EXTENSION );
	}
	else
	{
		g_CurrentOutputFileName.ChangeExtension( CONTENT_EXPORTER_FILE_EXTENSION );
	}
}

INT __cdecl _tmain(INT argc, _TCHAR* argv[])
{
	g_WorkingPath = ExportPath::GetCurrentPath();

    ExportLog::AddListener( &g_ConsoleOutListener );
#if _MSC_VER >= 1500
    if( IsDebuggerPresent() )
    {
        ExportLog::AddListener( &g_DebugSpewListener );
    }
#endif

#ifdef _DEBUG
    sprintf_s( g_strExporterName, "%s version %d.%d.%d (DEBUG)", CONTENT_EXPORTER_TITLE, CONTENT_EXPORTER_MAJOR_VERSION, CONTENT_EXPORTER_MINOR_VERSION, CONTENT_EXPORTER_REVISION );
#else
    sprintf_s( g_strExporterName, "%s version %d.%d.%d", CONTENT_EXPORTER_TITLE, CONTENT_EXPORTER_MAJOR_VERSION, CONTENT_EXPORTER_MINOR_VERSION, CONTENT_EXPORTER_REVISION );
#endif

    ExportLog::SetLogLevel( 1 );
    ExportLog::EnableLogging( TRUE );

    ExportLog::LogMsg( 0, "----------------------------------------------------------" );
    ExportLog::LogMsg( 0, g_strExporterName );
    ExportLog::LogMsg( 0, CONTENT_EXPORTER_VENDOR );
    ExportLog::LogMsg( 0, CONTENT_EXPORTER_COPYRIGHT );
    ExportLog::LogMsg( 0, "----------------------------------------------------------" );

    g_pScene = new ExportScene();
    g_pScene->SetDCCTransformer( &g_FBXTransformer );

    static const ExportEnumValue FileFormatEnums[] = {
        { CONTENT_EXPORTER_FILE_FILTER_DESCRIPTION, CONTENT_EXPORTER_FILE_EXTENSION, FILEFORMAT_XATG },
        { CONTENT_EXPORTER_BINARYFILE_FILTER_DESCRIPTION, CONTENT_EXPORTER_BINARYFILE_EXTENSION, FILEFORMAT_SDKMESH },
    };
    g_SettingsManager.AddEnum( g_SettingsManager.GetRootCategory( 0 ), "Output File Format", "fileformat", FILEFORMAT_XATG, FileFormatEnums, ARRAYSIZE( FileFormatEnums ), &g_ExportFileFormat );

    XATGInitializeSettings();
    ParseCommandLine( argc, argv );

    ExportLog::LogMsg( 9, "Microsoft C++ compiler version %d", _MSC_VER );

    DWORD dwInputFileCount = (DWORD)g_InputFileNames.size();
    if( dwInputFileCount == 0 )
    {
        ExportLog::LogError( "No input filename(s) provided." );
        PrintHelp();
        return E_FAIL;
    }

    ExportCoreSettings InitialSettings = g_pScene->Settings();

    ExportLog::LogMsg( 4, "Initializing FBX..." );
    FBXImport::Initialize();
    ExportLog::LogMsg( 4, "FBX has been initialized." );

    if( dwInputFileCount == 0 )
    {
        ExportLog::LogError( "No input filenames were specified." );
        return 0;
    }

    ExportMaterial::GetDirect3DDevice();

    for( DWORD i = 0; i < dwInputFileCount; ++i )
    {
        ExportPath InputFileName = g_InputFileNames[i];
		g_CurrentInputFileName = InputFileName;

        BuildOutputFileName( InputFileName );
        if( g_CurrentOutputFileName.IsEmpty() )
        {
            ExportLog::LogError( "Output filename is invalid." );
            return E_FAIL;
        }
        g_pScene->Statistics().StartExport();
        g_pScene->Statistics().StartSceneParse();

        HRESULT hr = FBXImport::ImportFile( InputFileName );
        if( FAILED(hr) )
        {
            ExportLog::LogError( "Could not load file \"%s\".", (const CHAR*)InputFileName );
            continue;
        }

        g_pScene->Statistics().StartSave();

        if( SUCCEEDED(hr) )
        {
            if( g_ExportFileFormat == FILEFORMAT_SDKMESH )
            {
                ExportTextureConverter::ProcessScene( g_pScene, &g_Manifest, "", TRUE );
                WriteSDKMeshFile( g_CurrentOutputFileName, &g_Manifest );
                ExportTextureConverter::PerformTextureFileOperations( &g_Manifest );
            }
            else
            {
                if( g_XATGSettings.bBundleTextures )
                {
                    ExportTextureConverter::ProcessScene( g_pScene, &g_Manifest, "textures\\", FALSE );
                    WriteXATGFile( g_CurrentOutputFileName, &g_Manifest );
                    ExportTextureConverter::PerformTextureFileOperations( &g_Manifest );
                    BundleTextures();
                }
                else
                {
                    ExportTextureConverter::ProcessScene( g_pScene, &g_Manifest, "textures\\", TRUE );
                    WriteXATGFile( g_CurrentOutputFileName, &g_Manifest );
                    ExportTextureConverter::PerformTextureFileOperations( &g_Manifest );
                }
            }
        }

        g_pScene->Statistics().EndExport();
        g_pScene->Statistics().FinalReport();
        ExportLog::GenerateLogReport();

        if( ( i + 1 ) < dwInputFileCount )
        {
            FBXImport::ClearScene();
            delete g_pScene;
            g_pScene = new ExportScene();
            g_pScene->SetDCCTransformer( &g_FBXTransformer );
            g_pScene->Settings() = InitialSettings;
            g_Manifest.Clear();
            ExportLog::ResetCounters();
        }
    }

    ExportMaterial::ReleaseDirect3DDevice();

	return 0;
}

