cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake-modules)
project(sapi-remote CXX)

file(STRINGS ../version.txt SAPI_VERSION LIMIT_COUNT 1)

if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
  set(BITS 64)
else()
  set(BITS 32)
endif()
if(CMAKE_HOST_WIN32)
  set(SAPI_ARCH "win${BITS}")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(SAPI_ARCH "linux${BITS}")
elseif(CMAKE_HOST_APPLE)
  set(SAPI_ARCH "osx") # No more 32-bit OSX
else()
  message(SEND_ERROR "Unknown system type")
endif()

# Components
option(ENABLE_TESTS "Enable core C++ unit tests" OFF)
option(ENABLE_PYTHON "Enable Python library" OFF)
option(ENABLE_PYTHON_TESTS "Enable Python unit tests" OFF)
option(ENABLE_MATLAB "Enable MATLAB library" OFF)
option(ENABLE_MATLAB_TESTS "Enable MATLAB library unit tests" OFF)
option(ENABLE_EXTRAS "Enable extra targets" OFF)

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

# Boost
find_package(Boost 1.53 REQUIRED COMPONENTS system)
add_definitions(-DBOOST_ALL_NO_LIB)
include_directories(SYSTEM ${Boost_INCLUDE_DIR})

# libcurl - not needed for tests
if(ENABLE_PYTHON OR ENABLE_MATLAB OR ENABLE_EXTRAS)
  find_package(CURL 7.28 REQUIRED)
  include_directories(SYSTEM ${CURL_INCLUDE_DIRS})
endif()

include_directories(include)
set(SAPIREMOTE_SOURCES
  ${CMAKE_SOURCE_DIR}/src/threadpool.cpp
  ${CMAKE_SOURCE_DIR}/src/answer-service.cpp
  ${CMAKE_SOURCE_DIR}/src/http-service.cpp
  ${CMAKE_SOURCE_DIR}/src/json.cpp
  ${CMAKE_SOURCE_DIR}/src/base64.cpp
  ${CMAKE_SOURCE_DIR}/src/problem-manager.cpp
  ${CMAKE_SOURCE_DIR}/src/sapi-service.cpp
  ${CMAKE_SOURCE_DIR}/src/retry-service.cpp
  ${CMAKE_SOURCE_DIR}/src/await.cpp
  ${CMAKE_SOURCE_DIR}/src/decode-answer.cpp
  ${CMAKE_SOURCE_DIR}/src/decode-qp.cpp
  ${CMAKE_SOURCE_DIR}/src/encode-qp.cpp
  ${CMAKE_SOURCE_DIR}/src/enum-strings.cpp)


if(ENABLE_TESTS)
  # Google Mock
  set(GMOCK_INCLUDEDIR "" CACHE PATH "Google Mock header file directory")
  set(GMOCK_LIBRARYDIR "" CACHE PATH "Google Mock library directory")
  find_path(GMock_INCLUDE_DIR gmock/gmock.h HINTS ${GMOCK_INCLUDEDIR}
    NO_DEFAULT_PATH)
  find_path(GMock_INCLUDE_DIR gmock/gmock.h HINTS ${GMOCK_INCLUDEDIR})
  find_library(GMock_LIBRARY gmock HINTS ${GMOCK_LIBRARYDIR})
  find_library(GMock_MAIN gmock_main HINTS ${GMOCK_LIBRARYDIR})

  # Google Test
  set(GTEST_INCLUDEDIR "" CACHE PATH "GTest header file directory")
  set(GTEST_LIBRARYDIR "" CACHE PATH "GTest library directory")
  set(GTEST_LIBRARYDIRS ${GTEST_LIBRARYDIR})
  if (GMOCK_LIBRARYDIR)
    list(APPEND GTEST_LIBRARYDIRS ${GMOCK_LIBRARYDIR}/gtest)
  endif()

  find_path(GTest_INCLUDE_DIR gtest/gtest.h PATHS ${GTEST_INCLUDEDIR}
    NO_DEFAULT_PATH)
  find_path(GTest_INCLUDE_DIR gtest/gtest.h PATHS ${GTEST_INCLUDEDIR})
  find_library(GTest_LIBRARY gtest HINTS ${GTEST_LIBRARYDIRS})

  add_subdirectory(test)
endif()

if(ENABLE_PYTHON OR ENABLE_PYTHON_TESTS)
  include(python)
endif()

if(ENABLE_PYTHON)
  add_subdirectory(lib/python)
endif()

if(ENABLE_PYTHON_TESTS)
  find_program(NOSETESTS_COMMAND nosetests)
  add_subdirectory(lib/python/test)
endif()

if(ENABLE_MATLAB OR ENABLE_MATLAB_TESTS)
  include(matlab)
endif()

if(ENABLE_MATLAB)
  list(APPEND CPACK_COMPONENTS_ALL matlab)
  add_subdirectory("lib/matlab")
endif()

if(ENABLE_MATLAB_TESTS)
  find_path(MATLAB_XUNIT_DIR initTestSuite.m
    DOC "Path to MATLAB xUnit framework")
  if(NOT MATLAB_XUNIT_DIR)
    message(SEND_ERROR "MATLAB xUnit framework not found")
  endif()
  add_subdirectory("lib/matlab/test")
endif()

if(ENABLE_EXTRAS)
  add_subdirectory(extras/http-service-grind)
  add_subdirectory(extras/json-parse-speed)
  add_subdirectory(extras/qp-encode-speed)
  add_subdirectory(extras/spam)
  add_subdirectory(extras/show-status)
endif()

# Packaging
set(CPACK_PACKAGE_NAME sapiremote)
set(CPACK_PACKAGE_VERSION "${SAPI_VERSION}")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_IGNORE_GROUPS 1)
include(CPack)
