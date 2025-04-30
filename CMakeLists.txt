cmake_minimum_required(VERSION 3.20)

project(xfs2json LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.c
    src/args.c
    src/util/binary_reader.c
)

set(ARGPARSE_STATIC ON)
set(ARGPARSE_SHARED OFF)
add_subdirectory(external/argparse)

add_executable(xfs2json ${SOURCES})
target_include_directories(xfs2json PRIVATE
    src
    external/argparse
)

target_link_libraries(xfs2json PRIVATE
    argparse_static
)
