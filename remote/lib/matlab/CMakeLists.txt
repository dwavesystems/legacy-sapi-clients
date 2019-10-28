set(SAPI_CLIENT MATLAB)
configure_file("${CMAKE_SOURCE_DIR}/src/user-agent.cpp.in" user-agent.cpp
  @ONLY ESCAPE_QUOTES)

add_mex(sapiremote_mex
  ${SAPIREMOTE_SOURCES}
  "${CMAKE_CURRENT_BINARY_DIR}/user-agent.cpp"
  connections.cpp
  sapiremote.cpp
  services.cpp
  solvers.cpp
  answers.cpp
  json-to-matlab.cpp)

target_link_libraries(sapiremote_mex ${Boost_SYSTEM_LIBRARY} ${CURL_LIBRARY})

set(INSTALL_DESTINATION "sapiremote-${SAPI_VERSION}-matlab")
install(FILES
  sapiremote_addproblem.m
  sapiremote_answer.m
  sapiremote_awaitcompletion.m
  sapiremote_awaitsubmission.m
  sapiremote_cancel.m
  sapiremote_retry.m
  sapiremote_connection.m
  sapiremote_decodeqp.m
  sapiremote_done.m
  sapiremote_encodeqp.m
  sapiremote_problemid.m
  sapiremote_status.m
  sapiremote_solve.m
  sapiremote_solvers.m
  sapiremote_submit.m
  DESTINATION "${INSTALL_DESTINATION}" COMPONENT matlab)
install(TARGETS sapiremote_mex LIBRARY
  DESTINATION "${INSTALL_DESTINATION}/private" COMPONENT matlab)
