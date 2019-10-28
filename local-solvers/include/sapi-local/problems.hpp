//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef SAPILOCAL_PROBLEMS_HPP_INCLUDED
#define SAPILOCAL_PROBLEMS_HPP_INCLUDED

#include <vector>
#include <utility>

namespace sapilocal {

enum ProblemType { PROBLEM_ISING, PROBLEM_QUBO };

struct MatrixEntry {
  int i;
  int j;
  double value;
};

inline MatrixEntry makeMatrixEntry(int i, int j, double value) {
  MatrixEntry e = {i, j, value};
  return e;
}

typedef std::vector<MatrixEntry> SparseMatrix;

struct IsingResult {
  static const signed char unusedVariable;
  std::vector<double> energies;
  std::vector<signed char> solutions;
  std::vector<int> numOccurrences;
};

} // namespace sapilocal

#endif
