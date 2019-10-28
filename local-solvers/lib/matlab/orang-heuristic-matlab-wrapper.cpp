//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <mex.h>

#include <sapi-local/sapi-local.hpp>

#include "conversions.hpp"
#include "errid.hpp"

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 3) mexErrMsgIdAndTxt(errid::numArgs, "3 input arguments required");
  if (nlhs > 1) mexErrMsgIdAndTxt(errid::numArgs, "too many output arguments");

  try {
    auto problemType = convertProblemType(prhs[0]);
    auto problem = convertSparseMatrix(prhs[1]);
    auto params = convertOrangHeuristicParams(prhs[2]);
	plhs[0] = convertIsingResult(sapilocal::isingHeuristic(problemType, problem, params));

  } catch (sapilocal::Exception& e) {
    mexErrMsgIdAndTxt(errid::solveFailed, "%s", e.what());
  }
}
