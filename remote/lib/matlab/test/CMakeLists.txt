include_directories(${CMAKE_SOURCE_DIR}/lib/test)

add_mex(jsontest EXCLUDE_FROM_ALL
  ${CMAKE_SOURCE_DIR}/src/json.cpp
  ../json-to-matlab.cpp
  jsontest.cpp)

add_mex(sapiremote_mex_test EXCLUDE_FROM_ALL
  ${CMAKE_SOURCE_DIR}/src/json.cpp
  ${CMAKE_SOURCE_DIR}/src/await.cpp
  ${CMAKE_SOURCE_DIR}/src/decode-answer.cpp
  ${CMAKE_SOURCE_DIR}/src/decode-qp.cpp
  ${CMAKE_SOURCE_DIR}/src/encode-qp.cpp
  ${CMAKE_SOURCE_DIR}/src/enum-strings.cpp
  ${CMAKE_SOURCE_DIR}/src/base64.cpp
  ${CMAKE_SOURCE_DIR}/lib/test/testimpl.cpp
  services-test.cpp
  ../sapiremote.cpp
  ../answers.cpp
  ../connections.cpp
  ../json-to-matlab.cpp
  ../solvers.cpp)
target_link_libraries(sapiremote_mex_test ${Boost_SYSTEM_LIBRARY})
set_target_properties(sapiremote_mex_test PROPERTIES
  OUTPUT_NAME sapiremote_mex)

set(TESTRESULTS_FILE "${CMAKE_CURRENT_BINARY_DIR}/test-results.xml")
configure_file(sapiremote_tests.m.in sapiremote_tests.m @ONLY)

set(MATLAB_ARGS -nosplash -nodesktop)
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
  list(APPEND MATLAB_ARGS -wait)
endif()
add_custom_target(check-matlab
  COMMAND ${MATLAB_COMMAND} ${MATLAB_ARGS} -r sapiremote_tests
  DEPENDS jsontest sapiremote_mex_test
    "${CMAKE_CURRENT_BINARY_DIR}/sapiremote_tests.m"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Running MATLAB unit tests" VERBATIM)
