# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(HFTDojo)

enable_testing()

# debug information
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")

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

set(DOJO_TEST_SOURCES
  ${CMAKE_SOURCE_DIR}/tests/array_test.cpp
  ${CMAKE_SOURCE_DIR}/tests/branchless_test.cpp
  ${CMAKE_SOURCE_DIR}/tests/flat_list_test.cpp
  ${CMAKE_SOURCE_DIR}/tests/map_reduce_test.cpp
  ${CMAKE_SOURCE_DIR}/tests/order_book_test.cpp
  ${CMAKE_SOURCE_DIR}/tests/price_levels_test.cpp
  ${CMAKE_SOURCE_DIR}/tests/ring_buffer_test.cpp
  ${CMAKE_SOURCE_DIR}/tests/task_executor_test.cpp
  ${CMAKE_SOURCE_DIR}/tests/task_worker_test.cpp
  ${CMAKE_SOURCE_DIR}/tests/trade_engine_test.cpp
)

foreach(dojo_test ${DOJO_TEST_SOURCES})
  add_dojo_test(${dojo_test})
endforeach()
