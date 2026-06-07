@echo off
setlocal EnableExtensions

set "ROOT=%~dp0.."
set "RELEASE_DIR=%ROOT%\build\Release"
set "EXE=%RELEASE_DIR%\QIoTest.exe"

if not exist "%EXE%" (
    echo QIoTest.exe not found. Build Release first: build_release.bat
    exit /b 1
)

set "WINDEPLOYQT="
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\installed\x64-windows\tools\qt5\bin\windeployqt.exe" (
        set "WINDEPLOYQT=%VCPKG_ROOT%\installed\x64-windows\tools\qt5\bin\windeployqt.exe"
    )
)
if not defined WINDEPLOYQT if exist "D:\vcpkg\installed\x64-windows\tools\qt5\bin\windeployqt.exe" (
    set "WINDEPLOYQT=D:\vcpkg\installed\x64-windows\tools\qt5\bin\windeployqt.exe"
)
if not defined WINDEPLOYQT (
    for /f "delims=" %%I in ('where windeployqt 2^>nul') do (
        if not defined WINDEPLOYQT set "WINDEPLOYQT=%%I"
    )
)
if not defined WINDEPLOYQT (
    echo windeployqt not found. Set VCPKG_ROOT or add Qt tools to PATH.
    exit /b 1
)

echo Deploying Qt runtime to %RELEASE_DIR%
"%WINDEPLOYQT%" --no-translations --no-angle --dir "%RELEASE_DIR%" "%EXE%"
if errorlevel 1 exit /b 1

echo Deployment complete.
exit /b 0
