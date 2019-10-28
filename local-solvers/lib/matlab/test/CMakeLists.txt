set(TESTRESULTS_FILE "${CMAKE_CURRENT_BINARY_DIR}/test-results.xml")
configure_file(sapilocal_tests.m.in sapilocal_tests.m @ONLY)

set(MATLAB_ARGS -nosplash -nodesktop)
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
  list(APPEND MATLAB_ARGS -wait)
endif()
add_custom_target(check-matlab
  COMMAND ${MATLAB_COMMAND} ${MATLAB_ARGS} -r sapilocal_tests
  DEPENDS sapilocal_orangSample sapilocal_orangOptimize sapilocal_orangHeuristic
    "${CMAKE_CURRENT_BINARY_DIR}/sapilocal_tests.m"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Running MATLAB unit tests" VERBATIM)
