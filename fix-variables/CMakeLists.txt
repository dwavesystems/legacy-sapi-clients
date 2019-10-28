cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake-modules)
project(fix-variables CXX C)

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
endif()

# Boost
find_package(Boost 1.53 REQUIRED)

set(FIXVARIABLES_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include)

add_custom_target(check-all)

option(ENABLE_MATLAB "Build fix variables MATLAB wrapper" OFF)
option(ENABLE_PYTHON "Build fix variables Python wrapper" OFF)
option(ENABLE_TESTS "Enable fix variables C++ unit tests" OFF)

if(ENABLE_MATLAB)
  include(matlab)
  add_subdirectory(matlab)
endif()

if(ENABLE_PYTHON)
  include(python)
  add_subdirectory(python)
endif()

if(ENABLE_TESTS)
  # Google Mock
  set(GMOCK_INCLUDEDIR "" CACHE PATH "Google Mock header file directory")
  set(GMOCK_LIBRARYDIR "" CACHE PATH "Google Mock library directory")
  find_path(GMock_INCLUDE_DIRS gmock/gmock.h HINTS ${GMOCK_INCLUDEDIR})
  find_library(GMock_LIBRARY gmock HINTS ${GMOCK_LIBRARYDIR})
  find_library(GMock_MAIN gmock_main HINTS ${GMOCK_LIBRARYDIR})

  # Google Test
  set(GTEST_INCLUDEDIR "" CACHE PATH "GTest header file directory")
  set(GTEST_LIBRARYDIR "" CACHE PATH "GTest library directory")
  set(GTEST_LIBRARYDIRS ${GTEST_LIBRARYDIR})
  if (GMOCK_LIBRARYDIR)
    list(APPEND GTEST_LIBRARYDIRS ${GMOCK_LIBRARYDIR}/gtest)
  endif()

  find_path(GTest_INCLUDE_DIRS gtest/gtest.h PATHS ${GTEST_INCLUDEDIR})
  find_library(GTest_LIBRARY gtest HINTS ${GTEST_LIBRARYDIRS})
  find_library(GTest_MAIN gtest_main HINTS ${GTEST_LIBRARYDIRS})

  add_subdirectory(test)
endif()

# Packaging
if(CMAKE_HOST_WIN32)
  string(REPLACE "\\" "\\\\" CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
endif()
set(CPACK_PACKAGE_NAME fix-variables)
set(CPACK_PACKAGE_VERSION "${SAPI_VERSION}")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_IGNORE_GROUPS 1)
include(CPack)
