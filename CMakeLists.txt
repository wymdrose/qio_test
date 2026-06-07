cmake_minimum_required(VERSION 3.16)

project(QIoTest VERSION 3.0.12 LANGUAGES CXX)

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

# QXlsx - included as subdirectory
set(QT_VERSION_MAJOR 5)
add_subdirectory(third_party/QXlsx/QXlsx)

# Source files
set(SOURCES
    src/main.cpp
    src/qiotest.cpp
    src/testprocess.cpp
)

# Header files
set(HEADERS
    src/qiotest.h
    src/global.h
    include/communicatelib.h
    include/xlsxfile.h
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

# Link QXlsx
target_link_libraries(${PROJECT_NAME} PRIVATE QXlsx::QXlsx)
target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_QXLSX)

# Windows-specific settings
if(WIN32)
    # Set Windows subsystem to avoid console window
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Copy runtime files next to the executable (multi-config safe)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/app.ini"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/app.ini"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/cfg"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/cfg"
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
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS
            "${_qt_bin_dir}"
            "${_qt_bin_dir}/../tools/qt5/bin"
            "$ENV{VCPKG_ROOT}/installed/x64-windows/tools/qt5/bin"
    )

    if(WINDEPLOYQT_EXECUTABLE)
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
        ")
    else()
        message(WARNING "windeployqt not found; installer will not bundle Qt runtime DLLs")
    endif()
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPackConfig.cmake)
include(CPack)

