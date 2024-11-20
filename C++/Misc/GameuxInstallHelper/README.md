# Gameux Install Helper

The **GameuxInstallHelper** is a DLL for use with install/setup programs to handle registration of Game Definition Files (GDF) with Windows Vista, Windows 7, and Windows 8 desktop games. For Windows 7 and Windows 8, the utility registers the GDFv2 schema data file using IGameExplorer2. For Windows Vista, the utility handles the additional manual steps required for registering a GDFv2 schema data file using the IGameExplorer interface.

> The Game Explorer was removed in Windows 10, Version 1803 or later.

The package also includes GDFInstall which is a command-line test tool and utility for using the GameuxInstallHelper DLL. It supports a number of command-line options and switches. Run it with /? to see the help dialog.

The technical article [Windows Games Explorer for Game Developers](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/windows-game-explorer-integration) covers usage of this install helper.

This utility was originally published as part of the legacy DirectX SDK. This version does not require the DirectX SDK to build and can be built using Visual Studio 2010 or Visual Studio 2012.

## Localization

The GameUxInstallHelper DLL is intended to be called by a install/setup program which handles all UI requirements, so there is no localization support.

## More Information

[Games for Windows Technical Requirements (TR 1.1 and 1.2)](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/games-for-windows-technical-requirements-1-1-0006)   
[Games for Windows Test Cases (TR 1.1 and 1.2)](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/games-for-windows-test-requirements-1-0-0006)
