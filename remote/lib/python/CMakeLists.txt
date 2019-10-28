set(SAPI_CLIENT Python)
configure_file("${CMAKE_SOURCE_DIR}/src/user-agent.cpp.in" user-agent.cpp
  @ONLY ESCAPE_QUOTES)

set(SWIG_WRAPPER sapiremote_wrap.cpp)
set(SWIG_WRAPPER_PY sapiremote.py)
set(SWIG_WRAPPER_PYC sapiremote.pyc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(OUTPUT ${SWIG_WRAPPER} ${SWIG_WRAPPER_PY}
  COMMAND ${SWIG_EXECUTABLE} -c++ -python -o ${SWIG_WRAPPER}
    ${CMAKE_CURRENT_SOURCE_DIR}/sapiremote.i
  DEPENDS sapiremote.i VERBATIM COMMENT "Generating sapiremote SWIG wrapper")

add_custom_command(OUTPUT ${SWIG_WRAPPER_PYC}
  COMMAND ${PYTHON_EXECUTABLE} -m py_compile ${SWIG_WRAPPER_PY}
  DEPENDS ${SWIG_WRAPPER_PY} VERBATIM
  COMMENT "Compiling sapiremote Python bytecode")

add_library(_sapiremote MODULE
  ${SWIG_WRAPPER}
  conversion.cpp
  python-api.cpp
  internals.cpp
  "${CMAKE_CURRENT_BINARY_DIR}/user-agent.cpp"
  ${SAPIREMOTE_SOURCES})

set_target_properties(_sapiremote PROPERTIES
  ${PYTHON_EXTENSION_PROPERTIES}
  PREFIX ""
  COMPILE_FLAGS "${PYTHON_CXXFLAGS}"
  LINK_FLAGS "${PYTHON_LDFLAGS}")
target_link_libraries(_sapiremote
  ${PYTHON_LIBRARIES} ${Boost_SYSTEM_LIBRARY} ${CURL_LIBRARY})

if(CMAKE_BUILD_TYPE MATCHES "Release|MinSizeRel")
  set(STRIP_ARGS)
  if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    set(STRIP_ARGS "-x")
  endif()
  if(CMAKE_STRIP)
    get_target_property(MODULE_OUTPUT_NAME _sapiremote LOCATION)
    add_custom_command(TARGET _sapiremote POST_BUILD
      COMMAND ${CMAKE_STRIP} ${STRIP_ARGS} ${MODULE_OUTPUT_NAME}
      COMMENT "Stripping sapiremote module"
      VERBATIM)
  endif()
endif()

configure_file(build_egg.py.in build_egg.py @ONLY)
execute_process(COMMAND ${PYTHON_EXECUTABLE} build_egg.py --egg-name
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  RESULT_VARIABLE EGGFILE_RESULT
  OUTPUT_VARIABLE EGGFILE
  ERROR_VARIABLE EGGFILE_ERROR
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(EGGFILE_RESULT EQUAL 0)
  add_custom_command(OUTPUT ${EGGFILE}
    COMMAND ${PYTHON_EXECUTABLE} build_egg.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS _sapiremote ${SWIG_WRAPPER_PY} ${SWIG_WRAPPER_PYC}
    VERBATIM COMMENT "Building sapiremote egg")
  add_custom_target(python-egg ALL DEPENDS ${EGGFILE})
else()
  message(SEND_ERROR "Error determining egg file name: ${EGGFILE_ERROR}")
endif()
