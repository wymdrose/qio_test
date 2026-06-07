@echo off
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
    echo Failed to initialize MSVC environment. Install VS 2022 Build Tools with C++ workload.
    exit /b 1
)
%*
