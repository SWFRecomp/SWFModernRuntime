cmake_minimum_required(VERSION 3.4)

project(SWFModernRuntime)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$\{ORIGIN\}")

set(SOURCES
    ${PROJECT_SOURCE_DIR}/src/libswf/swf.c
    ${PROJECT_SOURCE_DIR}/src/libswf/tag.c
    ${PROJECT_SOURCE_DIR}/src/actionmodern/action.c
    ${PROJECT_SOURCE_DIR}/src/actionmodern/variables.cpp
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -O2)

set(RENAME_ZCONF OFF)

add_subdirectory(${PROJECT_SOURCE_DIR}/lib/zlib)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/lzma)

target_link_libraries(${PROJECT_NAME} PRIVATE
    zlib
    lzma
)

# frick u ninja
if (${CMAKE_GENERATOR} MATCHES "Ninja")
set(CONFIG_DIR .)
else()
set(CONFIG_DIR $<CONFIG>)
endif()

if (WIN32)
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/${CONFIG_DIR}/SWFModernRuntime.lib ${PROJECT_SOURCE_DIR}/
)
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/lib/zlib/${CONFIG_DIR}/zlib.dll ${CMAKE_BINARY_DIR}/
)
else()
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/${CONFIG_DIR}/libSWFModernRuntime.a ${PROJECT_SOURCE_DIR}/
)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/actionmodern
    ${PROJECT_SOURCE_DIR}/include/libswf
    zlib
    lzma/liblzma/api
)