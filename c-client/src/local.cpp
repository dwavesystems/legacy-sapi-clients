//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <algorithm>

#include <boost/foreach.hpp>

#include <sapi-local/sapi-local.hpp>

#include <dwave_sapi.h>
#include <sapi-impl.hpp>
#include <local.hpp>

using std::copy;

using sapilocal::OrangStructure;

using sapi::SolverPtr;
using sapi::LocalC4OptimizeSolver;
using sapi::LocalC4SampleSolver;
using sapi::LocalIsingHeuristicSolver;
using sapi::InvalidParameterException;
using sapi::IsingResultPtr;

namespace {

namespace solvernames {
const auto sample = "c4-sw_sample";
const auto optimize = "c4-sw_optimize";
const auto heuristic = "ising-heuristic";
} // namespace solvernames

namespace props {
const char* spt[] = {"ising", "qubo"};
const auto sptProp = sapi_SupportedProblemTypeProperty{spt, sizeof(spt) / sizeof(spt[0])};
const int qubits[] = {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
     16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
     32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
     48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
     64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
     96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127
};
const sapi_Coupler couplers[] = {
    {  0,   4}, {  0,   5}, {  0,   6}, {  0,   7}, {  0,  32}, {  1,   4}, {  1,   5}, {  1,   6},
    {  1,   7}, {  1,  33}, {  2,   4}, {  2,   5}, {  2,   6}, {  2,   7}, {  2,  34}, {  3,   4},
    {  3,   5}, {  3,   6}, {  3,   7}, {  3,  35}, {  4,  12}, {  5,  13}, {  6,  14}, {  7,  15},
    {  8,  12}, {  8,  13}, {  8,  14}, {  8,  15}, {  8,  40}, {  9,  12}, {  9,  13}, {  9,  14},
    {  9,  15}, {  9,  41}, { 10,  12}, { 10,  13}, { 10,  14}, { 10,  15}, { 10,  42}, { 11,  12},
    { 11,  13}, { 11,  14}, { 11,  15}, { 11,  43}, { 12,  20}, { 13,  21}, { 14,  22}, { 15,  23},
    { 16,  20}, { 16,  21}, { 16,  22}, { 16,  23}, { 16,  48}, { 17,  20}, { 17,  21}, { 17,  22},
    { 17,  23}, { 17,  49}, { 18,  20}, { 18,  21}, { 18,  22}, { 18,  23}, { 18,  50}, { 19,  20},
    { 19,  21}, { 19,  22}, { 19,  23}, { 19,  51}, { 20,  28}, { 21,  29}, { 22,  30}, { 23,  31},
    { 24,  28}, { 24,  29}, { 24,  30}, { 24,  31}, { 24,  56}, { 25,  28}, { 25,  29}, { 25,  30},
    { 25,  31}, { 25,  57}, { 26,  28}, { 26,  29}, { 26,  30}, { 26,  31}, { 26,  58}, { 27,  28},
    { 27,  29}, { 27,  30}, { 27,  31}, { 27,  59}, { 32,  36}, { 32,  37}, { 32,  38}, { 32,  39},
    { 32,  64}, { 33,  36}, { 33,  37}, { 33,  38}, { 33,  39}, { 33,  65}, { 34,  36}, { 34,  37},
    { 34,  38}, { 34,  39}, { 34,  66}, { 35,  36}, { 35,  37}, { 35,  38}, { 35,  39}, { 35,  67},
    { 36,  44}, { 37,  45}, { 38,  46}, { 39,  47}, { 40,  44}, { 40,  45}, { 40,  46}, { 40,  47},
    { 40,  72}, { 41,  44}, { 41,  45}, { 41,  46}, { 41,  47}, { 41,  73}, { 42,  44}, { 42,  45},
    { 42,  46}, { 42,  47}, { 42,  74}, { 43,  44}, { 43,  45}, { 43,  46}, { 43,  47}, { 43,  75},
    { 44,  52}, { 45,  53}, { 46,  54}, { 47,  55}, { 48,  52}, { 48,  53}, { 48,  54}, { 48,  55},
    { 48,  80}, { 49,  52}, { 49,  53}, { 49,  54}, { 49,  55}, { 49,  81}, { 50,  52}, { 50,  53},
    { 50,  54}, { 50,  55}, { 50,  82}, { 51,  52}, { 51,  53}, { 51,  54}, { 51,  55}, { 51,  83},
    { 52,  60}, { 53,  61}, { 54,  62}, { 55,  63}, { 56,  60}, { 56,  61}, { 56,  62}, { 56,  63},
    { 56,  88}, { 57,  60}, { 57,  61}, { 57,  62}, { 57,  63}, { 57,  89}, { 58,  60}, { 58,  61},
    { 58,  62}, { 58,  63}, { 58,  90}, { 59,  60}, { 59,  61}, { 59,  62}, { 59,  63}, { 59,  91},
    { 64,  68}, { 64,  69}, { 64,  70}, { 64,  71}, { 64,  96}, { 65,  68}, { 65,  69}, { 65,  70},
    { 65,  71}, { 65,  97}, { 66,  68}, { 66,  69}, { 66,  70}, { 66,  71}, { 66,  98}, { 67,  68},
    { 67,  69}, { 67,  70}, { 67,  71}, { 67,  99}, { 68,  76}, { 69,  77}, { 70,  78}, { 71,  79},
    { 72,  76}, { 72,  77}, { 72,  78}, { 72,  79}, { 72, 104}, { 73,  76}, { 73,  77}, { 73,  78},
    { 73,  79}, { 73, 105}, { 74,  76}, { 74,  77}, { 74,  78}, { 74,  79}, { 74, 106}, { 75,  76},
    { 75,  77}, { 75,  78}, { 75,  79}, { 75, 107}, { 76,  84}, { 77,  85}, { 78,  86}, { 79,  87},
    { 80,  84}, { 80,  85}, { 80,  86}, { 80,  87}, { 80, 112}, { 81,  84}, { 81,  85}, { 81,  86},
    { 81,  87}, { 81, 113}, { 82,  84}, { 82,  85}, { 82,  86}, { 82,  87}, { 82, 114}, { 83,  84},
    { 83,  85}, { 83,  86}, { 83,  87}, { 83, 115}, { 84,  92}, { 85,  93}, { 86,  94}, { 87,  95},
    { 88,  92}, { 88,  93}, { 88,  94}, { 88,  95}, { 88, 120}, { 89,  92}, { 89,  93}, { 89,  94},
    { 89,  95}, { 89, 121}, { 90,  92}, { 90,  93}, { 90,  94}, { 90,  95}, { 90, 122}, { 91,  92},
    { 91,  93}, { 91,  94}, { 91,  95}, { 91, 123}, { 96, 100}, { 96, 101}, { 96, 102}, { 96, 103},
    { 97, 100}, { 97, 101}, { 97, 102}, { 97, 103}, { 98, 100}, { 98, 101}, { 98, 102}, { 98, 103},
    { 99, 100}, { 99, 101}, { 99, 102}, { 99, 103}, {100, 108}, {101, 109}, {102, 110}, {103, 111},
    {104, 108}, {104, 109}, {104, 110}, {104, 111}, {105, 108}, {105, 109}, {105, 110}, {105, 111},
    {106, 108}, {106, 109}, {106, 110}, {106, 111}, {107, 108}, {107, 109}, {107, 110}, {107, 111},
    {108, 116}, {109, 117}, {110, 118}, {111, 119}, {112, 116}, {112, 117}, {112, 118}, {112, 119},
    {113, 116}, {113, 117}, {113, 118}, {113, 119}, {114, 116}, {114, 117}, {114, 118}, {114, 119},
    {115, 116}, {115, 117}, {115, 118}, {115, 119}, {116, 124}, {117, 125}, {118, 126}, {119, 127},
    {120, 124}, {120, 125}, {120, 126}, {120, 127}, {121, 124}, {121, 125}, {121, 126}, {121, 127},
    {122, 124}, {122, 125}, {122, 126}, {122, 127}, {123, 124}, {123, 125}, {123, 126}, {123, 127}
};
const auto c4QpProps = sapi_QuantumSolverProperties{
    128, // num_qubits
    qubits, // qubits
    sizeof(qubits) / sizeof(qubits[0]), // qubits_len
    couplers, // couplers
    sizeof(couplers) / sizeof(couplers[0]) // couplers_len
};

const char* optParams[] = {
    "answer_mode",
    "max_answers",
    "num_reads"
};
const auto optParamProps = sapi_ParametersProperty{optParams, sizeof(optParams)/ sizeof(optParams[0])};

const char* sampParams[] = {
  "answer_mode",
  "beta",
  "max_answers",
  "num_reads",
  "random_seed",
  "use_random_seed"
};
const auto sampParamProps = sapi_ParametersProperty{sampParams, sizeof(sampParams)/ sizeof(sampParams[0])};

const char* heurParams[] = {
    "iteration_limit",
    "local_stuck_limit",
    "max_bit_flip_prob",
    "max_local_complexity",
    "min_bit_flip_prob",
    "num_perturbed_copies",
    "num_variables",
    "random_seed",
    "time_limit_seconds",
    "use_random_seed"
};
const auto heurParamProps = sapi_ParametersProperty{heurParams, sizeof(heurParams)/ sizeof(heurParams[0])};


const auto c4OptimizeSolverProperties = sapi_SolverProperties{&sptProp, &c4QpProps, 0, 0, 0, &optParamProps, 0};
const auto c4SampleSolverProperties = sapi_SolverProperties{&sptProp, &c4QpProps, 0, 0, 0, &sampParamProps, 0};
const auto heuristicSolverProperties = sapi_SolverProperties{&sptProp, 0, 0, 0, 0, &heurParamProps, 0};
} // namespace props

const int c4VarOrder[] = {
    0,  32,  64,  96,   1,  33,  65,  97,   2,  34,  66,  98,   3,  35,  67,  99,
    8,  40,  72, 104,   9,  41,  73, 105,  10,  42,  74, 106,  11,  43,  75, 107,
    16,  48,  80, 112,  17,  49,  81, 113,  18,  50,  82, 114,  19,  51,  83, 115,
    24,  56,  88, 120,  25,  57,  89, 121,  26,  58,  90, 122,  27,  59,  91, 123,
    4,   5,   6,   7,  36,  37,  38,  39,  68,  69,  70,  71, 100, 101, 102, 103,
    12,  13,  14,  15,  44,  45,  46,  47,  76,  77,  78,  79, 108, 109, 110, 111,
    20,  21,  22,  23,  52,  53,  54,  55,  84,  85,  86,  87, 116, 117, 118, 119,
    28,  29,  30,  31,  60,  61,  62,  63,  92,  93,  94,  95, 124, 125, 126, 127
};

sapilocal::ProblemType convertProblemType(sapi_ProblemType t) {
  switch (t) {
    case SAPI_PROBLEM_TYPE_ISING: return sapilocal::PROBLEM_ISING;
    case SAPI_PROBLEM_TYPE_QUBO: return sapilocal::PROBLEM_QUBO;
    default: throw InvalidParameterException("invalid problem type");
  }
}


sapilocal::SparseMatrix convertProblem(const sapi_Problem* p) {
  auto ret = sapilocal::SparseMatrix();
  for (size_t i = 0; i < p->len; ++i) {
    const auto& e = p->elements[i];
    ret.push_back(sapilocal::MatrixEntry{e.i, e.j, e.value});
  }
  return ret;
}


bool isHistogram(sapi_SolverParameterAnswerMode m) {
  switch (m) {
    case SAPI_ANSWER_MODE_HISTOGRAM: return true;
    case SAPI_ANSWER_MODE_RAW: return false;
    default: throw InvalidParameterException("invalid answer_mode");
  }
}


OrangStructure c4OrangStructure() {
  // this is bad: unused variables not reported properly
  // continue doing it this way to preserve existing behaviour
  auto s = OrangStructure();
  s.numVars = 128;
  s.activeVars.assign(props::qubits, props::qubits + props::c4QpProps.qubits_len);
  s.varOrder.assign(c4VarOrder, c4VarOrder + sizeof(c4VarOrder) / sizeof(c4VarOrder[0]));
  for (size_t i = 0; i < props::c4QpProps.couplers_len; ++i) {
    s.activeVarPairs.push_back(OrangStructure::VarPair{props::couplers[i].q1, props::couplers[i].q2});
  }
  return s;
}


sapilocal::OrangOptimizeParams convertOptimizeParams(const sapi_SolverParameters* p) {
  if (p->parameter_unique_id != SAPI_SW_OPTIMIZE_SOLVER_DEFAULT_PARAMETERS.parameter_unique_id) {
    throw InvalidParameterException("this solver requires parameters of type sapi_SwOptimizeSolverParameters");
  }
  const auto& optP = *reinterpret_cast<const sapi_SwOptimizeSolverParameters*>(p);
  sapilocal::OrangOptimizeParams ret;
  ret.answerHistogram = isHistogram(optP.answer_mode);
  ret.maxAnswers = optP.max_answers;
  ret.numReads = optP.num_reads;
  ret.s = c4OrangStructure();
  return ret;
}


sapilocal::OrangSampleParams convertSampleParams(const sapi_SolverParameters* p) {
  if (p->parameter_unique_id != SAPI_SW_SAMPLE_SOLVER_DEFAULT_PARAMETERS.parameter_unique_id) {
    throw InvalidParameterException("this solver requires parameters of type sapi_SwSampleSolverParameters");
  }
  const auto& optP = *reinterpret_cast<const sapi_SwSampleSolverParameters*>(p);
  sapilocal::OrangSampleParams ret;
  ret.answerHistogram = isHistogram(optP.answer_mode);
  ret.maxAnswers = optP.max_answers;
  ret.numReads = optP.num_reads;
  ret.beta = optP.beta;
  ret.randomSeed = optP.random_seed;
  ret.useSeed = optP.use_random_seed != 0;
  ret.s = c4OrangStructure();
  return ret;
}


sapilocal::OrangHeuristicParams convertHeuristicParams(const sapi_SolverParameters* p) {
  if (p->parameter_unique_id != SAPI_SW_HEURISTIC_SOLVER_DEFAULT_PARAMETERS.parameter_unique_id) {
    throw InvalidParameterException("this solver requires parameters of type sapi_SwHeuristicSolverParameters");
  }
  const auto& optP = *reinterpret_cast<const sapi_SwHeuristicSolverParameters*>(p);
  sapilocal::OrangHeuristicParams ret;
  ret.iterationLimit = optP.iteration_limit;
  ret.maxBitFlipProb = optP.max_bit_flip_prob;
  ret.maxComplexity = optP.max_local_complexity;
  ret.minBitFlipProb = optP.min_bit_flip_prob;
  ret.noProgressLimit = optP.local_stuck_limit;
  ret.numPerturbedCopies = optP.num_perturbed_copies;
  ret.numVariables = optP.num_variables;
  ret.rngSeed = optP.random_seed;
  ret.timeLimitSeconds = optP.time_limit_seconds;
  ret.useSeed = optP.use_random_seed != 0;
  return ret;
}


sapi::SolverMap localSolverMap() {
  auto ret = sapi::SolverMap();
  ret[solvernames::sample] = SolverPtr(new LocalC4SampleSolver);
  ret[solvernames::optimize] = SolverPtr(new LocalC4OptimizeSolver);
  ret[solvernames::heuristic] = SolverPtr(new LocalIsingHeuristicSolver);
  return ret;
}


IsingResultPtr convertResult(const sapilocal::IsingResult& result) {
  auto answer = IsingResultPtr(new sapi_IsingResult);
  answer->energies = 0;
  answer->num_occurrences = 0;
  answer->solutions = 0;
  answer->num_solutions = result.energies.size();
  answer->solution_len = answer->num_solutions == 0 ? 0 : result.solutions.size() / answer->num_solutions;
  answer->timing = sapi_Timing{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  answer->energies = new double[result.energies.size()];
  copy(result.energies.begin(), result.energies.end(), answer->energies);

  if (!result.numOccurrences.empty()) {
    answer->num_occurrences = new int[result.numOccurrences.size()];
    copy(result.numOccurrences.begin(), result.numOccurrences.end(), answer->num_occurrences);
  }

  answer->solutions = new int[result.solutions.size()];
  copy(result.solutions.begin(), result.solutions.end(), answer->solutions);

  return answer;
}


class C4OptimizeSubmittedProblem : public sapi_SubmittedProblem {
private:
  sapilocal::ProblemType type_;
  sapilocal::SparseMatrix problem_;
  sapilocal::OrangOptimizeParams params_;

  virtual sapiremote::SubmittedProblemPtr remoteSubmittedProblemImpl() const {
    return sapiremote::SubmittedProblemPtr();
  }
  virtual void cancelImpl() {}
  virtual bool doneImpl() const { return true; }

  virtual sapi::IsingResultPtr resultImpl() const {
    return convertResult(sapilocal::orangOptimize(type_, problem_, params_));
  }

public:
  C4OptimizeSubmittedProblem(sapi_ProblemType type, const sapi_Problem *problem, const sapi_SolverParameters *params) :
      type_(convertProblemType(type)), problem_(convertProblem(problem)), params_(convertOptimizeParams(params)) {}
};


class C4SampleSubmittedProblem : public sapi_SubmittedProblem {
private:
  sapilocal::ProblemType type_;
  sapilocal::SparseMatrix problem_;
  sapilocal::OrangSampleParams params_;

  virtual sapiremote::SubmittedProblemPtr remoteSubmittedProblemImpl() const {
    return sapiremote::SubmittedProblemPtr();
  }
  virtual void cancelImpl() {}
  virtual bool doneImpl() const { return true; }

  virtual sapi::IsingResultPtr resultImpl() const {
    return convertResult(sapilocal::orangSample(type_, problem_, params_));
  }

public:
  C4SampleSubmittedProblem(sapi_ProblemType type, const sapi_Problem *problem, const sapi_SolverParameters *params) :
      type_(convertProblemType(type)), problem_(convertProblem(problem)), params_(convertSampleParams(params)) {}
};


class HeuristicSubmittedProblem : public sapi_SubmittedProblem {
private:
  sapilocal::ProblemType type_;
  sapilocal::SparseMatrix problem_;
  sapilocal::OrangHeuristicParams params_;

  virtual sapiremote::SubmittedProblemPtr remoteSubmittedProblemImpl() const {
    return sapiremote::SubmittedProblemPtr();
  }
  virtual void cancelImpl() {}
  virtual bool doneImpl() const { return true; }

  virtual sapi::IsingResultPtr resultImpl() const {
    return convertResult(sapilocal::isingHeuristic(type_, problem_, params_));
  }

public:
  HeuristicSubmittedProblem(sapi_ProblemType type, const sapi_Problem *problem, const sapi_SolverParameters *params) :
      type_(convertProblemType(type)), problem_(convertProblem(problem)), params_(convertHeuristicParams(params)) {}
};

} // namespace {anonymous}

namespace sapi {

const sapi_SolverProperties* LocalC4OptimizeSolver::propertiesImpl() const {
  return &props::c4OptimizeSolverProperties;
}


SubmittedProblemPtr LocalC4OptimizeSolver::submitImpl(
    sapi_ProblemType type,
    const sapi_Problem *problem,
    const sapi_SolverParameters *params) const {

  return SubmittedProblemPtr(new C4OptimizeSubmittedProblem(type, problem, params));
}


const sapi_SolverProperties* LocalC4SampleSolver::propertiesImpl() const {
  return &props::c4SampleSolverProperties;
}


SubmittedProblemPtr LocalC4SampleSolver::submitImpl(
    sapi_ProblemType type,
    const sapi_Problem *problem,
    const sapi_SolverParameters *params) const {

  return SubmittedProblemPtr(new C4SampleSubmittedProblem(type, problem, params));
}


const sapi_SolverProperties* LocalIsingHeuristicSolver::propertiesImpl() const {
  return &props::heuristicSolverProperties;
}


SubmittedProblemPtr LocalIsingHeuristicSolver::submitImpl(
    sapi_ProblemType type,
    const sapi_Problem *problem,
    const sapi_SolverParameters *params) const {

  return SubmittedProblemPtr(new HeuristicSubmittedProblem(type, problem, params));
}


LocalConnection::LocalConnection() : sapi_Connection(localSolverMap()) {}

} // namespace sapi
