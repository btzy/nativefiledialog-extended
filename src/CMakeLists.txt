set(TARGET_NAME nfd)

set(SOURCE_FILES
  include/nfd.h
  include/nfd.hpp)

if(nfd_PLATFORM STREQUAL PLATFORM_WIN32)
  list(APPEND SOURCE_FILES nfd_win.cpp)
endif()

if(nfd_PLATFORM STREQUAL PLATFORM_LINUX)
  list(APPEND SOURCE_FILES nfd_gtk.cpp)
endif()

if(nfd_PLATFORM STREQUAL PLATFORM_MACOS)
  list(APPEND SOURCE_FILES nfd_cocoa.m)
endif()

# Define the library
add_library(${TARGET_NAME} STATIC
  ${SOURCE_FILES})

# Allow includes from include/
target_include_directories(${TARGET_NAME}
  PUBLIC include/)
