//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef CODING_HPP_INCLUDED
#define CODING_HPP_INCLUDED

#include <memory>
#include <utility>
#include <vector>

#include "json.hpp"
#include "types.hpp"

namespace sapiremote {

enum AnswerFormat {
  FORMAT_NONE, // answer isn't an object or has no "format" key
  FORMAT_UNKNOWN, // unrecognized "format" value
  FORMAT_QP // "qp"
};

AnswerFormat answerFormat(const json::Value& answer);

struct QpAnswer {
  std::vector<char> solutions;
  std::vector<double> energies;
  std::vector<int> numOccurrences;
};

QpAnswer decodeQpAnswer(const std::string& problemType, const json::Object& answer);

struct QpSolverInfo {
  std::vector<int> qubits;
  std::vector<std::pair<int, int>> couplers;
  std::unordered_map<int, int> qubitIndices;
};

std::unique_ptr<QpSolverInfo> extractQpSolverInfo(const json::Object& solverProps);

struct QpProblemEntry {
  int i;
  int j;
  double value;
};

typedef std::vector<QpProblemEntry> QpProblem;

json::Value encodeQpProblem(SolverPtr solver, QpProblem problem);

} // namespace sapiremote

#endif
