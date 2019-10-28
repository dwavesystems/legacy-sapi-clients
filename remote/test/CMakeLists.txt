add_executable(sapi-remote-tests EXCLUDE_FROM_ALL
  test-threadpool.cpp
  test-decode-qp-answer.cpp
  test-answer-format.cpp
  test-encode-qp-problem.cpp
  test-sapi-service.cpp
  test-problem-manager.cpp
  test-problem-manager-retry.cpp
  test-retry-service.cpp
  test-json.cpp
  test-base64.cpp
  test-await.cpp
  test-enum-strings.cpp
  test.cpp
  ${CMAKE_SOURCE_DIR}/src/threadpool.cpp
  ${CMAKE_SOURCE_DIR}/src/json.cpp
  ${CMAKE_SOURCE_DIR}/src/base64.cpp
  ${CMAKE_SOURCE_DIR}/src/sapi-service.cpp
  ${CMAKE_SOURCE_DIR}/src/problem-manager.cpp
  ${CMAKE_SOURCE_DIR}/src/retry-service.cpp
  ${CMAKE_SOURCE_DIR}/src/await.cpp
  ${CMAKE_SOURCE_DIR}/src/decode-answer.cpp
  ${CMAKE_SOURCE_DIR}/src/decode-qp.cpp
  ${CMAKE_SOURCE_DIR}/src/encode-qp.cpp
  ${CMAKE_SOURCE_DIR}/src/enum-strings.cpp)

if(CMAKE_COMPILER_IS_GNUCXX)
  set_target_properties(sapi-remote-tests PROPERTIES
    COMPILE_FLAGS -pthread
    LINK_FLAGS -pthread)
  if(GXX_VERSION VERSION_LESS 4.8)
    add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set_target_properties(sapi-remote-tests PROPERTIES
    COMPILE_FLAGS -pthread)
endif()

add_definitions(-DGTEST_LANG_CXX11=0)
include_directories(SYSTEM ${GTest_INCLUDE_DIR} ${GMock_INCLUDE_DIR})
target_link_libraries(sapi-remote-tests
  ${GTest_LIBRARY} ${GMock_LIBRARY} ${GMock_MAIN}
  ${Boost_SYSTEM_LIBRARY})

add_custom_target(check
  sapi-remote-tests --gtest_output=xml:test-results.xml
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running core unit tests" VERBATIM)

