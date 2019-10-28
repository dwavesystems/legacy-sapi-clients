set(TEST_FILES
  test_orang_sample.py
  test_orang_optimize.py
  test_orang_heuristic.py)

configure_file(runtests.py .. COPYONLY)
foreach(F ${TEST_FILES})
  configure_file(${F} . COPYONLY)
endforeach()

set(SWIG_WRAPPER sapilocal_wrap.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_custom_command(OUTPUT ${SWIG_WRAPPER}
  COMMAND ${SWIG_EXECUTABLE} -c++ -noproxy -python -o ${SWIG_WRAPPER}
    ${CMAKE_CURRENT_SOURCE_DIR}/../sapilocal.i
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../sapilocal.i VERBATIM COMMENT "Generating sapilocal SWIG wrapper")

add_definitions(-DBOOST_ALL_NO_LIB)

add_library(sapilocal-test MODULE
  ${SWIG_WRAPPER}
  ../python-api.cpp
  ../typemaps.cpp
  ${SAPILOCAL_SOURCES})

set_target_properties(sapilocal-test PROPERTIES
  ${PYTHON_EXTENSION_PROPERTIES}
  OUTPUT_NAME sapilocal
  PREFIX ""
  COMPILE_FLAGS "${PYTHON_CXXFLAGS}"
  LINK_FLAGS "${PYTHON_LDFLAGS}")

target_link_libraries(sapilocal-test ${PYTHON_LIBRARIES})

add_dependencies(sapilocal-test orang)

add_custom_target(check-python
    ${PYTHON_EXECUTABLE} -B runtests.py test
      --junit-xml=test/test-results.xml
    DEPENDS sapilocal-test runtests.py ${TEST_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
    COMMENT "Running Python library unit tests")
