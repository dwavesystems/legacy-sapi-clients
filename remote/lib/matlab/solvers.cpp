//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/foreach.hpp>

#include <mex.h>

#include <http-service.hpp>
#include <sapi-service.hpp>
#include <problem-manager.hpp>

#include "sapiremote-mex.hpp"

using std::unique_ptr;

using sapiremote::SolverPtr;

namespace {
namespace solverfields {
auto id = "id";
auto conn = "connection";
auto props = "properties";
}

const char* solverFieldsArray[] = { solverfields::id, solverfields::conn, solverfields::props };
const int numSolverFields = sizeof(solverFieldsArray) / sizeof(*solverFieldsArray);

} // namespace {anonymous}

namespace subfunctions {

void solvers(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 1) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 1) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  auto conn = getConnection(prhs[0]);
  conn->fetchSolvers();
  const auto& solvers = conn->solvers();

  auto solversArray = mxCreateStructMatrix(solvers.size(), 1, numSolverFields, solverFieldsArray);
  auto i = 0;
  BOOST_FOREACH( const auto& si, solvers ) {
    mxSetField(solversArray, i, solverfields::id, mxCreateString(si.first.c_str()));
    mxSetField(solversArray, i, solverfields::conn, mxDuplicateArray(prhs[0]));
    mxSetField(solversArray, i, solverfields::props, jsonToMatlab(si.second->properties()));
    ++i;
  }
  plhs[0] = solversArray;
}

} // namespace subfunctions

sapiremote::SolverPtr getSolver(const mxArray* handle) {
  auto idArray = mxGetField(handle, 0, solverfields::id);
  auto connArray = mxGetField(handle, 0, solverfields::conn);
  if (!idArray || !connArray) failBadHandle();

  auto id = unique_ptr<char, MxFreeDeleter>(mxArrayToString(idArray));
  if (!id) failBadHandle();

  auto conn = getConnection(connArray);
  return conn->solver(id.get());
}
