//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <dwave_sapi.h>
#include <sapi-impl.hpp>

using std::make_tuple;
using std::sort;
using std::tuple;
using std::vector;

namespace {

const auto magic = 9876;
const auto qubits = vector<int>{0, 1, 3};
const auto couplers = vector<sapi_Coupler>{{0, 1}, {1, 3}};

const auto qsProps = sapi_QuantumSolverProperties{
  4,
  qubits.data(), qubits.size(),
  couplers.data(), couplers.size(),
};

const auto props = sapi_SolverProperties{0, &qsProps, 0, 0, 0, 0, 0};

class SimpleSolver : public sapi_Solver {
private:
  virtual const sapi_SolverProperties* propertiesImpl() const { return &props; }

  virtual sapi::IsingResultPtr solveImpl(
      sapi_ProblemType type, const sapi_Problem *problem,
      const sapi_SolverParameters *params) const {

    if (type != SAPI_PROBLEM_TYPE_ISING) throw std::runtime_error("wrong problem type");
    if (params->parameter_unique_id != magic) throw std::runtime_error("wrong parameter type");

    auto hs = vector<double>(3);
    auto js = vector<double>(2);
    for (size_t i = 0; i < problem->len; ++i) {
      const auto& e = problem->elements[i];
      if (e.i == 0 && e.j == 0) hs[0] += e.value;
      else if (e.i == 1 && e.j == 1) hs[1] += e.value;
      else if (e.i == 3 && e.j == 3) hs[2] += e.value;
      else if (e.i == 0 && e.j == 1) js[0] += e.value;
      else if (e.i == 1 && e.j == 0) js[0] += e.value;
      else if (e.i == 1 && e.j == 3) js[1] += e.value;
      else if (e.i == 3 && e.j == 1) js[1] += e.value;
      else throw std::runtime_error("invalid problem index");
    }

    auto vals = vector<tuple<double, int>>{};
    for (auto i = 0; i < 8; ++i) {
      auto s0 = i & 1 ? 1 : -1;
      auto s1 = i & 2 ? 1 : -1;
      auto s2 = i & 4 ? 1 : -1;
      auto e = s0 * hs[0] + s1 * hs[1] + s2 * hs[2] + s0 * s1 * js[0] + s1 * s2 * js[1];
      vals.push_back(make_tuple(e, i));
    }
    sort(vals.begin(), vals.end());

    auto ret = sapi::IsingResultPtr(new sapi_IsingResult);
    ret->num_solutions = 8;
    ret->solution_len = 4;
    ret->energies = 0;
    ret->num_occurrences = 0;
    ret->solutions = new int[32];
    ret->energies = new double[8];

    for (auto i = 0; i < 8; ++i) {
      ret->energies[i] = std::get<0>(vals[i]);
      ret->solutions[4 * i + 0] = std::get<1>(vals[i]) & 1 ? 1 : -1;
      ret->solutions[4 * i + 1] = std::get<1>(vals[i]) & 2 ? 1 : -1;
      ret->solutions[4 * i + 2] = 3;
      ret->solutions[4 * i + 3] = std::get<1>(vals[i]) & 4 ? 1 : -1;
    }

    return ret;
  }

  virtual sapi::SubmittedProblemPtr submitImpl(
      sapi_ProblemType, const sapi_Problem*, const sapi_SolverParameters*) const {
    throw std::runtime_error("SimpleSolver::submitImpl() not implemented");
  }
};

sapi_Code objFunc(const int* states, size_t len, size_t numStates, void*, double* result) {
  *result = 0.0;
  size_t si = 0;
  for (size_t s = 0; s < numStates; ++s) {
    for (size_t i = 0; i < len / numStates; ++i) {
      *result += (states[si] == 1) == (i % 2 == 0) ? -1.0 : 1.0;
      ++si;
    }
  }
  return SAPI_OK;
}



} // namespace {anonymous}

TEST(QSageTest, Solve) {
  auto qsageObjFunc = sapi_QSageObjFunc{ objFunc, 0, 11 };
  char errMsg[SAPI_ERROR_MESSAGE_MAX_SIZE];

  auto solver = SimpleSolver();
  auto solverParams = sapi_SolverParameters{magic};

  auto qsageParams = SAPI_QSAGE_DEFAULT_PARAMETERS;
  qsageParams.timeout = 0.1;

  sapi_QSageResult* result;

  ASSERT_EQ(SAPI_OK, sapi_solveQSage(&qsageObjFunc, &solver, &solverParams, &qsageParams, &result, errMsg));
  sapi_freeQSageResult(result);
}
