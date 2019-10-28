# SAPI Remote Solver Library

## Dependencies

### Core

* libcurl >= 7.21.7
* Boost (headers, system library) >= 1.53
* Google Test >= 1.7.0 (unit tests)
* Google Mock >= 1.7.0 (unit tests)

### Python package

* Python 2.6 or 2.7
* Python development headers and library
* SWIG >= 1.3
* setuptools (or distribute) >= 0.6
* nosetests >= 0.11 (unit tests)

## CMake variables

* `BOOST_INCLUDEDIR`: path to Boost headers (so that
  `#include <boost/...>` works).
* `BOOST_LIBRARYDIR`: path to Boost library directory.
* `Boost_NO_SYSTEM_PATHS`: boolean value to prevent finding system Boost
  installations (i.e. force the use of `BOOST_INCLUDEDIR` and
  `BOOST_LIBRARYDIR`).
* `CURL_INCLUDE_DIR`: path to libcurl headers (so that
  `#include <curl/curl.h>` works).
* `CURL_LIBRARY`: path to libcurl library.
* `ENABLE_TESTS`: boolean value to enable/disable core unit tests.
    * `GTEST_INCLUDEDIR`: path to Google Test headers.
    * `GTEST_LIBRARYDIR`: path to Google Test library directory.
    * `GMOCK_INCLUDEDIR`: path to Google Mock headers.
    * `GMOCK_LIBRARYDIR`: path to Google Mock library directory.
* `ENABLE_PYTHON`: boolean value to enable/disable Python package.
    * `PYTHON_EXECUTABLE`: path to Python executable.
    * `PYTHON_INCLUDE_DIR`: path to Python.h header file.
    * `PYTHON_LIBRARY`: path to Python library.
    * `SWIG_EXECUTABLE`: path to SWIG executable.
* `ENABLE_PYTHON_TESTS`: boolean value to enable/disable Python unit tests.
    * `NOSETESTS_COMMAND`: path to nosetests executable.

