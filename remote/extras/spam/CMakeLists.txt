add_executable(spam main.cpp ${SAPIREMOTE_SOURCES})

if(CMAKE_COMPILER_IS_GNUCXX)
  set_target_properties(spam PROPERTIES
    COMPILE_OPTIONS -pthread
    LINK_FLAGS -pthread)
endif()

target_link_libraries(spam ${Boost_SYSTEM_LIBRARY} ${CURL_LIBRARY})
