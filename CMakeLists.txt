cmake_minimum_required(VERSION 3.10)

project(SWFModernRuntime)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$\{ORIGIN\}")

# Option to disable graphics support (console-only mode)
option(NO_GRAPHICS "Build without graphics support (console-only)" OFF)

# Core sources (always included)
set(CORE_SOURCES
    ${PROJECT_SOURCE_DIR}/src/actionmodern/action.c
    ${PROJECT_SOURCE_DIR}/src/actionmodern/variables.c
    ${PROJECT_SOURCE_DIR}/src/utils.c

    # Compile hashmap file directly to avoid undefined symbols on Linux
    ${PROJECT_SOURCE_DIR}/lib/c-hashmap/map.c
)

if(NO_GRAPHICS)
    # Console-only mode
    message(STATUS "Building in NO_GRAPHICS mode (console-only)")
    add_definitions(-DNO_GRAPHICS)

    set(SWF_SOURCES
        ${PROJECT_SOURCE_DIR}/src/libswf/swf_core.c
        ${PROJECT_SOURCE_DIR}/src/libswf/tag_stubs.c
    )

    set(SOURCES ${CORE_SOURCES} ${SWF_SOURCES})
else()
    # Full graphics mode
    message(STATUS "Building in full graphics mode")

    set(SWF_SOURCES
        ${PROJECT_SOURCE_DIR}/src/libswf/swf.c
        ${PROJECT_SOURCE_DIR}/src/libswf/tag.c
        ${PROJECT_SOURCE_DIR}/src/flashbang/flashbang.c
    )

    set(SOURCES ${CORE_SOURCES} ${SWF_SOURCES})
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES})

if (WIN32)
target_compile_options(${PROJECT_NAME} PRIVATE)
else()
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-format-truncation)
endif()

set(RENAME_ZCONF OFF)

add_subdirectory(${PROJECT_SOURCE_DIR}/lib/zlib)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/lzma)

if(NOT NO_GRAPHICS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/lib/SDL3)

    target_link_libraries(${PROJECT_NAME} PUBLIC
        zlibstatic
        lzma
        SDL3::SDL3
    )
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC
        zlibstatic
        lzma
    )
endif()

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
    ${PROJECT_SOURCE_DIR}/lib/c-hashmap
    zlib
    lzma/liblzma/api
)

if(NOT NO_GRAPHICS)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/include/flashbang
        ${PROJECT_SOURCE_DIR}/lib/SDL3/include
    )
endif()