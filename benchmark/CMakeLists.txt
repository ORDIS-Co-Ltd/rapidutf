cmake_minimum_required(VERSION 3.14)

project(rapidutfBenchmarks LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(rapidutf REQUIRED)
endif()

find_package(benchmark CONFIG REQUIRED)

# ---- Benchmarks ----

add_executable(rapidutf_benchmark source/rapidutf_benchmark.cpp)
target_link_libraries(
    rapidutf_benchmark PRIVATE
    rapidutf::rapidutf
    benchmark::benchmark
    benchmark::benchmark_main
)

# add include directories for target
target_include_directories(rapidutf_benchmark PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(rapidutf_benchmark PRIVATE cxx_std_17)
set_target_properties(rapidutf_benchmark PROPERTIES CXX_CLANG_TIDY "")

# ---- End-of-file commands ----

add_folders(Benchmark)