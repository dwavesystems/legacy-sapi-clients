find_program(MATLAB_COMMAND matlab DOC "MATLAB program")
if(MATLAB_COMMAND)
  get_filename_component(_MATLAB_REALPATH ${MATLAB_COMMAND} REALPATH)
  get_filename_component(_MATLAB_BINDIR ${_MATLAB_REALPATH} PATH)

  find_program(_MEXEXT_COMMAND mexext NAMES mexext.bat
    HINTS ${_MATLAB_BINDIR} NO_DEFAULT_PATH)
  if(_MEXEXT_COMMAND AND NOT MATLAB_MEXEXT)
    execute_process(OUTPUT_VARIABLE MATLAB_MEXEXT
      COMMAND ${_MEXEXT_COMMAND} OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(MATLAB_MEXEXT ${MATLAB_MEXEXT} CACHE STRING "MEX file extension" FORCE)
    mark_as_advanced(MATLAB_MEXEXT)
  endif()
  unset(_MEXEXT_COMMAND CACHE)

  if(MATLAB_MEXEXT STREQUAL mexa64)
    set(MATLAB_ARCH glnxa64 CACHE INTERNAL "" FORCE)
  elseif(MATLAB_MEXEXT STREQUAL mexglx)
    set(MATLAB_ARCH glnx86 CACHE INTERNAL "" FORCE)
  elseif(MATLAB_MEXEXT STREQUAL mexw64)
    set(MATLAB_ARCH win64 CACHE INTERNAL "" FORCE)
  elseif(MATLAB_MEXEXT STREQUAL mexw32)
    set(MATLAB_ARCH win32 CACHE INTERNAL "" FORCE)
  elseif(MATLAB_MEXEXT STREQUAL mexmaci64)
    set(MATLAB_ARCH maci64 CACHE INTERNAL "" FORCE)
  else()
    message(SEND_ERROR "Unknown MEX extension: ${MATLAB_MEXEXT}")
  endif()
  set(_MATLAB_ARCH_DIR ${_MATLAB_BINDIR}/${MATLAB_ARCH})
  set(_MATLAB_EXTERNARCH_DIR ${_MATLAB_BINDIR}/../extern/lib/${MATLAB_ARCH})

  find_library(MATLAB_mx_LIBRARY mx libmx
    HINTS ${_MATLAB_ARCH_DIR} ${_MATLAB_EXTERNARCH_DIR}/microsoft
    NO_DEFAULT_PATHS)
  find_library(MATLAB_mex_LIBRARY mex libmex
    HINTS ${_MATLAB_ARCH_DIR} ${_MATLAB_EXTERNARCH_DIR}/microsoft
    NO_DEFAULT_PATHS)
  find_library(MATLAB_mat_LIBRARY mat libmat
    HINTS ${_MATLAB_ARCH_DIR} ${_MATLAB_EXTERNARCH_DIR}/microsoft
    NO_DEFAULT_PATHS)
  find_library(MATLAB_ut_LIBRARY ut libut
    HINTS ${_MATLAB_ARCH_DIR} ${_MATLAB_EXTERNARCH_DIR}/microsoft
    NO_DEFAULT_PATHS)
  set(MATLAB_LIBRARIES
    ${MATLAB_mx_LIBRARY} ${MATLAB_mex_LIBRARY} ${MATLAB_mat_LIBRARY}
    ${MATLAB_ut_LIBRARY}
    CACHE INTERNAL "")

  if(MATLAB_ARCH MATCHES "^glnx")
    set(MATLAB_CXXFLAGS -D_GNU_SOURCE -fPIC -fno-omit-frame-pointer -pthread
      -I${_MATLAB_BINDIR}/../extern/include
      CACHE INTERNAL "" FORCE)
    set(MATLAB_LDFLAGS -pthread -Wl,--no-undefined
      -Wl,--version-script,${_MATLAB_EXTERNARCH_DIR}/mexFunction.map
      CACHE INTERNAL "" FORCE)
  elseif(MATLAB_ARCH MATCHES "^win")
    set(MATLAB_CXXFLAGS /GR /W3 /EHs /D_CRT_SECURE_NO_DEPRECATE
      /D_SCL_SECURE_NO_DEPRECATE /D_SECURE_SCL=0 /DMATLAB_MEX_FILE)
    set(MATLAB_LDFLAGS /export:mexFunction /manifest /incremental:NO)
  elseif(MATLAB_ARCH MATCHES "^mac")
    set(MATLAB_CXXFLAGS -fno-common -fexceptions CACHE INTERNAL "" FORCE)
    set(MATLAB_LDFLAGS
      -Wl,-exported_symbols_list,${_MATLAB_EXTERNARCH_DIR}/mexFunction.map
      CACHE INTERNAL "" FORCE)
  endif()
  string(REPLACE ";" " " MATLAB_CXXFLAGS "${MATLAB_CXXFLAGS}")
  string(REPLACE ";" " " MATLAB_LDFLAGS "${MATLAB_LDFLAGS}")

  function(add_mex _mex_name)
    add_library(${_mex_name} MODULE ${ARGN})
    include_directories("${_MATLAB_BINDIR}/../extern/include")
    target_link_libraries(${_mex_name} ${MATLAB_LIBRARIES})
    set_target_properties(${_mex_name} PROPERTIES
      PREFIX "" SUFFIX ".${MATLAB_MEXEXT}"
      COMPILE_FLAGS "${MATLAB_CXXFLAGS}"
      LINK_FLAGS "${MATLAB_LDFLAGS}")
  endfunction()

else()
  message(SEND_ERROR "MATLAB executable not found")
endif()

