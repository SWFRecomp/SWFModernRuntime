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
    ${PROJECT_SOURCE_DIR}/src/flashbang/flashbang.c
    ${PROJECT_SOURCE_DIR}/src/utils.c
    
    # Compile hashmap file directly to avoid undefined symbols on Linux
    ${PROJECT_SOURCE_DIR}/lib/c-hashmap/map.c
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

if (WIN32)
target_compile_options(${PROJECT_NAME} PRIVATE)
else()
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-format-truncation)
endif()

set(RENAME_ZCONF OFF)

add_subdirectory(${PROJECT_SOURCE_DIR}/lib/zlib)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/lzma)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/SDL3)

target_link_libraries(${PROJECT_NAME} PUBLIC
    zlibstatic
    lzma
    SDL3::SDL3
)

# frick u ninja
if (${CMAKE_GENERATOR} MATCHES "Ninja")
set(CONFIG_DIR .)
else()
set(CONFIG_DIR $<CONFIG>)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/actionmodern
    ${PROJECT_SOURCE_DIR}/include/libswf
    ${PROJECT_SOURCE_DIR}/include/flashbang
    ${PROJECT_SOURCE_DIR}/lib/c-hashmap
    ${PROJECT_SOURCE_DIR}/lib/SDL3/include
    zlib
    lzma/liblzma/api
)