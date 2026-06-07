@echo off
setlocal EnableExtensions
cd /d "%~dp0"

if not defined VCPKG_ROOT set "VCPKG_ROOT=D:\vcpkg"

set "MAKENSIS="
where makensis >nul 2>&1 && set "MAKENSIS=makensis"
if not defined MAKENSIS if exist "%ProgramFiles(x86)%\NSIS\makensis.exe" (
    set "MAKENSIS=%ProgramFiles(x86)%\NSIS\makensis.exe"
    set "PATH=%ProgramFiles(x86)%\NSIS;%PATH%"
)
if not defined MAKENSIS (
    echo NSIS not found. Install with: winget install NSIS.NSIS
    exit /b 1
)

echo === Building Release ===
call "%~dp0build_release.bat"
if errorlevel 1 exit /b 1

echo === Deploying Qt runtime ===
call "%~dp0scripts\deploy_release.cmd"
if errorlevel 1 exit /b 1

echo === Creating installer ===
call "%~dp0scripts\msvc_env.cmd" cmake --build build --config Release --target package
if errorlevel 1 exit /b 1

for %%F in ("build\QIoTest-*-win64.exe") do (
    echo.
    echo Installer created: %%~fF
    goto :done
)
echo.
echo Package target finished. Check build\ for QIoTest-*-win64.exe
:done

exit /b 0
