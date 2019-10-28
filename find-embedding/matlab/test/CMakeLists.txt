find_path(MATLAB_XUNIT_DIR initTestSuite.m
          DOC "Path to MATLAB xUnit framework")

if(NOT MATLAB_XUNIT_DIR)
  message(SEND_ERROR "MATLAB xUnit framework not found")
endif()

set(TESTRESULTS_FILE "${CMAKE_CURRENT_BINARY_DIR}/test-results.xml")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/runFindEmbeddingMexTests.m.in
               ${CMAKE_CURRENT_BINARY_DIR}/runFindEmbeddingMexTests.m)

set(CHECK_MATLAB_COMMAND
    COMMAND ${MATLAB_COMMAND} -nodesktop -nosplash -r runFindEmbeddingMexTests)

if(CMAKE_HOST_WIN32)
  set(CHECK_MATLAB_COMMAND ${CHECK_MATLAB_COMMAND} -wait)
endif()

add_custom_target(check-matlab ${CHECK_MATLAB_COMMAND}
                  COMMENT "Running Find Embedding MATLAB unit tests")

add_dependencies(check-matlab find_embedding_mex)

add_dependencies(check-all check-matlab)

