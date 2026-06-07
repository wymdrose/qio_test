@echo off
setlocal
cd /d "%~dp0"
set VCPKG_ROOT=D:\vcpkg
call "%~dp0scripts\msvc_env.cmd" cmake --preset windows-vcpkg -DVCPKG_MANIFEST_MODE=OFF -DVCPKG_MANIFEST_INSTALL=OFF -DVCPKG_INSTALLED_DIR=D:/work/qio_test/build/vcpkg_installed -DX_VCPKG_APPLOCAL_DEPS_INSTALL=ON %*
