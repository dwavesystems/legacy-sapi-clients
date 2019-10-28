find_path(MATLAB_XUNIT_DIR initTestSuite.m
          DOC "Path to MATLAB xUnit framework")

if(NOT MATLAB_XUNIT_DIR)
  message(SEND_ERROR "MATLAB xUnit framework not found")
endif()

set(TESTRESULTS_FILE "${CMAKE_CURRENT_BINARY_DIR}/test-results.xml")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/runMATLABClientTests.m.in
               ${CMAKE_CURRENT_BINARY_DIR}/runMATLABClientTests.m)

set(CHECK_MATLAB_COMMAND
    COMMAND ${MATLAB_COMMAND} -nodesktop -nosplash -r runMATLABClientTests)

if(CMAKE_HOST_WIN32)
  set(CHECK_MATLAB_COMMAND ${CHECK_MATLAB_COMMAND} -wait)
endif()

add_custom_target(check ${CHECK_MATLAB_COMMAND}
                  COMMENT "Running MATLAB Client unit tests")
