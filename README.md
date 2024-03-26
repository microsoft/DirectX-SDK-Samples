![DirectX Logo](https://raw.githubusercontent.com/wiki/Microsoft/DXUT/Dx_logo.GIF)

# DirectX SDK Samples

This repository contains an archive of DirectX samples that shipped in the legacy DirectX SDK. They have been updated to build with Visual Studio 2019 or later, require only the current Windows SDK, and make use the [Microsoft.DXSDK.D3DX](https://www.nuget.org/packages/Microsoft.DXSDK.D3DX) NuGet package.

## Disclaimer

The DirectX SDK itself is deprecated and legacy per [Microsoft Docs](https://learn.microsoft.com/windows/win32/directx-sdk--august-2009-), along with D3DX9, D3DX10, D3DX11, XACT, Managed DirectX 1.1, XInput 1.1-1.3, and XAudio 2.0-2.7. These samples are provided here for developer education, but new projects are *strongly encouraged* to seek out supported alternatives to these legacy components. They also have known security issues and bugs as well.

|Technology|Notes|
|--|--|
|DirectX 12|See [DirectX-Graphics-Samples](https://github.com/microsoft/DirectX-Graphics-Samples), [DirectX-Headers](https://github.com/microsoft/DirectX-Headers), and [DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler).|
|Microsoft GDK</br>Xbox Development| See [Xbox-GDK-Samples](https://github.com/microsoft/Xbox-GDK-Samples) and [Xbox-ATG-Samples](https://github.com/microsoft/Xbox-ATG-Samples).|
|Universal Windows Platform (UWP)| See [Windows-universal-samples](https://github.com/microsoft/Windows-universal-samples).|
|DXUT for Direct3D 11| See [DXUT](https://github.com/microsoft/DXUT/).|
|Effects for Direct3D 11| See [FX11](https://github.com/microsoft/FX11).|
|XNAMath| See [DirectXMath](https://github.com/microsoft/DirectXMath).|
|DirectX Capabilities Viewer| See [DxCapsViewer](https://github.com/microsoft/DxCapsViewer).|https://github.com/walbourn/directx-sdk-legacy-samples.git
|UVAtlas isochart texture atlas| See [UVAtlas](https://github.com/Microsoft/UVAtlas).|

## Release Notes

* The XInput samples have been updated to use XInput 1.4 or XInput 9.1.0. XInput 1.3 usage requires the legacy DirectX SDK and is *strongly discouraged*.

* The XACT samples are not included in this repository because they will only build with the legacy DirectX SDK.

* The legacy DirectSound samples from DirectX SDK (November 2007) are included for completeness. They require the optional "MFC" Visual Studio component be installed.

* The Managed DX 1.1 C# samples are not included in this repository because they require both legacy .NET 1.1 and the legacy DirectX SDK to build.

* There are three versions of DXUT included in this repository:

   1. "DXUT" is the Direct3D 9 / Direct3D 10 version and makes use of D3DX9/D3DX10 that shipped in the legacy DirectX SDK.

   1. "DXUT11" is the Direct3D 9 / Direct3D 11 version and makes use of D3DX9/D3DX11 that shipped in the legacy DirectX SDK.

   1. "DXUT11.1" is a Direct3D 11 only version that does not make use of legacy D3DX which shipped on [GitHub](https://github.com/microsoft/DXUT).

* The version of Effects for Direct3D 11 included in this repository matches the updated version which shipped on [GitHub](https://github.com/microsoft/FX11).

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft
trademarks or logos is subject to and must follow
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
