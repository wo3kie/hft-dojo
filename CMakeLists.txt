# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(HFTDojo)

enable_testing()

option(BUILD_DOJO_TESTS "Build unit tests" ON)
option(BUILD_DOJO_BENCHMARKS "Build benchmarks" OFF)

# Set output directory for all executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Require C++20 standard for all targets
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

function(add_dojo_test source_file)
  get_filename_component(target_name ${source_file} NAME_WE)
  string(REGEX REPLACE "_test$" "" test_name "${target_name}")

  add_executable(${target_name} ${source_file})
  target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR})
  add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${target_name}>)
endfunction()

function(add_dojo_benchmark source_file)
  get_filename_component(target_name ${source_file} NAME_WE)

  add_executable(${target_name} ${source_file})
  target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR})
endfunction()

if(BUILD_DOJO_TESTS)
  # Discover all test translation units under tests/.
  file(GLOB DOJO_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*_test.cpp")

  foreach(dojo_test ${DOJO_TEST_SOURCES})
    add_dojo_test(${dojo_test})
  endforeach()
endif()

if(BUILD_DOJO_BENCHMARKS)
  file(GLOB DOJO_BENCHMARK_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/benchmarks/*_bench.cpp")

  foreach(dojo_benchmark ${DOJO_BENCHMARK_SOURCES})
    add_dojo_benchmark(${dojo_benchmark})
  endforeach()
endif()
