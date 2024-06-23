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
