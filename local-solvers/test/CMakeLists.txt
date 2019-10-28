add_executable(sapi-local-tests EXCLUDE_FROM_ALL
  ${CMAKE_SOURCE_DIR}/src/orang-solvers.cpp
  ${CMAKE_SOURCE_DIR}/src/orang-heuristic.cpp
  ${CMAKE_SOURCE_DIR}/src/problems.cpp
  test-orang-sample.cpp
  test-orang-optimize.cpp
  test-orang-heuristic.cpp)

add_definitions(-DGTEST_LANG_CXX11=0 -DBOOST_ALL_NO_LIB)
include_directories(SYSTEM ${GTest_INCLUDE_DIR})
target_link_libraries(sapi-local-tests ${GTest_LIBRARY} ${GTest_MAIN})

add_dependencies(sapi-local-tests orang)

if(CMAKE_COMPILER_IS_GNUCXX)
  set_target_properties(sapi-local-tests PROPERTIES
    COMPILE_FLAGS -pthread
    LINK_FLAGS -pthread)
  if(GXX_VERSION VERSION_LESS 4.8)
    add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set_target_properties(sapi-local-tests PROPERTIES
    COMPILE_FLAGS -pthread)
endif()

add_custom_target(check
  sapi-local-tests --gtest_output=xml:test-results.xml
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS sapi-local-tests
  COMMENT "Running core unit tests" VERBATIM)
