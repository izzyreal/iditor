cmake_minimum_required(VERSION 3.19)

include(cmake/CPM.cmake)
include(cmake/ResourceBundling.cmake)

set(CMAKE_CXX_STANDARD 17)

project(iditor)


CPMAddPackage(
        NAME fltk
        GIT_TAG master
        GITHUB_REPOSITORY fltk/fltk
        OPTIONS "FLTK_BUILD_TEST OFF"
)

CPMAddPackage(
        NAME tree-sitter
        GIT_TAG master
        GITHUB_REPOSITORY commercial-emacs/tree-sitter
        DOWNLOAD_ONLY YES
)

CPMAddPackage(
        NAME tree-sitter-cpp
        GIT_TAG master
        GITHUB_REPOSITORY tree-sitter/tree-sitter-cpp
        DOWNLOAD_ONLY YES
)

CPMAddPackage(
        NAME sqlite
        GIT_TAG master
        GITHUB_REPOSITORY alex85k/sqlite3-cmake
)

CPMAddPackage(
        NAME corrosion
        GIT_TAG master
        GITHUB_REPOSITORY corrosion-rs/corrosion
)

CPMAddPackage("gh:catchorg/Catch2#v3.1.0")

corrosion_import_crate(MANIFEST_PATH ${tree-sitter_SOURCE_DIR}/highlight/Cargo.toml)

get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

add_executable(iditor
        src/main/main.cpp
        src/main/Editor.cpp
        src/main/EditorDraw.cpp
        src/main/Highlighter.cpp
        src/main/Preproc.cpp
        src/main/Include.cpp
        src/main/Globals.cpp
        src/main/Declarations.cpp
        src/main/TreeDiff.cpp
        src/main/Project.cpp
        src/main/Db.cpp
        src/main/decl/Decl.cpp
        src/main/iditor_tokenizer.cpp
        ${tree-sitter_SOURCE_DIR}/lib/src/lib.c
        ${tree-sitter-cpp_SOURCE_DIR}/src/parser.c
        ${tree-sitter-cpp_SOURCE_DIR}/src/scanner.cc
        )

target_include_directories(iditor PUBLIC ${fltk_SOURCE_DIR})
target_include_directories(iditor PUBLIC ${fltk_BINARY_DIR})

target_include_directories(iditor PUBLIC ${tree-sitter_SOURCE_DIR}/lib/src ${tree-sitter_SOURCE_DIR}/lib/include ${tree-sitter_SOURCE_DIR}/highlight/include)

target_link_directories(iditor PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(iditor PUBLIC fltk sqlite3 tree-sitter-highlight)

if (APPLE)
    target_link_libraries(iditor PUBLIC "-framework Cocoa -framework Foundation")
endif()

add_executable(iditor-tests
        src/main/Preproc.cpp
        src/main/Include.cpp
        src/main/Globals.cpp
        src/main/Declarations.cpp
        src/main/Db.cpp
        src/main/Project.cpp
        src/main/TreeDiff.cpp
        src/main/decl/Decl.cpp
        src/main/iditor_tokenizer.cpp
        src/test/DeclarationsTest.cpp
        src/test/TreeDiffTest.cpp
        ${tree-sitter_SOURCE_DIR}/lib/src/lib.c
        ${tree-sitter-cpp_SOURCE_DIR}/src/parser.c
        ${tree-sitter-cpp_SOURCE_DIR}/src/scanner.cc
        )

target_include_directories(iditor-tests PUBLIC ${fltk_SOURCE_DIR})
target_include_directories(iditor-tests PUBLIC ${fltk_BINARY_DIR})
target_include_directories(iditor-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/main ${tree-sitter_SOURCE_DIR}/lib/src ${tree-sitter_SOURCE_DIR}/lib/include ${tree-sitter_SOURCE_DIR}/highlight/include)
target_link_directories(iditor-tests PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(iditor-tests PUBLIC fltk sqlite3 tree-sitter-highlight Catch2::Catch2WithMain)

_bundle_resources(iditor)

