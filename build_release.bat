@echo off
setlocal
cd /d "%~dp0"
call "%~dp0scripts\msvc_env.cmd" cmake --build build --config Release %*
