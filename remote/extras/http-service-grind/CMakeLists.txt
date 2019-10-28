set(SAPI_CLIENT http-service-grind)
configure_file("${CMAKE_SOURCE_DIR}/src/user-agent.cpp.in" user-agent.cpp
  @ONLY ESCAPE_QUOTES)

add_executable(http-service-grind main.cpp "${CMAKE_CURRENT_BINARY_DIR}/user-agent.cpp" ${SAPIREMOTE_SOURCES})
target_link_libraries(http-service-grind ${Boost_SYSTEM_LIBRARY} ${CURL_LIBRARY})