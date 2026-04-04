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
    Network
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
    Qt5::Network
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
    VERBATIM
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cfg")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/cfg"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/cfg"
        VERBATIM
    )
endif()

# Install rules
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# --- Tools ---
option(BUILD_TOOLS "Build auxiliary tools (VspServer, etc.)" ON)
if(BUILD_TOOLS)
    add_subdirectory(tool/vsp_server)
    add_subdirectory(tool/gpcom_client_test)
endif()
