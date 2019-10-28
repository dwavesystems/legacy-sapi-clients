find_package(PythonInterp 3.6 REQUIRED)
find_package(PythonLibs 3.6 REQUIRED)

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
    DEPENDS sapilocal ${SWIG_WRAPPER_PY} ${SWIG_WRAPPER_PYC}
    VERBATIM COMMENT "Building sapilocal egg")
  add_custom_target(python-egg ALL DEPENDS ${EGGFILE})
else()
  if(EGGFILE_ERROR)
    message(SEND_ERROR "Error determining egg file name: ${EGGFILE_ERROR}")
  else()
    message(SEND_ERROR "Error determining egg file name: ${EGGFILE_RESULT}")
  endif()
endif()

set(SWIG_WRAPPER sapilocal_wrap.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(OUTPUT ${SWIG_WRAPPER}
  COMMAND ${SWIG_EXECUTABLE} -c++ -noproxy -python -o ${SWIG_WRAPPER}
    ${CMAKE_CURRENT_SOURCE_DIR}/sapilocal.i
  DEPENDS sapilocal.i VERBATIM COMMENT "Generating sapilocal SWIG wrapper")

add_definitions(-DBOOST_ALL_NO_LIB)

add_library(sapilocal MODULE
  ${SWIG_WRAPPER}
  python-api.cpp
  typemaps.cpp
  ${SAPILOCAL_SOURCES})

set_target_properties(sapilocal PROPERTIES
  ${PYTHON_EXTENSION_PROPERTIES}
  PREFIX ""
  COMPILE_FLAGS "${PYTHON_CXXFLAGS}"
  LINK_FLAGS "${PYTHON_LDFLAGS}")
target_link_libraries(sapilocal ${PYTHON_LIBRARIES})

add_dependencies(sapilocal orang)

if(CMAKE_BUILD_TYPE MATCHES "Release|MinSizeRel")
  set(STRIP_ARGS)
  if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    set(STRIP_ARGS "-x")
  endif()
  if(CMAKE_STRIP)
    get_target_property(MODULE_OUTPUT_NAME sapilocal LOCATION)
    add_custom_command(TARGET sapilocal POST_BUILD
      COMMAND ${CMAKE_STRIP} ${STRIP_ARGS} ${MODULE_OUTPUT_NAME}
      COMMENT "Stripping sapilocal module"
      VERBATIM)
  endif()
endif()



option(ENABLE_PYTHON_TESTS "Enable Python unit tests" OFF)
if(ENABLE_PYTHON_TESTS)
  add_subdirectory(test)
endif()
