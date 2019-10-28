//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef SAPILOCAL_MATLAB_CONVERSIONS_HPP_INCLUDED
#define SAPILOCAL_MATLAB_CONVERSIONS_HPP_INCLUDED

#include <mex.h>
#include <sapi-local/sapi-local.hpp>

sapilocal::ProblemType convertProblemType(const mxArray* ptArray);
sapilocal::SparseMatrix convertSparseMatrix(const mxArray* problemArray);
sapilocal::OrangSampleParams convertOrangSampleParams(const mxArray* ospArray);
sapilocal::OrangOptimizeParams convertOrangOptimizeParams(const mxArray* ospArray);
sapilocal::OrangHeuristicParams convertOrangHeuristicParams(const mxArray* ospArray);

mxArray* convertIsingResult(const sapilocal::IsingResult& result);

#endif
