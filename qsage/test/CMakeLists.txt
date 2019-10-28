add_executable(qsage-tests EXCLUDE_FROM_ALL
               test-qsage.cpp
               ${CMAKE_SOURCE_DIR}/src/blackbox.cpp)

if(CMAKE_COMPILER_IS_GNUCXX)
  set_target_properties(qsage-tests PROPERTIES
                        COMPILE_FLAGS -pthread
                        LINK_FLAGS -pthread)
  if(GXX_VERSION VERSION_LESS 4.8)
    add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set_target_properties(qsage-tests PROPERTIES
                        COMPILE_FLAGS -pthread)
endif()

if(CMAKE_HOST_WIN32)
  add_definitions(-D_VARIADIC_MAX=10)
endif()

add_definitions(-DGTEST_LANG_CXX11=0)

include_directories(${BLACKBOX_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${COINOR_INCLUDE_DIR})
include_directories(SYSTEM ${GTest_INCLUDE_DIRS})

target_link_libraries(qsage-tests
                      ${COINOR_LIBRARIES} ${GTest_LIBRARY} ${GTest_MAIN})

add_custom_target(check
                  qsage-tests --gtest_output=xml:test-results.xml
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Running QSage unit tests" VERBATIM)

add_dependencies(check-all check)
