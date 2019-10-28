//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef SAPILOCAL_PYTHON_API_HPP_INCLUDED
#define SAPILOCAL_PYTHON_API_HPP_INCLUDED

#include <sapi-local/sapi-local.hpp>

sapilocal::IsingResult orang_sample(
  sapilocal::ProblemType problem_type,
  const sapilocal::SparseMatrix& problem,
  const sapilocal::OrangSampleParams& params);

sapilocal::IsingResult orang_optimize(
  sapilocal::ProblemType problem_type,
  const sapilocal::SparseMatrix& problem,
  const sapilocal::OrangOptimizeParams& params);

sapilocal::IsingResult orang_heuristic(
  sapilocal::ProblemType problem_type,
  const sapilocal::SparseMatrix& problem,
  const sapilocal::OrangHeuristicParams& params);

#endif
