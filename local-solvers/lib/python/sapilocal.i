//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

%module(docstring="D-Wave Solver API local solvers") sapilocal
%{

#include <cctype>
#include <cstring>

#include <sapi-local/sapi-local.hpp>

#include "typemaps.hpp"
#include "python-exception.hpp"
#include "python-api.hpp"

#define CATCH_ALL                                      \
  catch (PythonException&) {                           \
    SWIG_fail;                                         \
  } catch (std::bad_alloc&) {                          \
    SWIG_exception(SWIG_MemoryError, "Out of memory"); \
  } catch (std::exception& e) {                        \
    SWIG_exception(SWIG_RuntimeError, e.what());       \
  } catch (...) {                                      \
    SWIG_exception(SWIG_RuntimeError, "");             \
  }

%}

%include exception.i

%exception {
  try {
    $action
  } CATCH_ALL
}

%typemap(in) sapilocal::ProblemType {
  auto s = PyBytes_AsString($input);
  if (!s) SWIG_fail;
  for (auto p = s; *p != '\0'; ++p) *p = std::tolower(*p);
  if (std::strcmp(s, "ising") == 0) {
    $1 = sapilocal::PROBLEM_ISING;
  } else if (std::strcmp(s, "qubo") == 0) {
    $1 = sapilocal::PROBLEM_QUBO;
  } else {
    SWIG_exception(SWIG_ValueError, "Unknown problem type");
  }
}

%typemap(in) sapilocal::SparseMatrix& (sapilocal::SparseMatrix val) {
  try {
    val = typemaps::inSparseMatrix($input);
    $1 = &val;
  } CATCH_ALL
}

%typemap(in) sapilocal::OrangSampleParams& (sapilocal::OrangSampleParams val) {
  try {
    val = typemaps::inOrangSampleParams($input);
    $1 = &val;
  } CATCH_ALL;
}

%typemap(in) sapilocal::OrangOptimizeParams& (sapilocal::OrangOptimizeParams val) {
  try {
    val = typemaps::inOrangOptimizeParams($input);
    $1 = &val;
  } CATCH_ALL;
}

%typemap(in) sapilocal::OrangHeuristicParams& (sapilocal::OrangHeuristicParams val) {
  try {
    val = typemaps::inOrangHeuristicParams($input);
    $1 = &val;
  } CATCH_ALL;
}

%typemap(out) sapilocal::IsingResult {
  $result = typemaps::outIsingResult($1);
}

%include "python-api.hpp"
