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

# Collect all test translation units in the root directory.
file(GLOB TEST_DOJOS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/*.test.cpp")

# For each test file, create a separate executable 
foreach(DOJO ${TEST_DOJOS})
  get_filename_component(FILE_NAME ${DOJO} NAME)
  string(REGEX REPLACE "\\.cpp$" "" TEST_NAME "${FILE_NAME}")
  string(REGEX REPLACE "\\.test$" "" TARGET_NAME "${TEST_NAME}")

  # Create an executable for this lesson
  add_executable(${TARGET_NAME} ${DOJO})
  set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME "${TEST_NAME}")  
  add_test(NAME ${TEST_NAME} COMMAND $<TARGET_FILE:${TARGET_NAME}>)
endforeach()
