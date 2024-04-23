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
  if(WIN32)
    add_executable(test_sdl2 WIN32 test_sdl.c test_sdl.manifest)
  else()
    add_executable(test_sdl2 test_sdl.c)
  endif()
  target_link_libraries(test_sdl2 PRIVATE nfd)
  target_include_directories(test_sdl2 PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(test_sdl2 PRIVATE ${SDL2_LINK_LIBRARIES})
  target_compile_options(test_sdl2 PUBLIC ${SDL2_CFLAGS_OTHER})
endif()
