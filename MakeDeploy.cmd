@echo off
echo .
echo First builds all the samples
echo .
echo Then creates 'bin' folders for each area (only contains x64 versions)
echo .
pause
set LOG_DIR=%cd%
set MSB_LOGGING=/fl1 /fl2 /fl3 /flp1:logfile=%LOG_DIR%\build.log;append=true /flp2:logfile=%LOG_DIR%\build.err;errorsonly;append=true /flp3:logfile=%LOG_DIR%\build.wrn;warningsonly;append=true
del build.log
del build.err
del build.wrn
msbuild BuildAllSolutions.targets /p:Configuration=Release /p:Platform=x64 %MSB_LOGGING%
@if ERRORLEVEL 1 goto error
@echo --- BUILD COMPLETE ---
type build.wrn
@echo --- DEPLOY ---
rd /q /s "C++\Direct3D\bin"
rd /q /s "C++\Direct3D10\bin"
rd /q /s "C++\Direct3D11\bin"
rd /q /s "C++\XAudio2\bin"
rd /q /s "C++\XInput\bin"
rd /q /s "C++\Misc\bin"
rd /q /s "C++\DirectInput\bin"
rd /q /s "C++\DirectSound\bin"
REM Direct3D
pushd "C++\Direct3D"
md bin\x64
copy /y BasicHLSL\x64\Release\D3DCompiler_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
copy /y BasicHLSL\x64\Release\d3dx9_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
copy /y BasicHLSL\x64\Release\d3dx10_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
for /R %%1 in (Release\*.exe) do copy /y "%%1" "bin\x64\%%~nx1" >NUL
@if ERRORLEVEL 1 goto error
popd
REM Direct3D10
pushd "C++\Direct3D10"
md bin\x64
copy /y BasicHLSL10\x64\Release\D3DCompiler_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
copy /y BasicHLSL10\x64\Release\d3dx9_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
copy /y BasicHLSL10\x64\Release\d3dx10_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
for /R %%1 in (Release\*.exe) do copy /y "%%1" "bin\x64\%%~nx1" >NUL
@if ERRORLEVEL 1 goto error
popd
popd
REM Direct3D11
pushd "C++\Direct3D11"
md bin\x64
copy /y BasicHLSL11\x64\Release\D3DCompiler_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
copy /y BasicHLSL11\x64\Release\d3dx9_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
copy /y BasicHLSL11\x64\Release\d3dx11_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
for /R %%1 in (Release\*.exe) do copy /y "%%1" "bin\x64\%%~nx1" >NUL
@if ERRORLEVEL 1 goto error
popd
REM XAudio2
pushd "C++\XAudio2"
md bin\x64
copy XAudio2BasicSound\x64\Release_NuGet\xaudio2_9redist.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
for /R %%1 in (Release_NuGet\*.exe) do copy /y "%%1" "bin\x64\%%~nx1" >NUL
@if ERRORLEVEL 1 goto error
popd
REM XInput
pushd "C++\XInput"
md bin\x64
copy /y x64\Release\D3DCompiler_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
copy /y x64\Release\d3dx9_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
copy /y x64\Release\d3dx10_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
for /R %%1 in (Release\*.exe) do copy /y "%%1" "bin\x64\%%~nx1" >NUL
@if ERRORLEVEL 1 goto error
popd
REM Misc
pushd "C++\Misc"
md bin\x64
copy /y InstallOnDemand\x64\Release\D3DCompiler_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
copy /y InstallOnDemand\x64\Release\d3dx9_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
copy /y InstallOnDemand\x64\Release\d3dx10_43.dll "bin\x64" >NUL
@if ERRORLEVEL 1 goto error
for /R %%1 in (Release\*.exe) do copy /y "%%1" "bin\x64\%%~nx1" >NUL
@if ERRORLEVEL 1 goto error
popd
REM DirectInput
pushd "C++\DirectInput"
md bin\x64
for /R %%1 in (Release\*.exe) do copy /y "%%1" "bin\x64\%%~nx1" >NUL
@if ERRORLEVEL 1 goto error
popd
REM DirectSound
pushd "C++\DirectSound"
md bin\x64
for /R %%1 in (Release\*.exe) do copy /y "%%1" "bin\x64\%%~nx1" >NUL
@if ERRORLEVEL 1 goto error
popd
@echo --- DEPLOY COMPLETE ---
@goto end
:error
@echo --- ERROR: FAILED ---
:end
