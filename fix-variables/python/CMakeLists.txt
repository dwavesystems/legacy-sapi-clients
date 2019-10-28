if(CMAKE_HOST_APPLE)
  find_library(CORESERVICES_FRAMEWORK CoreServices)
endif()

configure_file(build_egg.py.in build_egg.py @ONLY)

include(${SWIG_USE_FILE})

include_directories(${FIXVARIABLES_INCLUDE_DIRS}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${PYTHON_INCLUDE_PATH}
                    ${Boost_INCLUDE_DIR})

set_source_files_properties(fix_variables_impl.i PROPERTIES CPLUSPLUS ON KEYWORD ON)

swig_add_module(fix_variables_impl python fix_variables_impl.i
                fix_variables_python_wrapper.cpp
                ${CMAKE_SOURCE_DIR}/src/fix_variables.cpp)

set_target_properties(_fix_variables_impl PROPERTIES
                      COMPILE_FLAGS "${PYTHON_CXXFLAGS}"
                      LINK_FLAGS "${PYTHON_LDFLAGS}")

swig_link_libraries(fix_variables_impl ${PYTHON_LIBRARIES} ${CORESERVICES_FRAMEWORK})

set(SWIG_WRAPPER_PY fix_variables_impl.py)

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
                     DEPENDS _fix_variables_impl ${SWIG_WRAPPER_PY} #${SWIG_WRAPPER_PYC}
                     VERBATIM COMMENT "Building fix variables egg")

  add_custom_target(python-egg ALL DEPENDS ${EGGFILE})
else()
  message(SEND_ERROR "Error determining egg file name: ${EGGFILE_ERROR}")
endif()

option(ENABLE_PYTHON_TESTS "Enable fix variables Python tests" OFF)
if(ENABLE_PYTHON_TESTS)
  add_subdirectory(test)
endif()
