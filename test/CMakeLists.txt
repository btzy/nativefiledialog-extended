if(${NFD_BUILD_TESTS})
  set(TEST_LIST
    test_opendialog.c
    test_opendialog_cpp.cpp
    test_opendialog_native.c
    test_opendialog_with.c
    test_opendialog_native_with.c
    test_opendialogmultiple.c
    test_opendialogmultiple_cpp.cpp
    test_opendialogmultiple_native.c
    test_opendialogmultiple_enum.c
    test_opendialogmultiple_enum_native.c
    test_pickfolder.c
    test_pickfolder_cpp.cpp
    test_pickfolder_native.c
    test_pickfolder_with.c
    test_pickfolder_native_with.c
    test_pickfoldermultiple.c
    test_pickfoldermultiple_native.c
    test_savedialog.c
    test_savedialog_native.c
    test_savedialog_with.c
    test_savedialog_native_with.c)

  foreach (TEST ${TEST_LIST})
    string(REPLACE "." "_" CLEAN_TEST_NAME ${TEST})
    add_executable(${CLEAN_TEST_NAME}
      ${TEST})
    target_link_libraries(${CLEAN_TEST_NAME}
      PRIVATE nfd)
  endforeach()
endif()

if(${NFD_BUILD_SDL2_TESTS})
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED sdl2 SDL2_ttf)
  if(nfd_PLATFORM STREQUAL PLATFORM_WIN32)
    add_executable(test_sdl2 WIN32 test_sdl.c test.manifest)
  else()
    add_executable(test_sdl2 test_sdl.c)
  endif()
  target_link_libraries(test_sdl2 PRIVATE nfd)
  target_include_directories(test_sdl2 PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(test_sdl2 PRIVATE ${SDL2_LINK_LIBRARIES})
  target_compile_options(test_sdl2 PUBLIC ${SDL2_CFLAGS_OTHER})
endif()

if(${NFD_BUILD_GLFW3_TESTS})
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GLFW3 REQUIRED glfw3)
  if(nfd_PLATFORM STREQUAL PLATFORM_WIN32)
    add_executable(test_glfw3 WIN32 test_glfw.c test.manifest)
  else()
    add_executable(test_glfw3 test_glfw.c)
  endif()
  target_link_libraries(test_glfw3 PRIVATE nfd)
  target_include_directories(test_glfw3 PRIVATE ${GLFW3_INCLUDE_DIRS})
  target_link_libraries(test_glfw3 PRIVATE ${GLFW3_LINK_LIBRARIES})
  target_compile_options(test_glfw3 PUBLIC ${GLFW3_CFLAGS_OTHER})
  if(nfd_PLATFORM STREQUAL PLATFORM_WIN32)
    target_link_libraries(test_glfw3 PRIVATE opengl32)
    target_compile_definitions(test_glfw3 PRIVATE GLFW_EXPOSE_NATIVE_WIN32)
  elseif(nfd_PLATFORM STREQUAL PLATFORM_MACOS)
    find_package(OpenGL REQUIRED)
    target_include_directories(test_glfw3 PRIVATE ${OPENGL_INCLUDE_DIRS})
    target_link_libraries(test_glfw3 PRIVATE ${OPENGL_LIBRARIES})
    target_compile_definitions(test_glfw3 PRIVATE GLFW_EXPOSE_NATIVE_COCOA)
    target_compile_definitions(test_glfw3 PRIVATE GL_SILENCE_DEPRECATION) # OpenGL is deprecated on macOS
  elseif(nfd_PLATFORM STREQUAL PLATFORM_LINUX)
    pkg_check_modules(OPENGL REQUIRED gl)
    target_include_directories(test_glfw3 PRIVATE ${OPENGL_INCLUDE_DIRS})
    target_link_libraries(test_glfw3 PRIVATE ${OPENGL_LINK_LIBRARIES})
    target_compile_options(test_glfw3 PUBLIC ${OPENGL_CFLAGS_OTHER})
    if(${NFD_X11})
      include(CheckCXXSourceCompiles)
      set(CMAKE_REQUIRED_INCLUDES ${GLFW3_INCLUDE_DIRS})
      set(CMAKE_REQUIRED_LIBRARIES ${GLFW3_LINK_LIBRARIES})
      check_cxx_source_compiles(
        "
        #define GLFW_EXPOSE_NATIVE_X11
        #include <GLFW/glfw3.h>
        #include <GLFW/glfw3native.h>
        int main() { glfwGetX11Window(nullptr); return 0; }
        "
        NFD_GLFW_HAS_X11
      )
      unset(CMAKE_REQUIRED_LIBRARIES)
      unset(CMAKE_REQUIRED_INCLUDES)
      if(NFD_GLFW_HAS_X11)
        target_compile_definitions(test_glfw3 PRIVATE GLFW_EXPOSE_NATIVE_X11)
      endif()
    endif()
    if(${NFD_WAYLAND})
      include(CheckCXXSourceCompiles)
      set(CMAKE_REQUIRED_INCLUDES ${GLFW3_INCLUDE_DIRS})
      set(CMAKE_REQUIRED_LIBRARIES ${GLFW3_LINK_LIBRARIES})
      check_cxx_source_compiles(
        "
        #define GLFW_EXPOSE_NATIVE_WAYLAND
        #include <GLFW/glfw3.h>
        #include <GLFW/glfw3native.h>
        int main() { glfwGetWaylandWindow(nullptr); return 0; }
        "
        NFD_GLFW_HAS_WAYLAND
      )
      unset(CMAKE_REQUIRED_LIBRARIES)
      unset(CMAKE_REQUIRED_INCLUDES)
      if(NFD_GLFW_HAS_WAYLAND)
        target_compile_definitions(test_glfw3 PRIVATE GLFW_EXPOSE_NATIVE_WAYLAND)
      endif()
    endif()
  endif()
endif()
