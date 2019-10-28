cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake-modules)

file(STRINGS ../version.txt SAPI_VERSION LIMIT_COUNT 1)

include(matlab)

option(ENABLE_TESTS "Enable MATLAB Client tests" OFF)

if(NOT ENABLE_TESTS)

  include(ExternalProject)

  # Boost
  find_package(Boost 1.53 REQUIRED COMPONENTS system)

  # libcurl
  find_package(CURL 7.28 REQUIRED)

  # coin-or
  include(coinor)


  set(PRODUCT_NAME "D-WAVE MATLAB PACK")

  configure_file(${CMAKE_SOURCE_DIR}/licenses/eula.txt.in
                 ${CMAKE_CURRENT_BINARY_DIR}/licenses/eula.txt @ONLY)

  set(DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/download)

  ExternalProject_Add(find-embedding
                      SOURCE_DIR ${CMAKE_SOURCE_DIR}/../find-embedding
                      CMAKE_CACHE_ARGS -DCMAKE_BUILD_TYPE:STRING=Release
                                       -DENABLE_MATLAB:BOOL=ON
                                       "-DMATLAB_COMMAND:STRING=${MATLAB_COMMAND}"
                                       "-DBoost_INCLUDE_DIR:STRING=${Boost_INCLUDE_DIR}"
                      INSTALL_COMMAND ""
                      BUILD_COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_CURRENT_BINARY_DIR}/find-embedding-prefix/src/find-embedding-build)

  ExternalProject_Add(fix-variables
                      SOURCE_DIR ${CMAKE_SOURCE_DIR}/../fix-variables
                      CMAKE_CACHE_ARGS -DCMAKE_BUILD_TYPE:STRING=Release
                                       -DENABLE_MATLAB:BOOL=ON
                                       "-DMATLAB_COMMAND:STRING=${MATLAB_COMMAND}"
                                       "-DBoost_INCLUDE_DIR:STRING=${Boost_INCLUDE_DIR}"
                      INSTALL_COMMAND ""
                      BUILD_COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_CURRENT_BINARY_DIR}/fix-variables-prefix/src/fix-variables-build)

  ExternalProject_Add(local-solvers
                      SOURCE_DIR ${CMAKE_SOURCE_DIR}/../local-solvers
                      CMAKE_CACHE_ARGS -DCMAKE_BUILD_TYPE:STRING=Release
                                      -DENABLE_MATLAB:BOOL=ON
                                      "-DMATLAB_COMMAND:STRING=${MATLAB_COMMAND}"
                                      "-DBoost_INCLUDE_DIR:STRING=${Boost_INCLUDE_DIR}"
                      INSTALL_COMMAND ""
                      BUILD_COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_CURRENT_BINARY_DIR}/local-solvers-prefix/src/local-solvers-build)

  ExternalProject_Add(qsage
                      SOURCE_DIR ${CMAKE_SOURCE_DIR}/../qsage
                      CMAKE_CACHE_ARGS -DCMAKE_BUILD_TYPE:STRING=Release
                                       -DENABLE_MATLAB:BOOL=ON
                                       "-DMATLAB_COMMAND:STRING=${MATLAB_COMMAND}"
                                       "-DBoost_INCLUDE_DIR:STRING=${Boost_INCLUDE_DIR}"
                                       "-DCOINOR_INCLUDE_DIR:STRING=${COINOR_INCLUDE_DIR}"
                                       "-DCOINOR_CLP_LIBRARY:STRING=${COINOR_CLP_LIBRARY}"
                                       "-DCOINOR_COINUTILS_LIBRARY:STRING=${COINOR_COINUTILS_LIBRARY}"
                      INSTALL_COMMAND ""
                      BUILD_COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_CURRENT_BINARY_DIR}/qsage-prefix/src/qsage-build)

  ExternalProject_Add(remote
                      SOURCE_DIR ${CMAKE_SOURCE_DIR}/../remote
                      CMAKE_CACHE_ARGS -DCMAKE_BUILD_TYPE:STRING=Release
                                       -DENABLE_MATLAB:BOOL=ON
                                       "-DMATLAB_COMMAND:STRING=${MATLAB_COMMAND}"
                                       "-DBOOST_INCLUDEDIR:STRING=${Boost_INCLUDE_DIR}"
                                       "-DBOOST_LIBRARYDIR:PATH=${Boost_LIBRARY_DIR}"
                                       "-DBoost_SYSTEM_LIBRARY:FILEPATH=${Boost_SYSTEM_LIBRARY}"
                                       "-DBoost_USE_STATIC_LIBS:BOOL=${Boost_USE_STATIC_LIBS}"
                                       "-DCURL_INCLUDE_DIR:STRING=${CURL_INCLUDE_DIR}"
                                       "-DCURL_LIBRARY:STRING=${CURL_LIBRARY}"
                                       "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}"
                                       "-DCMAKE_MODULE_LINKER_FLAGS:STRING=${CMAKE_MODULE_LINKER_FLAGS}"
                      INSTALL_COMMAND ""
                      BUILD_COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_CURRENT_BINARY_DIR}/remote-prefix/src/remote-build)

  install(FILES
          checkSolverParams.m
          sapi_solveParams.m
          ${CMAKE_CURRENT_BINARY_DIR}/local-solvers-prefix/src/local-solvers-build/lib/matlab/sapilocal_orangSample.${MATLAB_MEXEXT}
          ${CMAKE_SOURCE_DIR}/../local-solvers/lib/matlab/sapilocal_orangSample.m
          ${CMAKE_CURRENT_BINARY_DIR}/local-solvers-prefix/src/local-solvers-build/lib/matlab/sapilocal_orangOptimize.${MATLAB_MEXEXT}
          ${CMAKE_SOURCE_DIR}/../local-solvers/lib/matlab/sapilocal_orangOptimize.m
          ${CMAKE_CURRENT_BINARY_DIR}/local-solvers-prefix/src/local-solvers-build/lib/matlab/sapilocal_orangHeuristic.${MATLAB_MEXEXT}
          ${CMAKE_SOURCE_DIR}/../local-solvers/lib/matlab/sapilocal_orangHeuristic.m
          ${CMAKE_CURRENT_BINARY_DIR}/remote-prefix/src/remote-build/lib/matlab/sapiremote_mex.${MATLAB_MEXEXT}
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_addproblem.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_answer.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_awaitcompletion.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_awaitsubmission.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_cancel.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_connection.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_decodeqp.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_done.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_encodeqp.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_problemid.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_retry.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_solve.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_solvers.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_status.m
          ${CMAKE_SOURCE_DIR}/../remote/lib/matlab/sapiremote_submit.m
          ${CMAKE_CURRENT_BINARY_DIR}/fix-variables-prefix/src/fix-variables-build/matlab/fix_variables_mex.${MATLAB_MEXEXT}
          ${CMAKE_SOURCE_DIR}/../fix-variables/matlab/fix_variables_mex.m
          ${CMAKE_CURRENT_BINARY_DIR}/find-embedding-prefix/src/find-embedding-build/matlab/find_embedding_mex.${MATLAB_MEXEXT}
          ${CMAKE_SOURCE_DIR}/../find-embedding/matlab/find_embedding_mex.m
          ${CMAKE_CURRENT_BINARY_DIR}/qsage-prefix/src/qsage-build/matlab/solve_qsage_mex.${MATLAB_MEXEXT}
          ${CMAKE_SOURCE_DIR}/../qsage/matlab/solve_qsage_mex.m
          DESTINATION private)

  install(DIRECTORY examples DESTINATION .)

  install(FILES
          ${CMAKE_CURRENT_BINARY_DIR}/licenses/eula.txt
          ../licenses.txt
          DESTINATION licenses)

  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(README_LINUX_NSS "
Linux users: you might see an error message like this (paths abbreviated):

  Invalid MEX-file
  '.../sapiremote_mex.mexa64':
  .../sys/jxbrowser/glnxa64/xulrunner/xulrunner-linux-64/libssl3.so:
  version `NSS_3.14' not found (required by .../sapiremote_mex.mexa64)

Try closing MATLAB's help browser and restarting the program.  MATLAB
provides an older version of a library required by the Solver API package.
If you use SAPI functions before opening the help browser, the correct
library will be loaded.  Be sure that you have either the libnss3 (Ubuntu,
Debian) or nss (Fedora, Red Hat) package installed on your system.")
  else()
    set(README_LINUX_NSS)
  endif()
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/README.txt.in
                 ${CMAKE_CURRENT_BINARY_DIR}/README.txt @ONLY)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sapiVersion.m.in
                 ${CMAKE_CURRENT_BINARY_DIR}/sapiVersion.m @ONLY)

  install(FILES
          chimeraToLinearIndex.m
          getChimeraAdjacency.m
          getHardwareAdjacency.m
          isingToQubo.m
          linearToChimeraIndex.m
          makeQuadratic.m
          quboToIsing.m
          reduceDegree.m
          sapiAsyncDone.m
          sapiAsyncResult.m
          sapiAsyncRetry.m
          sapiAsyncSolveIsing.m
          sapiAsyncSolveQubo.m
          sapiAsyncStatus.m
          sapiAwaitCompletion.m
          sapiAwaitSubmission.m
          sapiCancelSubmittedProblem.m
          sapiEmbedProblem.m
          sapiFindEmbedding.m
          sapiFixVariables.m
          sapiListSolvers.m
          sapiLocalConnection.m
          sapiRemoteConnection.m
          sapiSolveIsing.m
          sapiSolveQSage.m
          sapiSolveQubo.m
          sapiSolver.m
          sapiSolverProperties.m
          sapiUnembedAnswer.m
          ${CMAKE_CURRENT_BINARY_DIR}/sapiVersion.m
          ${CMAKE_CURRENT_BINARY_DIR}/README.txt
          DESTINATION .)

  # Packaging
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

  set(CPACK_PACKAGE_NAME sapi-matlab-client)
  set(CPACK_PACKAGE_VERSION "${SAPI_VERSION}")
  include(CPack)
elseif(ENABLE_TESTS)
  add_subdirectory(test)
endif()
