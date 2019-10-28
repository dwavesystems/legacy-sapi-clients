add_executable(fix-variables-tests EXCLUDE_FROM_ALL
               test-fix-variables.cpp
               ${CMAKE_SOURCE_DIR}/src/fix_variables.cpp)

if(CMAKE_COMPILER_IS_GNUCXX)
  set_target_properties(fix-variables-tests PROPERTIES
                        COMPILE_FLAGS -pthread
                        LINK_FLAGS -pthread)
  if(GXX_VERSION VERSION_LESS 4.8)
    add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set_target_properties(fix-variables-tests PROPERTIES
                        COMPILE_FLAGS -pthread)
endif()

if(CMAKE_HOST_WIN32)
  add_definitions(-D_VARIADIC_MAX=10)
endif()

add_definitions(-DGTEST_LANG_CXX11=0)

include_directories(${FIXVARIABLES_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})
include_directories(SYSTEM ${GTest_INCLUDE_DIRS})

target_link_libraries(fix-variables-tests
                      ${GTest_LIBRARY} ${GTest_MAIN})

add_custom_target(check
                  fix-variables-tests --gtest_output=xml:test-results.xml
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Running fix variables unit tests" VERBATIM)

add_dependencies(check-all check)
