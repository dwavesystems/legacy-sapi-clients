find_path(MATLAB_XUNIT_DIR initTestSuite.m
          DOC "Path to MATLAB xUnit framework")

if(NOT MATLAB_XUNIT_DIR)
  message(SEND_ERROR "MATLAB xUnit framework not found")
endif()

set(TESTRESULTS_FILE "${CMAKE_CURRENT_BINARY_DIR}/test-results.xml")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/runSolveQSageMexTests.m.in
               ${CMAKE_CURRENT_BINARY_DIR}/runSolveQSageMexTests.m)

set(CHECK_MATLAB_COMMAND
    COMMAND ${MATLAB_COMMAND} -nodesktop -nosplash -r runSolveQSageMexTests)

if(CMAKE_HOST_WIN32)
  set(CHECK_MATLAB_COMMAND ${CHECK_MATLAB_COMMAND} -wait)
endif()

add_custom_target(check-matlab ${CHECK_MATLAB_COMMAND}
                  COMMENT "Running QSage MATLAB unit tests")

add_dependencies(check-matlab solve_qsage_mex)

add_dependencies(check-all check-matlab)

