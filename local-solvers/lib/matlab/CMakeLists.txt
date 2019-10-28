add_definitions(-DBOOST_ALL_NO_LIB)

add_mex(sapilocal_orangSample
  ${SAPILOCAL_SOURCES}
  conversions.cpp
  errid.cpp
  orang-sample.cpp)

add_dependencies(sapilocal_orangSample orang)

add_mex(sapilocal_orangOptimize
  ${SAPILOCAL_SOURCES}
  conversions.cpp
  errid.cpp
  orang-optimize.cpp)

add_dependencies(sapilocal_orangOptimize orang)

add_mex(sapilocal_orangHeuristic
  ${SAPILOCAL_SOURCES}
  conversions.cpp
  errid.cpp
  orang-heuristic-matlab-wrapper.cpp)

add_dependencies(sapilocal_orangHeuristic orang)

install(TARGETS sapilocal_orangSample sapilocal_orangOptimize sapilocal_orangHeuristic
  LIBRARY DESTINATION matlab COMPONENT matlab)
install(FILES sapilocal_orangSample.m sapilocal_orangOptimize.m sapilocal_orangHeuristic.m
  DESTINATION matlab COMPONENT matlab)
