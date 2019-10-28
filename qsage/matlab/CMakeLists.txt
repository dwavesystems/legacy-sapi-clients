set(PRODUCT_NAME "D-WAVE QSage MATLAB Wrapper")

option(ENABLE_MATLAB_TESTS "Enable QSage MATLAB tests" OFF)

if(ENABLE_MATLAB_TESTS)
  add_subdirectory(test)
endif()

add_library(solve_qsage_mex MODULE
            ${CMAKE_SOURCE_DIR}/src/blackbox.cpp
            blackbox_matlab_wrapper.cpp)

include_directories(${BLACKBOX_INCLUDE_DIRS}
                    ${_MATLAB_BINDIR}/../extern/include
                    ${Boost_INCLUDE_DIR}
                    ${COINOR_INCLUDE_DIR})

target_link_libraries(solve_qsage_mex ${COINOR_LIBRARIES} ${MATLAB_LIBRARIES})

set_target_properties(solve_qsage_mex PROPERTIES PREFIX "" SUFFIX ".${MATLAB_MEXEXT}" COMPILE_FLAGS "${MATLAB_CXXFLAGS}" LINK_FLAGS "${MATLAB_LDFLAGS}")

set(INSTALL_DESTINATION "qsage-${SAPI_VERSION}-matlab")

install(FILES
        solve_qsage_mex.m
        DESTINATION ${INSTALL_DESTINATION} COMPONENT matlab)

install(TARGETS solve_qsage_mex LIBRARY DESTINATION ${INSTALL_DESTINATION} COMPONENT matlab)

set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} matlab PARENT_SCOPE)
