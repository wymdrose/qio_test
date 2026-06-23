cmake_minimum_required(VERSION 3.16)

project(QIoTest VERSION 4.0.12 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# MSVC: treat sources as UTF-8 (fixes C4819 / string parsing)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

# Set the search path for UI files
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

# Find Qt5 packages (via vcpkg toolchain or system Qt)
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    SerialPort
    Sql
    Multimedia
)

# xlnt - Excel file library (via vcpkg or system install)
find_package(Xlnt REQUIRED)
find_package(libzip CONFIG REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/qiotest.cpp
    src/testprocess.cpp
    src/xlsx_load.cpp
)

# Header files
set(HEADERS
    src/qiotest.h
    src/global.h
    include/communicatelib.h
    include/xlsxfile.h
    include/xlsx_load.h
    include/mysignal.h
    include/mysqlite.h
    include/dologs.h
)

# UI files
set(UI_FILES
    ui/qiotest.ui
)

# Resource files
set(RESOURCES
    resources/qiotest.qrc
)

# Create executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}  # For generated UI headers
)

# Link Qt5 libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::SerialPort
    Qt5::Sql
    Qt5::Multimedia
)

# Link xlnt
target_link_libraries(${PROJECT_NAME} PRIVATE xlnt::xlnt libzip::zip)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    HAVE_XLNT
    QIOTEST_VERSION="${PROJECT_VERSION}"
)

# Windows-specific settings
if(WIN32)
    # Set Windows subsystem to avoid console window
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Copy runtime files next to the executable (multi-config safe)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DSRC=${CMAKE_CURRENT_SOURCE_DIR}/app.ini
        -DDST=$<TARGET_FILE_DIR:${PROJECT_NAME}>/app.ini
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_if_missing.cmake
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/cfg"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/cfg"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/find.wav"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/find.wav"
    VERBATIM
)

# Install rules (Windows: use bin/ so CPack NSIS shortcuts point to the right path)
if(WIN32)
    set(_install_runtime_dest "bin")
else()
    set(_install_runtime_dest "${CMAKE_INSTALL_BINDIR}")
endif()

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${_install_runtime_dest}
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/app.ini" DESTINATION bin)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/find.wav" DESTINATION bin)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cfg/"
    DESTINATION bin/cfg
    FILES_MATCHING
    PATTERN "*"
)

if(WIN32)
    get_target_property(_qt_core_location Qt5::Core IMPORTED_LOCATION_RELEASE)
    if(NOT _qt_core_location)
        get_target_property(_qt_core_location Qt5::Core IMPORTED_LOCATION)
    endif()
    if(_qt_core_location)
        get_filename_component(_qt_bin_dir "${_qt_core_location}" DIRECTORY)
    endif()
    set(_qt5_windeployqt_hints "")
    if(_qt_bin_dir)
        list(APPEND _qt5_windeployqt_hints "${_qt_bin_dir}/../tools/qt5/bin")
    endif()
    if(DEFINED ENV{VCPKG_ROOT})
        list(APPEND _qt5_windeployqt_hints "$ENV{VCPKG_ROOT}/installed/x64-windows/tools/qt5/bin")
    endif()
    if(DEFINED VCPKG_INSTALLED_DIR)
        list(APPEND _qt5_windeployqt_hints "${VCPKG_INSTALLED_DIR}/x64-windows/tools/qt5/bin")
    endif()
    unset(WINDEPLOYQT_EXECUTABLE CACHE)
    set(WINDEPLOYQT_EXECUTABLE "")
    foreach(_hint IN LISTS _qt5_windeployqt_hints)
        if(EXISTS "${_hint}/windeployqt.exe")
            set(WINDEPLOYQT_EXECUTABLE "${_hint}/windeployqt.exe")
            break()
        endif()
    endforeach()

    set(_vcpkg_installed_bin "")
    if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/installed/x64-windows/bin")
        set(_vcpkg_installed_bin "$ENV{VCPKG_ROOT}/installed/x64-windows/bin")
    elseif(DEFINED VCPKG_INSTALLED_DIR AND EXISTS "${VCPKG_INSTALLED_DIR}/x64-windows/bin")
        set(_vcpkg_installed_bin "${VCPKG_INSTALLED_DIR}/x64-windows/bin")
    elseif(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/bin")
        set(_vcpkg_installed_bin "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/bin")
    endif()

    set(_vcpkg_applocal_ps1 "")
    if(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg\\.cmake$")
        get_filename_component(_vcpkg_toolchain_dir "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
        set(_vcpkg_applocal_ps1 "${_vcpkg_toolchain_dir}/msbuild/applocal.ps1")
    endif()

    find_program(_VCPKG_POWERSHELL powershell.exe)

    if(WINDEPLOYQT_EXECUTABLE)
        set(_install_applocal_code "")
        if(_vcpkg_installed_bin AND _vcpkg_applocal_ps1 AND _VCPKG_POWERSHELL)
            set(_install_applocal_code "
            execute_process(
                COMMAND \"${_VCPKG_POWERSHELL}\" -noprofile -executionpolicy Bypass
                    -file \"${_vcpkg_applocal_ps1}\"
                    -targetBinary \"\${_exe}\"
                    -installedDir \"${_vcpkg_installed_bin}\"
                RESULT_VARIABLE _applocal_result
                OUTPUT_VARIABLE _applocal_output
                ERROR_VARIABLE _applocal_error
            )
            if(NOT _applocal_result EQUAL 0)
                message(FATAL_ERROR \"applocal.ps1 failed: \${_applocal_output}\${_applocal_error}\")
            endif()
            set(_sqlite_plugin \"\${CMAKE_INSTALL_PREFIX}/bin/sqldrivers/qsqlite.dll\")
            if(EXISTS \"\${_sqlite_plugin}\")
                execute_process(
                    COMMAND \"${_VCPKG_POWERSHELL}\" -noprofile -executionpolicy Bypass
                        -file \"${_vcpkg_applocal_ps1}\"
                        -targetBinary \"\${_sqlite_plugin}\"
                        -installedDir \"${_vcpkg_installed_bin}\"
                    RESULT_VARIABLE _applocal_sqlite_result
                    OUTPUT_VARIABLE _applocal_sqlite_output
                    ERROR_VARIABLE _applocal_sqlite_error
                )
                if(NOT _applocal_sqlite_result EQUAL 0)
                    message(FATAL_ERROR \"applocal.ps1 failed for qsqlite: \${_applocal_sqlite_output}\${_applocal_sqlite_error}\")
                endif()
            endif()
            ")
        else()
            message(WARNING "vcpkg applocal not configured; installer may miss runtime DLLs")
        endif()

        install(CODE "
            set(_exe \"\${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}.exe\")
            if(NOT EXISTS \"\${_exe}\")
                message(FATAL_ERROR \"Expected executable not found: \${_exe}\")
            endif()
            execute_process(
                COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                    --no-translations
                    --no-angle
                    --dir \"\${CMAKE_INSTALL_PREFIX}/bin\"
                    \"\${_exe}\"
                RESULT_VARIABLE _deploy_result
                OUTPUT_VARIABLE _deploy_output
                ERROR_VARIABLE _deploy_error
            )
            if(NOT _deploy_result EQUAL 0)
                message(FATAL_ERROR \"windeployqt failed: \${_deploy_output}\${_deploy_error}\")
            endif()
            ${_install_applocal_code}
        ")
    else()
        message(WARNING "windeployqt not found; installer will not bundle Qt runtime DLLs")
    endif()
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPackConfig.cmake)
include(CPack)

