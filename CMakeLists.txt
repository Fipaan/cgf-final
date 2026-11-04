cmake_minimum_required(VERSION 3.20)

project(cgf
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

if(EMSCRIPTEN)
    message(FATAL_ERROR "WASM is not supported")
elseif(WIN32)
    set(MY_PLATFORM "win64")
elseif(APPLE)
    set(MY_PLATFORM "mac")
    message(WARNING "Mac was not tested!")
elseif(UNIX)
    set(MY_PLATFORM "linux_x86_64")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "Building ${PROJECT_NAME}")
message(STATUS "Platform: ${MY_PLATFORM}")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(${PROJECT_NAME}
    ${SOURCES}
    src/glad/glad.c
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ABORT_ON_ANY_GL_ERROR
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(${PROJECT_NAME} PRIVATE -fmax-errors=5)

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
FetchContent_MakeAvailable(freetype)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    freetype
)
