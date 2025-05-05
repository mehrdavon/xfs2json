cmake_minimum_required(VERSION 3.20)

project(xfs2json LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.c
    src/args.c
    src/util/binary_reader.c
    src/util/binary_writer.c
    src/xfs/xfs.c
    src/xfs/xfs_json.c
    src/xfs/convert.c
    src/xfs/v16/arch_32.c
)

set(ARGPARSE_STATIC ON)
set(ARGPARSE_SHARED OFF)
add_subdirectory(external/argparse)

set(BUILD_SHARED_LIBS OFF)
set(CJSON_BUILD_SHARED_LIBS OFF)
add_subdirectory(external/cJSON)

add_executable(xfs2json ${SOURCES})
target_include_directories(xfs2json PRIVATE
    src
    external/argparse
    external/cJSON
)

target_link_libraries(xfs2json PRIVATE
    argparse_static
    cjson
)

if (WIN32)
    target_compile_definitions(xfs2json PRIVATE
        _CRT_SECURE_NO_WARNINGS
        strdup=_strdup
    )
endif()
