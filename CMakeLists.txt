cmake_minimum_required(VERSION 3.16)

project(QIoTest VERSION 3.0.12 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Set the search path for UI files
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    SerialPort
    Sql
    Multimedia
)

# Optional: Qt6SerialBus (for Modbus support)
find_package(Qt6 COMPONENTS SerialBus QUIET)
if(Qt6SerialBus_FOUND)
    message(STATUS "Qt6SerialBus found - Modbus support enabled")
else()
    message(STATUS "Qt6SerialBus not found - Modbus support disabled")
endif()

# QXlsx - included as subdirectory
set(QT_VERSION_MAJOR 6)
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
    include/modbusmodel.h
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

# Link Qt6 libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::SerialPort
    Qt6::Sql
    Qt6::Multimedia
)

# Link Qt6SerialBus if available
if(Qt6SerialBus_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::SerialBus)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_SERIALBUS)
endif()

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

# Copy config folder to build directory (if it exists)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cfg)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cfg DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
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
endif()
