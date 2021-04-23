set(TARGET_NAME nfd)

set(PUBLIC_HEADER_FILES
  include/nfd.h
  include/nfd.hpp)

set(SOURCE_FILES ${PUBLIC_HEADER_FILES})

if(nfd_PLATFORM STREQUAL PLATFORM_WIN32)
  list(APPEND SOURCE_FILES nfd_win.cpp)
endif()

if(nfd_PLATFORM STREQUAL PLATFORM_LINUX)
  find_package(PkgConfig REQUIRED)
  set(NFD_GTK_VERSION "" CACHE STRING "GTK version for Linux builds ('3' or '4')")
  set_property(CACHE NFD_GTK_VERSION PROPERTY STRINGS "" 3 4)
  # For Linux, we support both GTK3 and GTK4.
  # If NFD_GTK_VERSION is not explicitly set, then we take one that is available.
  # Otherwise, we find the version that the user wants.
  if(NFD_GTK_VERSION STREQUAL "")
    pkg_search_module(GTK REQUIRED gtk+-3.0 gtk4)
    if(DEFINED GTK_gtk+-3.0_VERSION)
      set(GTK_VERSION ${GTK_gtk+-3.0_VERSION})
    elseif(DEFINED GTK_gtk4_VERSION)
      set(GTK_VERSION ${GTK_gtk4_VERSION})
    endif()
  elseif(NFD_GTK_VERSION STREQUAL 3)
    pkg_check_modules(GTK REQUIRED gtk+-3.0)
  elseif(NFD_GTK_VERSION STREQUAL 4)
    pkg_check_modules(GTK REQUIRED gtk4)
  else()
    message(FATAL_ERROR "Unsupported GTK version: ${NFD_GTK_VERSION}")
  endif()

  message("Using GTK version: ${GTK_VERSION}")
  list(APPEND SOURCE_FILES nfd_gtk.cpp)
endif()

if(nfd_PLATFORM STREQUAL PLATFORM_MACOS)
  find_library(APPKIT_LIBRARY AppKit)
  list(APPEND SOURCE_FILES nfd_cocoa.m)
endif()

# Define the library
add_library(${TARGET_NAME} STATIC
  ${SOURCE_FILES})

# Allow includes from include/
target_include_directories(${TARGET_NAME}
  PUBLIC include/)

if(nfd_PLATFORM STREQUAL PLATFORM_LINUX)
  target_include_directories(${TARGET_NAME}
    PRIVATE ${GTK_INCLUDE_DIRS})
  target_link_libraries(${TARGET_NAME}
    PRIVATE ${GTK_LIBRARIES})
  string(REPLACE "." ";" GTK_VERSION_LIST ${GTK_VERSION})
  list(GET GTK_VERSION_LIST 0 GTK_VERSION_MAJOR)
  target_compile_definitions(${TARGET_NAME} PUBLIC NFD_GTK_VERSION=${GTK_VERSION_MAJOR})
endif()

if(nfd_PLATFORM STREQUAL PLATFORM_MACOS)
  target_link_libraries(${TARGET_NAME}
    PRIVATE ${APPKIT_LIBRARY})
endif()

if(nfd_COMPILER STREQUAL COMPILER_MSVC)
  string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY STATIC_LIBRARY_OPTIONS /NODEFAULTLIB)
endif()

if(nfd_COMPILER STREQUAL COMPILER_GNU)
  target_compile_options(${TARGET_NAME} PRIVATE -nostdlib -fno-exceptions -fno-rtti)
endif()

set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADER_FILES}")

install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include)
