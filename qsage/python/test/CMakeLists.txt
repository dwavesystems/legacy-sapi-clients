find_program(NOSETESTS_COMMAND nosetests)

mark_as_advanced(NOSETESTS_COMMAND)
if(NOT NOSETESTS_COMMAND)
  message(SEND_ERROR "nosetests program not found")
endif()

set(TEST_ARGS --with-xunit --xunit-file=test-results.xml)

configure_file(test_qsage.py test_qsage.py COPY)

set(SWIG_WRAPPER qsage_implPYTHON_wrap.cxx)
set(SWIG_WRAPPER_PY qsage_impl.py)

add_custom_command(OUTPUT ${SWIG_WRAPPER} ${SWIG_WRAPPER_PY}
                   COMMAND ${SWIG_EXECUTABLE} -c++ -python -o ${SWIG_WRAPPER}
                   ${CMAKE_CURRENT_SOURCE_DIR}/../qsage_impl.i
                   DEPENDS ../qsage_impl.i VERBATIM COMMENT "SWIG wrapper")
  
add_library(_qsage-test MODULE EXCLUDE_FROM_ALL
            ${SWIG_WRAPPER}
            ../blackbox_python_wrapper.cpp
            ${CMAKE_SOURCE_DIR}/src/blackbox.cpp)
  
set_target_properties(_qsage-test PROPERTIES
                      ${PYTHON_EXTENSION_PROPERTIES}
                      OUTPUT_NAME _qsage_impl
                      PREFIX ""
                      COMPILE_FLAGS "${PYTHON_CXXFLAGS}"
                      LINK_FLAGS "${PYTHON_LDFLAGS}")

include_directories(${BLACKBOX_INCLUDE_DIRS}
                    ${CMAKE_CURRENT_SOURCE_DIR}/..
                    ${PYTHON_INCLUDE_PATH}
                    ${Boost_INCLUDE_DIR}
                    ${COINOR_INCLUDE_DIR})

target_link_libraries(_qsage-test ${COINOR_LIBRARIES} ${PYTHON_LIBRARIES} ${CORESERVICES_FRAMEWORK})


add_custom_target(check-python
                  COMMAND ${NOSETESTS_COMMAND} ${TEST_ARGS})

add_dependencies(check-python ${SWIG_WRAPPER_PY} _qsage-test)

add_dependencies(check-all check-python)
