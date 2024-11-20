# SimpleSample11

This is the DirectX SDK's Direct3D 11 sample updated to use the Windows 10 SDK without any dependencies on legacy DirectX SDK content. This sample is a Win32 desktop DirectX 11.0 application for Windows 10, Windows 8.1, Windows 8, and Windows 7. 

## Description

This sample can be used as a starting point for your own Win32 desktop Direct3D 11 application. This builds on EmptyProject11 by adding a simple status HUD, common controls, and access to the device settings dialog. 

> This version uses the latest DXUT and does not include a Direct3D 9 fallback.

## Dependencies

DXUT-based samples typically make use of runtime HLSL compilation. Build-time compilation is recommended for all production Direct3D applications, but for experimentation and samples development runtime HLSL compilation is preferred. Effects11-based samples also need the D3DCompile APIs for reflection. Therefore, the D3DCompile*.DLL must be available in the search path when these programs are executed.

> When using the Windows 10 SDK and targeting Windows Vista or later, you can include the D3DCompile_47 DLL side-by-side with your application copying the file from the REDIST folder. `%ProgramFiles(x86)%\Windows kits\10\Redist\D3D\x86 or x64`

## More information

[DXUT for Win32 Desktop Update](https://walbourn.github.io/dxut-for-win32-desktop-update/)   
