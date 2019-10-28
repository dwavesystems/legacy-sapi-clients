cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake-modules)
project(sapi-python-client CXX)

file(STRINGS ../version.txt SAPI_VERSION LIMIT_COUNT 1)

find_package(PythonInterp 3.6 REQUIRED)
set(PYTHON_VERSION "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")

function(eggname OUTVAR PKGNAME PKGTYPE)
  if(PKGTYPE STREQUAL "PURE")
    set(_PLAT None)
    set(_PARAM False)
  elseif(PKGTYPE STREQUAL "EXT")
    set(_PLAT "get_build_platform()")
    set(_PARAM False)
  elseif(PKGTYPE STREQUAL "PARAM")
    set(_PLAT "get_build_platform()")
    set(_PARAM True)
  else()
    message(SEND_ERROR "egg type must be PURE or EXT")
  endif()
  string(CONFIGURE
    "from pkg_resources import get_build_platform, Distribution
eggname = Distribution(None, None, '@PKGNAME@', '@SAPI_VERSION@',
                       '@PYTHON_VERSION@', @_PLAT@).egg_name()
if @_PARAM@:
    eggname = eggname.replace(get_build_platform().rsplit('-', 1)[-1],
                              '{plat}')
print eggname"
    PYCMD @ONLY)
  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "${PYCMD}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    RESULT_VARIABLE RESULT
    ERROR_VARIABLE EGGNAME_ERROR
    OUTPUT_VARIABLE EGGNAME
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT RESULT EQUAL 0)
    message(SEND_ERROR "Failed to get egg name: ${RESULT}: ${EGGNAME_ERROR}")
  endif()
  set(${OUTVAR} "${EGGNAME}.egg" PARENT_SCOPE)
endfunction()

# Copy source directory to build directory since setup.py only works in-tree
set(DWAVE_SAPI2_FILES
  dwave_sapi2/__init__.py
  dwave_sapi2/core.py
  dwave_sapi2/embedding.py
  dwave_sapi2/fix_variables.py
  dwave_sapi2/local.py
  dwave_sapi2/qsage.py
  dwave_sapi2/remote.py
  dwave_sapi2/util.py)
file(MAKE_DIRECTORY dwave_sapi2)
foreach(F IN LISTS DWAVE_SAPI2_FILES)
  configure_file("${F}" "${F}" COPYONLY)
endforeach()
configure_file(setup.py . COPYONLY)
configure_file(../version.txt . COPYONLY)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(README_LINUX_NSS "
Linux users: if you see an error message like this:

  libssl3.so: cannot open shared object file: No such file or directory

your system is likely missing NSS libraries.  Install the libnss3 (Ubuntu,
Debian) or nss (Fedora, Red Hat) package.")
else()
  set(README_LINUX_NSS)
endif()
configure_file(README.txt.in README.txt @ONLY ESCAPE_QUOTES)

# Build egg file
eggname(DWAVE_SAPI2_EGG dwave_sapi2 PURE)
add_custom_command(OUTPUT "${DWAVE_SAPI2_EGG}"
  COMMENT "Building dwave_sapi2 egg"
  COMMAND "${PYTHON_EXECUTABLE}" setup.py
    --quiet bdist_egg --dist-dir ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS setup.py ${DWAVE_SAPI2_FILES} VERBATIM)
add_custom_target(python-egg ALL DEPENDS "${DWAVE_SAPI2_EGG}")


###########################################################################
#
# Build dependencies
#
find_package(PythonLibs REQUIRED)
find_package(SWIG REQUIRED)
find_package(Boost 1.53 REQUIRED COMPONENTS system)
find_package(CURL 7.28 REQUIRED)
include(coinor)

include(ExternalProject)
set_directory_properties(PROPERTIES EP_BASE "${CMAKE_BINARY_DIR}/external")
set(EXT_BUILDDIR "${CMAKE_BINARY_DIR}/external/Build")

# ExternalProject_Add doesn't escape backslashes properly, so convert all
# paths to CMake form (i.e. / instead of \)
file(TO_CMAKE_PATH "${PYTHON_EXECUTABLE}" PYTHON_EXECUTABLE)
file(TO_CMAKE_PATH "${PYTHON_INCLUDE_DIRS}" PYTHON_INCLUDE_DIRS)
file(TO_CMAKE_PATH "${PYTHON_LIBRARIES}" PYTHON_LIBRARIES)
file(TO_CMAKE_PATH "${SWIG_EXECUTABLE}" SWIG_EXECUTABLE)
file(TO_CMAKE_PATH "${Boost_INCLUDE_DIR}" Boost_INCLUDE_DIR)
file(TO_CMAKE_PATH "${Boost_SYSTEM_LIBRARY}" Boost_SYSTEM_LIBRARY)
file(TO_CMAKE_PATH "${CURL_INCLUDE_DIR}" CURL_INCLUDE_DIR)
file(TO_CMAKE_PATH "${CURL_LIBRARY}" CURL_LIBRARY)
file(TO_CMAKE_PATH "${COINOR_INCLUDE_DIR}" COINOR_INCLUDE_DIR)
file(TO_CMAKE_PATH "${COINOR_CLP_LIBRARY}" COINOR_CLP_LIBRARY)
file(TO_CMAKE_PATH "${COINOR_COINUTILS_LIBRARY}" COINOR_COINUTILS_LIBRARY)

ExternalProject_Add(find-embedding
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/../find-embedding
  BUILD_COMMAND ${CMAKE_COMMAND}
    --build "${EXT_BUILDDIR}/find-embedding"
    --target python-egg
  INSTALL_COMMAND ""
  CMAKE_CACHE_ARGS
    "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
    -DENABLE_PYTHON:BOOL=ON
    "-DBOOST_INCLUDEDIR:PATH=${Boost_INCLUDE_DIR}"
    -DBoost_NO_SYSTEM_PATHS:BOOL=ON
    "-DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}"
    "-DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIRS}"
    "-DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARIES}"
    "-DSWIG_EXECUTABLE:FILEPATH=${SWIG_EXECUTABLE}")

ExternalProject_Add(fix-variables
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/../fix-variables
  BUILD_COMMAND ${CMAKE_COMMAND}
    --build "${EXT_BUILDDIR}/fix-variables"
    --target python-egg
  INSTALL_COMMAND ""
  CMAKE_CACHE_ARGS
    "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
    -DENABLE_PYTHON:BOOL=ON
    "-DBOOST_INCLUDEDIR:PATH=${Boost_INCLUDE_DIR}"
    -DBoost_NO_SYSTEM_PATHS:BOOL=ON
    "-DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}"
    "-DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIRS}"
    "-DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARIES}"
    "-DSWIG_EXECUTABLE:FILEPATH=${SWIG_EXECUTABLE}")

ExternalProject_Add(local-solvers
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/../local-solvers
  BUILD_COMMAND ${CMAKE_COMMAND}
    --build "${EXT_BUILDDIR}/local-solvers"
    --target python-egg
  INSTALL_COMMAND ""
  CMAKE_CACHE_ARGS
    "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
    -DENABLE_PYTHON:BOOL=ON
    "-DBOOST_INCLUDEDIR:PATH=${Boost_INCLUDE_DIR}"
    -DBoost_NO_SYSTEM_PATHS:BOOL=ON
    "-DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}"
    "-DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIRS}"
    "-DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARIES}"
    "-DSWIG_EXECUTABLE:FILEPATH=${SWIG_EXECUTABLE}")

ExternalProject_Add(qsage
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/../qsage
  BUILD_COMMAND ${CMAKE_COMMAND}
    --build "${EXT_BUILDDIR}/qsage"
    --target python-egg
  INSTALL_COMMAND ""
  CMAKE_CACHE_ARGS
    "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
    -DENABLE_PYTHON:BOOL=ON
    "-DCOINOR_INCLUDE_DIR:PATH=${COINOR_INCLUDE_DIR}"
    "-DCOINOR_CLP_LIBRARY:PATH=${COINOR_CLP_LIBRARY}"
    "-DCOINOR_COINUTILS_LIBRARY:PATH=${COINOR_COINUTILS_LIBRARY}"
    "-DBOOST_INCLUDEDIR:PATH=${Boost_INCLUDE_DIR}"
    -DBoost_NO_SYSTEM_PATHS:BOOL=ON
    "-DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}"
    "-DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIRS}"
    "-DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARIES}"
    "-DSWIG_EXECUTABLE:FILEPATH=${SWIG_EXECUTABLE}")

set(REMOTE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE STRING
  "Remote library C++ compiler flags")
set(REMOTE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS} CACHE STRING
  "Remote library linker flags")
ExternalProject_Add(remote
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/../remote
  BUILD_COMMAND ${CMAKE_COMMAND}
    --build "${EXT_BUILDDIR}/remote"
    --target python-egg
  INSTALL_COMMAND ""
  CMAKE_CACHE_ARGS
    "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
    "-DCMAKE_CXX_FLAGS:STRING=${REMOTE_CXX_FLAGS}"
    "-DCMAKE_MODULE_LINKER_FLAGS:STRING=${REMOTE_MODULE_LINKER_FLAGS}"
    -DENABLE_PYTHON:BOOL=ON
    "-DBOOST_INCLUDEDIR:PATH=${Boost_INCLUDE_DIR}"
    "-DBOOST_LIBRARYDIR:PATH=${Boost_LIBRARY_DIR}"
    "-DBoost_SYSTEM_LIBRARY:FILEPATH=${Boost_SYSTEM_LIBRARY}"
    "-DBoost_USE_STATIC_LIBS:BOOL=${Boost_USE_STATIC_LIBS}"
    -DBoost_NO_SYSTEM_PATHS:BOOL=ON
    "-DCURL_INCLUDE_DIR:PATH=${CURL_INCLUDE_DIR}"
    "-DCURL_LIBRARY:FILEPATH=${CURL_LIBRARY}"
    "-DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}"
    "-DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIRS}"
    "-DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARIES}"
    "-DSWIG_EXECUTABLE:FILEPATH=${SWIG_EXECUTABLE}")

###########################################################################
#
# Installation
#

set(DESTDIR ".")
install(FILES ${CMAKE_BINARY_DIR}/README.txt DESTINATION "${DESTDIR}")
install(FILES licenses/eula.txt ../licenses.txt
  DESTINATION "${DESTDIR}/licenses")
install(DIRECTORY examples DESTINATION "${DESTDIR}")

eggname(FIXVARS_EGG fix-variables EXT)
eggname(FINDEMB_EGG find-embedding EXT)
eggname(LOCAL_EGG sapilocal EXT)
eggname(QSAGE_EGG qsage EXT)
eggname(REMOTE_EGG sapiremote EXT)

install(FILES "${CMAKE_BINARY_DIR}/${DWAVE_SAPI2_EGG}"
  "${EXT_BUILDDIR}/fix-variables/python/${FIXVARS_EGG}"
  "${EXT_BUILDDIR}/find-embedding/python/${FINDEMB_EGG}"
  "${EXT_BUILDDIR}/local-solvers/lib/python/${LOCAL_EGG}"
  "${EXT_BUILDDIR}/qsage/python/${QSAGE_EGG}"
  "${EXT_BUILDDIR}/remote/lib/python/${REMOTE_EGG}"
  DESTINATION "${DESTDIR}/eggs")

# Handle x86_64 vs intel on OSX
if(CMAKE_HOST_APPLE)
  execute_process(COMMAND ${PYTHON_EXECUTABLE}
  -c "from pkg_resources import get_build_platform
plat = get_build_platform().rpartition('-')[-1]
others = {'x86_64', 'intel'}
others.remove(plat)
print '%s;%s' % (plat, others.pop())"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  RESULT_VARIABLE RESULT
  ERROR_VARIABLE PLATS_ERROR
  OUTPUT_VARIABLE PLATS
  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT RESULT EQUAL 0)
    message(SEND_ERROR
      "Failed to get platform names: ${RESULT}: ${PLATS_ERROR}")
  endif()

  foreach(EGG IN ITEMS
      "${FIXVARS_EGG}" "${FINDEMB_EGG}" "${LOCAL_EGG}" "${QSAGE_EGG}"
      "${REMOTE_EGG}")
    string(REPLACE ${PLATS} OTHER_EGG "${EGG}")
    # note that \${CMAKE_INSTALL_PREFIX} is escaped to delay substitution
    # until install time (so that CPack's altered value is noticed)
    install(CODE "execute_process(COMMAND ln -sf \"${EGG}\" \"${OTHER_EGG}\"
      WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/eggs\")")
  endforeach()
endif()

eggname(FIXVARS_EGG_P fix-variables PARAM)
eggname(FINDEMB_EGG_P find-embedding PARAM)
eggname(LOCAL_EGG_P sapilocal PARAM)
eggname(QSAGE_EGG_P qsage PARAM)
eggname(REMOTE_EGG_P sapiremote PARAM)
configure_file(install.py.in install.py ESCAPE_QUOTES @ONLY)
install(FILES "${CMAKE_BINARY_DIR}/install.py" DESTINATION "${DESTDIR}")

###########################################################################
#
# Packaging
#

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

set(CPACK_PACKAGE_NAME "sapi-python-client")
set(CPACK_PACKAGE_VERSION "${SAPI_VERSION}")

include(CPack)
