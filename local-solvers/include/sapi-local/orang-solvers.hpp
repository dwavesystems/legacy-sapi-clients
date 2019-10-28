//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef SAPILOCAL_ORANG_SOLVERS_HPP_INCLUDED
#define SAPILOCAL_ORANG_SOLVERS_HPP_INCLUDED

#include <vector>
#include <utility>

#include "problems.hpp"

namespace sapilocal {

struct OrangStructure {
  typedef int Var;
  typedef std::vector<Var> VarVector;
  typedef std::pair<Var, Var> VarPair;
  typedef std::vector<VarPair> VarPairVector;

  int numVars;
  VarVector activeVars;
  VarPairVector activeVarPairs;
  VarVector varOrder;
};

struct OrangSampleParams {
  OrangStructure s;
  int numReads;
  int maxAnswers;
  bool answerHistogram;
  double beta;
  unsigned int randomSeed;
  bool useSeed;
};

struct OrangOptimizeParams {
  OrangStructure s;
  int numReads;
  int maxAnswers;
  bool answerHistogram;
};

IsingResult orangSample(ProblemType problemType, const SparseMatrix& problem, const OrangSampleParams& params);
IsingResult orangOptimize(ProblemType problemType, const SparseMatrix& problem, const OrangOptimizeParams& params);

} // namespace sapilocal

#endif
