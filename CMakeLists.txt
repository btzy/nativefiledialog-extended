cmake_minimum_required(VERSION 3.2)
project(nativefiledialog-extended)

set(nfd_PLATFORM Undefined)
if(WIN32)
  set(nfd_PLATFORM PLATFORM_WIN32)
elseif(APPLE)
  set(nfd_PLATFORM PLATFORM_MACOS)
elseif(UNIX AND NOT APPLE)
  set(nfd_PLATFORM PLATFORM_LINUX)
endif()

message("nfd Platform: ${nfd_PLATFORM}")

set(nfd_COMPILER Undefined)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
  # This is clang-cl, which has different compiler options
  set(nfd_COMPILER COMPILER_CLANGCL)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(nfd_COMPILER COMPILER_MSVC)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(nfd_COMPILER COMPILER_GNU)
endif()

message("nfd Compiler: ${nfd_COMPILER}")

# Use latest C++ by default (should be the best one), but let user override it
if(NOT DEFINED CMAKE_CXX_STANDARD)
  if(CMAKE_VERSION VERSION_LESS "3.8")
    set (CMAKE_CXX_STANDARD 14)
  elseif(CMAKE_VERSION VERSION_LESS "3.12")
    set (CMAKE_CXX_STANDARD 17)
  elseif(CMAKE_VERSION VERSION_LESS "3.20")
    set (CMAKE_CXX_STANDARD 20)
  else()
    set (CMAKE_CXX_STANDARD 23)
  endif()
endif()

# For setting the filter list, macOS introduced allowedContentTypes in version 11.0 and deprecated allowedFileTypes in 12.0.
# By default (set to ON), NFDe will use allowedContentTypes when targeting macOS >= 11.0.
# Set this option to OFF to always use allowedFileTypes regardless of the target macOS version.
# This is mainly needed for applications that are built on macOS >= 11.0 but should be able to run on lower versions
# and should not be used otherwise.
option(NFD_USE_ALLOWEDCONTENTTYPES "Use allowedContentTypes for filter lists on macOS >= 11.0" ON)

add_subdirectory(src)

option(NFD_BUILD_TESTS "Build tests for nfd" OFF)
if(${NFD_BUILD_TESTS})
  add_subdirectory(test)
endif()
