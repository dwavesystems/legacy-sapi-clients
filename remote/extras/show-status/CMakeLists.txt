set(SAPI_CLIENT status)
configure_file("${CMAKE_SOURCE_DIR}/src/user-agent.cpp.in" user-agent.cpp
  @ONLY ESCAPE_QUOTES)

add_executable(show-status main.cpp "${CMAKE_CURRENT_BINARY_DIR}/user-agent.cpp" ${SAPIREMOTE_SOURCES})
target_link_libraries(show-status ${Boost_SYSTEM_LIBRARY} ${CURL_LIBRARY})

if(CMAKE_COMPILER_IS_GNUCXX)
  set_target_properties(show-status PROPERTIES
    COMPILE_OPTIONS -pthread
    LINK_FLAGS -pthread)
endif()
