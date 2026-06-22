cmake_minimum_required(VERSION 3.16)
project(load_workbook_test LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Xlnt REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core)
find_package(libzip CONFIG REQUIRED)
add_executable(load_workbook_test
    main_load.cpp
    xlsx_load.cpp
)
target_include_directories(load_workbook_test PRIVATE include)
target_link_libraries(load_workbook_test PRIVATE xlnt::xlnt libzip::zip Qt5::Core)
target_compile_definitions(load_workbook_test PRIVATE HAVE_XLNT)
