//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <exception>
#include <map>
#include <memory>
#include <utility>

#include <boost/foreach.hpp>

#include <compressed_matrix.hpp>
#include <fix_variables.hpp>
#include "dwave_sapi.h"
#include "internal.hpp"

using std::current_exception;
using std::make_pair;
using std::map;
using std::pair;
using std::unique_ptr;

using fix_variables_::FixVariablesResult;
using fix_variables_::fixQuboVariables;
using compressed_matrix::CompressedMatrix;

using sapi::handleException;

namespace {

sapi_FixVariablesResult* convertResult(const FixVariablesResult& result0) {
  auto result = unique_ptr<sapi_FixVariablesResult>{new sapi_FixVariablesResult};
  auto fixedVars = unique_ptr<sapi_FixedVariable[]>{new sapi_FixedVariable[result0.fixedVars.size()]};
  auto problemEntries = unique_ptr<sapi_ProblemEntry[]>{new sapi_ProblemEntry[result0.newQ.nnz()]};

  result->fixed_variables = fixedVars.release();
  result->fixed_variables_len = result0.fixedVars.size();
  result->offset = result0.offset;
  result->new_problem.elements = problemEntries.release();
  result->new_problem.len = result0.newQ.nnz();

  for (size_t i = 0; i < result->fixed_variables_len; ++i) {
    result->fixed_variables[i].var = result0.fixedVars[i].first - 1; // ARRRRRRRRGH!
    result->fixed_variables[i].value = result0.fixedVars[i].second;
  }

  auto iter = result0.newQ.begin();
  for(size_t i = 0; i < result->new_problem.len; ++i) {
    result->new_problem.elements[i].i = iter.row();
    result->new_problem.elements[i].j = iter.col();
    result->new_problem.elements[i].value = *iter;
    ++iter;
  }

  return result.release();
}

} // namespace {anonymous}

DWAVE_SAPI sapi_Code sapi_fixVariables(
    const sapi_Problem* problem,
    sapi_FixVariablesMethod fix_variables_method,
    sapi_FixVariablesResult** fix_variables_result,
    char* err_msg) {

  try {

    auto dim = -1;
    auto problemMap = map<pair<int, int>, double>();
    for (size_t i = 0; i < problem->len; ++i) {
      const auto& e = problem->elements[i];
      problemMap[make_pair(e.i, e.j)] += e.value;
      if (e.i > dim) dim = e.i;
      if (e.j > dim) dim = e.j;
    }
    ++dim;
    auto problemCm = CompressedMatrix<double>{dim, dim, problemMap};

    int method = fix_variables_method == SAPI_FIX_VARIABLES_METHOD_OPTIMIZED ? 1 : 2;
    FixVariablesResult result = fixQuboVariables(problemCm, method);
    *fix_variables_result = convertResult(result);
    return SAPI_OK;

  } catch (...) {
    return handleException(current_exception(), err_msg);
  }
}


DWAVE_SAPI void sapi_freeFixVariablesResult(sapi_FixVariablesResult* result) {
  if (result) {
    delete[] result->fixed_variables;
    delete[] result->new_problem.elements;
    delete result;
  }
}

