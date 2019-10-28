# Find Embedding Library

## Dependencies

* Boost (headers) >= 1.53

### Testing

* Google Test 1.7.0
* Google Mock 1.7.0

## CMake variables

* `BOOST_INCLUDEDIR`: path to Boost headers (so that
  `#include <boost/...>` works).
* `ENABLE_TESTS`: boolean value to enable/disable unit tests.
* `GTEST_INCLUDEDIR`: path to Google Test headers.
* `GTEST_LIBRARYDIR`: path to Google Test library directory.
* `GMOCK_INCLUDEDIR`: path to Google Mock headers.
* `GMOCK_LIBRARYDIR`: path to Google Mock library directory.