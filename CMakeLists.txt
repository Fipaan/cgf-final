cmake_minimum_required(VERSION 3.20)

if(WIN32)
    set(CMAKE_C_COMPILER       clang)
    set(CMAKE_CXX_COMPILER     clang++)
endif()

project(cgf
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_link_options(-fuse-ld=lld)
endif()

include(FetchContent)

if(EMSCRIPTEN)
    message(FATAL_ERROR "WASM is not supported")
elseif(WIN32)
    set(_platform "win64")
elseif(APPLE)
    set(_platform "mac")
    message(WARNING "Mac was not tested!")
elseif(UNIX)
    set(_platform "linux_x86_64")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "========== Build Configuration ==========")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Platform: ${_platform}")
message(STATUS "=========================================")
message(STATUS "System:       ${CMAKE_SYSTEM_NAME}")
message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMake:        ${CMAKE_VERSION}")
message(STATUS "Generator:    ${CMAKE_GENERATOR}")
message(STATUS "Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler:   ${CMAKE_C_COMPILER}")
message(STATUS "C Version:    ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "CXX Version:  ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CXX Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Linker flags: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "==========================================")

# Sources

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(${PROJECT_NAME}
    ${SOURCES}
    src/glad/glad.c
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ABORT_ON_ANY_GL_ERROR
)

# Include

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Flags

target_compile_options(${PROJECT_NAME} PRIVATE
    "-Wall" "-Wextra"
    "-Wno-missing-designated-field-initializers"
)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE "-g")
else()
    target_compile_options(${PROJECT_NAME} PRIVATE "-O3")
endif()

set(ERROR_LIMIT 1)
if(WIN32)
    target_compile_options(${PROJECT_NAME} PRIVATE -ferror-limit=${ERROR_LIMIT})
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -fmax-errors=${ERROR_LIMIT})
endif()

# Fetching

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
    GIT_TAG VER-2-13-3
)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

# NOTE: freetype has some issues with versioning, which is why it was disabled
set(_prev_warn_deprecated "${CMAKE_WARN_DEPRECATED}")
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(freetype)
set(CMAKE_WARN_DEPRECATED "${_prev_warn_deprecated}" CACHE BOOL "" FORCE)

# Linking

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    freetype
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++exp)
endif()