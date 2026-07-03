cmake_minimum_required(VERSION 3.20)

project(SpaceMonger VERSION 1.4.0 LANGUAGES CXX RC)

if(NOT MSVC)
  message(FATAL_ERROR "SpaceMonger is a Win32 MFC application and currently requires MSVC.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "Configure with a 32-bit generator/platform, for example: cmake --preset vs2022-win32")
endif()

get_filename_component(_msvc_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(_msvc_host_dir "${_msvc_cl_dir}" DIRECTORY)
get_filename_component(_msvc_bin_dir "${_msvc_host_dir}" DIRECTORY)
get_filename_component(_msvc_tool_dir "${_msvc_bin_dir}" DIRECTORY)
if(NOT EXISTS "${_msvc_tool_dir}/ATLMFC/include/afxwin.h")
  message(FATAL_ERROR
    "MFC headers were not found for the selected MSVC toolset. "
    "Install the Visual Studio component 'C++ MFC for latest v143 build tools (x86 & x64)' "
    "and then rerun: cmake --preset vs2022-win32"
  )
endif()

enable_testing()

set(SPACEMONGER_SOURCES
  AboutDialog.cpp
  DriveDialog.cpp
  DiskUsage.cpp
  FolderEntryArrays.cpp
  FolderTree.cpp
  FolderView.cpp
  Freedoc.cpp
  Freeview.cpp
  lang_en.cpp
  lang_fr.cpp
  Mainfrm.cpp
  PathUtil.cpp
  SetupDlg.cpp
  SpaceMonger.cpp
  StdAfx.cpp
  TipWnd.cpp
  SpaceMonger.rc
)

add_executable(SpaceMonger WIN32 ${SPACEMONGER_SOURCES})
target_include_directories(SpaceMonger PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(SpaceMonger PRIVATE WIN32 _WINDOWS _MBCS)
target_compile_options(SpaceMonger PRIVATE /W3 /EHsc)
target_precompile_headers(SpaceMonger PRIVATE Stdafx.h)
set_source_files_properties(PathUtil.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
set_target_properties(SpaceMonger PROPERTIES
  MFC_FLAG 1
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)


add_executable(DiskUsage_test
  tests/DiskUsage_test.cpp
  DiskUsage.cpp
  DiskUsage.h
)
target_include_directories(DiskUsage_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(DiskUsage_test PRIVATE WIN32 _WINDOWS _MBCS)
target_compile_options(DiskUsage_test PRIVATE /W3 /EHsc)
set_target_properties(DiskUsage_test PROPERTIES
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
add_test(NAME DiskUsage_test COMMAND DiskUsage_test)

add_executable(PathUtil_test
  tests/PathUtil_test.cpp
  PathUtil.cpp
  PathUtil.h
)
target_include_directories(PathUtil_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(PathUtil_test PRIVATE WIN32 _WINDOWS _MBCS)
target_compile_options(PathUtil_test PRIVATE /W3 /EHsc)
set_target_properties(PathUtil_test PROPERTIES
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
add_test(NAME PathUtil_test COMMAND PathUtil_test)

add_executable(StringArena_test
  tests/StringArena_test.cpp
  StringArena.h
)
target_include_directories(StringArena_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(StringArena_test PRIVATE WIN32 _WINDOWS _MBCS)
target_compile_options(StringArena_test PRIVATE /W3 /EHsc)
set_target_properties(StringArena_test PROPERTIES
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
add_test(NAME StringArena_test COMMAND StringArena_test)

add_executable(FolderEntryArrays_test
  tests/FolderEntryArrays_test.cpp
  FolderEntryArrays.cpp
  FolderEntryArrays.h
)
target_include_directories(FolderEntryArrays_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FolderEntryArrays_test PRIVATE WIN32 _WINDOWS _MBCS)
target_compile_options(FolderEntryArrays_test PRIVATE /W3 /EHsc)
set_target_properties(FolderEntryArrays_test PROPERTIES
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
add_test(NAME FolderEntryArrays_test COMMAND FolderEntryArrays_test)
