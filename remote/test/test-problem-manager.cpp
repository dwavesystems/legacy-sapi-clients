//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <algorithm>
#include <exception>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include <boost/foreach.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <exceptions.hpp>
#include <threadpool.hpp>
#include <answer-service.hpp>
#include <sapi-service.hpp>
#include <retry-service.hpp>
#include <problem-manager.hpp>
#include <types.hpp>

#include "test.hpp"
#include "json-builder.hpp"

using std::bad_alloc;
using std::current_exception;
using std::exception_ptr;
using std::get;
using std::make_shared;
using std::shared_ptr;
using std::sort;
using std::string;
using std::vector;

using testing::InSequence;
using testing::SaveArg;
using testing::WithArg;
using testing::Return;
using testing::Invoke;
using testing::InvokeArgument;
using testing::AnyNumber;
using testing::ElementsAre;
using testing::SizeIs;
using testing::HasSubstr;
using testing::NiceMock;
using testing::StrictMock;
using testing::_;

using sapiremote::AnswerService;
using sapiremote::SubmittedProblemObserver;
using sapiremote::SubmittedProblemObserverPtr;
using sapiremote::SapiService;
using sapiremote::RetryTimer;
using sapiremote::RetryTimerPtr;
using sapiremote::RetryNotifiableWeakPtr;
using sapiremote::SolversSapiCallbackPtr;
using sapiremote::StatusSapiCallbackPtr;
using sapiremote::CancelSapiCallbackPtr;
using sapiremote::FetchAnswerSapiCallbackPtr;
using sapiremote::AnswerServicePtr;
using sapiremote::makeAnswerService;
using sapiremote::AnswerCallback;
using sapiremote::AnswerCallbackPtr;
using sapiremote::makeProblemManager;
using sapiremote::ProblemManagerLimits;
using sapiremote::SolverInfo;
using sapiremote::RemoteProblemInfo;
using sapiremote::Problem;
using sapiremote::NetworkException;
using sapiremote::CommunicationException;
using sapiremote::AuthenticationException;
using sapiremote::NoAnswerException;
using sapiremote::SolveException;

namespace submittedstates = sapiremote::submittedstates;
namespace remotestatuses = sapiremote::remotestatuses;
namespace errortypes = sapiremote::errortypes;

namespace {

auto o = jsonObject();
auto a = jsonArray();

const sapiremote::RetryTiming dummyRetryTiming = { 1, 1, 1.0f };
const ProblemManagerLimits minLimits = {1, 1, 1};

class MockAnswerCallback : public AnswerCallback {
public:
  MOCK_METHOD1(errorImpl, void(std::exception_ptr));
  MOCK_METHOD2(answerImpl, void(string&, json::Value&));
};

class MockSubmittedProblemObserver : public SubmittedProblemObserver {
public:
  MOCK_METHOD0(notifySubmittedImpl, void());
  MOCK_METHOD0(notifyDoneImpl, void());
  MOCK_METHOD0(notifyErrorImpl, void());
};

class MockAnswerService : public AnswerService {
public:
  MOCK_METHOD1(postDoneImpl, void(SubmittedProblemObserverPtr));
  MOCK_METHOD1(postSubmittedImpl, void(SubmittedProblemObserverPtr));
  MOCK_METHOD1(postErrorImpl, void(SubmittedProblemObserverPtr));
  MOCK_METHOD3(postAnswerImpl, void(AnswerCallbackPtr, std::string&, json::Value&));
  MOCK_METHOD2(postAnswerErrorImpl, void(AnswerCallbackPtr, exception_ptr));
};

class MockSapiService : public SapiService {
public:
  MOCK_METHOD1(fetchSolversImpl, void(SolversSapiCallbackPtr));
  MOCK_METHOD2(submitProblemsImpl, void(vector<Problem>&, StatusSapiCallbackPtr));
  MOCK_METHOD2(multiProblemStatusImpl,  void(const vector<string>& ids, StatusSapiCallbackPtr callback));
  MOCK_METHOD2(fetchAnswerImpl, void(const string& id, FetchAnswerSapiCallbackPtr callback));
  MOCK_METHOD2(cancelProblemsImpl, void(const vector<string>& ids, CancelSapiCallbackPtr));
};

class MockRetryTimer : public RetryTimer {
public:
  MOCK_METHOD0(retryImpl, RetryTimer::RetryAction());
  MOCK_METHOD0(successImpl, void());
  MockRetryTimer() {
    ON_CALL(*this, retryImpl()).WillByDefault(Return(RetryTimer::SHUTDOWN));
  }
};

class MockRetryTimerService : public sapiremote::RetryTimerService {
public:
  MOCK_METHOD0(shutdownImpl, void());
  MOCK_METHOD2(createRetryTimerImpl, RetryTimerPtr(const RetryNotifiableWeakPtr&, const sapiremote::RetryTiming&));
  MockRetryTimerService() {
    ON_CALL(*this, createRetryTimerImpl(_, _)).WillByDefault(Return(make_shared<NiceMock<MockRetryTimer>>()));
  }
};

class SolversCallback {
private:
  vector<SolverInfo> solverInfo_;
public:
  SolversCallback(vector<SolverInfo> solverInfo) : solverInfo_(solverInfo) {}
  void operator()(SolversSapiCallbackPtr callback) const { callback->complete(solverInfo_); }
};

bool byId(const SolverInfo& a, const SolverInfo& b) {
  return a.id < b.id;
}

ACTION_P2(CompleteWithAnswer, type, answer) { arg0->complete(type, answer); }

void CompleteAnswerCallback(AnswerCallbackPtr callback, std::string type, json::Value answer) {
  callback->answer(type, answer);
}

void FailAnswerCallback(AnswerCallbackPtr callback, exception_ptr e) {
  callback->error(e);
}

void CompleteCancelCallback(vector<string>, CancelSapiCallbackPtr callback) {
  callback->complete();
}

}



TEST(ProblemManagerTest, fetchSolvers) {
  vector<SolverInfo> providedSolverInfo;
  providedSolverInfo.push_back(makeSolverInfo("s1", (o, "hello", 123)));
  providedSolverInfo.push_back(makeSolverInfo("s2", (o, "asdf", false)));
  providedSolverInfo.push_back(makeSolverInfo("s3", (o, "111", 222, "qwerty", json::Null())));

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).WillOnce(Invoke(SolversCallback(providedSolverInfo)));
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto answerService = make_shared<MockAnswerService>();
  auto retryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, answerService, retryService, dummyRetryTiming, minLimits);
  auto solvers = problemManager->fetchSolvers();

  vector<SolverInfo> returnedSolverInfo;
  BOOST_FOREACH( const auto& e, solvers ) {
    returnedSolverInfo.push_back(makeSolverInfo(e.first, e.second->properties()));
  }
  sort(returnedSolverInfo.begin(), returnedSolverInfo.end(), byId);
  EXPECT_EQ(returnedSolverInfo, providedSolverInfo);
}



TEST(ProblemManagerTest, submitProblem) {
  Problem problem("solver-a", "blarg", (a, 123, 456).value(), (o, "hello", 234).object());
  vector<Problem> problems;
  problems.push_back(problem);

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(problems, _)).Times(1);
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
}



TEST(ProblemManagerTest, addProblem) {
  auto problemId = string("that one");

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(ElementsAre(problemId), _)).Times(1);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  EXPECT_CALL(*mockAnswerService, postSubmittedImpl(_)).Times(0);
  EXPECT_CALL(*mockAnswerService, postErrorImpl(_)).Times(AnyNumber());

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  problemManager->addProblem(problemId);
}



TEST(ProblemManagerTest, problemStatus) {
  auto problemId = string("2345");

  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(ElementsAre(problemId), _)).Times(3)
    .WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  vector<RemoteProblemInfo> problemInfo;
  problemInfo.push_back(makeProblemInfo(problemId, "", remotestatuses::IN_PROGRESS));

  auto sp = problemManager->addProblem(problemId);
  ASSERT_TRUE(!!statusCallback);
  auto cb = statusCallback;
  statusCallback.reset();
  cb->complete(problemInfo);
  ASSERT_TRUE(!!statusCallback && cb != statusCallback);
  StatusSapiCallbackPtr(statusCallback)->complete(problemInfo);
}



TEST(ProblemManagerTest, bugSC45) {
  auto problem1 = string("p1");
  auto problem2 = string("p2");

  StatusSapiCallbackPtr singleStatusCallback;
  StatusSapiCallbackPtr pairStatusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(SizeIs(1), _))
    .WillRepeatedly(SaveArg<1>(&singleStatusCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(SizeIs(2), _))
    .WillOnce(SaveArg<1>(&pairStatusCallback));

  auto mockAnswerService = make_shared<MockAnswerService>();
  EXPECT_CALL(*mockAnswerService, postSubmittedImpl(_)).Times(AnyNumber());
  EXPECT_CALL(*mockAnswerService, postErrorImpl(_)).Times(2);

  vector<RemoteProblemInfo> singleProblemInfo;
  singleProblemInfo.push_back(makeProblemInfo("", "", remotestatuses::IN_PROGRESS));

  vector<RemoteProblemInfo> pairProblemInfo;
  pairProblemInfo.push_back(makeProblemInfo("", "", remotestatuses::FAILED));
  pairProblemInfo.push_back(makeProblemInfo("", "", remotestatuses::CANCELED));

  auto limits = minLimits;
  limits.maxIdsPerStatusQuery = 100;
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, limits);
  auto sp1 = problemManager->addProblem(problem1);
  auto sp2 = problemManager->addProblem(problem2);
  auto mockObserver1 = make_shared<MockSubmittedProblemObserver>();
  auto mockObserver2 = make_shared<MockSubmittedProblemObserver>();
  sp1->addSubmittedProblemObserver(mockObserver1);
  sp2->addSubmittedProblemObserver(mockObserver2);

  ASSERT_TRUE(!!singleStatusCallback);
  singleStatusCallback->complete(singleProblemInfo);
  singleStatusCallback.reset();

  ASSERT_TRUE(!!pairStatusCallback);
  pairStatusCallback->complete(pairProblemInfo);
}



TEST(ProblemManagerTest, problemIdSubmit) {
  Problem problem("solver-a", "blarg", json::Null(), o);
  vector<Problem> problems;
  problems.push_back(problem);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(problems, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(AnyNumber());
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  vector<RemoteProblemInfo> problemInfo;
  problemInfo.push_back(makeProblemInfo("111", "", remotestatuses::IN_PROGRESS));

  EXPECT_EQ("", sp->problemId());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(problemInfo);
  EXPECT_EQ("111", sp->problemId());
}



TEST(ProblemManagerTest, problemIdAdd) {
  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(AnyNumber());
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->addProblem("p123");
  EXPECT_EQ("p123", sp->problemId());
}



TEST(ProblemManagerTest, doneSubmitCompleted) {
  Problem problem("solver-a", "blarg", json::Null(), o);
  vector<Problem> problems;
  problems.push_back(problem);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(problems, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  vector<RemoteProblemInfo> problemInfo;
  problemInfo.push_back(makeProblemInfo("1234", "", remotestatuses::COMPLETED));

  EXPECT_FALSE(sp->done());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(problemInfo);
  EXPECT_TRUE(sp->done());
}



TEST(ProblemManagerTest, doneSubmitFailed) {
  Problem problem("solver-a", "blarg", json::Null(), o);
  vector<Problem> problems;
  problems.push_back(problem);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(problems, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  vector<RemoteProblemInfo> problemInfo;
  problemInfo.push_back(makeProblemInfo("1234", "", remotestatuses::FAILED));

  EXPECT_FALSE(sp->done());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(problemInfo);
  EXPECT_TRUE(sp->done());
}


TEST(ProblemManagerTest, DoneSubmitFailedCommunication) {
  Problem problem("solver-a", "blarg", json::Null(), o);
  vector<Problem> problems;
  problems.push_back(problem);

  exception_ptr e;
  try {
    throw CommunicationException("test", "test://blah");
  } catch (CommunicationException& ex) {
    e = current_exception();
  }

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(problems, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  EXPECT_FALSE(sp->done());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);
  EXPECT_TRUE(sp->done());
}


TEST(ProblemManagerTest, doneSubmitCancelled) {
  Problem problem("solver-a", "blarg", json::Null(), o);
  vector<Problem> problems;
  problems.push_back(problem);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(problems, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  vector<RemoteProblemInfo> problemInfo;
  problemInfo.push_back(makeProblemInfo("1234", "", remotestatuses::CANCELED));

  EXPECT_FALSE(sp->done());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(problemInfo);
  EXPECT_TRUE(sp->done());
}



TEST(ProblemManagerTest, doneStatusCompleted) {
  auto problemId = string("the-id");
  Problem problem("solver-a", "blarg", json::Null(), o);
  vector<Problem> problems;
  problems.push_back(problem);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(problems, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(ElementsAre(problemId), _))
    .WillOnce(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  vector<RemoteProblemInfo> submitInfo;
  submitInfo.push_back(makeProblemInfo(problemId, "", remotestatuses::PENDING));
  vector<RemoteProblemInfo> statusInfo;
  statusInfo.push_back(makeProblemInfo(problemId, "", remotestatuses::COMPLETED));

  EXPECT_FALSE(sp->done());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(submitInfo);
  EXPECT_FALSE(sp->done());
  ASSERT_TRUE(!!statusCallback);
  statusCallback->complete(statusInfo);
  EXPECT_TRUE(sp->done());
}



TEST(ProblemManagerTest, doneStatusFailed) {
  auto problemId = string("the-id");
  Problem problem("solver-a", "blarg", json::Null(), o);
  vector<Problem> problems;
  problems.push_back(problem);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(problems, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(ElementsAre(problemId), _))
    .WillOnce(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  vector<RemoteProblemInfo> submitInfo;
  submitInfo.push_back(makeProblemInfo(problemId, "", remotestatuses::PENDING));
  vector<RemoteProblemInfo> statusInfo;
  statusInfo.push_back(makeProblemInfo(problemId, "", remotestatuses::FAILED));

  EXPECT_FALSE(sp->done());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(submitInfo);
  EXPECT_FALSE(sp->done());
  ASSERT_TRUE(!!statusCallback);
  statusCallback->complete(statusInfo);
  EXPECT_TRUE(sp->done());
}



TEST(ProblemManagerTest, doneStatusCancelled) {
  auto problemId = string("the-id");
  Problem problem("solver-a", "blarg", json::Null(), o);
  vector<Problem> problems;
  problems.push_back(problem);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(problems, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(ElementsAre(problemId), _))
    .WillOnce(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  vector<RemoteProblemInfo> submitInfo;
  submitInfo.push_back(makeProblemInfo(problemId, "", remotestatuses::PENDING));
  vector<RemoteProblemInfo> statusInfo;
  statusInfo.push_back(makeProblemInfo(problemId, "", remotestatuses::CANCELED));

  EXPECT_FALSE(sp->done());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(submitInfo);
  EXPECT_FALSE(sp->done());
  ASSERT_TRUE(!!statusCallback);
  statusCallback->complete(statusInfo);
  EXPECT_TRUE(sp->done());
}



TEST(ProblemManagerTest, answer) {
  auto problemType = string("trouble");
  auto problemId = string("12345");
  auto problemIds = vector<string>(1, problemId);
  StatusSapiCallbackPtr statusCallback;

  auto expectedAnswer = (o, "data", 789).value();

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(problemIds, _)).WillOnce(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(problemId, _))
      .WillOnce(WithArg<1>(CompleteWithAnswer(problemType, expectedAnswer)));
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  EXPECT_CALL(*mockAnswerService, postAnswerImpl(_, _, _)).WillOnce(Invoke(CompleteAnswerCallback));
  EXPECT_CALL(*mockAnswerService, postAnswerErrorImpl(_, _)).WillOnce(Invoke(FailAnswerCallback));

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->addProblem(problemId);
  EXPECT_THROW(sp->answer(), NoAnswerException);

  ASSERT_TRUE(!!statusCallback);
  vector<RemoteProblemInfo> statusInfo;
  statusInfo.push_back(makeProblemInfo(problemId, problemType, remotestatuses::COMPLETED));
  statusCallback->complete(statusInfo);

  auto answer = sp->answer();
  EXPECT_EQ(problemType, get<0>(answer));
  EXPECT_EQ(expectedAnswer, get<1>(answer));
}



TEST(ProblemManagerTest, answerCallback) {
  auto problemType = string("blarg");
  auto problemId = string("3456");
  auto problemIds = vector<string>(1, problemId);
  StatusSapiCallbackPtr statusCallback;

  auto answer = (o, "data", 678).value();
  auto mockAnswerCallback1 = make_shared<MockAnswerCallback>();
  EXPECT_CALL(*mockAnswerCallback1, errorImpl(Thrown(NoAnswerException))).Times(1);
  auto mockAnswerCallback2 = make_shared<MockAnswerCallback>();
  EXPECT_CALL(*mockAnswerCallback2, answerImpl(problemType, answer)).Times(1);

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(problemIds, _)).WillOnce(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(problemId, _))
      .WillOnce(WithArg<1>(CompleteWithAnswer(problemType, answer)));
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  EXPECT_CALL(*mockAnswerService, postAnswerImpl(_, _, _)).WillOnce(Invoke(CompleteAnswerCallback));
  EXPECT_CALL(*mockAnswerService, postAnswerErrorImpl(_, _)).WillOnce(Invoke(FailAnswerCallback));

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->addProblem(problemId);
  sp->answer(mockAnswerCallback1);

  ASSERT_TRUE(!!statusCallback);
  vector<RemoteProblemInfo> statusInfo;
  statusInfo.push_back(makeProblemInfo(problemId, "", remotestatuses::COMPLETED));
  statusCallback->complete(statusInfo);

  sp->answer(mockAnswerCallback2);
}



TEST(ProblemManagerTest, answerCallbackFailedSubmission) {
  StatusSapiCallbackPtr submitCallback;

  exception_ptr e;
  try {
    throw NetworkException("test");
  } catch (NetworkException&) {
    e = current_exception();
  }

  auto mockAnswerCallback1 = make_shared<MockAnswerCallback>();
  EXPECT_CALL(*mockAnswerCallback1, errorImpl(Thrown(NoAnswerException))).Times(1);
  auto mockAnswerCallback2 = make_shared<MockAnswerCallback>();
  EXPECT_CALL(*mockAnswerCallback2, errorImpl(Thrown(NetworkException))).Times(1);

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  EXPECT_CALL(*mockAnswerService, postAnswerErrorImpl(_, _)).WillRepeatedly(Invoke(FailAnswerCallback));

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem("", "", json::Null(), json::Object());
  sp->answer(mockAnswerCallback1);;

  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);
  sp->answer(mockAnswerCallback2);
}



TEST(ProblemManagerTest, answerCallbackFailedSolve) {
  auto problemId = string("aaa");
  StatusSapiCallbackPtr statusCallback;

  auto mockAnswerCallback1 = make_shared<MockAnswerCallback>();
  EXPECT_CALL(*mockAnswerCallback1, errorImpl(Thrown(NoAnswerException))).Times(1);
  auto mockAnswerCallback2 = make_shared<MockAnswerCallback>();
  EXPECT_CALL(*mockAnswerCallback2, errorImpl(Thrown(SolveException))).Times(1);

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(ElementsAre(problemId), _))
    .WillOnce(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  EXPECT_CALL(*mockAnswerService, postAnswerErrorImpl(_, _)).WillRepeatedly(Invoke(FailAnswerCallback));

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->addProblem(problemId);
  sp->answer(mockAnswerCallback1);;

  ASSERT_TRUE(!!statusCallback);
  vector<RemoteProblemInfo> statusInfo;
  statusInfo.push_back(makeProblemInfo(problemId, "", remotestatuses::FAILED));
  statusCallback->complete(statusInfo);

  sp->answer(mockAnswerCallback2);
}



TEST(ProblemManagerTest, cancel) {
  auto problemId = string("11122");

  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(vector<string>(1, problemId), _))
    .Times(2).WillRepeatedly(Invoke(CompleteCancelCallback));

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->addProblem(problemId);
  sp->cancel();
  sp->cancel();

  ASSERT_TRUE(!!statusCallback);
  vector<RemoteProblemInfo> statusInfo;
  statusInfo.push_back(makeProblemInfo(problemId, "", remotestatuses::IN_PROGRESS));
  StatusSapiCallbackPtr(statusCallback)->complete(statusInfo);

  sp->cancel();
  StatusSapiCallbackPtr(statusCallback)->complete(statusInfo);
}



TEST(ProblemManagerTest, errorMessage) {
  auto msg = "bad news!";
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillOnce(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  EXPECT_CALL(*mockAnswerService, postAnswerErrorImpl(_, _)).WillRepeatedly(Invoke(FailAnswerCallback));
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->addProblem("123");

  ASSERT_TRUE(!!statusCallback);
  vector<RemoteProblemInfo> statusInfo;
  statusInfo.push_back(makeFailedProblemInfo("123", "qwerty", msg));
  StatusSapiCallbackPtr(statusCallback)->complete(statusInfo);

  bool caught = false;
  try {
    sp->answer();
  } catch (SolveException& e) {
    EXPECT_NE(string::npos, string(e.what()).find(msg));
    caught = true;
  }
  EXPECT_TRUE(caught);
}



TEST(ProblemManagerTest, problemInfoCompleted) {
  auto problemId = string("the-id");
  Problem problem("solver-a", "blarg", json::Null(), o);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback1;
  StatusSapiCallbackPtr statusCallback2;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _))
    .WillOnce(SaveArg<1>(&statusCallback1))
    .WillOnce(SaveArg<1>(&statusCallback2));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  auto pendingInfo = vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::PENDING)};
  auto inProgressInfo = vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::IN_PROGRESS)};
  auto completedInfo = vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::COMPLETED)};

  auto spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::SUBMITTING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);

  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(pendingInfo);
  spi = sp->status();
  EXPECT_EQ(problemId, spi.problemId);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::PENDING, spi.remoteStatus);

  ASSERT_TRUE(!!statusCallback1);
  statusCallback1->complete(inProgressInfo);
  spi = sp->status();
  EXPECT_EQ(problemId, spi.problemId);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::IN_PROGRESS, spi.remoteStatus);

  ASSERT_TRUE(!!statusCallback2);
  statusCallback2->complete(completedInfo);
  spi = sp->status();
  EXPECT_EQ(problemId, spi.problemId);
  EXPECT_EQ(submittedstates::DONE, spi.state);
  EXPECT_EQ(submittedstates::DONE, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::COMPLETED, spi.remoteStatus);
}



TEST(ProblemManagerTest, problemInfoCancelled) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  auto cancelledInfo = vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::CANCELED)};
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(cancelledInfo);

  auto spi = sp->status();
  EXPECT_EQ(problemId, spi.problemId);
  EXPECT_EQ(submittedstates::DONE, spi.state);
  EXPECT_EQ(submittedstates::DONE, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::CANCELED, spi.remoteStatus);
  EXPECT_EQ(errortypes::SOLVE, spi.error.type);
  EXPECT_THAT(spi.error.message, HasSubstr("cancel"));
}



TEST(ProblemManagerTest, problemInfoFailed) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);
  auto errorMessage = "bad news";

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  auto cancelledInfo = vector<RemoteProblemInfo>{makeFailedProblemInfo(problemId, "", errorMessage)};
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(cancelledInfo);

  auto spi = sp->status();
  EXPECT_EQ(problemId, spi.problemId);
  EXPECT_EQ(submittedstates::DONE, spi.state);
  EXPECT_EQ(submittedstates::DONE, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::FAILED, spi.remoteStatus);
  EXPECT_EQ(errortypes::SOLVE, spi.error.type);
  EXPECT_THAT(spi.error.message, HasSubstr(errorMessage));
}



TEST(ProblemManagerTest, problemInfoNetworkError) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  exception_ptr e;
  string errorMessage;
  try {
    throw NetworkException("test");
  } catch (NetworkException& ex) {
    e = current_exception();
    errorMessage = ex.what();
  }

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);

  // error on submission
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);

  auto spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);

  // error on status poll
  submitCallback.reset();
  sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  auto statusInfo = vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::IN_PROGRESS)};
  submitCallback->complete(statusInfo);
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);

  spi = sp->status();
  EXPECT_EQ(problemId, spi.problemId);
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::IN_PROGRESS, spi.remoteStatus);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);
}



TEST(ProblemManagerTest, problemInfoCommunicationError) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  exception_ptr e;
  string errorMessage;
  try {
    throw CommunicationException("test", "test://blah");
  } catch (CommunicationException& ex) {
    e = current_exception();
    errorMessage = ex.what();
  }

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);

  // error on submission
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);

  auto spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::PROTOCOL, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);

  // error on status poll
  submitCallback.reset();
  sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  auto statusInfo = vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::IN_PROGRESS)};
  submitCallback->complete(statusInfo);
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);

  spi = sp->status();
  EXPECT_EQ(problemId, spi.problemId);
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::IN_PROGRESS, spi.remoteStatus);
  EXPECT_EQ(errortypes::PROTOCOL, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);
}



TEST(ProblemManagerTest, problemInfoAuthenticationError) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  exception_ptr e;
  string errorMessage;
  try {
    throw AuthenticationException();
  } catch (AuthenticationException& ex) {
    e = current_exception();
    errorMessage = ex.what();
  }

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);

  // error on submission
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);

  auto spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::AUTH, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);

  // error on status poll
  submitCallback.reset();
  sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  auto statusInfo = vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::IN_PROGRESS)};
  submitCallback->complete(statusInfo);
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);

  spi = sp->status();
  EXPECT_EQ(problemId, spi.problemId);
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::IN_PROGRESS, spi.remoteStatus);
  EXPECT_EQ(errortypes::AUTH, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);
}



TEST(ProblemManagerTest, problemInfoMemoryError) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  exception_ptr e;
  try {
    throw bad_alloc();
  } catch (bad_alloc& ex) {
    e = current_exception();
  }

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);

  // error on submission
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);

  auto spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::MEMORY, spi.error.type);
  EXPECT_TRUE(spi.error.message.empty());

  // error on status poll
  submitCallback.reset();
  sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  auto statusInfo = vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::IN_PROGRESS)};
  submitCallback->complete(statusInfo);
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);

  spi = sp->status();
  EXPECT_EQ(problemId, spi.problemId);
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::IN_PROGRESS, spi.remoteStatus);
  EXPECT_EQ(errortypes::MEMORY, spi.error.type);
  EXPECT_TRUE(spi.error.message.empty());
}



TEST(ProblemManagerTest, problemInfoInternalError) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  exception_ptr e;
  try {
    throw 5;
  } catch (...) {
    e = current_exception();
  }

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);

  // error on submission
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);

  auto spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::INTERNAL, spi.error.type);

  // error on status poll
  submitCallback.reset();
  sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  auto statusInfo = vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::IN_PROGRESS)};
  submitCallback->complete(statusInfo);
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);

  spi = sp->status();
  EXPECT_EQ(problemId, spi.problemId);
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::IN_PROGRESS, spi.remoteStatus);
  EXPECT_EQ(errortypes::INTERNAL, spi.error.type);
}



TEST(ProblemManagerTest, ProblemInfoTime) {
  auto problemId = string("the-id");
  Problem problem("solver-a", "blarg", json::Null(), o);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback1;
  StatusSapiCallbackPtr statusCallback2;

  auto mockSapiService = make_shared<StrictMock<MockSapiService>>();
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _))
    .WillOnce(SaveArg<1>(&statusCallback1))
    .WillOnce(SaveArg<1>(&statusCallback2));

  auto mockAnswerService = make_shared<MockAnswerService>();
  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  auto pendingInfo = vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::PENDING)};
  auto inProgressInfo = vector<RemoteProblemInfo>{
      makeProblemInfo(problemId, "", remotestatuses::IN_PROGRESS, "sometime")};
  auto completedInfo = vector<RemoteProblemInfo>{
      makeProblemInfo(problemId, "", remotestatuses::COMPLETED, "haha, changed", "who knows")};

  auto spi = sp->status();
  EXPECT_TRUE(spi.submittedOn.empty());
  EXPECT_TRUE(spi.solvedOn.empty());

  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(pendingInfo);
  spi = sp->status();
  EXPECT_TRUE(spi.submittedOn.empty());
  EXPECT_TRUE(spi.solvedOn.empty());

  ASSERT_TRUE(!!statusCallback1);
  statusCallback1->complete(inProgressInfo);
  spi = sp->status();
  EXPECT_EQ("sometime", spi.submittedOn);
  EXPECT_TRUE(spi.solvedOn.empty());

  ASSERT_TRUE(!!statusCallback2);
  statusCallback2->complete(completedInfo);
  spi = sp->status();
  EXPECT_EQ("haha, changed", spi.submittedOn);
  EXPECT_EQ("who knows", spi.solvedOn);
}
