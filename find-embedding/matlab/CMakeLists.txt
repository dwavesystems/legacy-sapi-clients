set(PRODUCT_NAME "D-WAVE Find Embedding MATLAB Wrapper")

set(MEX_EXTRA_ARGS "" CACHE "Extra arguments to mex command" STRING)

option(ENABLE_MATLAB_TESTS "Enable Find Embedding MATLAB tests" OFF)

if(ENABLE_MATLAB_TESTS)
  add_subdirectory(test)
endif()

add_library(find_embedding_mex MODULE
            ${CMAKE_SOURCE_DIR}/src/find_embedding.cpp
            find_embedding_matlab_wrapper.cpp)

include_directories(${Boost_INCLUDE_DIR}
                    ${_MATLAB_BINDIR}/../extern/include
                    ${FINDEMBEDDING_INCLUDE_DIRS})

target_link_libraries(find_embedding_mex ${MATLAB_LIBRARIES})

set_target_properties(find_embedding_mex PROPERTIES PREFIX "" SUFFIX ".${MATLAB_MEXEXT}" COMPILE_FLAGS "${MATLAB_CXXFLAGS}" LINK_FLAGS "${MATLAB_LDFLAGS}")

set(INSTALL_DESTINATION "find-embedding-${SAPI_VERSION}-matlab")

install(FILES
        find_embedding_mex.m
        DESTINATION ${INSTALL_DESTINATION} COMPONENT matlab)

install(TARGETS find_embedding_mex LIBRARY DESTINATION ${INSTALL_DESTINATION} COMPONENT matlab)

set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} matlab PARENT_SCOPE)
