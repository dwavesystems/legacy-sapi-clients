# SAPI Local Solvers

## CMake variables

* `ORANG_INCLUDEDIR`: path to Orang headers.
* `BOOST_INCLUDEDIR`: path to Boost headers.
* `ENABLE_TESTS`: boolean value to enable/disable core unit tests.
    * `GTEST_INCLUDEDIR`: path to Google Test headers.
    * `GTEST_LIBRARYDIR`: path to Google Test libraries.
* `ENABLE_PYTHON`: boolean value to enable/disable Python package.
    * `PYTHON_EXECUTABLE`: path to Python executable.
    * `PYTHON_INCLUDE_DIR`: path to Python.h header file.
    * `PYTHON_LIBRARY`: path to Python library.
    * `SWIG_EXECUTABLE`: path to SWIG executable.
* `ENABLE_PYTHON_TESTS`: boolean value to enable/disable Python unit tests.
    * `NOSETESTS_COMMAND`: path to nosetests executable.
* `ENABLE_MATLAB`: boolean value to enable/disable MATLAB package.
    * `MATLAB_COMMAND`: path to MATLAB executable.
* `ENABLE_MATLAB_TESTS`: boolean value to enable/disable MATLAB unit tests.
    * `MATLAB_XUNIT_DIR`: path to MATLAB xUnit directory.
* `ENABLE_SOURCE_PACKAGE`: boolean value to enable/disable source package.
