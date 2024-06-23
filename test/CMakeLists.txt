cmake_minimum_required(VERSION 3.14)

project(rapidutfTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(rapidutf REQUIRED)
  enable_testing()
endif()

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(rapidutf_test source/rapidutf_test.cpp)
target_link_libraries(
    rapidutf_test PRIVATE
    rapidutf::rapidutf
    Catch2::Catch2WithMain
)
target_compile_features(rapidutf_test PRIVATE cxx_std_17)

catch_discover_tests(rapidutf_test)

# ---- End-of-file commands ----

add_folders(Test)
