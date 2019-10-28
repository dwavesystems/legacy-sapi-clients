set(SWIG_WRAPPER sapiremote_wrap.cpp)
set(SWIG_WRAPPER_PY sapiremote.py)

include_directories(
  ${CMAKE_SOURCE_DIR}/lib/test
  ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_custom_command(OUTPUT ${SWIG_WRAPPER} ${SWIG_WRAPPER_PY}
  COMMAND ${SWIG_EXECUTABLE} -c++ -python -o ${SWIG_WRAPPER}
    ${CMAKE_CURRENT_SOURCE_DIR}/../sapiremote.i
  DEPENDS ../sapiremote.i VERBATIM COMMENT "SWIG wrapper")

add_library(_sapiremote-test MODULE EXCLUDE_FROM_ALL
  ${SWIG_WRAPPER}
  ../conversion.cpp
  ../python-api.cpp
  python-api-test.cpp
  ${CMAKE_SOURCE_DIR}/lib/test/testimpl.cpp
  ${CMAKE_SOURCE_DIR}/src/await.cpp
  ${CMAKE_SOURCE_DIR}/src/json.cpp
  ${CMAKE_SOURCE_DIR}/src/base64.cpp
  ${CMAKE_SOURCE_DIR}/src/decode-answer.cpp
  ${CMAKE_SOURCE_DIR}/src/decode-qp.cpp
  ${CMAKE_SOURCE_DIR}/src/encode-qp.cpp
  ${CMAKE_SOURCE_DIR}/src/enum-strings.cpp)

set_target_properties(_sapiremote-test PROPERTIES
  ${PYTHON_EXTENSION_PROPERTIES}
  OUTPUT_NAME _sapiremote
  PREFIX ""
  COMPILE_FLAGS "${PYTHON_CXXFLAGS}"
  LINK_FLAGS "${PYTHON_LDFLAGS}")

target_link_libraries(_sapiremote-test
  ${PYTHON_LIBRARIES}
  ${Boost_SYSTEM_LIBRARY})

configure_file(tests.py tests.py COPYONLY)

set(XML_RESULTS test-results.xml)

add_custom_target(check-python
  ${NOSETESTS_COMMAND} tests.py --with-xunit --xunit-file=${XML_RESULTS}
  DEPENDS ${SWIG_WRAPPER_PY} _sapiremote-test
  VERBATIM COMMENT "Python library unit tests")
