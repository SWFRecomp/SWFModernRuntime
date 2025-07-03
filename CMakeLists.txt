cmake_minimum_required(VERSION 3.10)

project(SWFModernRuntime)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$\{ORIGIN\}")

set(SOURCES
    ${PROJECT_SOURCE_DIR}/src/libswf/swf.c
    ${PROJECT_SOURCE_DIR}/src/libswf/tag.c
    ${PROJECT_SOURCE_DIR}/src/actionmodern/action.c
    ${PROJECT_SOURCE_DIR}/src/actionmodern/variables.c
    ${PROJECT_SOURCE_DIR}/src/utils.c
    
    # Compile hashmap file directly to avoid undefined symbols on Linux
    ${PROJECT_SOURCE_DIR}/lib/c-hashmap/map.c
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

if (WIN32)
target_compile_options(${PROJECT_NAME} PRIVATE -O2)
else()
target_compile_options(${PROJECT_NAME} PRIVATE -O2 -Wno-format-truncation)
endif()

set(RENAME_ZCONF OFF)

add_subdirectory(${PROJECT_SOURCE_DIR}/lib/zlib)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/lzma)

target_link_libraries(${PROJECT_NAME} PRIVATE
    zlibstatic
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
    ${PROJECT_SOURCE_DIR}/lib/c-hashmap
    zlib
    lzma/liblzma/api
)