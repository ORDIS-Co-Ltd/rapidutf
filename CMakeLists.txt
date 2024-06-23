cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    rapidutf
    VERSION 0.1.0
    DESCRIPTION "RapidUTF is a lightweight, high-performance C++ library designed for fast Unicode conversions between UTF-8, UTF-16, and UTF-32 encodings. Leveraging SIMD (Single Instruction, Multiple Data) instructions, RapidUTF achieves exceptional speed in character encoding transformations, making it ideal for applications requiring efficient text processing and manipulation."
    HOMEPAGE_URL "https://ordis.co.th"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    rapidutf_rapidutf
    source/rapidutf.cpp
)
add_library(rapidutf::rapidutf ALIAS rapidutf_rapidutf)

######################################################################
# Check cpu for capabilities
######################################################################

include(CheckCXXCompilerFlag)

# Check for ARM64 architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    target_compile_definitions(rapidutf_rapidutf PRIVATE RAPIDUTF_USE_NEON)
    message(STATUS "RapidUTF: Using NEON instructions for ARM64")
# Check for x64 architecture
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|x64")
    if(MSVC)
        # AVX2 and SSE 4.2 are enabled by default in MSVC for x64
        target_compile_definitions(rapidutf_rapidutf PRIVATE RAPIDUTF_USE_AVX2)
        message(STATUS "RapidUTF: Using AVX2 instructions (MSVC x64)")
    else()
        # Check for AVX2
        check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
        if(COMPILER_SUPPORTS_AVX2)
            target_compile_options(rapidutf_rapidutf PRIVATE -mavx2)
            target_compile_definitions(rapidutf_rapidutf PRIVATE RAPIDUTF_USE_AVX2)
            message(STATUS "RapidUTF: Using AVX2 instructions")
        else()
            # If no AVX2, check for SSE4.2
            check_cxx_compiler_flag("-msse4.2" COMPILER_SUPPORTS_SSE4_2)
            if(COMPILER_SUPPORTS_SSE4_2)
                target_compile_options(rapidutf_rapidutf PRIVATE -msse4.2)
                target_compile_definitions(rapidutf_rapidutf PRIVATE RAPIDUTF_USE_SSE_4_2)
                message(STATUS "RapidUTF: Using SSE 4.2 instructions")
            else()
                message(STATUS "RapidUTF: No SIMD instructions used (x64 without AVX2 or SSE4.2 support)")
            endif()
        endif()
    endif()
else()
    message(STATUS "RapidUTF: No SIMD instructions used (not ARM64 or x64)")
endif()

include(GenerateExportHeader)
generate_export_header(
    rapidutf_rapidutf
    BASE_NAME rapidutf
    EXPORT_FILE_NAME export/rapidutf/rapidutf_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(rapidutf_rapidutf PUBLIC RAPIDUTF_STATIC_DEFINE)
endif()

set_target_properties(
    rapidutf_rapidutf PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME rapidutf
    OUTPUT_NAME rapidutf
)

target_include_directories(
    rapidutf_rapidutf ${warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    rapidutf_rapidutf SYSTEM
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(rapidutf_rapidutf PUBLIC cxx_std_17)

find_package(fmt REQUIRED)
target_link_libraries(rapidutf_rapidutf PRIVATE fmt::fmt)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${rapidutf_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT rapidutf_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of rapidutf"
  )
endif()

include(cmake/dev-mode.cmake)
