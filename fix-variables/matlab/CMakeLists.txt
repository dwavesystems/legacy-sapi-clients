set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake-modules)

set(PRODUCT_NAME "D-WAVE Fix Variables MATLAB Wrapper")

set(MEX_EXTRA_ARGS "" CACHE "Extra arguments to mex command" STRING)

option(ENABLE_MATLAB_TESTS "Enable Fix Variables MATLAB tests" OFF)

if(ENABLE_MATLAB_TESTS)
  add_subdirectory(test)
endif()

add_mex(fix_variables_mex
        ${CMAKE_SOURCE_DIR}/src/fix_variables.cpp
        fix_variables_matlab_wrapper.cpp)

include_directories(${Boost_INCLUDE_DIR}
                    ${_MATLAB_BINDIR}/../extern/include
                    ${FIXVARIABLES_INCLUDE_DIRS})

target_link_libraries(fix_variables_mex ${MATLAB_LIBRARIES})

set_target_properties(fix_variables_mex PROPERTIES PREFIX "" SUFFIX ".${MATLAB_MEXEXT}" COMPILE_FLAGS "${MATLAB_CXXFLAGS}" LINK_FLAGS "${MATLAB_LDFLAGS}")

set(INSTALL_DESTINATION "fix-variables-${SAPI_VERSION}-matlab")

install(FILES
        fix_variables_mex.m
        DESTINATION ${INSTALL_DESTINATION} COMPONENT matlab)

install(TARGETS fix_variables_mex LIBRARY DESTINATION ${INSTALL_DESTINATION} COMPONENT matlab)

set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} matlab PARENT_SCOPE)
