include_directories(${CMAKE_SOURCE_DIR})
include_directories(SYSTEM ${GTest_INCLUDE_DIRS} ${GMock_INCLUDE_DIRS})

add_definitions(-DGTEST_LANG_CXX11=0)

add_executable(test_dwave_sapi EXCLUDE_FROM_ALL
    main.cpp
    test_dwave_sapi.cpp
    test-embed-problem.cpp
    test-unembed-answer.cpp
    test-fixvars.cpp
    test-global.cpp
    test-remote.cpp
    test-remote-props.cpp
    test-local.cpp
    test-solvers.cpp
    test-qsage.cpp
    test-errors.cpp
    test.cpp
    ${CMAKE_SOURCE_DIR}/src/dwave_sapi.cpp
    ${CMAKE_SOURCE_DIR}/src/embed-problem.cpp
    ${CMAKE_SOURCE_DIR}/src/unembed-answer.cpp
    ${CMAKE_SOURCE_DIR}/src/fix-variables.cpp
    ${CMAKE_SOURCE_DIR}/src/conversions.cpp
    ${CMAKE_SOURCE_DIR}/src/internal.cpp
    ${CMAKE_SOURCE_DIR}/src/global.cpp
    ${CMAKE_SOURCE_DIR}/src/sapi-impl.cpp
    ${CMAKE_SOURCE_DIR}/src/remote.cpp
    ${CMAKE_SOURCE_DIR}/src/local.cpp
    ${CMAKE_SOURCE_DIR}/src/defaults.cpp
    ${CMAKE_SOURCE_DIR}/src/freefuncs.cpp
    ${CMAKE_SOURCE_DIR}/../remote/src/json.cpp
    ${FIND_EMBEDDING_SOURCES}
    ${FIX_VARIABLES_SOURCES}
    ${QSAGE_SOURCES})

if(CMAKE_COMPILER_IS_GNUCXX)
  set_target_properties(test_dwave_sapi PROPERTIES
      COMPILE_FLAGS -pthread
      LINK_FLAGS -pthread)
  if(GXX_VERSION VERSION_LESS 4.8)
    add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set_target_properties(test_dwave_sapi PROPERTIES
      COMPILE_FLAGS -pthread)
endif()

set_target_properties(test_dwave_sapi PROPERTIES
    COMPILE_DEFINITIONS DWAVE_SAPI_BUILD)

target_link_libraries(test_dwave_sapi
    ${Boost_SYSTEM_LIBRARY}
    ${COINOR_LIBRARIES}
    ${GTest_LIBRARY} ${GMock_LIBRARY})

add_dependencies(test_dwave_sapi orang)

add_custom_target(check
    test_dwave_sapi --gtest_output=xml:test-results.xml
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running sapi c client tests" VERBATIM)
