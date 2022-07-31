cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)

project(iditor)

include(cmake/CPM.cmake)

CPMAddPackage(
        NAME fltk
        GIT_TAG master
        GITHUB_REPOSITORY fltk/fltk
        OPTIONS "FLTK_BUILD_TEST OFF"
)

CPMAddPackage(
        NAME tree-sitter
        GIT_TAG master
        GITHUB_REPOSITORY tree-sitter/tree-sitter
        DOWNLOAD_ONLY YES
)

CPMAddPackage(
        NAME tree-sitter-cpp
        GIT_TAG master
        GITHUB_REPOSITORY tree-sitter/tree-sitter-cpp
        DOWNLOAD_ONLY YES
)

add_executable(iditor
        src/main/main.cpp
        src/main/Editor.cpp
        src/main/Preproc.cpp
        src/main/Include.cpp
        src/main/Globals.cpp
        ${tree-sitter_SOURCE_DIR}/lib/src/lib.c
        ${tree-sitter-cpp_SOURCE_DIR}/src/parser.c
        ${tree-sitter-cpp_SOURCE_DIR}/src/scanner.cc
        )

target_include_directories(iditor PUBLIC ${fltk_SOURCE_DIR})
target_include_directories(iditor PUBLIC ${fltk_BINARY_DIR})

target_include_directories(iditor PUBLIC ${tree-sitter_SOURCE_DIR}/lib/src ${tree-sitter_SOURCE_DIR}/lib/include)

target_link_directories(iditor PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(iditor PUBLIC fltk)

if (APPLE)
    target_link_libraries(iditor PUBLIC "-framework Cocoa -framework Foundation")
endif()