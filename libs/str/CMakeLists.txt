# set a minumum version of cmake
cmake_minimum_required(VERSION 3.31.2)

# name of the project
project(libstr
    VERSION 1.0.0
    DESCRIPTION "A custom C str library"
    LANGUAGES C
)

set(LIB_NAME ${PROJECT_NAME})
# 
option(BUILD_AS_SHARED "build as a shared library" OFF)

if(BUILD_AS_SHARED)
    add_library(${LIB_NAME} SHARED)
else()
    add_library(${LIB_NAME} STATIC)
endif()

# NOTE: add_library can also accept a list of source files but for better maintainability, target_sources is used
target_sources(${LIB_NAME}
    PRIVATE
    src/str.c
)

# Add the include directory, which contains the header files. 
# From the directory registered here, `#include` searches for header files.
target_include_directories(${LIB_NAME}
    PUBLIC
    # PROJECT_SOURCE_DIR => Absolute path to the directory containing this CMakeLists.txt file
    ${PROJECT_SOURCE_DIR}/include
)

# add an option to build example project (can be turned on or off from cmake GUI or command line)
option(BUILD_EXAMPLE "Whether or not to build example project" OFF)
if(BUILD_EXAMPLE)
    message(STATUS "Building example...")
    add_subdirectory(examples EXCLUDE_FROM_ALL)
endif()
