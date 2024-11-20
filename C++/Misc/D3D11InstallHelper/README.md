# Direct3D 11 Install Helper

The **D3D11InstallHelper** sample is designed to simplify detection of the Direct3D 11 API, automatically install the system update if applicable to an end-user's computer, and to provide appropriate messages to the end-user on manual procedure if a newer Service Pack is required.

> This isn't needed for any application that has a minimum system requirement of Windows 7 or better.

See the [Direct3D 11 Deployment for Game Developers](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/direct3d11-deployment) article for more details.

## D3D11InstallHelper.dll

`D3D11InstallHelper.dll` hosts the core functionality for detecting Direct3D 11 components, and performing the system update through the Windows Update service if applicable. The DLL displays no messages or dialog boxes directly.

## D3D11Install.exe

``D3D11Install.exe`` is a tool for using `D3D11InstallHelper.dll` as a stand-alone installer complete with UI and end-user messages, as well as acting as an example for proper use of the DLL. The process exits with a 0 if Direct3D 11 is already installed, if the system update applies successfully without requiring a system restart, if a Service Pack installation is required, or if Direct3D 11 is not supported by this computer. A 1 is returned if the system update is applied successfully and requires a system restart to complete. A 2 is returned for other error conditions. Note that this executable file requires administrator rights to run, and it has a manifest that requests elevation when run on Windows Vista or Windows 7 with UAC enabled. D3D11Install.exe can be used as a stand-alone tool for deploying the Direct3D 11 update, or it can be used directly by installers.

The message depends on the system and the current configuration.

### Windows Vista / Windows Server 2008 Service Pack 2 w/ KB 971644, Windows 7, Windows 8, Windows 8.1, Windows 10, or Windows 11

```
Direct3D 11 is present on this system. No update required
```

### Windows Vista / Windows Server 2008 RTM or Service Pack 1

```
Direct3D 11 is not isntalled, but is available for this version of Windows.

Please install the latest Service Pack

For isntructions on installing the latest Service Pack see Microsoft Knowledge base article KB935791.

Note: You may need to install KB971644 after applying the latest Service Pack to complete installation of Direct3D 11.
```

### Windows Vista / Windows Server 2008 Service Pack 2 without the KB 971644 update

```
Direct3D 11 is not installed, but is available for this version of Windows through Windows Update (KB71644).

Do you want to update your system now ?
```

### Windows XP

```
Direct3D 11 is not supported on this version of Winodws.
```

## Localization

The D3D11Install program is localized for Brazilian Portuguese, Dutch, English, French, German, Italian, Japanese, Korean, Polish, Russian, Simplified Chinese, Spanish, Swedish, Traditional Chinese, Czech and Norwegian (Bokmal).

## More Information

[Games for Windows Technical Requirements (TR 1.1 and 1.2)](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/games-for-windows-technical-requirements-1-1-0006)   
[Games for Windows Test Cases (TR 1.1 and 1.2)](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/games-for-windows-test-requirements-1-0-0006)   
[Games for Windows and DirectX SDK blog](https://walbourn.github.io/)   
[Visual Studio 2012 Update 1](https://walbourn.github.io/visual-studio-2012-update-1/)
