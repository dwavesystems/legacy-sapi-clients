configure_file(build_egg.py.in build_egg.py @ONLY)

execute_process(COMMAND ${PYTHON_EXECUTABLE} build_egg.py --egg-name
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                RESULT_VARIABLE EGGFILE_RESULT
                OUTPUT_VARIABLE EGGFILE
                ERROR_VARIABLE EGGFILE_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE)

if(EGGFILE_RESULT EQUAL 0)
  add_custom_command(OUTPUT ${EGGFILE}
                     COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/build_egg.py
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                     DEPENDS _qsage_impl ${SWIG_WRAPPER_PY} #${SWIG_WRAPPER_PYC}
                     VERBATIM COMMENT "Building qsage egg")

  add_custom_target(python-egg ALL DEPENDS ${EGGFILE})
else()
  message(SEND_ERROR "Error determining egg file name: ${EGGFILE_ERROR}")
endif()

if(CMAKE_HOST_APPLE)
  find_library(CORESERVICES_FRAMEWORK CoreServices)
endif()

include(${SWIG_USE_FILE})

include_directories(${BLACKBOX_INCLUDE_DIRS}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${PYTHON_INCLUDE_PATH}
                    ${Boost_INCLUDE_DIR}
                    ${COINOR_INCLUDE_DIR})

set_source_files_properties(qsage_impl.i PROPERTIES CPLUSPLUS ON KEYWORD ON)

swig_add_module(qsage_impl python qsage_impl.i
                blackbox_python_wrapper.cpp
                ${CMAKE_SOURCE_DIR}/src/blackbox.cpp)

set_target_properties(_qsage_impl PROPERTIES
                      COMPILE_FLAGS "${PYTHON_CXXFLAGS}"
                      LINK_FLAGS "${PYTHON_LDFLAGS}")

swig_link_libraries(qsage_impl ${COINOR_LIBRARIES} ${PYTHON_LIBRARIES} ${CORESERVICES_FRAMEWORK})

set(SWIG_WRAPPER_PY qsage_impl.py)

option(ENABLE_PYTHON_TESTS "Enable qsage Python tests" OFF)
if(ENABLE_PYTHON_TESTS)
  add_subdirectory(test)
endif()
