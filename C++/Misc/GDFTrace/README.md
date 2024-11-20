# Game Definition File Validator

The **Game Definition File Validator** (GDFTrace.exe) utility is a debugging aid for working with Game Definition File Editor (GDF) files for Windows Vista, Windows 7, Windows 8, and Windows 8.1. It is a command-line tool for validating the XML against the schema, displaying a 'human readable' summary, and generating validation warnings. It can be used either directly extracting the GDF from the containing EXE/DLL file, or against the 'raw' XML file.

## Description

If run without any command-line parameters, it displays help.

## Requirements

The GDFTrace.EXE must have the both the GamesExplorerBaseTypes.v1.0.0.0.XSD and GDFSchema.v1.0.0.0.XSD files located in the same directory as the EXE to fully validate the GDF files.

## More Information

[Games for Windows Technical Requirements (TR 1.1 and 1.2)](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/games-for-windows-technical-requirements-1-1-0006)   
[Games for Windows Test Cases (TR 1.1 and 1.2)](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/games-for-windows-test-requirements-1-0-0006)
