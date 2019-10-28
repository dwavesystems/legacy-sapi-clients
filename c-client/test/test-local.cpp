//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <algorithm>
#include <string>
#include <set>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sapi-local/sapi-local.hpp>

#include <local.hpp>

#include "test.hpp"

using std::find;
using std::make_pair;
using std::pair;
using std::string;
using std::set;
using std::sort;
using std::vector;

using sapi::LocalConnection;
using sapi::LocalC4OptimizeSolver;
using sapi::LocalC4SampleSolver;
using sapi::LocalIsingHeuristicSolver;


namespace {

const auto isingRawResult = sapilocal::IsingResult{
    {-999.5, -333, 123.25},
    {-1, -1, -1, 3, 1, 1, 1, 3, -1, 1, -1, 3},
    {}
};

const auto isingHistResult = sapilocal::IsingResult{
    {1.0, 2.0, 333.0, 44.5},
    {3, -1, -1, 3, 3, 3, 1, 1, 3, 3, 3, -1, 1, 3, 3, 3, 1, -1, 3, 3},
    {123, 321, 456, 987}
};

const auto quboRawResult = sapilocal::IsingResult{
    {111.0},
    {3, 0, 0, 3, 1, 1, 3},
    {}
};

const auto quboHistResult = sapilocal::IsingResult{
    {1234.5, 6789.0},
    {3, 0, 0, 0, 0, 0, 0, 3, 1, 1, 1, 1, 1, 1},
    {100, 4}
};

vector<sapi_ProblemEntry> lastProblem;
sapilocal::OrangSampleParams lastSampleParams;
sapilocal::OrangOptimizeParams lastOptimizeParams;
sapilocal::OrangHeuristicParams lastHeuristicParams;

vector<sapi_ProblemEntry> convertSparseMatrix(const sapilocal::SparseMatrix& m) {
  auto ret = vector<sapi_ProblemEntry>();
  for (auto it = m.begin(); it != m.end(); ++it) {
    ret.push_back(sapi_ProblemEntry{it->i, it->j, it->value});
  }
  return ret;
}

} // namespace {anonymous}

namespace sapilocal {

IsingResult orangSample(ProblemType problemType, const SparseMatrix& problem, const OrangSampleParams& params) {
  lastProblem = convertSparseMatrix(problem);
  lastSampleParams = params;
  switch (problemType) {
    case PROBLEM_ISING: return params.answerHistogram ? isingHistResult : isingRawResult;
    case PROBLEM_QUBO: return params.answerHistogram ? quboHistResult : quboRawResult;
    default: throw std::runtime_error("bad problemType");
  }
}

IsingResult orangOptimize(ProblemType problemType, const SparseMatrix& problem, const OrangOptimizeParams& params) {
  lastProblem = convertSparseMatrix(problem);
  lastOptimizeParams = params;
  switch (problemType) {
    case PROBLEM_ISING: return params.answerHistogram ? isingHistResult : isingRawResult;
    case PROBLEM_QUBO: return params.answerHistogram ? quboHistResult : quboRawResult;
    default: throw std::runtime_error("bad problemType");
  }
}

IsingResult isingHeuristic(ProblemType problemType, const SparseMatrix& problem, const OrangHeuristicParams& params) {
  lastProblem = convertSparseMatrix(problem);
  lastHeuristicParams = params;
  switch (problemType) {
    case PROBLEM_ISING: return isingRawResult;
    case PROBLEM_QUBO: return quboRawResult;
    default: throw std::runtime_error("bad problemType");
  }
}

} // namespace sapilocal


TEST(LocalSolversTest, Connection) {
  const auto validNames = set<string>{"c4-sw_optimize", "c4-sw_sample", "ising-heuristic"};

  GlobalState gs;
  LocalConnection conn;
  auto names = conn.solverNames();

  ASSERT_TRUE(!!names);
  ASSERT_TRUE(!!names[0]);
  ASSERT_TRUE(!!names[1]);
  ASSERT_TRUE(!!names[2]);
  ASSERT_FALSE(!!names[3]);

  auto namesSet = set<string>{names[0], names[1], names[2]};
  EXPECT_EQ(validNames, namesSet);

  EXPECT_TRUE(!!conn.getSolver("c4-sw_optimize"));
  EXPECT_TRUE(!!conn.getSolver("c4-sw_sample"));
  EXPECT_TRUE(!!conn.getSolver("ising-heuristic"));
  EXPECT_FALSE(!!conn.getSolver("nope"));
}


TEST(LocalSolversTest, ConnectionApi) {
  const auto validNames = set<string>{"c4-sw_optimize", "c4-sw_sample", "ising-heuristic"};

  GlobalState gs;
  auto conn = sapi_localConnection();
  auto names = sapi_listSolvers(conn);

  ASSERT_TRUE(!!names);
  ASSERT_TRUE(!!names[0]);
  ASSERT_TRUE(!!names[1]);
  ASSERT_TRUE(!!names[2]);
  ASSERT_FALSE(!!names[3]);

  auto namesSet = set<string>{names[0], names[1], names[2]};
  EXPECT_EQ(validNames, namesSet);

  EXPECT_TRUE(!!sapi_getSolver(conn, "c4-sw_optimize"));
  EXPECT_TRUE(!!sapi_getSolver(conn, "c4-sw_sample"));
  EXPECT_TRUE(!!sapi_getSolver(conn, "ising-heuristic"));
  EXPECT_FALSE(!!sapi_getSolver(conn, "nope"));
}


TEST(LocalSolversTest, FreeNoOp) {
  GlobalState gs;
  auto conn = sapi_localConnection();
  EXPECT_TRUE(!!conn);
  sapi_freeConnection(conn);
  conn = sapi_localConnection();
  EXPECT_TRUE(!!conn);
  sapi_freeConnection(conn);
}


TEST(LocalSolversTest, OptimizeParamsAnswerMode) {
  auto p = sapi_Problem();
  auto solver = LocalC4OptimizeSolver();
  auto params = SAPI_SW_OPTIMIZE_SOLVER_DEFAULT_PARAMETERS;
  auto solveParams = reinterpret_cast<const sapi_SolverParameters*>(&params);

  params.answer_mode = SAPI_ANSWER_MODE_HISTOGRAM;
  solver.solve(SAPI_PROBLEM_TYPE_ISING, &p, solveParams);
  EXPECT_TRUE(lastOptimizeParams.answerHistogram);

  params.answer_mode = SAPI_ANSWER_MODE_RAW;
  solver.solve(SAPI_PROBLEM_TYPE_ISING, &p, solveParams);
  EXPECT_FALSE(lastOptimizeParams.answerHistogram);
}


TEST(LocalSolversTest, OptimizeParamsRest) {
  auto p = sapi_Problem();
  auto solver = LocalC4OptimizeSolver();
  auto params = SAPI_SW_OPTIMIZE_SOLVER_DEFAULT_PARAMETERS;
  params.num_reads = 98765;
  params.max_answers = 333444;
  solver.solve(SAPI_PROBLEM_TYPE_ISING, &p, reinterpret_cast<const sapi_SolverParameters*>(&params));
  EXPECT_EQ(params.num_reads, lastOptimizeParams.numReads);
  EXPECT_EQ(params.max_answers, lastOptimizeParams.maxAnswers);
}


TEST(LocalSolversTest, OptimizeParamsBadType) {
  auto p = sapi_Problem();
  auto solver = LocalC4OptimizeSolver();
  auto params = sapi_SolverParameters{-999};
  EXPECT_THROW(solver.solve(SAPI_PROBLEM_TYPE_ISING, &p, &params), sapi::InvalidParameterException);
}


TEST(LocalSolversTest, OptimizeProblem) {
  auto params = reinterpret_cast<const sapi_SolverParameters*>(&SAPI_SW_OPTIMIZE_SOLVER_DEFAULT_PARAMETERS);
  auto problemElts = vector<sapi_ProblemEntry>{{0, 0, 1.0}, {12, 34, -6.5}, {666, 777, 1234.5}};
  auto problem = sapi_Problem{problemElts.data(), problemElts.size()};
  auto solver = LocalC4OptimizeSolver();
  solver.solve(SAPI_PROBLEM_TYPE_ISING, &problem, params);

  EXPECT_EQ(problemElts, lastProblem);
}


TEST(LocalSolversTest, OptimizeIsingRaw) {
  auto params = SAPI_SW_OPTIMIZE_SOLVER_DEFAULT_PARAMETERS;
  params.answer_mode = SAPI_ANSWER_MODE_RAW;
  auto problem = sapi_Problem{0, 0};
  auto solver = LocalC4OptimizeSolver();
  auto answer = solver.solve(SAPI_PROBLEM_TYPE_ISING, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(3, answer->num_solutions);
  EXPECT_EQ(4, answer->solution_len);
  auto energies = vector<double>(answer->energies, answer->energies + answer->num_solutions);
  auto solutions = vector<signed char>(answer->solutions,
      answer->solutions + answer->num_solutions * answer->solution_len);
  EXPECT_EQ(isingRawResult.energies, energies);
  EXPECT_EQ(isingRawResult.solutions, solutions);
  EXPECT_FALSE(!!answer->num_occurrences);
  EXPECT_EQ(-1, answer->timing.anneal_time_per_run);
  EXPECT_EQ(-1, answer->timing.post_processing_overhead_time);
  EXPECT_EQ(-1, answer->timing.readout_time_per_run);
  EXPECT_EQ(-1, answer->timing.run_time_chip);
  EXPECT_EQ(-1, answer->timing.qpu_access_time);
  EXPECT_EQ(-1, answer->timing.qpu_programming_time);
  EXPECT_EQ(-1, answer->timing.qpu_sampling_time);
  EXPECT_EQ(-1, answer->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(-1, answer->timing.total_post_processing_time);
  EXPECT_EQ(-1, answer->timing.total_real_time);
}


TEST(LocalSolversTest, OptimizeIsingHist) {
  auto params = SAPI_SW_OPTIMIZE_SOLVER_DEFAULT_PARAMETERS;
  params.answer_mode = SAPI_ANSWER_MODE_HISTOGRAM;
  auto problem = sapi_Problem{0, 0};
  auto solver = LocalC4OptimizeSolver();
  auto answer = solver.solve(SAPI_PROBLEM_TYPE_ISING, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(4, answer->num_solutions);
  EXPECT_EQ(5, answer->solution_len);
  auto energies = vector<double>(answer->energies, answer->energies + answer->num_solutions);
  auto solutions = vector<signed char>(answer->solutions,
      answer->solutions + answer->num_solutions * answer->solution_len);
  auto numOcc = vector<int>(answer->num_occurrences, answer->num_occurrences + answer->num_solutions);
  EXPECT_EQ(isingHistResult.energies, energies);
  EXPECT_EQ(isingHistResult.solutions, solutions);
  EXPECT_EQ(isingHistResult.numOccurrences, numOcc);
  EXPECT_EQ(-1, answer->timing.anneal_time_per_run);
  EXPECT_EQ(-1, answer->timing.post_processing_overhead_time);
  EXPECT_EQ(-1, answer->timing.readout_time_per_run);
  EXPECT_EQ(-1, answer->timing.run_time_chip);
  EXPECT_EQ(-1, answer->timing.qpu_access_time);
  EXPECT_EQ(-1, answer->timing.qpu_programming_time);
  EXPECT_EQ(-1, answer->timing.qpu_sampling_time);
  EXPECT_EQ(-1, answer->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(-1, answer->timing.total_post_processing_time);
  EXPECT_EQ(-1, answer->timing.total_real_time);
}


TEST(LocalSolversTest, OptimizeQuboRaw) {
  auto params = SAPI_SW_OPTIMIZE_SOLVER_DEFAULT_PARAMETERS;
  params.answer_mode = SAPI_ANSWER_MODE_RAW;
  auto problem = sapi_Problem{0, 0};
  auto solver = LocalC4OptimizeSolver();
  auto answer = solver.solve(SAPI_PROBLEM_TYPE_QUBO, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(1, answer->num_solutions);
  EXPECT_EQ(7, answer->solution_len);
  auto energies = vector<double>(answer->energies, answer->energies + answer->num_solutions);
  auto solutions = vector<signed char>(answer->solutions,
      answer->solutions + answer->num_solutions * answer->solution_len);
  EXPECT_EQ(quboRawResult.energies, energies);
  EXPECT_EQ(quboRawResult.solutions, solutions);
  EXPECT_FALSE(!!answer->num_occurrences);
  EXPECT_EQ(-1, answer->timing.anneal_time_per_run);
  EXPECT_EQ(-1, answer->timing.post_processing_overhead_time);
  EXPECT_EQ(-1, answer->timing.readout_time_per_run);
  EXPECT_EQ(-1, answer->timing.run_time_chip);
  EXPECT_EQ(-1, answer->timing.qpu_access_time);
  EXPECT_EQ(-1, answer->timing.qpu_programming_time);
  EXPECT_EQ(-1, answer->timing.qpu_sampling_time);
  EXPECT_EQ(-1, answer->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(-1, answer->timing.total_post_processing_time);
  EXPECT_EQ(-1, answer->timing.total_real_time);
}


TEST(LocalSolversTest, OptimizeQuboHist) {
  auto params = SAPI_SW_OPTIMIZE_SOLVER_DEFAULT_PARAMETERS;
  params.answer_mode = SAPI_ANSWER_MODE_HISTOGRAM;
  auto problem = sapi_Problem{0, 0};
  auto solver = LocalC4OptimizeSolver();
  auto answer = solver.solve(SAPI_PROBLEM_TYPE_QUBO, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(2, answer->num_solutions);
  EXPECT_EQ(7, answer->solution_len);
  auto energies = vector<double>(answer->energies, answer->energies + answer->num_solutions);
  auto solutions = vector<signed char>(answer->solutions,
      answer->solutions + answer->num_solutions * answer->solution_len);
  auto numOcc = vector<int>(answer->num_occurrences, answer->num_occurrences + answer->num_solutions);
  EXPECT_EQ(quboHistResult.energies, energies);
  EXPECT_EQ(quboHistResult.solutions, solutions);
  EXPECT_EQ(quboHistResult.numOccurrences, numOcc);
  EXPECT_EQ(-1, answer->timing.anneal_time_per_run);
  EXPECT_EQ(-1, answer->timing.post_processing_overhead_time);
  EXPECT_EQ(-1, answer->timing.readout_time_per_run);
  EXPECT_EQ(-1, answer->timing.run_time_chip);
  EXPECT_EQ(-1, answer->timing.qpu_access_time);
  EXPECT_EQ(-1, answer->timing.qpu_programming_time);
  EXPECT_EQ(-1, answer->timing.qpu_sampling_time);
  EXPECT_EQ(-1, answer->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(-1, answer->timing.total_post_processing_time);
  EXPECT_EQ(-1, answer->timing.total_real_time);
}


TEST(LocalSolversTest, SampleParamsAnswerMode) {
  auto p = sapi_Problem();
  auto solver = LocalC4SampleSolver();
  auto params = SAPI_SW_SAMPLE_SOLVER_DEFAULT_PARAMETERS;
  auto solveParams = reinterpret_cast<const sapi_SolverParameters*>(&params);

  params.answer_mode = SAPI_ANSWER_MODE_HISTOGRAM;
  solver.solve(SAPI_PROBLEM_TYPE_ISING, &p, solveParams);
  EXPECT_TRUE(lastSampleParams.answerHistogram);

  params.answer_mode = SAPI_ANSWER_MODE_RAW;
  solver.solve(SAPI_PROBLEM_TYPE_ISING, &p, solveParams);
  EXPECT_FALSE(lastSampleParams.answerHistogram);
}


TEST(LocalSolversTest, SampleParamsRest) {
  auto p = sapi_Problem();
  auto solver = LocalC4SampleSolver();
  auto params = SAPI_SW_SAMPLE_SOLVER_DEFAULT_PARAMETERS;
  params.num_reads = 98765;
  params.max_answers = 333444;
  params.beta = 1234.5;
  params.random_seed = 1324;
  params.use_random_seed = 1;
  solver.solve(SAPI_PROBLEM_TYPE_ISING, &p, reinterpret_cast<const sapi_SolverParameters*>(&params));
  EXPECT_EQ(params.num_reads, lastSampleParams.numReads);
  EXPECT_EQ(params.max_answers, lastSampleParams.maxAnswers);
  EXPECT_EQ(params.beta, lastSampleParams.beta);
  EXPECT_EQ(params.random_seed, lastSampleParams.randomSeed);
  EXPECT_EQ(params.use_random_seed != 0, lastSampleParams.useSeed);
}


TEST(LocalSolversTest, SampleParamsBadType) {
  auto p = sapi_Problem();
  auto solver = LocalC4SampleSolver();
  auto params = sapi_SolverParameters{-999};
  EXPECT_THROW(solver.solve(SAPI_PROBLEM_TYPE_ISING, &p, &params), sapi::InvalidParameterException);
}


TEST(LocalSolversTest, SampleProblem) {
  auto params = reinterpret_cast<const sapi_SolverParameters*>(&SAPI_SW_SAMPLE_SOLVER_DEFAULT_PARAMETERS);
  auto problemElts = vector<sapi_ProblemEntry>{{0, 0, 1.0}, {12, 34, -6.5}, {666, 777, 1234.5}};
  auto problem = sapi_Problem{problemElts.data(), problemElts.size()};
  auto solver = LocalC4SampleSolver();
  solver.solve(SAPI_PROBLEM_TYPE_ISING, &problem, params);

  EXPECT_EQ(problemElts, lastProblem);
}


TEST(LocalSolversTest, SampleIsingRaw) {
  auto params = SAPI_SW_SAMPLE_SOLVER_DEFAULT_PARAMETERS;
  params.answer_mode = SAPI_ANSWER_MODE_RAW;
  auto problem = sapi_Problem{0, 0};
  auto solver = LocalC4SampleSolver();
  auto answer = solver.solve(SAPI_PROBLEM_TYPE_ISING, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(3, answer->num_solutions);
  EXPECT_EQ(4, answer->solution_len);
  auto energies = vector<double>(answer->energies, answer->energies + answer->num_solutions);
  auto solutions = vector<signed char>(answer->solutions,
      answer->solutions + answer->num_solutions * answer->solution_len);
  EXPECT_EQ(isingRawResult.energies, energies);
  EXPECT_EQ(isingRawResult.solutions, solutions);
  EXPECT_FALSE(!!answer->num_occurrences);
  EXPECT_EQ(-1, answer->timing.anneal_time_per_run);
  EXPECT_EQ(-1, answer->timing.post_processing_overhead_time);
  EXPECT_EQ(-1, answer->timing.readout_time_per_run);
  EXPECT_EQ(-1, answer->timing.run_time_chip);
  EXPECT_EQ(-1, answer->timing.qpu_access_time);
  EXPECT_EQ(-1, answer->timing.qpu_programming_time);
  EXPECT_EQ(-1, answer->timing.qpu_sampling_time);
  EXPECT_EQ(-1, answer->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(-1, answer->timing.total_post_processing_time);
  EXPECT_EQ(-1, answer->timing.total_real_time);
}


TEST(LocalSolversTest, SampleIsingHist) {
  auto params = SAPI_SW_SAMPLE_SOLVER_DEFAULT_PARAMETERS;
  params.answer_mode = SAPI_ANSWER_MODE_HISTOGRAM;
  auto problem = sapi_Problem{0, 0};
  auto solver = LocalC4SampleSolver();
  auto answer = solver.solve(SAPI_PROBLEM_TYPE_ISING, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(4, answer->num_solutions);
  EXPECT_EQ(5, answer->solution_len);
  auto energies = vector<double>(answer->energies, answer->energies + answer->num_solutions);
  auto solutions = vector<signed char>(answer->solutions,
      answer->solutions + answer->num_solutions * answer->solution_len);
  auto numOcc = vector<int>(answer->num_occurrences, answer->num_occurrences + answer->num_solutions);
  EXPECT_EQ(isingHistResult.energies, energies);
  EXPECT_EQ(isingHistResult.solutions, solutions);
  EXPECT_EQ(isingHistResult.numOccurrences, numOcc);
  EXPECT_EQ(-1, answer->timing.anneal_time_per_run);
  EXPECT_EQ(-1, answer->timing.post_processing_overhead_time);
  EXPECT_EQ(-1, answer->timing.readout_time_per_run);
  EXPECT_EQ(-1, answer->timing.run_time_chip);
  EXPECT_EQ(-1, answer->timing.qpu_access_time);
  EXPECT_EQ(-1, answer->timing.qpu_programming_time);
  EXPECT_EQ(-1, answer->timing.qpu_sampling_time);
  EXPECT_EQ(-1, answer->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(-1, answer->timing.total_post_processing_time);
  EXPECT_EQ(-1, answer->timing.total_real_time);
}


TEST(LocalSolversTest, SampleQuboRaw) {
  auto params = SAPI_SW_SAMPLE_SOLVER_DEFAULT_PARAMETERS;
  params.answer_mode = SAPI_ANSWER_MODE_RAW;
  auto problem = sapi_Problem{0, 0};
  auto solver = LocalC4SampleSolver();
  auto answer = solver.solve(SAPI_PROBLEM_TYPE_QUBO, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(1, answer->num_solutions);
  EXPECT_EQ(7, answer->solution_len);
  auto energies = vector<double>(answer->energies, answer->energies + answer->num_solutions);
  auto solutions = vector<signed char>(answer->solutions,
      answer->solutions + answer->num_solutions * answer->solution_len);
  EXPECT_EQ(quboRawResult.energies, energies);
  EXPECT_EQ(quboRawResult.solutions, solutions);
  EXPECT_FALSE(!!answer->num_occurrences);
  EXPECT_EQ(-1, answer->timing.anneal_time_per_run);
  EXPECT_EQ(-1, answer->timing.post_processing_overhead_time);
  EXPECT_EQ(-1, answer->timing.readout_time_per_run);
  EXPECT_EQ(-1, answer->timing.run_time_chip);
  EXPECT_EQ(-1, answer->timing.qpu_access_time);
  EXPECT_EQ(-1, answer->timing.qpu_programming_time);
  EXPECT_EQ(-1, answer->timing.qpu_sampling_time);
  EXPECT_EQ(-1, answer->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(-1, answer->timing.total_post_processing_time);
  EXPECT_EQ(-1, answer->timing.total_real_time);
}


TEST(LocalSolversTest, SampleQuboHist) {
  auto params = SAPI_SW_SAMPLE_SOLVER_DEFAULT_PARAMETERS;
  params.answer_mode = SAPI_ANSWER_MODE_HISTOGRAM;
  auto problem = sapi_Problem{0, 0};
  auto solver = LocalC4SampleSolver();
  auto answer = solver.solve(SAPI_PROBLEM_TYPE_QUBO, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(2, answer->num_solutions);
  EXPECT_EQ(7, answer->solution_len);
  auto energies = vector<double>(answer->energies, answer->energies + answer->num_solutions);
  auto solutions = vector<signed char>(answer->solutions,
      answer->solutions + answer->num_solutions * answer->solution_len);
  auto numOcc = vector<int>(answer->num_occurrences, answer->num_occurrences + answer->num_solutions);
  EXPECT_EQ(quboHistResult.energies, energies);
  EXPECT_EQ(quboHistResult.solutions, solutions);
  EXPECT_EQ(quboHistResult.numOccurrences, numOcc);
  EXPECT_EQ(-1, answer->timing.anneal_time_per_run);
  EXPECT_EQ(-1, answer->timing.post_processing_overhead_time);
  EXPECT_EQ(-1, answer->timing.readout_time_per_run);
  EXPECT_EQ(-1, answer->timing.run_time_chip);
  EXPECT_EQ(-1, answer->timing.qpu_access_time);
  EXPECT_EQ(-1, answer->timing.qpu_programming_time);
  EXPECT_EQ(-1, answer->timing.qpu_sampling_time);
  EXPECT_EQ(-1, answer->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(-1, answer->timing.total_post_processing_time);
  EXPECT_EQ(-1, answer->timing.total_real_time);
}


TEST(LocalSolversTest, HeuristicParams) {
  auto p = sapi_Problem();
  auto solver = LocalIsingHeuristicSolver();
  auto params = SAPI_SW_HEURISTIC_SOLVER_DEFAULT_PARAMETERS;
  params.random_seed = 123;
  params.use_random_seed = 1;
  params.iteration_limit = 234;
  params.local_stuck_limit = 345;
  params.max_bit_flip_prob = 456.0;
  params.max_local_complexity = 567;
  params.min_bit_flip_prob = 678.0;
  params.num_perturbed_copies = 789;
  params.time_limit_seconds = 890.0;
  params.num_variables = 901;

  solver.solve(SAPI_PROBLEM_TYPE_ISING, &p, reinterpret_cast<const sapi_SolverParameters*>(&params));
  EXPECT_EQ(params.random_seed, lastHeuristicParams.rngSeed);
  EXPECT_EQ(params.use_random_seed != 0, lastHeuristicParams.useSeed);
  EXPECT_EQ(params.iteration_limit, lastHeuristicParams.iterationLimit);
  EXPECT_EQ(params.local_stuck_limit, lastHeuristicParams.noProgressLimit);
  EXPECT_EQ(params.max_bit_flip_prob, lastHeuristicParams.maxBitFlipProb);
  EXPECT_EQ(params.max_local_complexity, lastHeuristicParams.maxComplexity);
  EXPECT_EQ(params.min_bit_flip_prob, lastHeuristicParams.minBitFlipProb);
  EXPECT_EQ(params.num_perturbed_copies, lastHeuristicParams.numPerturbedCopies);
  EXPECT_EQ(params.time_limit_seconds, lastHeuristicParams.timeLimitSeconds);
  EXPECT_EQ(params.num_variables, lastHeuristicParams.numVariables);
}


TEST(LocalSolversTest, HeuristicParamsBadType) {
  auto p = sapi_Problem();
  auto solver = LocalIsingHeuristicSolver();
  auto params = sapi_SolverParameters{-999};
  EXPECT_THROW(solver.solve(SAPI_PROBLEM_TYPE_ISING, &p, &params), sapi::InvalidParameterException);
}


TEST(LocalSolversTest, HeuristicProblem) {
  auto params = reinterpret_cast<const sapi_SolverParameters*>(&SAPI_SW_HEURISTIC_SOLVER_DEFAULT_PARAMETERS);
  auto problemElts = vector<sapi_ProblemEntry>{{0, 0, 1.0}, {12, 34, -6.5}, {666, 777, 1234.5}};
  auto problem = sapi_Problem{problemElts.data(), problemElts.size()};
  auto solver = LocalIsingHeuristicSolver();
  solver.solve(SAPI_PROBLEM_TYPE_ISING, &problem, params);

  EXPECT_EQ(problemElts, lastProblem);
}


TEST(LocalSolversTest, HeuristicIsing) {
  auto params = SAPI_SW_HEURISTIC_SOLVER_DEFAULT_PARAMETERS;
  auto problem = sapi_Problem{0, 0};
  auto solver = LocalIsingHeuristicSolver();
  auto answer = solver.solve(SAPI_PROBLEM_TYPE_ISING, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(3, answer->num_solutions);
  EXPECT_EQ(4, answer->solution_len);
  auto energies = vector<double>(answer->energies, answer->energies + answer->num_solutions);
  auto solutions = vector<signed char>(answer->solutions,
      answer->solutions + answer->num_solutions * answer->solution_len);
  EXPECT_EQ(isingRawResult.energies, energies);
  EXPECT_EQ(isingRawResult.solutions, solutions);
  EXPECT_FALSE(!!answer->num_occurrences);
  EXPECT_EQ(-1, answer->timing.anneal_time_per_run);
  EXPECT_EQ(-1, answer->timing.post_processing_overhead_time);
  EXPECT_EQ(-1, answer->timing.readout_time_per_run);
  EXPECT_EQ(-1, answer->timing.run_time_chip);
  EXPECT_EQ(-1, answer->timing.qpu_access_time);
  EXPECT_EQ(-1, answer->timing.qpu_programming_time);
  EXPECT_EQ(-1, answer->timing.qpu_sampling_time);
  EXPECT_EQ(-1, answer->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(-1, answer->timing.total_post_processing_time);
  EXPECT_EQ(-1, answer->timing.total_real_time);
}



TEST(LocalSolversTest, HeuristicQuboRaw) {
  auto params = SAPI_SW_HEURISTIC_SOLVER_DEFAULT_PARAMETERS;
  auto problem = sapi_Problem{0, 0};
  auto solver = LocalIsingHeuristicSolver();
  auto answer = solver.solve(SAPI_PROBLEM_TYPE_QUBO, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(1, answer->num_solutions);
  EXPECT_EQ(7, answer->solution_len);
  auto energies = vector<double>(answer->energies, answer->energies + answer->num_solutions);
  auto solutions = vector<signed char>(answer->solutions,
      answer->solutions + answer->num_solutions * answer->solution_len);
  EXPECT_EQ(quboRawResult.energies, energies);
  EXPECT_EQ(quboRawResult.solutions, solutions);
  EXPECT_FALSE(!!answer->num_occurrences);
  EXPECT_EQ(-1, answer->timing.anneal_time_per_run);
  EXPECT_EQ(-1, answer->timing.post_processing_overhead_time);
  EXPECT_EQ(-1, answer->timing.readout_time_per_run);
  EXPECT_EQ(-1, answer->timing.run_time_chip);
  EXPECT_EQ(-1, answer->timing.qpu_access_time);
  EXPECT_EQ(-1, answer->timing.qpu_programming_time);
  EXPECT_EQ(-1, answer->timing.qpu_sampling_time);
  EXPECT_EQ(-1, answer->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(-1, answer->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(-1, answer->timing.total_post_processing_time);
  EXPECT_EQ(-1, answer->timing.total_real_time);
}



TEST(LocalSolversTest, OptimizeProps) {
  auto solver = LocalC4OptimizeSolver();
  auto props = solver.properties();

  EXPECT_FALSE(!!props->ising_ranges);
  EXPECT_FALSE(!!props->anneal_offset);

  ASSERT_TRUE(!!props->supported_problem_types);
  ASSERT_EQ(2, props->supported_problem_types->len);
  auto spt0 = string(props->supported_problem_types->elements[0]);
  auto spt1 = string(props->supported_problem_types->elements[1]);
  EXPECT_TRUE(spt0 != spt1);
  EXPECT_TRUE(spt0 == "ising" || spt0 == "qubo");
  EXPECT_TRUE(spt1 == "ising" || spt1 == "qubo");

  auto qs = props->quantum_solver;
  ASSERT_TRUE(!!qs);
  EXPECT_EQ(128, qs->num_qubits);
  EXPECT_EQ(128, qs->qubits_len);
  vector<int> expectedQubits;
  for (auto i = 0; i < 128; ++i) expectedQubits.push_back(i);
  EXPECT_EQ(expectedQubits, vector<int>(qs->qubits, qs->qubits + qs->qubits_len));
  EXPECT_EQ(352, qs->couplers_len);
  vector<pair<int, int>> expectedCouplers;
  for (auto i = 0; i < 128; i += 8) {
    for (auto j = i; j < i + 4; ++j) {
      for (auto k = i + 4; k < i + 8; ++k) {
        expectedCouplers.push_back(make_pair(j, k));
      }
    }
  }
  for (auto i = 0; i < 128; i += 32) {
    for (auto j = 0; j < 24; j += 8) {
      for (auto k = 4; k < 8; ++k) {
        expectedCouplers.push_back(make_pair(i + j + k, i + j + k + 8));
      }
    }
  }
  for (auto i = 0; i < 96; i += 32) {
    for (auto j = 0; j < 32; j += 8) {
      for (auto k = 0; k < 4; ++k) {
        expectedCouplers.push_back(make_pair(i + j + k, i + j + k + 32));
      }
    }
  }
  sort(expectedCouplers.begin(), expectedCouplers.end());
  vector<pair<int, int>> couplers;
  for (size_t i = 0; i < qs->couplers_len; ++i) {
    couplers.push_back(make_pair(qs->couplers[i].q1, qs->couplers[i].q2));
  }
  EXPECT_EQ(expectedCouplers, couplers);

  auto expectedParams = vector<string>{"answer_mode", "max_answers", "num_reads"};
  sort(expectedParams.begin(), expectedParams.end());
  auto ps = props->parameters;
  ASSERT_TRUE(!!ps);
  auto params = vector<string>(ps->elements, ps->elements + ps->len);
  EXPECT_EQ(expectedParams, params);
}



TEST(LocalSolversTest, SampleProps) {
  auto solver = LocalC4SampleSolver();
  auto props = solver.properties();

  EXPECT_FALSE(!!props->ising_ranges);
  EXPECT_FALSE(!!props->anneal_offset);

  ASSERT_TRUE(!!props->supported_problem_types);
  ASSERT_EQ(2, props->supported_problem_types->len);
  auto spt0 = string(props->supported_problem_types->elements[0]);
  auto spt1 = string(props->supported_problem_types->elements[1]);
  EXPECT_TRUE(spt0 != spt1);
  EXPECT_TRUE(spt0 == "ising" || spt0 == "qubo");
  EXPECT_TRUE(spt1 == "ising" || spt1 == "qubo");

  auto qs = props->quantum_solver;
  ASSERT_TRUE(!!qs);
  EXPECT_EQ(128, qs->num_qubits);
  EXPECT_EQ(128, qs->qubits_len);
  vector<int> expectedQubits;
  for (auto i = 0; i < 128; ++i) expectedQubits.push_back(i);
  EXPECT_EQ(expectedQubits, vector<int>(qs->qubits, qs->qubits + qs->qubits_len));
  EXPECT_EQ(352, qs->couplers_len);
  vector<pair<int, int>> expectedCouplers;
  for (auto i = 0; i < 128; i += 8) {
    for (auto j = i; j < i + 4; ++j) {
      for (auto k = i + 4; k < i + 8; ++k) {
        expectedCouplers.push_back(make_pair(j, k));
      }
    }
  }
  for (auto i = 0; i < 128; i += 32) {
    for (auto j = 0; j < 24; j += 8) {
      for (auto k = 4; k < 8; ++k) {
        expectedCouplers.push_back(make_pair(i + j + k, i + j + k + 8));
      }
    }
  }
  for (auto i = 0; i < 96; i += 32) {
    for (auto j = 0; j < 32; j += 8) {
      for (auto k = 0; k < 4; ++k) {
        expectedCouplers.push_back(make_pair(i + j + k, i + j + k + 32));
      }
    }
  }
  sort(expectedCouplers.begin(), expectedCouplers.end());
  vector<pair<int, int>> couplers;
  for (size_t i = 0; i < qs->couplers_len; ++i) {
    couplers.push_back(make_pair(qs->couplers[i].q1, qs->couplers[i].q2));
  }
  EXPECT_EQ(expectedCouplers, couplers);

  auto expectedParams = vector<string>{
      "answer_mode", "beta", "max_answers", "num_reads", "use_random_seed", "random_seed"
  };
  sort(expectedParams.begin(), expectedParams.end());
  auto ps = props->parameters;
  ASSERT_TRUE(!!ps);
  auto params = vector<string>(ps->elements, ps->elements + ps->len);
  EXPECT_EQ(expectedParams, params);
}



TEST(LocalSolversTest, HeuristicProps) {
  auto solver = LocalIsingHeuristicSolver();
  auto props = solver.properties();

  EXPECT_FALSE(!!props->quantum_solver);
  EXPECT_FALSE(!!props->ising_ranges);
  EXPECT_FALSE(!!props->anneal_offset);

  ASSERT_TRUE(!!props->supported_problem_types);
  ASSERT_EQ(2, props->supported_problem_types->len);
  auto spt0 = string(props->supported_problem_types->elements[0]);
  auto spt1 = string(props->supported_problem_types->elements[1]);
  EXPECT_TRUE(spt0 != spt1);
  EXPECT_TRUE(spt0 == "ising" || spt0 == "qubo");
  EXPECT_TRUE(spt1 == "ising" || spt1 == "qubo");

  auto expectedParams = vector<string>{
      "iteration_limit", "max_bit_flip_prob", "max_local_complexity", "min_bit_flip_prob", "local_stuck_limit",
      "num_perturbed_copies", "num_variables", "use_random_seed", "random_seed", "time_limit_seconds"
  };
  sort(expectedParams.begin(), expectedParams.end());
  auto ps = props->parameters;
  ASSERT_TRUE(!!ps);
  auto params = vector<string>(ps->elements, ps->elements + ps->len);
  EXPECT_EQ(expectedParams, params);
}
