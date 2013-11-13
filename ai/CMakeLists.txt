cmake_minimum_required(VERSION 2.8.12)

find_package(LLVM)

# Define add_llvm_* macro's.
include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

SET(CMAKE_BUILD_TYPE distribution)
SET(CMAKE_CXX_FLAGS_DISTRIBUTION "-O3 -std=c++0x")
SET(CMAKE_C_FLAGS_DISTRIBUTION "-O3 -std=c++0x")

add_llvm_external_project(ai-rt)
add_subdirectory(llvm)
add_subdirectory(tools)
