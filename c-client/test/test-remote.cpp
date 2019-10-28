//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <json.hpp>
#include <problem-manager.hpp>
#include <coding.hpp>

#include <remote.hpp>

#include "test.hpp"
#include "json-builder.hpp"

using std::dynamic_pointer_cast;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::unique_ptr;
using std::vector;

using testing::StrictMock;
using testing::Return;
using testing::_;

using sapiremote::AnswerCallbackPtr;
using sapiremote::QpAnswer;
using sapiremote::QpProblem;
using sapiremote::QpProblemEntry;
using sapiremote::SubmittedProblemInfo;
using sapiremote::SubmittedProblemObserverPtr;
using sapiremote::SubmittedProblemPtr;

using sapi::InvalidParameterException;
using sapi::RemoteConnection;
using sapi::RemoteSolver;
using sapi::quantumParametersToJson;
using sapi::decodeRemoteIsingResult;

namespace {

class MockProblemManager : public sapiremote::ProblemManager {
public:
  MOCK_METHOD4(submitProblemImpl, SubmittedProblemPtr(string&, string&, json::Value&, json::Object&));
  MOCK_METHOD1(addProblemImpl, SubmittedProblemPtr(const string&));
  MOCK_METHOD0(fetchSolversImpl, sapiremote::SolverMap());
};

class MockRemoteSolver : public sapiremote::Solver {
private:
  json::Value encodedProblem_;
  QpProblem lastProblem_;

public:
  json::Value encodedProblem() const { return encodedProblem_; }
  void encodedProblem(json::Value p) { encodedProblem_ = std::move(p); }
  const QpProblem& lastProblem() const { return lastProblem_; }
  void lastProblem(QpProblem p) { lastProblem_ = std::move(p); }

  MOCK_CONST_METHOD3(submitProblemImpl, sapiremote::SubmittedProblemPtr(
      string& type, json::Value& problem, json::Object& params));
  MockRemoteSolver(string id, json::Object properties) :
      sapiremote::Solver(std::move(id), std::move(properties)) {}
};

class MockRemoteSubmittedProblem : public sapiremote::SubmittedProblem {
public:
  MOCK_CONST_METHOD0(problemIdImpl, string());
  MOCK_CONST_METHOD0(doneImpl, bool());
  MOCK_CONST_METHOD0(statusImpl, SubmittedProblemInfo());
  MOCK_CONST_METHOD0(answerImpl, tuple<string, json::Value>());
  MOCK_CONST_METHOD1(answerImpl, void(AnswerCallbackPtr));
  MOCK_METHOD0(cancelImpl, void());
  MOCK_METHOD0(retryImpl, void());
  MOCK_METHOD1(addSubmittedProblemObserverImpl, void(const SubmittedProblemObserverPtr&));
};

const auto isingRawAnswer = QpAnswer{
    {1, 1, 1, -1, -1, -1, 1, -1, 1, -1, 1, -1},
    {-10.0, -3.0, 4.0, 1.0},
    {}
};
const auto isingHistAnswer = QpAnswer{
    {1, -1, -1, -1, -1, 1, -1, -1, -1, -1, 1, -1},
    {-1.0, -2.0, -3.0},
    {1000, 200, 30}
};
const auto quboRawAnswer = QpAnswer{
    {0, 0, 1, 1, 0, 1, 1, 0},
    {-1234.0, 1234.0},
    {}
};
const auto quboHistAnswer = QpAnswer{
    {1, 1, 1, 1, 1},
    {0.0},
    {1001}
};

auto o = jsonObject();
auto a = jsonArray();

typedef tuple<string, json::Value> RemoteResult;

} // namespace {anonymous}

namespace sapiremote {

AnswerFormat answerFormat(const json::Value&) {
  return FORMAT_QP;
}

unique_ptr<QpSolverInfo> extractQpSolverInfo(const json::Object&) {
  return unique_ptr<QpSolverInfo>();
}


QpAnswer decodeQpAnswer(const string& problemType, const json::Object& answer) {
  auto iter = answer.find("type");
  if (iter != answer.end() && iter->second.isString()) {
    const auto& type = iter->second.getString();
    auto raw = type == "raw" ? 1 : 0;
    auto hist = type == "histogram" ? 2 : 0;
    auto ising = problemType == "ising" ? 4 : 0;
    auto qubo = problemType == "qubo" ? 8 : 0;
    switch (raw + hist + ising + qubo) {
      case 5: return isingRawAnswer;
      case 6: return isingHistAnswer;
      case 9: return quboRawAnswer;
      case 10: return quboHistAnswer;
      default: ; // shut up, warnings
    }
  }
  return QpAnswer();
}

json::Value encodeQpProblem(SolverPtr solver, QpProblem p) {
  auto mockSolver = dynamic_pointer_cast<MockRemoteSolver>(solver);
  if (mockSolver) {
    mockSolver->lastProblem(std::move(p));
    return mockSolver->encodedProblem();
  } else {
    return json::Null();
  }
}

bool operator==(const QpProblemEntry& a, const QpProblemEntry& b) {
  return a.i == b.i && a.j == b.j && a.value == b.value;
}

} // namespace sapiremote



TEST(RemoteConnectionTest, NoSolvers) {
  auto pm = make_shared<StrictMock<MockProblemManager>>();
  EXPECT_CALL(*pm, fetchSolversImpl()).WillOnce(Return(sapiremote::SolverMap()));
  RemoteConnection conn(pm);
  EXPECT_EQ(static_cast<const char*>(0), conn.solverNames()[0]);
  EXPECT_FALSE(!!conn.getSolver("nope"));
}


TEST(RemoteConnectionTest, Solvers) {
  auto rsolver1 = make_shared<StrictMock<MockRemoteSolver>>("solver1", json::Object());
  auto rsolver2 = make_shared<StrictMock<MockRemoteSolver>>("solver2", json::Object());
  auto solverMap = sapiremote::SolverMap{{rsolver1->id(), rsolver1}, {rsolver2->id(), rsolver2}};
  auto mockProblemManager = make_shared<StrictMock<MockProblemManager>>();
  EXPECT_CALL(*mockProblemManager, fetchSolversImpl()).WillOnce(Return(solverMap));

  RemoteConnection conn(mockProblemManager);
  auto solverNames = conn.solverNames();
  ASSERT_TRUE(!!solverNames);
  ASSERT_TRUE(!!solverNames[0]);
  ASSERT_TRUE(!!solverNames[1]);
  ASSERT_FALSE(!!solverNames[2]);

  if (solverNames[0] == rsolver1->id()) {
    EXPECT_EQ(rsolver1->id(), solverNames[0]);
    EXPECT_EQ(rsolver2->id(), solverNames[1]);
  } else {
    EXPECT_EQ(rsolver2->id(), solverNames[0]);
    EXPECT_EQ(rsolver1->id(), solverNames[1]);
  }

  EXPECT_TRUE(!!conn.getSolver(rsolver1->id()));
  EXPECT_TRUE(!!conn.getSolver(rsolver2->id()));
}


TEST(RemoteConnectionTest, Api) {
  GlobalState gs;
  sapi_Connection* conn;
  ASSERT_EQ(SAPI_OK, sapi_remoteConnection("", "", "", &conn, 0));
  ASSERT_TRUE(!!conn);
  sapi_freeConnection(conn);
}




TEST(RemoteParameterTest, Default) {
  EXPECT_EQ(json::Object(), quantumParametersToJson(SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS));
}


TEST(RemoteParameterTest, AnswerMode) {
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  auto expectedParams = json::Object();

  params.answer_mode = SAPI_ANSWER_MODE_RAW;
  expectedParams["answer_mode"] = "raw";
  EXPECT_EQ(expectedParams, quantumParametersToJson(params));

  params.answer_mode = SAPI_ANSWER_MODE_HISTOGRAM;
  expectedParams["answer_mode"] = "histogram";
  EXPECT_EQ(expectedParams, quantumParametersToJson(params));
}


TEST(RemoteParameterTest, AnswerModeInvalid) {
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  auto expectedParams = json::Object();

  params.answer_mode = static_cast<sapi_SolverParameterAnswerMode>(-999);
  EXPECT_THROW(quantumParametersToJson(params), sapi::InvalidParameterException);
}


TEST(RemoteParameterTest, Postprocess) {
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  auto expectedParams = json::Object();

  params.postprocess = SAPI_POSTPROCESS_NONE;
  expectedParams["postprocess"] = "";
  EXPECT_EQ(expectedParams, quantumParametersToJson(params));

  params.postprocess = SAPI_POSTPROCESS_OPTIMIZATION;
  expectedParams["postprocess"] = "optimization";
  EXPECT_EQ(expectedParams, quantumParametersToJson(params));

  params.postprocess = SAPI_POSTPROCESS_SAMPLING;
  expectedParams["postprocess"] = "sampling";
  EXPECT_EQ(expectedParams, quantumParametersToJson(params));
}


TEST(RemoteParameterTest, PostprocessInvalid) {
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  auto expectedParams = json::Object();

  params.postprocess = static_cast<sapi_Postprocess>(-999);
  EXPECT_THROW(quantumParametersToJson(params), sapi::InvalidParameterException);
}


TEST(RemoteParameterTest, ChainsTrivial) {
  const auto chains = sapi_Chains{0, 0};
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.chains = &chains;

  auto expectedParams = json::Object();
  expectedParams["chains"] = json::Array();

  EXPECT_EQ(expectedParams, quantumParametersToJson(params));
}


TEST(RemoteParameterTest, ChainsSingletonOnly) {
  const int chainsElts[] = {-1, -2, -1000, -1, -1};
  const auto chains = sapi_Chains{ const_cast<int*>(chainsElts), sizeof(chainsElts) / sizeof(chainsElts[0])};
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.chains = &chains;

  auto expectedParams = json::Object();
  expectedParams["chains"] = json::Array();

  EXPECT_EQ(expectedParams, quantumParametersToJson(params));
}


TEST(RemoteParameterTest, Chains) {
  const int chainsElts[] = {123, 12, -1, -1, -1, -1, -1, 12, 12, 12, 0, 0, 123, 12, 12, 12, -2};
  const auto chains = sapi_Chains{ const_cast<int*>(chainsElts), sizeof(chainsElts) / sizeof(chainsElts[0])};
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.chains = &chains;

  auto expectedParams = json::Object();
  expectedParams["chains"] = (a, (a, 0, 12), (a, 1, 7, 8, 9, 13, 14, 15), (a, 10, 11));

  EXPECT_EQ(expectedParams, quantumParametersToJson(params));
}


TEST(RemoteParameterTest, AnnealOffsetsTrivial) {
  const auto anneal_offsets = sapi_DoubleArray{0, 0};
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.anneal_offsets = &anneal_offsets;

  auto expectedParams = (o, "anneal_offsets", a).object();
  EXPECT_EQ(expectedParams, quantumParametersToJson(params));
}


TEST(RemoteParameterTest, AnnealOffsets) {
  const double aoElts[] = {0.0, 0.25, 1.0, -1234.5, -6.0, -5.5, 1111111.0, 0.0, 0.0, 0.0, -1.0};
  const auto anneal_offsets = sapi_AnnealOffsets{ const_cast<double*>(aoElts), sizeof(aoElts) / sizeof(aoElts[0]) };
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.anneal_offsets = &anneal_offsets;

  auto expectedParams = (o,
    "anneal_offsets", (a, 0.0, 0.25, 1.0, -1234.5, -6.0, -5.5, 1111111.0, 0.0, 0.0, 0.0, -1.0)).object();
  EXPECT_EQ(expectedParams, quantumParametersToJson(params));
}


TEST(RemoteParameterTest, ReverseAnneal) {
  int reElts[] = {-1, 0, 1, 3};
  const auto reverse_anneal = sapi_ReverseAnneal{ reElts, sizeof(reElts) / sizeof(reElts[0]), 1 };
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.reverse_anneal = &reverse_anneal;

  auto expectedParams = (o,
    "initial_state", (a, -1, 0, 1, 3),
    "reinitialize_state", true).object();
  EXPECT_EQ(expectedParams, quantumParametersToJson(params));
}


TEST(RemoteParameterTest, FluxBiases) {
  const double fbElts[] = {0.0, 0.25, 1.0, -1234.5, -6.0, -5.5, 1111111.0, 0.0, 0.0, 0.0, -1.0};
  const auto flux_biases = sapi_DoubleArray{ const_cast<double*>(fbElts), sizeof(fbElts) / sizeof(fbElts[0]) };
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.flux_biases = &flux_biases;

  auto expectedParams = (o,
    "flux_biases", (a, 0.0, 0.25, 1.0, -1234.5, -6.0, -5.5, 1111111.0, 0.0, 0.0, 0.0, -1.0)).object();
  EXPECT_EQ(expectedParams, quantumParametersToJson(params));
}


TEST(RemoteParameterTest, All) {
  const int chainsElts[] = {0, 100, 0, -1, 100};
  const auto chains = sapi_Chains{ const_cast<int*>(chainsElts), sizeof(chainsElts) / sizeof(chainsElts[0])};
  const double aoElts[] = {-1.0, 0.5, 1.0, -0.5};
  const auto annealOffsets = sapi_DoubleArray{ const_cast<double*>(aoElts), sizeof(aoElts) / sizeof(aoElts[0]) };
  const sapi_AnnealSchedulePoint asElts[] = {{0.0, 0.0}, {10.0, 0.5}, {12.5, 1.0}};
  const auto annealSchedule = sapi_AnnealSchedule{
    const_cast<sapi_AnnealSchedulePoint*>(asElts),
    sizeof(asElts) / sizeof(asElts[0]) };
  const int reElts[] = {3, 3, 3, 1};
  const auto reverseAnneal = sapi_ReverseAnneal{ const_cast<int*>(reElts), sizeof(reElts) / sizeof(reElts[0]), 0 };
  const double fbElts[] = {1.0, -1.0, -1.0, 0.5};
  const auto fluxBiases = sapi_DoubleArray{ const_cast<double*>(fbElts), sizeof(fbElts) / sizeof(fbElts[0]) };
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.annealing_time = 10;
  params.answer_mode = SAPI_ANSWER_MODE_RAW;
  params.auto_scale = 0;
  params.beta = 1234.5;
  params.chains = &chains;
  params.max_answers = 11;
  params.num_reads = 12;
  params.num_spin_reversal_transforms = 13;
  params.postprocess = SAPI_POSTPROCESS_NONE;
  params.programming_thermalization = 14;
  params.readout_thermalization = 15;
  params.anneal_offsets = &annealOffsets;
  params.anneal_schedule = &annealSchedule;
  params.reverse_anneal = &reverseAnneal;
  params.flux_biases = &fluxBiases;
  params.flux_drift_compensation = 0;
  params.reduce_intersample_correlation = 1;

  auto expectedParams = json::Object();
  expectedParams["annealing_time"] = 10;
  expectedParams["answer_mode"] = "raw";
  expectedParams["auto_scale"] = false;
  expectedParams["beta"] = 1234.5;
  expectedParams["chains"] = (a, (a, 0, 2), (a, 1, 4));
  expectedParams["max_answers"] = 11;
  expectedParams["num_reads"] = 12;
  expectedParams["num_spin_reversal_transforms"] = 13;
  expectedParams["postprocess"] = "";
  expectedParams["programming_thermalization"] = 14;
  expectedParams["readout_thermalization"] = 15;
  expectedParams["anneal_offsets"] = (a, -1.0, 0.5, 1.0, -0.5);
  expectedParams["anneal_schedule"] = (a, (a, 0, 0), (a, 10, 0.5), (a, 12.5, 1));
  expectedParams["initial_state"] = (a, 3, 3, 3, 1);
  expectedParams["reinitialize_state"] = false;
  expectedParams["flux_biases"] = (a, 1.0, -1.0, -1.0, 0.5);
  expectedParams["flux_drift_compensation"] = false;
  expectedParams["reduce_intersample_correlation"] = true;

  EXPECT_EQ(json::Value(expectedParams), json::Value(quantumParametersToJson(params)));
}



TEST(RemoteSolverTest, ParameterValidationPassAllParams) {
  auto props = (o, "parameters", (o, "annealing_time", 0, "answer_mode", 0, "auto_scale", 0, "beta", 0,
    "chains", 0, "max_answers", 0, "num_reads", 0, "num_spin_reversal_transforms", 0, "postprocess", 0,
    "programming_thermalization", 0, "readout_thermalization", 0, "anneal_offsets", 0, "anneal_schedule", 0,
    "initial_state", 0, "reinitialize_state", 0, "flux_biases", 0, "flux_drift_compensation", 0,
    "reduce_intersample_correlation", 0)
  ).object();

  auto mockSubmittedProblem = make_shared<StrictMock<MockRemoteSubmittedProblem>>();
  auto mockSolver = make_shared<StrictMock<MockRemoteSolver>>("", props);
  auto solver = RemoteSolver(mockSolver);
  EXPECT_CALL(*mockSolver, submitProblemImpl(_, _, _)).WillOnce(Return(mockSubmittedProblem));

  const int chainsElts[] = {0, 100, 0, -1, 100};
  const auto chains = sapi_Chains{ const_cast<int*>(chainsElts), sizeof(chainsElts) / sizeof(chainsElts[0])};
  const double aoElts[] = {-1.0, 0.5, 1.0, -0.5};
  const auto annealOffsets = sapi_DoubleArray{ const_cast<double*>(aoElts), sizeof(aoElts) / sizeof(aoElts[0]) };
  const sapi_AnnealSchedulePoint asElts[] = {{0.0, 0.0}, {10.0, 0.5}, {12.5, 1.0}};
  const auto annealSchedule = sapi_AnnealSchedule{
    const_cast<sapi_AnnealSchedulePoint*>(asElts),
    sizeof(asElts) / sizeof(asElts[0]) };
  const int reElts[] = {3, 3, 3, 1};
  const auto reverseAnneal = sapi_ReverseAnneal{ const_cast<int*>(reElts), sizeof(reElts) / sizeof(reElts[0]), 0 };
  const double fbElts[] = {1.0, -1.0, -1.0, 0.5};
  const auto fluxBiases = sapi_DoubleArray{ const_cast<double*>(fbElts), sizeof(fbElts) / sizeof(fbElts[0]) };
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.annealing_time = 10;
  params.answer_mode = SAPI_ANSWER_MODE_RAW;
  params.auto_scale = 0;
  params.beta = 1234.5;
  params.chains = &chains;
  params.max_answers = 11;
  params.num_reads = 12;
  params.num_spin_reversal_transforms = 13;
  params.postprocess = SAPI_POSTPROCESS_NONE;
  params.programming_thermalization = 14;
  params.readout_thermalization = 15;
  params.anneal_offsets = &annealOffsets;
  params.anneal_schedule = &annealSchedule;
  params.reverse_anneal = &reverseAnneal;
  params.flux_biases = &fluxBiases;
  params.flux_drift_compensation = true;
  params.reduce_intersample_correlation = false;

  sapi_Problem problem{0, 0};
  solver.submit(SAPI_PROBLEM_TYPE_ISING, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));
}


TEST(RemoteSolverTest, ParameterValidationPassNoParams) {
  auto props = (o, "parameters", o).object();

  auto mockSubmittedProblem = make_shared<StrictMock<MockRemoteSubmittedProblem>>();
  auto mockSolver = make_shared<StrictMock<MockRemoteSolver>>("", props);
  auto solver = RemoteSolver(mockSolver);
  EXPECT_CALL(*mockSolver, submitProblemImpl(_, _, _)).WillOnce(Return(mockSubmittedProblem));

  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  sapi_Problem problem{0, 0};
  solver.submit(SAPI_PROBLEM_TYPE_ISING, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));
}


TEST(RemoteSolverTest, ParameterValidationFail) {
  auto props = (o, "parameters", (o, "annealing_time", 0, "answer_mode", 0, "auto_scale", 0, "beta", 0,
    "chains", 0, "max_answers", 0, "num_reads", 0, "num_spin_reversal_transforms", 0, "postprocess", 0,
    "programming_thermalization", 0, "readout_thermalization", 0)).object();

  auto mockSubmittedProblem = make_shared<StrictMock<MockRemoteSubmittedProblem>>();
  auto mockSolver = make_shared<StrictMock<MockRemoteSolver>>("", props);
  auto solver = RemoteSolver(mockSolver);

  const double aoElts[] = {-1.0, 0.5, 1.0, -0.5};
  const auto annealOffsets = sapi_DoubleArray{ const_cast<double*>(aoElts), sizeof(aoElts) / sizeof(aoElts[0]) };
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.anneal_offsets = &annealOffsets;

  sapi_Problem problem{0, 0};

  EXPECT_THROW(solver.submit(SAPI_PROBLEM_TYPE_ISING, &problem, reinterpret_cast<sapi_SolverParameters*>(&params)),
      InvalidParameterException);
}


TEST(RemoteSolverTest, SubmitIsing) {
  auto type = string("ising");
  auto rproblem = json::Value("the problem");
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.num_reads = 9997;
  auto rparams = quantumParametersToJson(params);
  sapi_ProblemEntry problemElts[] = {{111, 222, 34.5}, {678, 876, 999.0}};
  auto problem = sapi_Problem{ problemElts, sizeof(problemElts) / sizeof(problemElts[0])};
  auto expectedProblem = QpProblem{{111, 222, 34.5}, {678, 876, 999.0}};

  auto mockSubmittedProblem = make_shared<StrictMock<MockRemoteSubmittedProblem>>();

  auto mockSolver = make_shared<StrictMock<MockRemoteSolver>>("", json::Object());
  mockSolver->encodedProblem(rproblem);
  EXPECT_CALL(*mockSolver, submitProblemImpl(type, rproblem, rparams)).WillOnce(Return(mockSubmittedProblem));

  auto solver = RemoteSolver(mockSolver);
  auto sp = solver.submit(SAPI_PROBLEM_TYPE_ISING, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(expectedProblem, mockSolver->lastProblem());
}


TEST(RemoteSolverTest, SubmitQubo) {
  auto type = string("qubo");
  auto rproblem = json::Value("the problem");
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.max_answers = 1235;
  auto rparams = quantumParametersToJson(params);
  sapi_ProblemEntry problemElts[] = {{111, 222, 34.5}, {678, 876, 999.0}};
  auto problem = sapi_Problem{ problemElts, sizeof(problemElts) / sizeof(problemElts[0])};
  auto expectedProblem = QpProblem{{111, 222, 34.5}, {678, 876, 999.0}};

  auto mockSubmittedProblem = make_shared<StrictMock<MockRemoteSubmittedProblem>>();

  auto mockSolver = make_shared<StrictMock<MockRemoteSolver>>("", json::Object());
  mockSolver->encodedProblem(rproblem);
  EXPECT_CALL(*mockSolver, submitProblemImpl(type, rproblem, rparams)).WillOnce(Return(mockSubmittedProblem));

  auto solver = RemoteSolver(mockSolver);
  auto sp = solver.submit(SAPI_PROBLEM_TYPE_QUBO, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(expectedProblem, mockSolver->lastProblem());
}


TEST(RemoteSolverTest, SubmitBadProblemType) {
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  auto problem = sapi_Problem{0, 0};

  auto mockSolver = make_shared<StrictMock<MockRemoteSolver>>("", json::Object());
  auto solver = RemoteSolver(mockSolver);
  EXPECT_THROW(solver.submit(static_cast<sapi_ProblemType>(-999), &problem,
      reinterpret_cast<sapi_SolverParameters*>(&params)), InvalidParameterException);
}


TEST(RemoteSolverTest, SubmitBadParamType) {
  auto params = sapi_SolverParameters{-876};
  auto problem = sapi_Problem{0, 0};

  auto mockSolver = make_shared<StrictMock<MockRemoteSolver>>("", json::Object());
  auto solver = RemoteSolver(mockSolver);
  EXPECT_THROW(solver.submit(SAPI_PROBLEM_TYPE_ISING, &problem, &params), InvalidParameterException);
}


TEST(RemoteSolverTest, SolveIsing) {
  auto type = string("ising");
  auto rproblem = json::Value("the problem");
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.annealing_time = 1020304;
  auto rparams = quantumParametersToJson(params);
  sapi_ProblemEntry problemElts[] = {{123, 456, 789.25}, {100, 10, -1.0}};
  auto problem = sapi_Problem{ problemElts, sizeof(problemElts) / sizeof(problemElts[0])};
  auto expectedProblem = QpProblem{{123, 456, 789.25}, {100, 10, -1.0}};
  auto ranswer = json::Object();
  ranswer["type"] = "histogram";

  auto mockSubmittedProblem = make_shared<StrictMock<MockRemoteSubmittedProblem>>();
  EXPECT_CALL(*mockSubmittedProblem, answerImpl()).WillOnce(Return(make_tuple(string("ising"), ranswer)));

  auto mockSolver = make_shared<StrictMock<MockRemoteSolver>>("", json::Object());
  mockSolver->encodedProblem(rproblem);
  EXPECT_CALL(*mockSolver, submitProblemImpl(type, rproblem, rparams)).WillOnce(Return(mockSubmittedProblem));

  auto solver = RemoteSolver(mockSolver);
  auto answer = solver.solve(SAPI_PROBLEM_TYPE_ISING, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(isingHistAnswer.energies.size(), answer->num_solutions);
  EXPECT_EQ(isingHistAnswer.solutions.size() / isingHistAnswer.energies.size(), answer->solution_len);
  EXPECT_EQ(isingHistAnswer.energies, vector<double>(answer->energies, answer->energies + answer->num_solutions));
  EXPECT_EQ(isingHistAnswer.numOccurrences,
      vector<int>(answer->num_occurrences, answer->num_occurrences + answer->num_solutions));
  EXPECT_EQ(vector<int>(isingHistAnswer.solutions.begin(), isingHistAnswer.solutions.end()),
      vector<int>(answer->solutions, answer->solutions + answer->solution_len * answer->num_solutions));
}


TEST(RemoteSolverTest, SolveQubo) {
  auto type = string("qubo");
  auto rproblem = json::Value("the problem");
  auto params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
  params.auto_scale = 0;
  auto rparams = quantumParametersToJson(params);
  sapi_ProblemEntry problemElts[] = {{1, 2, 3.0}, {4, 5, 6.0}, {7, 8, 9.0}};
  auto problem = sapi_Problem{ problemElts, sizeof(problemElts) / sizeof(problemElts[0])};
  auto expectedProblem = QpProblem{{1, 2, 3.0}, {4, 5, 6.0}, {7, 8, 9.0}};
  auto ranswer = json::Object();
  ranswer["type"] = "raw";

  auto mockSubmittedProblem = make_shared<StrictMock<MockRemoteSubmittedProblem>>();
  EXPECT_CALL(*mockSubmittedProblem, answerImpl()).WillOnce(Return(make_tuple(string("qubo"), ranswer)));

  auto mockSolver = make_shared<StrictMock<MockRemoteSolver>>("", json::Object());
  mockSolver->encodedProblem(rproblem);
  EXPECT_CALL(*mockSolver, submitProblemImpl(type, rproblem, rparams)).WillOnce(Return(mockSubmittedProblem));

  auto solver = RemoteSolver(mockSolver);
  auto answer = solver.solve(SAPI_PROBLEM_TYPE_QUBO, &problem, reinterpret_cast<sapi_SolverParameters*>(&params));

  EXPECT_EQ(quboRawAnswer.energies.size(), answer->num_solutions);
  EXPECT_EQ(quboRawAnswer.solutions.size() / quboRawAnswer.energies.size(), answer->solution_len);
  EXPECT_EQ(quboRawAnswer.energies, vector<double>(answer->energies, answer->energies + answer->num_solutions));
  EXPECT_FALSE(!!answer->num_occurrences);
  EXPECT_EQ(vector<int>(quboRawAnswer.solutions.begin(), quboRawAnswer.solutions.end()),
      vector<int>(answer->solutions, answer->solutions + answer->solution_len * answer->num_solutions));
}


TEST(RemoteTimingTest, NoTiming) {
  auto result = decodeRemoteIsingResult(RemoteResult("ising", json::Object()));
  EXPECT_EQ(-1, result->timing.anneal_time_per_run);
  EXPECT_EQ(-1, result->timing.readout_time_per_run);
  EXPECT_EQ(-1, result->timing.run_time_chip);
  EXPECT_EQ(-1, result->timing.total_real_time);
  EXPECT_EQ(-1, result->timing.qpu_access_time);
  EXPECT_EQ(-1, result->timing.qpu_programming_time);
  EXPECT_EQ(-1, result->timing.qpu_sampling_time);
  EXPECT_EQ(-1, result->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(-1, result->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(-1, result->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(-1, result->timing.post_processing_overhead_time);
  EXPECT_EQ(-1, result->timing.total_post_processing_time);
}


TEST(RemoteTimingTest, EmptyTiming) {
  auto answerJson = (o, "timing", o).object();
  auto result = decodeRemoteIsingResult(RemoteResult("ising", answerJson));
  EXPECT_EQ(-1, result->timing.anneal_time_per_run);
  EXPECT_EQ(-1, result->timing.readout_time_per_run);
  EXPECT_EQ(-1, result->timing.run_time_chip);
  EXPECT_EQ(-1, result->timing.total_real_time);
  EXPECT_EQ(-1, result->timing.qpu_access_time);
  EXPECT_EQ(-1, result->timing.qpu_programming_time);
  EXPECT_EQ(-1, result->timing.qpu_sampling_time);
  EXPECT_EQ(-1, result->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(-1, result->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(-1, result->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(-1, result->timing.post_processing_overhead_time);
  EXPECT_EQ(-1, result->timing.total_post_processing_time);
}


TEST(RemoteTimingTest, FullTiming) {
  auto answerJson = (o, "timing", (o,
      "anneal_time_per_run", 1001,
      "readout_time_per_run", 1002,
      "run_time_chip", 1003,
      "total_real_time", 1004,
      "qpu_access_time", 2001,
      "qpu_programming_time", 2002,
      "qpu_sampling_time", 2003,
      "qpu_anneal_time_per_sample", 2004,
      "qpu_readout_time_per_sample", 2005,
      "qpu_delay_time_per_sample", 2006,
      "post_processing_overhead_time", 1005,
      "total_post_processing_time", 1006)).object();
  auto result = decodeRemoteIsingResult(RemoteResult("ising", answerJson));

  EXPECT_EQ(1001, result->timing.anneal_time_per_run);
  EXPECT_EQ(1002, result->timing.readout_time_per_run);
  EXPECT_EQ(1003, result->timing.run_time_chip);
  EXPECT_EQ(1004, result->timing.total_real_time);
  EXPECT_EQ(2001, result->timing.qpu_access_time);
  EXPECT_EQ(2002, result->timing.qpu_programming_time);
  EXPECT_EQ(2003, result->timing.qpu_sampling_time);
  EXPECT_EQ(2004, result->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(2005, result->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(2006, result->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(1005, result->timing.post_processing_overhead_time);
  EXPECT_EQ(1006, result->timing.total_post_processing_time);
}


TEST(RemoteTimingTest, BackCompatTiming) {
  auto answerJson = (o, "timing", (o,
      "qpu_access_time", 2001,
      "qpu_programming_time", 2002,
      "qpu_sampling_time", 2003,
      "qpu_anneal_time_per_sample", 2004,
      "qpu_readout_time_per_sample", 2005,
      "qpu_delay_time_per_sample", 2006,
      "post_processing_overhead_time", 1005,
      "total_post_processing_time", 1006)).object();
  auto result = decodeRemoteIsingResult(RemoteResult("ising", answerJson));

  EXPECT_EQ(2004, result->timing.anneal_time_per_run);
  EXPECT_EQ(2005, result->timing.readout_time_per_run);
  EXPECT_EQ(2003, result->timing.run_time_chip);
  EXPECT_EQ(2001, result->timing.total_real_time);
  EXPECT_EQ(2001, result->timing.qpu_access_time);
  EXPECT_EQ(2002, result->timing.qpu_programming_time);
  EXPECT_EQ(2003, result->timing.qpu_sampling_time);
  EXPECT_EQ(2004, result->timing.qpu_anneal_time_per_sample);
  EXPECT_EQ(2005, result->timing.qpu_readout_time_per_sample);
  EXPECT_EQ(2006, result->timing.qpu_delay_time_per_sample);
  EXPECT_EQ(1005, result->timing.post_processing_overhead_time);
  EXPECT_EQ(1006, result->timing.total_post_processing_time);
}
