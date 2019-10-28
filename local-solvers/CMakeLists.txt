cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake-modules)
project(sapi-local CXX)
include(ExternalProject)

file(STRINGS ../version.txt SAPI_VERSION LIMIT_COUNT 1)

# Enable C++11 support
# requires g++ >=4.4 on Linux
if(CMAKE_COMPILER_IS_GNUCXX)
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
    OUTPUT_VARIABLE GXX_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  if(GXX_VERSION VERSION_LESS 4.4)
    message(SEND_ERROR "g++ >= 4.4 required")
  elseif(GXX_VERSION VERSION_LESS 4.7)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
elseif(MSVC)
  add_definitions(/D_WIN32_WINNT=0x501)
  if(MSVC_VERSION EQUAL 1700)
    add_definitions(/D_VARIADIC_MAX=10)
  endif()
endif()

# Source files
include_directories(include)
set(SAPILOCAL_SOURCES
  ${CMAKE_SOURCE_DIR}/src/problems.cpp
  ${CMAKE_SOURCE_DIR}/src/orang-solvers.cpp
  ${CMAKE_SOURCE_DIR}/src/orang-heuristic.cpp)

# Common headers: orang, boost, include dir
ExternalProject_Add(orang
                    GIT_REPOSITORY "http://git.dwavesys.local/scm/~marshall/orang.git"
                    GIT_TAG "v1.1"
                    UPDATE_COMMAND ""
                    CONFIGURE_COMMAND ""
                    CMAKE_COMMAND ""
                    BUILD_COMMAND ""
                    INSTALL_COMMAND "")

find_package(Boost 1.53 REQUIRED)
include_directories(include ${CMAKE_CURRENT_BINARY_DIR}/orang-prefix/src/orang/src ${Boost_INCLUDE_DIR})

# Core unit tests
option(ENABLE_TESTS "Enable core C++ unit tests" OFF)
if(ENABLE_TESTS)
  set(GTEST_INCLUDEDIR "" CACHE PATH "GTest header file directory")
  set(GTEST_LIBRARYDIR "" CACHE PATH "GTest library directory")
  find_path(GTest_INCLUDE_DIR gtest/gtest.h
    PATHS ${GTEST_INCLUDEDIR} NO_DEFAULT_PATH)
  find_path(GTest_INCLUDE_DIR gtest/gtest.h)
  find_library(GTest_LIBRARY gtest PATHS ${GTEST_LIBRARYDIR})
  find_library(GTest_MAIN gtest_main PATHS ${GTEST_LIBRARYDIR})
  if(NOT GTest_INCLUDE_DIR)
    message(SEND_ERROR "GTest headers not found")
  endif()
  if(NOT GTest_LIBRARY)
    message(SEND_ERROR "GTest library not found")
  endif()
  if(NOT GTest_MAIN)
    message(SEND_ERROR "GTest main library not found")
  endif()
  mark_as_advanced(GTest_INCLUDE_DIR GTest_LIBRARY GTest_MAIN)

  add_subdirectory(test)
endif()

### Source package
option(ENABLE_SOURCE_PACKAGE "Enable source code packaging" OFF)
if(ENABLE_SOURCE_PACKAGE)
  list(APPEND CPACK_COMPONENTS_ALL source)
  install(DIRECTORY include/sapi-local ${Orang_INCLUDE_DIR}/orang
    DESTINATION include COMPONENT source
    FILES_MATCHING REGEX ".*\\.h(pp)?$")
  install(FILES ${SAPILOCAL_SOURCES} DESTINATION src/sapi-local
    COMPONENT source)
endif()

### Libraries

# Python
option(ENABLE_PYTHON "Enable Python library" OFF)
if(ENABLE_PYTHON)
  include(python)
  add_subdirectory(lib/python)
endif()

# MATLAB
option(ENABLE_MATLAB "Enable MATLAB library" OFF)
if(ENABLE_MATLAB)
  list(APPEND CPACK_COMPONENTS_ALL matlab)
  include(matlab)
  add_subdirectory(lib/matlab)

  option(ENABLE_MATLAB_TESTS "Enable MATLAB unit tests" OFF)
  if(ENABLE_MATLAB_TESTS)
    find_path(MATLAB_XUNIT_DIR initTestSuite.m
      DOC "Path to MATLAB xUnit framework")
    if(NOT MATLAB_XUNIT_DIR)
      message(SEND_ERROR "MATLAB xUnit framework not found")
    endif()
    add_subdirectory(lib/matlab/test)
  endif()
endif()

### Packaging
set(CPACK_PACKAGE_NAME sapilocal)
set(CPACK_PACKAGE_VERSION "${SAPI_VERSION}")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_IGNORE_GROUPS 1)
include(CPack)
