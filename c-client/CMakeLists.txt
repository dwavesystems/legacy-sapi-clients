cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake-modules)
project(sapi-c-client CXX C)
include(ExternalProject)

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
set(SAPI_CLIENT C)
configure_file("${CMAKE_SOURCE_DIR}/../remote/src/user-agent.cpp.in"
    user-agent.cpp @ONLY ESCAPE_QUOTES)

set(PRODUCT_NAME "D-WAVE C PACK")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(README_LINUX_NSS "
Linux users: if you see an error message like this:

  libssl3.so: cannot open shared object file: No such file or directory

your system is likely missing NSS libraries.  Install the libnss3 (Ubuntu,
Debian) or nss (Fedora, Red Hat) package.")
else()
  set(README_LINUX_NSS)
endif()
configure_file(README.txt.in README.txt @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/licenses/eula.txt.in
    ${CMAKE_CURRENT_BINARY_DIR}/licenses/eula.txt @ONLY)

set(DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/download)

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

configure_file(src/version.c.in version.c @ONLY ESCAPE_QUOTES)

# Boost
find_package(Boost 1.53 REQUIRED COMPONENTS system)
add_definitions(-DBOOST_ALL_NO_LIB)
include_directories(SYSTEM ${Boost_INCLUDE_DIR})

# coin-or
include(coinor)
include_directories(${COINOR_INCLUDE_DIR})

# External: orang
ExternalProject_Add(orang
    GIT_REPOSITORY "http://git.dwavesys.local/scm/~marshall/orang.git"
    GIT_TAG "v1.1"
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    CMAKE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND "")

set(FIND_EMBEDDING_SOURCES
    ${CMAKE_SOURCE_DIR}/../find-embedding/src/find_embedding.cpp)

set(FIX_VARIABLES_SOURCES
    ${CMAKE_SOURCE_DIR}/../fix-variables/src/fix_variables.cpp)

set(SAPI_LOCAL_SOURCES
    ${CMAKE_SOURCE_DIR}/../local-solvers/src/orang-heuristic.cpp
    ${CMAKE_SOURCE_DIR}/../local-solvers/src/orang-solvers.cpp
    ${CMAKE_SOURCE_DIR}/../local-solvers/src/problems.cpp)

set(QSAGE_SOURCES
    ${CMAKE_SOURCE_DIR}/../qsage/src/blackbox.cpp)

set(SAPI_REMOTE_SOURCES
    ${CMAKE_SOURCE_DIR}/../remote/src/answer-service.cpp
    ${CMAKE_SOURCE_DIR}/../remote/src/await.cpp
    ${CMAKE_SOURCE_DIR}/../remote/src/base64.cpp
    ${CMAKE_SOURCE_DIR}/../remote/src/decode-answer.cpp
    ${CMAKE_SOURCE_DIR}/../remote/src/decode-qp.cpp
    ${CMAKE_SOURCE_DIR}/../remote/src/encode-qp.cpp
    ${CMAKE_SOURCE_DIR}/../remote/src/http-service.cpp
    ${CMAKE_SOURCE_DIR}/../remote/src/json.cpp
    ${CMAKE_SOURCE_DIR}/../remote/src/problem-manager.cpp
    ${CMAKE_SOURCE_DIR}/../remote/src/retry-service.cpp
    ${CMAKE_SOURCE_DIR}/../remote/src/sapi-service.cpp
    ${CMAKE_SOURCE_DIR}/../remote/src/threadpool.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/user-agent.cpp)

include_directories(${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../find-embedding/include
    ${CMAKE_SOURCE_DIR}/../fix-variables/include
    ${CMAKE_SOURCE_DIR}/../local-solvers/include
    ${CMAKE_SOURCE_DIR}/../qsage/include
    ${CMAKE_SOURCE_DIR}/../remote/include
    ${CMAKE_CURRENT_BINARY_DIR}/orang-prefix/src/orang/src)


option(ENABLE_LIBRARY "Build client library" ON)
if(ENABLE_LIBRARY)
  # libcurl
  find_package(CURL 7.28 REQUIRED)
  include_directories(SYSTEM ${CURL_INCLUDE_DIRS})

  add_library(dwave_sapi SHARED
      src/dwave_sapi.cpp
      src/embed-problem.cpp
      src/unembed-answer.cpp
      src/fix-variables.cpp
      src/conversions.cpp
      src/internal.cpp
      src/global.cpp
      src/remote.cpp
      src/local.cpp
      src/freefuncs.cpp
      src/defaults.cpp
      src/sapi-impl.cpp
      ${CMAKE_BINARY_DIR}/version.c
      ${FIND_EMBEDDING_SOURCES}
      ${FIX_VARIABLES_SOURCES}
      ${SAPI_LOCAL_SOURCES}
      ${QSAGE_SOURCES}
      ${SAPI_REMOTE_SOURCES})

  target_link_libraries(dwave_sapi
      ${Boost_SYSTEM_LIBRARY}
      ${CURL_LIBRARY}
      ${COINOR_LIBRARIES})

  add_dependencies(dwave_sapi orang)

  get_target_property(DWAVE_SAPI_LINK_FLAGS dwave_sapi LINK_FLAGS)

  if(NOT DWAVE_SAPI_LINK_FLAGS)
    set(DWAVE_SAPI_LINK_FLAGS)
  endif()

  if(CMAKE_HOST_APPLE)
    list(APPEND DWAVE_SAPI_LINK_FLAGS
        "-Wl,-exported_symbols_list,${CMAKE_SOURCE_DIR}/exported-symbols-osx.map")
  elseif(CMAKE_HOST_UNIX)
    list(APPEND DWAVE_SAPI_LINK_FLAGS
        "-Wl,--version-script=${CMAKE_SOURCE_DIR}/version.map")
  endif()

  set_target_properties(dwave_sapi PROPERTIES
      COMPILE_DEFINITIONS DWAVE_SAPI_BUILD
      LINK_FLAGS "${DWAVE_SAPI_LINK_FLAGS}")

  if(CMAKE_BUILD_TYPE MATCHES "Release|MinSizeRel")
    set(STRIP_ARGS)
    if(CMAKE_HOST_APPLE)
      set(STRIP_ARGS "-x")
    endif()

    if(CMAKE_STRIP)
      get_target_property(DWAVE_SAPI_OUTPUT_NAME dwave_sapi LOCATION)
      add_custom_command(TARGET dwave_sapi POST_BUILD
          COMMAND ${CMAKE_STRIP} ${STRIP_ARGS} ${DWAVE_SAPI_OUTPUT_NAME}
          COMMENT "Stripping dwave_sapi c library"
          VERBATIM)
    endif()
  endif()

  install(FILES
      ${CMAKE_BINARY_DIR}/README.txt
      ${CMAKE_CURRENT_SOURCE_DIR}/include/dwave_sapi.h
      DESTINATION .)

  install(TARGETS dwave_sapi DESTINATION .)

  install(DIRECTORY examples DESTINATION .)

  install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/licenses/eula.txt
      ../licenses.txt
      DESTINATION licenses)

  # Packaging

  if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(BITS 64)
  else()
    set(BITS 32)
  endif()

  if(CMAKE_HOST_WIN32)
    set(CPACK_SYSTEM_NAME "win${BITS}")
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CPACK_SYSTEM_NAME "linux${BITS}")
  elseif(CMAKE_HOST_APPLE)
    set(CPACK_SYSTEM_NAME "osx") # No more 32-bit OSX
  else()
    message(SEND_ERROR "Unknown system type")
  endif()

  set(CPACK_PACKAGE_NAME sapi-c-client)
  set(CPACK_PACKAGE_VERSION "${SAPI_VERSION}")
  include(CPack)
endif()

option(ENABLE_TESTS "Enable sapi c client tests" OFF)
if(ENABLE_TESTS)
  # Google Mock
  set(GMOCK_INCLUDEDIR "" CACHE PATH "Google Mock header file directory")
  set(GMOCK_LIBRARYDIR "" CACHE PATH "Google Mock library directory")
  find_path(GMock_INCLUDE_DIRS gmock/gmock.h HINTS ${GMOCK_INCLUDEDIR})
  find_library(GMock_LIBRARY gmock HINTS ${GMOCK_LIBRARYDIR})

  # Google Test
  set(GTEST_INCLUDEDIR "" CACHE PATH "GTest header file directory")
  set(GTEST_LIBRARYDIR "" CACHE PATH "GTest library directory")
  set(GTEST_LIBRARYDIRS ${GTEST_LIBRARYDIR})
  if(GMOCK_LIBRARYDIR)
    list(APPEND GTEST_LIBRARYDIRS ${GMOCK_LIBRARYDIR}/gtest)
  endif()

  find_path(GTest_INCLUDE_DIRS gtest/gtest.h PATHS ${GTEST_INCLUDEDIR})
  find_library(GTest_LIBRARY gtest HINTS ${GTEST_LIBRARYDIRS})

  add_subdirectory(test)
endif()
