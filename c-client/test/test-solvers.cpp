//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <dwave_sapi.h>
#include <sapi-impl.hpp>
#include <internal.hpp>

using std::make_shared;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

using testing::Return;
using testing::ReturnPointee;
using testing::StrictMock;
using testing::Throw;

using sapi::SolverPtr;
using sapi::SolverMap;
using sapi::IsingResultPtr;
using sapi::SubmittedProblemPtr;

namespace {

class MockSolver : public sapi_Solver {
private:
  virtual IsingResultPtr solveImpl(
      sapi_ProblemType type, const sapi_Problem* problem,
      const sapi_SolverParameters* params) const { return IsingResultPtr(solveImpl2(type, problem, params)); }

  virtual SubmittedProblemPtr submitImpl(
      sapi_ProblemType type, const sapi_Problem* problem,
      const sapi_SolverParameters* params) const { return SubmittedProblemPtr(submitImpl2(type, problem, params)); }

public:
  MOCK_CONST_METHOD0(propertiesImpl, const sapi_SolverProperties*());
  MOCK_CONST_METHOD3(solveImpl2,
      sapi_IsingResult*(sapi_ProblemType type, const sapi_Problem* problem, const sapi_SolverParameters* params));
  MOCK_CONST_METHOD3(submitImpl2, sapi_SubmittedProblem*(sapi_ProblemType type, const sapi_Problem* problem,
      const sapi_SolverParameters* params));
};


class MockSubmittedProblem : public sapi_SubmittedProblem {
private:
  virtual IsingResultPtr resultImpl() const { return IsingResultPtr(resultImpl2()); }
public:
  MOCK_CONST_METHOD0(remoteSubmittedProblemImpl, sapiremote::SubmittedProblemPtr());
  MOCK_METHOD0(cancelImpl, void());
  MOCK_CONST_METHOD0(doneImpl, bool());
  MOCK_CONST_METHOD0(resultImpl2, sapi_IsingResult*());
};

class MockRemoteSubmittedProblem : public sapiremote::SubmittedProblem {
public:
  MOCK_CONST_METHOD0(problemIdImpl, string());
  MOCK_CONST_METHOD0(doneImpl, bool());
  MOCK_CONST_METHOD0(statusImpl, sapiremote::SubmittedProblemInfo());
  MOCK_CONST_METHOD0(answerImpl, std::tuple<string, json::Value>());
  MOCK_CONST_METHOD1(answerImpl, void(sapiremote::AnswerCallbackPtr));
  MOCK_METHOD0(cancelImpl, void());
  MOCK_METHOD0(retryImpl, void());
  MOCK_METHOD1(addSubmittedProblemObserverImpl, void(const sapiremote::SubmittedProblemObserverPtr&));
};

bool awaitCompletionResult = true;
vector<sapiremote::SubmittedProblem*> lastAwaitCompletionProblems;
int lastAwaitCompletionMinDone = -1;
double lastAwaitCompletionTimeout = -1.0;

struct EnableAwaitCompletion {
~EnableAwaitCompletion() { awaitCompletionResult = true; }
};

} // namespace {anonymous}


namespace sapiremote {

bool awaitCompletion(const vector<SubmittedProblemPtr>& problems, int minDone, double timeout) {
  lastAwaitCompletionProblems.clear();
  for (auto it = problems.begin(); it != problems.end(); ++it) {
    lastAwaitCompletionProblems.push_back(it->get());
  }
  lastAwaitCompletionMinDone = minDone;
  lastAwaitCompletionTimeout = timeout;
  return awaitCompletionResult;
}

} // namespace sapiremote



TEST(SolversApiTest, ListSolvers) {
  auto expectedSolverNames = set<string>{"solver-1", "solver-a", "another solver", "something"};
  auto solverMap = SolverMap();
  for(auto it = expectedSolverNames.begin(); it != expectedSolverNames.end(); ++it) {
    solverMap[*it] = make_shared<MockSolver>();
  }

  sapi_Connection conn(solverMap);
  auto names = sapi_listSolvers(&conn);

  ASSERT_TRUE(!!names);
  ASSERT_TRUE(!!names[0]);
  ASSERT_TRUE(!!names[1]);
  ASSERT_TRUE(!!names[2]);
  ASSERT_TRUE(!!names[3]);
  ASSERT_FALSE(!!names[4]);
  auto solverNames = set<string>{names[0], names[1], names[2], names[3]};

  EXPECT_EQ(expectedSolverNames, solverNames);
}


TEST(SolversApiTest, GetSolver) {
  auto solvers = SolverMap{
      {"solver-3", make_shared<MockSolver>()},
      {"solver-r", make_shared<MockSolver>()},
      {"yet another solver", make_shared<MockSolver>()},
      {"something else", make_shared<MockSolver>()}
  };

  sapi_Connection conn(solvers);
  EXPECT_EQ(solvers["solver-3"].get(), sapi_getSolver(&conn, "solver-3"));
  EXPECT_EQ(solvers["solver-r"].get(), sapi_getSolver(&conn, "solver-r"));
  EXPECT_EQ(solvers["yet another solver"].get(), sapi_getSolver(&conn, "yet another solver"));
  EXPECT_EQ(solvers["something else"].get(), sapi_getSolver(&conn, "something else"));
  EXPECT_FALSE(!!sapi_getSolver(&conn, "nope"));
}


TEST(SolversApiTest, SolverProperties) {
  auto props = sapi_SolverProperties();
  auto solver = make_shared<MockSolver>();
  EXPECT_CALL(*solver, propertiesImpl()).WillOnce(Return(&props));
  EXPECT_EQ(&props, sapi_getSolverProperties(solver.get()));
}


TEST(SolversApiTest, SolveIsing) {
  auto problem = sapi_Problem();
  auto params = sapi_SolverParameters{-999};
  auto result = sapi_IsingResult();

  auto solver = make_shared<StrictMock<MockSolver>>();
  EXPECT_CALL(*solver, solveImpl2(SAPI_PROBLEM_TYPE_ISING, &problem, &params)).WillOnce(Return(&result));

  sapi_IsingResult* returned;
  ASSERT_EQ(SAPI_OK, sapi_solveIsing(solver.get(), &problem, &params, &returned, 0));
  EXPECT_EQ(&result, returned);
}


TEST(SolversApiTest, SolveIsingError) {
  auto problem = sapi_Problem();
  auto params = sapi_SolverParameters{-999};

  auto solver = make_shared<StrictMock<MockSolver>>();
  EXPECT_CALL(*solver, solveImpl2(SAPI_PROBLEM_TYPE_ISING, &problem, &params)).WillOnce(Throw(0));

  sapi_IsingResult* returned = 0;
  ASSERT_NE(SAPI_OK, sapi_solveIsing(solver.get(), &problem, &params, &returned, 0));
  EXPECT_EQ(static_cast<sapi_IsingResult*>(0), returned);
}


TEST(SolversApiTest, SolveQubo) {
  auto problem = sapi_Problem();
  auto params = sapi_SolverParameters{-999};
  auto result = sapi_IsingResult();

  auto solver = make_shared<StrictMock<MockSolver>>();
  EXPECT_CALL(*solver, solveImpl2(SAPI_PROBLEM_TYPE_QUBO, &problem, &params)).WillOnce(Return(&result));

  sapi_IsingResult* returned;
  ASSERT_EQ(SAPI_OK, sapi_solveQubo(solver.get(), &problem, &params, &returned, 0));
  EXPECT_EQ(&result, returned);
}


TEST(SolversApiTest, SolveQuboError) {
  auto problem = sapi_Problem();
  auto params = sapi_SolverParameters{-999};

  auto solver = make_shared<StrictMock<MockSolver>>();
  EXPECT_CALL(*solver, solveImpl2(SAPI_PROBLEM_TYPE_QUBO, &problem, &params)).WillOnce(Throw(0));

  sapi_IsingResult* returned = 0;
  ASSERT_NE(SAPI_OK, sapi_solveQubo(solver.get(), &problem, &params, &returned, 0));
  EXPECT_EQ(static_cast<sapi_IsingResult*>(0), returned);
}


TEST(SolversApiTest, AsyncSolveIsing) {
  auto problem = sapi_Problem();
  auto params = sapi_SolverParameters{-999};
  StrictMock<MockSubmittedProblem> result;

  auto solver = make_shared<StrictMock<MockSolver>>();
  EXPECT_CALL(*solver, submitImpl2(SAPI_PROBLEM_TYPE_ISING, &problem, &params)).WillOnce(Return(&result));

  sapi_SubmittedProblem* returned;
  ASSERT_EQ(SAPI_OK, sapi_asyncSolveIsing(solver.get(), &problem, &params, &returned, 0));
  EXPECT_EQ(&result, returned);
}


TEST(SolversApiTest, AsyncSolveIsingError) {
  auto problem = sapi_Problem();
  auto params = sapi_SolverParameters{-999};

  auto solver = make_shared<StrictMock<MockSolver>>();
  EXPECT_CALL(*solver, submitImpl2(SAPI_PROBLEM_TYPE_ISING, &problem, &params)).WillOnce(Throw(0));

  sapi_SubmittedProblem* returned = 0;
  ASSERT_NE(SAPI_OK, sapi_asyncSolveIsing(solver.get(), &problem, &params, &returned, 0));
  EXPECT_EQ(static_cast<sapi_SubmittedProblem*>(0), returned);
}


TEST(SolversApiTest, AsyncSolveQubo) {
  auto problem = sapi_Problem();
  auto params = sapi_SolverParameters{-999};
  StrictMock<MockSubmittedProblem> result;

  auto solver = make_shared<StrictMock<MockSolver>>();
  EXPECT_CALL(*solver, submitImpl2(SAPI_PROBLEM_TYPE_QUBO, &problem, &params)).WillOnce(Return(&result));

  sapi_SubmittedProblem* returned;
  ASSERT_EQ(SAPI_OK, sapi_asyncSolveQubo(solver.get(), &problem, &params, &returned, 0));
  EXPECT_EQ(&result, returned);
}


TEST(SolversApiTest, AsyncSolveQuboError) {
  auto problem = sapi_Problem();
  auto params = sapi_SolverParameters{-999};

  auto solver = make_shared<StrictMock<MockSolver>>();
  EXPECT_CALL(*solver, submitImpl2(SAPI_PROBLEM_TYPE_QUBO, &problem, &params)).WillOnce(Throw(0));

  sapi_SubmittedProblem* returned = 0;
  ASSERT_NE(SAPI_OK, sapi_asyncSolveQubo(solver.get(), &problem, &params, &returned, 0));
  EXPECT_EQ(static_cast<sapi_SubmittedProblem*>(0), returned);
}


TEST(SolversApiTest, Cancel) {
  StrictMock<MockSubmittedProblem> sp;
  EXPECT_CALL(sp, cancelImpl()).Times(1);
  sapi_cancelSubmittedProblem(&sp);
}


TEST(SolversApiTest, AsyncDone) {
  StrictMock<MockSubmittedProblem> sp;
  EXPECT_CALL(sp, doneImpl()).WillOnce(Return(true)).WillOnce(Return(false));
  EXPECT_EQ(1, sapi_asyncDone(&sp));
  EXPECT_EQ(0, sapi_asyncDone(&sp));
}


TEST(SolversApiTest, AsyncResult) {
  sapi_IsingResult result;
  StrictMock<MockSubmittedProblem> sp;
  EXPECT_CALL(sp, resultImpl2()).WillOnce(Return(&result));
  sapi_IsingResult* returned;
  ASSERT_EQ(SAPI_OK, sapi_asyncResult(&sp, &returned, 0));
  EXPECT_EQ(&result, returned);
}


TEST(SolversApiTest, AsyncResultError) {
  StrictMock<MockSubmittedProblem> sp;
  EXPECT_CALL(sp, resultImpl2()).WillOnce(Throw(0));
  sapi_IsingResult* returned = 0;
  ASSERT_NE(SAPI_OK, sapi_asyncResult(&sp, &returned, 0));
  EXPECT_EQ(static_cast<sapi_IsingResult*>(0), returned);
}


TEST(SolversApiTest, AwaitCompletionNoRemote) {
  auto sp1 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto sp2 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto sp3 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto problems = vector<const sapi_SubmittedProblem*>{sp1.get(), sp2.get(), sp3.get()};

  EXPECT_CALL(*sp1, remoteSubmittedProblemImpl()).WillOnce(Return(sapiremote::SubmittedProblemPtr()));
  EXPECT_CALL(*sp2, remoteSubmittedProblemImpl()).WillOnce(Return(sapiremote::SubmittedProblemPtr()));
  EXPECT_CALL(*sp3, remoteSubmittedProblemImpl()).WillOnce(Return(sapiremote::SubmittedProblemPtr()));

  lastAwaitCompletionProblems.clear();
  lastAwaitCompletionMinDone = -1;
  lastAwaitCompletionTimeout = -1.0;
  ASSERT_EQ(1, sapi_awaitCompletion(problems.data(), problems.size(), 4, 1234.5));
  EXPECT_TRUE(lastAwaitCompletionProblems.empty());
  EXPECT_EQ(-1, lastAwaitCompletionMinDone);
  EXPECT_EQ(-1.0, lastAwaitCompletionTimeout);
}


TEST(SolversApiTest, AwaitCompletionNoRemoteCall) {
  auto sp1 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto sp2 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto sp3 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto sp4 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto problems = vector<const sapi_SubmittedProblem*>{sp1.get(), sp2.get(), sp3.get(), sp4.get()};

  auto rsp1 = make_shared<MockRemoteSubmittedProblem>();
  auto rsp4 = make_shared<MockRemoteSubmittedProblem>();

  EXPECT_CALL(*sp1, remoteSubmittedProblemImpl()).WillOnce(Return(rsp1));
  EXPECT_CALL(*sp2, remoteSubmittedProblemImpl()).WillOnce(Return(sapiremote::SubmittedProblemPtr()));
  EXPECT_CALL(*sp3, remoteSubmittedProblemImpl()).WillOnce(Return(sapiremote::SubmittedProblemPtr()));
  EXPECT_CALL(*sp4, remoteSubmittedProblemImpl()).WillOnce(Return(rsp4));

  lastAwaitCompletionProblems.clear();
  lastAwaitCompletionMinDone = -1;
  lastAwaitCompletionTimeout = -1.0;
  ASSERT_EQ(1, sapi_awaitCompletion(problems.data(), problems.size(), 2, 1234.5));
  EXPECT_TRUE(lastAwaitCompletionProblems.empty());
  EXPECT_EQ(-1, lastAwaitCompletionMinDone);
  EXPECT_EQ(-1.0, lastAwaitCompletionTimeout);
}

TEST(SolversApiTest, AwaitCompletionAllRemote) {
  auto sp1 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto sp2 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto problems = vector<const sapi_SubmittedProblem*>{sp1.get(), sp2.get()};

  auto rsp1 = make_shared<MockRemoteSubmittedProblem>();
  auto rsp2 = make_shared<MockRemoteSubmittedProblem>();

  EXPECT_CALL(*sp1, remoteSubmittedProblemImpl()).WillOnce(Return(rsp1));
  EXPECT_CALL(*sp2, remoteSubmittedProblemImpl()).WillOnce(Return(rsp2));

  lastAwaitCompletionMinDone = -1;
  lastAwaitCompletionTimeout = -1.0;
  ASSERT_EQ(1, sapi_awaitCompletion(problems.data(), problems.size(), 4, 1234.5));
  auto expectedProblems = vector<sapiremote::SubmittedProblem*>{rsp1.get(), rsp2.get()};
  EXPECT_EQ(expectedProblems, lastAwaitCompletionProblems);
  EXPECT_LE(2, lastAwaitCompletionMinDone);
  EXPECT_EQ(1234.5, lastAwaitCompletionTimeout);
}


TEST(SolversApiTest, AwaitCompletionMixedNotDone) {
  auto sp1 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto sp2 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto sp3 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto sp4 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto sp5 = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto problems = vector<const sapi_SubmittedProblem*>{sp1.get(), sp2.get(), sp3.get(), sp4.get(), sp5.get()};

  auto rsp2 = make_shared<MockRemoteSubmittedProblem>();
  auto rsp5 = make_shared<MockRemoteSubmittedProblem>();

  EXPECT_CALL(*sp1, remoteSubmittedProblemImpl()).WillOnce(Return(sapiremote::SubmittedProblemPtr()));
  EXPECT_CALL(*sp2, remoteSubmittedProblemImpl()).WillOnce(Return(rsp2));
  EXPECT_CALL(*sp3, remoteSubmittedProblemImpl()).WillOnce(Return(sapiremote::SubmittedProblemPtr()));
  EXPECT_CALL(*sp4, remoteSubmittedProblemImpl()).WillOnce(Return(sapiremote::SubmittedProblemPtr()));
  EXPECT_CALL(*sp5, remoteSubmittedProblemImpl()).WillOnce(Return(rsp5));

  lastAwaitCompletionMinDone = -1;
  lastAwaitCompletionTimeout = -1.0;
  EnableAwaitCompletion e;
  awaitCompletionResult = false;
  ASSERT_EQ(0, sapi_awaitCompletion(problems.data(), problems.size(), 4, 1234.5));
  auto expectedProblems = vector<sapiremote::SubmittedProblem*>{rsp2.get(), rsp5.get()};
  EXPECT_EQ(expectedProblems, lastAwaitCompletionProblems);
  EXPECT_LE(1, lastAwaitCompletionMinDone);
  EXPECT_EQ(1234.5, lastAwaitCompletionTimeout);
}


TEST(SolversApiTest, RetryRemote) {
  auto problem = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto rsp = make_shared<MockRemoteSubmittedProblem>();
  EXPECT_CALL(*problem, remoteSubmittedProblemImpl()).WillOnce(Return(rsp));
  EXPECT_CALL(*rsp, retryImpl()).Times(1);

  sapi_asyncRetry(problem.get());
}


TEST(SolversApiTest, RetryLocal) {
  auto problem = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  EXPECT_CALL(*problem, remoteSubmittedProblemImpl()).WillOnce(Return(sapiremote::SubmittedProblemPtr()));

  sapi_asyncRetry(problem.get());
}


TEST(SolversApiTest, StatusLocal) {
  auto problem = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  EXPECT_CALL(*problem, remoteSubmittedProblemImpl()).WillOnce(Return(sapiremote::SubmittedProblemPtr()));

  sapi_ProblemStatus status;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ('\0', status.problem_id[0]);
  EXPECT_EQ('\0', status.time_received[0]);
  EXPECT_EQ('\0', status.time_solved[0]);
  EXPECT_EQ(SAPI_STATE_DONE, status.state);
  EXPECT_EQ(SAPI_STATE_DONE, status.last_good_state);
  EXPECT_EQ(SAPI_STATUS_COMPLETED, status.remote_status);
  EXPECT_EQ(SAPI_OK, status.error_code);
  EXPECT_EQ('\0', status.error_message[0]);
}


TEST(SolversApiTest, StatusProblemId) {
  sapiremote::SubmittedProblemInfo spi;
  sapi_ProblemStatus status;

  auto problem = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto rsp = make_shared<MockRemoteSubmittedProblem>();
  EXPECT_CALL(*problem, remoteSubmittedProblemImpl()).Times(2).WillRepeatedly(Return(rsp));
  EXPECT_CALL(*rsp, statusImpl()).Times(2).WillRepeatedly(ReturnPointee(&spi));

  spi.problemId = "";
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(spi.problemId, status.problem_id);

  spi.problemId = "the-problem-id";
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(spi.problemId, status.problem_id);
}


TEST(SolversApiTest, StatusTimes) {
  sapiremote::SubmittedProblemInfo spi;
  sapi_ProblemStatus status;

  auto problem = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto rsp = make_shared<MockRemoteSubmittedProblem>();
  EXPECT_CALL(*problem, remoteSubmittedProblemImpl()).Times(2).WillRepeatedly(Return(rsp));
  EXPECT_CALL(*rsp, statusImpl()).Times(2).WillRepeatedly(ReturnPointee(&spi));

  spi.submittedOn = "";
  spi.solvedOn = "1:23";
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(spi.submittedOn, status.time_received);
  EXPECT_EQ(spi.solvedOn, status.time_solved);

  spi.submittedOn = "4:56";
  spi.solvedOn = "";
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(spi.submittedOn, status.time_received);
  EXPECT_EQ(spi.solvedOn, status.time_solved);
}


TEST(SolversApiTest, StatusState) {
  sapiremote::SubmittedProblemInfo spi;
  sapi_ProblemStatus status;

  auto problem = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto rsp = make_shared<MockRemoteSubmittedProblem>();
  EXPECT_CALL(*problem, remoteSubmittedProblemImpl()).Times(5).WillRepeatedly(Return(rsp));
  EXPECT_CALL(*rsp, statusImpl()).Times(5).WillRepeatedly(ReturnPointee(&spi));

  spi.state = sapiremote::submittedstates::SUBMITTING;
  spi.lastGoodState = sapiremote::submittedstates::SUBMITTING;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_STATE_SUBMITTING, status.state);
  EXPECT_EQ(SAPI_STATE_SUBMITTING, status.last_good_state);

  spi.state = sapiremote::submittedstates::SUBMITTED;
  spi.lastGoodState = sapiremote::submittedstates::SUBMITTED;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_STATE_SUBMITTED, status.state);
  EXPECT_EQ(SAPI_STATE_SUBMITTED, status.last_good_state);

  spi.state = sapiremote::submittedstates::DONE;
  spi.lastGoodState = sapiremote::submittedstates::DONE;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_STATE_DONE, status.state);
  EXPECT_EQ(SAPI_STATE_DONE, status.last_good_state);

  spi.state = sapiremote::submittedstates::FAILED;
  spi.lastGoodState = sapiremote::submittedstates::SUBMITTED;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_STATE_FAILED, status.state);
  EXPECT_EQ(SAPI_STATE_SUBMITTED, status.last_good_state);

  spi.state = sapiremote::submittedstates::RETRYING;
  spi.lastGoodState = sapiremote::submittedstates::SUBMITTING;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_STATE_RETRYING, status.state);
  EXPECT_EQ(SAPI_STATE_SUBMITTING, status.last_good_state);
}


TEST(SolversApiTest, StatusRemoteStatus) {
  sapiremote::SubmittedProblemInfo spi;
  sapi_ProblemStatus status;

  auto problem = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto rsp = make_shared<MockRemoteSubmittedProblem>();
  EXPECT_CALL(*problem, remoteSubmittedProblemImpl()).Times(6).WillRepeatedly(Return(rsp));
  EXPECT_CALL(*rsp, statusImpl()).Times(6).WillRepeatedly(ReturnPointee(&spi));

  spi.remoteStatus = sapiremote::remotestatuses::UNKNOWN;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_STATUS_UNKNOWN, status.remote_status);

  spi.remoteStatus = sapiremote::remotestatuses::PENDING;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_STATUS_PENDING, status.remote_status);

  spi.remoteStatus = sapiremote::remotestatuses::IN_PROGRESS;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_STATUS_IN_PROGRESS, status.remote_status);

  spi.remoteStatus = sapiremote::remotestatuses::COMPLETED;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_STATUS_COMPLETED, status.remote_status);

  spi.remoteStatus = sapiremote::remotestatuses::FAILED;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_STATUS_FAILED, status.remote_status);

  spi.remoteStatus = sapiremote::remotestatuses::CANCELED;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_STATUS_CANCELED, status.remote_status);
}


TEST(SolversApiTest, StatusNoError) {
  sapiremote::SubmittedProblemInfo spi;
  sapi_ProblemStatus status;

  auto problem = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto rsp = make_shared<MockRemoteSubmittedProblem>();
  EXPECT_CALL(*problem, remoteSubmittedProblemImpl()).Times(3).WillRepeatedly(Return(rsp));
  EXPECT_CALL(*rsp, statusImpl()).Times(3).WillRepeatedly(ReturnPointee(&spi));

  spi.state = sapiremote::submittedstates::SUBMITTING;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_OK, status.error_code);
  EXPECT_EQ('\0', status.error_message[0]);

  spi.state = sapiremote::submittedstates::SUBMITTED;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_OK, status.error_code);
  EXPECT_EQ('\0', status.error_message[0]);

  spi.state = sapiremote::submittedstates::DONE;
  spi.remoteStatus = sapiremote::remotestatuses::COMPLETED;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_OK, status.error_code);
  EXPECT_EQ('\0', status.error_message[0]);
}


TEST(SolversApiTest, StatusError) {
  sapiremote::SubmittedProblemInfo spi;
  sapi_ProblemStatus status;

  auto problem = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto rsp = make_shared<MockRemoteSubmittedProblem>();
  EXPECT_CALL(*problem, remoteSubmittedProblemImpl()).Times(6).WillRepeatedly(Return(rsp));
  EXPECT_CALL(*rsp, statusImpl()).Times(6).WillRepeatedly(ReturnPointee(&spi));

  spi.state = sapiremote::submittedstates::FAILED;
  spi.error.type = sapiremote::errortypes::SOLVE;
  spi.error.message = "oh no";
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, status.error_code);
  EXPECT_EQ(spi.error.message, status.error_message);

  spi.state = sapiremote::submittedstates::RETRYING;
  spi.error.type = sapiremote::errortypes::AUTH;
  spi.error.message = "uh oh";
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_ERR_AUTHENTICATION, status.error_code);
  EXPECT_EQ(spi.error.message, status.error_message);

  spi.state = sapiremote::submittedstates::DONE;
  spi.remoteStatus = sapiremote::remotestatuses::CANCELED;
  spi.error.type = sapiremote::errortypes::INTERNAL;
  spi.error.message = "nooo";
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_ERR_SOLVE_FAILED, status.error_code);
  EXPECT_EQ(spi.error.message, status.error_message);

  spi.state = sapiremote::submittedstates::DONE;
  spi.remoteStatus = sapiremote::remotestatuses::FAILED;
  spi.error.type = sapiremote::errortypes::MEMORY;
  spi.error.message = "qqq";
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_ERR_OUT_OF_MEMORY, status.error_code);
  EXPECT_EQ(spi.error.message, status.error_message);

  spi.state = sapiremote::submittedstates::FAILED;
  spi.error.type = sapiremote::errortypes::NETWORK;
  spi.error.message = "www";
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_ERR_NETWORK, status.error_code);
  EXPECT_EQ(spi.error.message, status.error_message);

  spi.state = sapiremote::submittedstates::FAILED;
  spi.error.type = sapiremote::errortypes::PROTOCOL;
  spi.error.message = "eee";
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  EXPECT_EQ(SAPI_ERR_COMMUNICATION, status.error_code);
  EXPECT_EQ(spi.error.message, status.error_message);
}


TEST(SolversApiTest, StatusStringLengths) {
  sapiremote::SubmittedProblemInfo spi;
  spi.problemId.assign(10000, 'x');
  spi.state = sapiremote::submittedstates::FAILED;
  spi.error.message.assign(10000, 'y');

  auto problem = unique_ptr<MockSubmittedProblem>(new StrictMock<MockSubmittedProblem>);
  auto rsp = make_shared<MockRemoteSubmittedProblem>();
  EXPECT_CALL(*problem, remoteSubmittedProblemImpl()).WillOnce(Return(rsp));
  EXPECT_CALL(*rsp, statusImpl()).WillOnce(Return(spi));

  sapi_ProblemStatus status;
  ASSERT_EQ(SAPI_OK, sapi_asyncStatus(problem.get(), &status));
  auto expectedProblemId = spi.problemId;
  expectedProblemId.resize(sizeof(status.problem_id) - 1);
  EXPECT_EQ(expectedProblemId, status.problem_id);
  auto expectedErrorMessage = spi.error.message;
  expectedErrorMessage.resize(sizeof(status.error_message) - 1);
  EXPECT_EQ(expectedErrorMessage, status.error_message);
}
