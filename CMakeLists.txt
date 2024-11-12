cmake_minimum_required(VERSION 3.4)

project(SWFModernRuntime)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/swf.c
    ${CMAKE_SOURCE_DIR}/src/tag.c
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -O2)

set(RENAME_ZCONF OFF)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/zlib)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/lzma)

target_link_libraries(${PROJECT_NAME} PRIVATE
    zlib
    lzma
)

if (WIN32)
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/lib/zlib/zlib.dll... ${CMAKE_BINARY_DIR}/
)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    zlib
    lzma/liblzma/api
)