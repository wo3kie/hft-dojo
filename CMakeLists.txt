# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(HFTDojo)

enable_testing()

# Set output directory for all executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Require C++20 standard for all targets
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

function(add_dojo_executable source_file)
  get_filename_component(target_name ${source_file} NAME_WE)

  add_executable(${target_name} ${source_file})
  target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR})
endfunction()

file(GLOB DOJO_EXECUTABLE_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/*.cpp")

foreach(dojo_source ${DOJO_EXECUTABLE_SOURCES})
  add_dojo_executable(${dojo_source})
endforeach()
