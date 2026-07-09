cmake_minimum_required(VERSION 3.23)

set(NAME "FastDecompressSkyrim")
set(VERSION 1.4.0)

message(">>> Start build for '${NAME}' v${VERSION}")

set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SOURCE_DIR "${ROOT_DIR}/src")
set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR ">> In-source builds not allowed.")
endif()

# Setup vcpkg BEFORE project()
macro(set_from_environment VARIABLE)
  if(NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
    set(${VARIABLE} $ENV{${VARIABLE}})
  endif()
endmacro()

set_from_environment(VCPKG_ROOT)
if(DEFINED VCPKG_ROOT)
  set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
  set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "")
  set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_SOURCE_DIR}/cmake" CACHE STRING "")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "")
else()
  message(FATAL_ERROR ">> Variable VCPKG_ROOT is not set!")
endif()

project(${NAME} VERSION ${VERSION} LANGUAGES CXX C)
message(">>> Building project '${PROJECT_NAME}' v${PROJECT_VERSION}")

add_compile_definitions(
    _CRT_SECURE_NO_WARNINGS
    SKYRIMSE
)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

# CommonLibSSE-NG (via vcpkg)
find_package(CommonLibSSE CONFIG REQUIRED)

# libdeflate (for whole-buffer zlib decompression)
find_package(libdeflate CONFIG REQUIRED)

# Microsoft Detours (for function hooking — handles all x86-64 prologues)
find_path(DETOURS_INCLUDE_DIRS "detours/detours.h")
find_library(DETOURS_LIBRARY detours REQUIRED)

# zlib-ng compat mode (built from source via FetchContent)
include(FetchContent)
FetchContent_Declare(
    zlibng
    GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git
    GIT_TAG        2.1.5
)
set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)
set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(ZLIBNG_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(WITH_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(INSTALL_LIB OFF CACHE BOOL "" FORCE)
set(INSTALL_INC OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(zlibng)
message(">>> zlib-ng compat mode built from source")

# LZ4 (latest version, built from source via FetchContent)
FetchContent_Declare(
    lz4
    GIT_REPOSITORY https://github.com/lz4/lz4.git
    GIT_TAG        v1.10.0
    SOURCE_SUBDIR  build/cmake
)
set(LZ4_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(lz4)
# Optimize LZ4: direct memory access (safe on x86), AVX2 codegen
target_compile_definitions(lz4_static PRIVATE LZ4_FORCE_MEMORY_ACCESS=2)
if(MSVC)
    target_compile_options(lz4_static PRIVATE /arch:AVX2 /O2 /Oi)
endif()
message(">>> LZ4 v1.10.0 built from source (AVX2 + direct memory access)")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.h"
)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${SOURCES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${PROJECT_NAME} PRIVATE "${SOURCE_DIR}/PCH.h")

target_include_directories(${PROJECT_NAME} PRIVATE
    "${SOURCE_DIR}"
    "${zlibng_BINARY_DIR}"    # for generated zconf.h
    "${zlibng_SOURCE_DIR}"    # for zlib.h
    "${lz4_SOURCE_DIR}/lib"   # for lz4.h
    ${DETOURS_INCLUDE_DIRS}   # for detours/detours.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    libdeflate::libdeflate_static
    zlibstatic
    lz4_static
    ${DETOURS_LIBRARY}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3
        /permissive-
        /Zc:preprocessor
        /Zc:__cplusplus
        /arch:AVX2
        /Zi
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        /DEBUG
        /OPT:REF
        /OPT:ICF
    )
endif()

message(">>> ${PROJECT_NAME} configured successfully")
