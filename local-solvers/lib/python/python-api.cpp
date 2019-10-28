//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <sapi-local/sapi-local.hpp>

#include "python-api.hpp"

sapilocal::IsingResult orang_sample(
  sapilocal::ProblemType problem_type,
  const sapilocal::SparseMatrix& problem,
  const sapilocal::OrangSampleParams& params) {

  return sapilocal::orangSample(problem_type, problem, params);
}

sapilocal::IsingResult orang_optimize(
  sapilocal::ProblemType problem_type,
  const sapilocal::SparseMatrix& problem,
  const sapilocal::OrangOptimizeParams& params) {

  return sapilocal::orangOptimize(problem_type, problem, params);
}

sapilocal::IsingResult orang_heuristic(
  sapilocal::ProblemType problem_type,
  const sapilocal::SparseMatrix& problem,
  const sapilocal::OrangHeuristicParams& params) {

  return sapilocal::isingHeuristic(problem_type, problem, params);
}
