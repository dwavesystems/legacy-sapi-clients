add_executable(find-embedding-tests EXCLUDE_FROM_ALL
               test-find-embedding.cpp
               validation.cpp
               ${CMAKE_SOURCE_DIR}/src/find_embedding.cpp)

if(CMAKE_COMPILER_IS_GNUCXX)
  set_target_properties(find-embedding-tests PROPERTIES
                        COMPILE_FLAGS -pthread
                        LINK_FLAGS -pthread)
  if(GXX_VERSION VERSION_LESS 4.8)
    add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set_target_properties(find-embedding-tests PROPERTIES
                        COMPILE_FLAGS -pthread)
endif()

add_definitions(-DGTEST_LANG_CXX11=0)

include_directories(${FINDEMBEDDING_INCLUDE_DIRS})
include_directories(${BOOST_INCLUDEDIR})
include_directories(SYSTEM ${GMock_INCLUDE_DIRS} ${GTest_INCLUDE_DIRS})

target_link_libraries(find-embedding-tests
                      ${GTest_LIBRARY} ${GMock_LIBRARY} ${GMock_MAIN})

add_custom_target(check
                  find-embedding-tests --gtest_output=xml:test-results.xml
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Running find embedding unit tests" VERBATIM)
