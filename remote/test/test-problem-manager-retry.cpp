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

using std::current_exception;
using std::exception_ptr;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::vector;

using testing::AnyNumber;
using testing::DoAll;
using testing::SaveArg;
using testing::Return;
using testing::Invoke;
using testing::NiceMock;
using testing::_;

using sapiremote::AnswerService;
using sapiremote::SubmittedProblemObserverPtr;
using sapiremote::SapiService;
using sapiremote::RetryTimer;
using sapiremote::RetryTimerPtr;
using sapiremote::RetryNotifiable;
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
using sapiremote::RemoteProblemInfo;
using sapiremote::Problem;
using sapiremote::NetworkException;
using sapiremote::CommunicationException;

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

class MockRetryNotifiable : public RetryNotifiable {
public:
  MOCK_METHOD0(notifyImpl, void());
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

void CompleteAnswerCallback(AnswerCallbackPtr callback, std::string type, json::Value answer) {
  callback->answer(type, answer);
}

void FailAnswerCallback(AnswerCallbackPtr callback, exception_ptr e) {
  callback->error(e);
}

}



TEST(ProblemManagerAutoRetryTest, SubmitUntilSuccess) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto e = exception_ptr();
  string errorMessage;
  try {
    throw NetworkException("something bad");
  } catch (NetworkException& ne) {
    e = current_exception();
    errorMessage = ne.what();
  }
  ASSERT_NE(exception_ptr(), e);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(1);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(2).WillRepeatedly(Return(RetryTimer::RETRY));

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // fail 1
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);
  auto spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);

  // fail 2
  submitCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);
  spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);

  // success
  submitCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::COMPLETED)});
  spi = sp->status();
  EXPECT_EQ(problemId, spi.problemId);
  EXPECT_EQ(submittedstates::DONE, spi.state);
  EXPECT_EQ(submittedstates::DONE, spi.lastGoodState);
}



TEST(ProblemManagerAutoRetryTest, SubmitUntilNonNetworkFailure) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto networkError = exception_ptr();
  string networkErrorMsg;
  try {
    throw NetworkException("network error");
  } catch (NetworkException& ne) {
    networkError = current_exception();
    networkErrorMsg = ne.what();
  }
  ASSERT_NE(exception_ptr(), networkError);

  auto otherError = exception_ptr();
  string otherErrorMsg;
  try {
    throw CommunicationException("protocol error", "test://test");
  } catch (CommunicationException& ce) {
    otherError = current_exception();
    otherErrorMsg = ce.what();
  }
  ASSERT_NE(exception_ptr(), otherError);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(1);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(2).WillRepeatedly(Return(RetryTimer::RETRY));

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // network fail 1
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(networkError);
  auto spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(networkErrorMsg, spi.error.message);

  // network fail 2
  submitCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(networkError);
  spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(networkErrorMsg, spi.error.message);

  // other fail
  submitCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(otherError);
  spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::PROTOCOL, spi.error.type);
  EXPECT_EQ(otherErrorMsg, spi.error.message);

  // problem manager still working
  submitCallback.reset();
  auto sp2 = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
}



TEST(ProblemManagerAutoRetryTest, SubmitUntilFailure) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto e = exception_ptr();
  string errorMessage;
  try {
    throw NetworkException("something bad");
  } catch (NetworkException& ne) {
    e = current_exception();
    errorMessage = ne.what();
  }
  ASSERT_NE(exception_ptr(), e);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(0);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(1).WillRepeatedly(Return(RetryTimer::FAIL));
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(2).WillRepeatedly(Return(RetryTimer::RETRY)).RetiresOnSaturation();

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // retry fail 1
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);
  auto spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);

  // retry fail 2
  submitCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);
  spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);

  // real fail
  submitCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);
  spi = sp->status();
  EXPECT_TRUE(spi.problemId.empty());
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);
}



TEST(ProblemManagerAutoRetryTest, StatusUntilSuccess) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto e = exception_ptr();
  string errorMessage;
  try {
    throw NetworkException("something bad");
  } catch (NetworkException& ne) {
    e = current_exception();
    errorMessage = ne.what();
  }
  ASSERT_NE(exception_ptr(), e);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(1);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(2).WillRepeatedly(Return(RetryTimer::RETRY));

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::PENDING)});

  // fail 1
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);
  auto spi = sp->status();
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);

  // fail 2
  statusCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);
  spi = sp->status();
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);

  // success
  statusCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::IN_PROGRESS)});
  spi = sp->status();
  EXPECT_EQ(submittedstates::SUBMITTED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::IN_PROGRESS, spi.remoteStatus);
}



TEST(ProblemManagerAutoRetryTest, StatusUntilNonNetworkFailure) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto networkError = exception_ptr();
  string networkErrorMsg;
  try {
    throw NetworkException("network error");
  } catch (NetworkException& ne) {
    networkError = current_exception();
    networkErrorMsg = ne.what();
  }
  ASSERT_NE(exception_ptr(), networkError);

  auto otherError = exception_ptr();
  string otherErrorMsg;
  try {
    throw CommunicationException("protocol error", "test://test");
  } catch (CommunicationException& ce) {
    otherError = current_exception();
    otherErrorMsg = ce.what();
  }
  ASSERT_NE(exception_ptr(), otherError);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(1);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(2).WillRepeatedly(Return(RetryTimer::RETRY));

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::PENDING)});

  // network fail 1
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(networkError);
  auto spi = sp->status();
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(networkErrorMsg, spi.error.message);

  // network fail 2
  statusCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(networkError);
  spi = sp->status();
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(networkErrorMsg, spi.error.message);

  // other fail
  statusCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(otherError);
  spi = sp->status();
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(errortypes::PROTOCOL, spi.error.type);
  EXPECT_EQ(otherErrorMsg, spi.error.message);

  // problem manager still working
  submitCallback.reset();
  auto sp2 = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
}



TEST(ProblemManagerAutoRetryTest, StatusUntilFailure) {
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto e = exception_ptr();
  string errorMessage;
  try {
    throw NetworkException("something bad");
  } catch (NetworkException& ne) {
    e = current_exception();
    errorMessage = ne.what();
  }
  ASSERT_NE(exception_ptr(), e);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(0);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(1).WillRepeatedly(Return(RetryTimer::FAIL));
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(2).WillRepeatedly(Return(RetryTimer::RETRY)).RetiresOnSaturation();

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo("id", "", remotestatuses::PENDING)});

  // retry fail 1
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);
  auto spi = sp->status();
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);

  // retry fail 2
  statusCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);
  spi = sp->status();
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);

  // real fail
  statusCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);
  spi = sp->status();
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_EQ(errorMessage, spi.error.message);
}



TEST(ProblemManagerAutoRetryTest, FetchAnswerUntilSuccess) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);
  auto answer = (o, "stuff", 123).value();

  auto e = exception_ptr();
  try {
    throw NetworkException("something bad");
  } catch (NetworkException& ne) {
    e = current_exception();
  }
  ASSERT_NE(exception_ptr(), e);

  StatusSapiCallbackPtr submitCallback;
  FetchAnswerSapiCallbackPtr fetchCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(problemId, _)).Times(3).WillRepeatedly(SaveArg<1>(&fetchCallback));
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  EXPECT_CALL(*mockAnswerService, postAnswerImpl(_, _, _)).WillOnce(Invoke(CompleteAnswerCallback));
  EXPECT_CALL(*mockAnswerService, postAnswerErrorImpl(_, _)).Times(0);

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(1);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(2).WillRepeatedly(Return(RetryTimer::RETRY));

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::COMPLETED)});

  auto mockAnswerCallback = make_shared<MockAnswerCallback>();
  EXPECT_CALL(*mockAnswerCallback, answerImpl(problem.type(), answer)).Times(1);
  sp->answer(mockAnswerCallback);

  // fail 1
  ASSERT_TRUE(!!fetchCallback);
  fetchCallback->error(e);

  // fail 2
  fetchCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!fetchCallback);
  fetchCallback->error(e);

  // success
  fetchCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!fetchCallback);
  fetchCallback->complete(problem.type(), answer);
}



TEST(ProblemManagerAutoRetryTest, FetchAnswerUntilNonNetworkFailure) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);
  auto answer = (o, "stuff", 123).value();

  auto networkError = exception_ptr();
  try {
    throw NetworkException("network error");
  } catch (NetworkException& ne) {
    networkError = current_exception();
  }
  ASSERT_NE(exception_ptr(), networkError);

  auto otherError = exception_ptr();
  try {
    throw CommunicationException("protocol error", "test://test");
  } catch (CommunicationException& ce) {
    otherError = current_exception();
  }
  ASSERT_NE(exception_ptr(), otherError);

  StatusSapiCallbackPtr submitCallback;
  FetchAnswerSapiCallbackPtr fetchCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(problemId, _)).Times(3).WillRepeatedly(SaveArg<1>(&fetchCallback));
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  EXPECT_CALL(*mockAnswerService, postAnswerImpl(_, _, _)).Times(0);
  EXPECT_CALL(*mockAnswerService, postAnswerErrorImpl(_, _)).WillOnce(Invoke(FailAnswerCallback));

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(1);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(2).WillRepeatedly(Return(RetryTimer::RETRY));

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::COMPLETED)});

  auto mockAnswerCallback = make_shared<MockAnswerCallback>();
  EXPECT_CALL(*mockAnswerCallback, answerImpl(problem.type(), answer)).Times(0);
  EXPECT_CALL(*mockAnswerCallback, errorImpl(Thrown(CommunicationException))).Times(1);
  sp->answer(mockAnswerCallback);

  // network fail 1
  ASSERT_TRUE(!!fetchCallback);
  fetchCallback->error(networkError);

  // network fail 2
  fetchCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!fetchCallback);
  fetchCallback->error(networkError);

  // other fail
  fetchCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!fetchCallback);
  fetchCallback->error(otherError);

  // problem manager still working
  submitCallback.reset();
  auto sp2 = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
}



TEST(ProblemManagerAutoRetryTest, FetchAnswerUntilFailure) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto e = exception_ptr();
  try {
    throw NetworkException("something bad");
  } catch (NetworkException& ne) {
    e = current_exception();
  }
  ASSERT_NE(exception_ptr(), e);

  StatusSapiCallbackPtr submitCallback;
  FetchAnswerSapiCallbackPtr fetchCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(problemId, _)).Times(3).WillRepeatedly(SaveArg<1>(&fetchCallback));
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockAnswerService = make_shared<MockAnswerService>();
  EXPECT_CALL(*mockAnswerService, postAnswerImpl(_, _, _)).Times(0);
  EXPECT_CALL(*mockAnswerService, postAnswerErrorImpl(_, _)).WillOnce(Invoke(FailAnswerCallback));

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(0);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(1).WillRepeatedly(Return(RetryTimer::FAIL));
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(2).WillRepeatedly(Return(RetryTimer::RETRY)).RetiresOnSaturation();

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, mockAnswerService, mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::COMPLETED)});

  auto mockAnswerCallback = make_shared<MockAnswerCallback>();
  EXPECT_CALL(*mockAnswerCallback, errorImpl(Thrown(NetworkException))).Times(1);
  sp->answer(mockAnswerCallback);

  // fail 1
  ASSERT_TRUE(!!fetchCallback);
  fetchCallback->error(e);

  // fail 2
  fetchCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!fetchCallback);
  fetchCallback->error(e);

  // real fail
  fetchCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!fetchCallback);
  fetchCallback->error(e);
}



TEST(ProblemManagerAutoRetryTest, CancelUntilSuccess) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto e = exception_ptr();
  try {
    throw NetworkException("something bad");
  } catch (NetworkException& ne) {
    e = current_exception();
  }
  ASSERT_NE(exception_ptr(), e);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;
  CancelSapiCallbackPtr cancelCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(vector<string>{problemId}, _))
    .Times(3).WillRepeatedly(SaveArg<1>(&cancelCallback));

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(1);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(4).WillRepeatedly(Return(RetryTimer::RETRY));

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  sp->cancel();
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::PENDING)});
  submitCallback.reset();

  // fail 1
  ASSERT_TRUE(!!cancelCallback);
  cancelCallback->error(e);
  cancelCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);
  statusCallback.reset();

  // fail 2
  notifiable->notify();
  ASSERT_TRUE(!!cancelCallback);
  cancelCallback->error(e);
  cancelCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);
  statusCallback.reset();

  // success
  notifiable->notify();
  ASSERT_TRUE(!!cancelCallback);
  cancelCallback->complete();
  cancelCallback.reset();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->complete({makeProblemInfo(problemId, "", remotestatuses::IN_PROGRESS)});
  statusCallback.reset();
}



TEST(ProblemManagerAutoRetryTest, CancelUntilNonNetworkFailure) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto networkError = exception_ptr();
  try {
    throw NetworkException("network error");
  } catch (NetworkException& ne) {
    networkError = current_exception();
  }
  ASSERT_NE(exception_ptr(), networkError);

  auto otherError = exception_ptr();
  try {
    throw CommunicationException("protocol error", "test://test");
  } catch (CommunicationException& ce) {
    otherError = current_exception();
  }
  ASSERT_NE(exception_ptr(), otherError);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;
  CancelSapiCallbackPtr cancelCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(vector<string>{problemId}, _))
    .Times(3).WillRepeatedly(SaveArg<1>(&cancelCallback));

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(1);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(4).WillRepeatedly(Return(RetryTimer::RETRY));

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  sp->cancel();
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::PENDING)});
  submitCallback.reset();

  // network fail 1
  ASSERT_TRUE(!!cancelCallback);
  cancelCallback->error(networkError);
  cancelCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(networkError);
  statusCallback.reset();

  // network fail 2
  notifiable->notify();
  ASSERT_TRUE(!!cancelCallback);
  cancelCallback->error(networkError);
  cancelCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(networkError);
  statusCallback.reset();

  // other fail
  notifiable->notify();
  ASSERT_TRUE(!!cancelCallback);
  cancelCallback->error(otherError);
  cancelCallback.reset();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(otherError);
  statusCallback.reset();

  // problem manager still working
  submitCallback.reset();
  auto sp2 = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
}



TEST(ProblemManagerAutoRetryTest, CancelUntilFailure) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto e = exception_ptr();
  try {
    throw NetworkException("something bad");
  } catch (NetworkException& ne) {
    e = current_exception();
  }
  ASSERT_NE(exception_ptr(), e);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;
  CancelSapiCallbackPtr cancelCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(vector<string>{problemId}, _))
    .Times(3).WillRepeatedly(SaveArg<1>(&cancelCallback));

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(0);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).WillOnce(Return(RetryTimer::RETRY));
  EXPECT_CALL(*mockRetryTimer, retryImpl()).WillOnce(Return(RetryTimer::FAIL)).RetiresOnSaturation();
  EXPECT_CALL(*mockRetryTimer, retryImpl()).Times(4).WillRepeatedly(Return(RetryTimer::RETRY)).RetiresOnSaturation();

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());

  sp->cancel();
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::PENDING)});
  submitCallback.reset();

  // fail 1
  ASSERT_TRUE(!!cancelCallback);
  cancelCallback->error(e);
  cancelCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);
  statusCallback.reset();

  // fail 2
  notifiable->notify();
  ASSERT_TRUE(!!cancelCallback);
  cancelCallback->error(e);
  cancelCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);
  statusCallback.reset();

  // real fail
  notifiable->notify();
  ASSERT_TRUE(!!cancelCallback);
  cancelCallback->error(e);
  cancelCallback.reset();
  notifiable->notify();
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);
  statusCallback.reset();

  // stay failed
  notifiable->notify();
  ASSERT_FALSE(!!cancelCallback);
}



TEST(ProblemManagerAutoRetryTest, Starvation) {
  auto problem1 = Problem("solver-a", "blarg", 1, o);
  auto problem2 = Problem("solver-b", "blah", 2, o);
  auto pv1 = vector<Problem>{problem1};
  auto pv2 = vector<Problem>{problem2};

  auto e = exception_ptr();
  try {
    throw NetworkException("something bad");
  } catch (NetworkException& ne) {
    e = current_exception();
  }
  ASSERT_NE(exception_ptr(), e);

  StatusSapiCallbackPtr submitCallback1;
  StatusSapiCallbackPtr submitCallback2;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(pv1, _)).WillOnce(SaveArg<1>(&submitCallback1));
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(pv2, _)).WillOnce(SaveArg<1>(&submitCallback2));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryTimer = make_shared<MockRetryTimer>();
  EXPECT_CALL(*mockRetryTimer, successImpl()).Times(0);
  EXPECT_CALL(*mockRetryTimer, retryImpl()).WillRepeatedly(Return(RetryTimer::FAIL));

  RetryNotifiableWeakPtr weakNotifiable;

  auto mockRetryService = make_shared<MockRetryTimerService>();
  EXPECT_CALL(*mockRetryService, createRetryTimerImpl(_, _))
    .WillOnce(DoAll(SaveArg<0>(&weakNotifiable), Return(mockRetryTimer)));

  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);
  auto notifiable = weakNotifiable.lock();
  ASSERT_TRUE(!!notifiable);

  // first problem fails with no retry
  auto sp1 = problemManager->submitProblem(problem1.solver(), problem1.type(), problem1.data(), problem1.params());
  ASSERT_TRUE(!!submitCallback1);
  submitCallback1->error(e);
  submitCallback1.reset();

  // retry notification received -- nothing to do
  notifiable->notify();
  ASSERT_FALSE(!!submitCallback1);
  ASSERT_FALSE(!!submitCallback2);

  // submit second problem
  auto sp2 = problemManager->submitProblem(problem2.solver(), problem2.type(), problem2.data(), problem2.params());
  ASSERT_TRUE(!!submitCallback2);
}



TEST(ProblemManagerManualRetryTest, RetrySubmit) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto e = exception_ptr();
  try {
    throw NetworkException("something bad");
  } catch (NetworkException& ne) {
    e = current_exception();
  }
  ASSERT_NE(exception_ptr(), e);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).Times(2).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);
  auto spi = sp->status();
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  submitCallback.reset();

  sp->retry();
  spi = sp->status();
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_TRUE(!!submitCallback);
}



TEST(ProblemManagerManualRetryTest, RetryStatus) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto e = exception_ptr();
  try {
    throw CommunicationException("something bad", "test://test");
  } catch (CommunicationException& ne) {
    e = current_exception();
  }
  ASSERT_NE(exception_ptr(), e);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(2).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::PENDING)});

  // fail
  ASSERT_TRUE(!!statusCallback);
  statusCallback->error(e);
  auto spi = sp->status();
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::PENDING, spi.remoteStatus);
  EXPECT_EQ(errortypes::PROTOCOL, spi.error.type);

  // retry
  statusCallback.reset();
  sp->retry();
  spi = sp->status();
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::PENDING, spi.remoteStatus);
  EXPECT_EQ(errortypes::PROTOCOL, spi.error.type);
  ASSERT_TRUE(!!statusCallback);
}



TEST(ProblemManagerManualRetryTest, NoRetrySubmitting) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(AnyNumber());
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  auto spi = sp->status();
  EXPECT_EQ(submittedstates::SUBMITTING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  sp->retry();
  spi = sp->status();
  EXPECT_EQ(submittedstates::SUBMITTING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);

  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::PENDING)});
}



TEST(ProblemManagerManualRetryTest, NoRetrySubmitted) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  StatusSapiCallbackPtr submitCallback;
  StatusSapiCallbackPtr statusCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(2).WillRepeatedly(SaveArg<1>(&statusCallback));
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::PENDING)});

  auto spi = sp->status();
  EXPECT_EQ(submittedstates::SUBMITTED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::PENDING, spi.remoteStatus);
  sp->retry();
  spi = sp->status();
  EXPECT_EQ(submittedstates::SUBMITTED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTED, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::PENDING, spi.remoteStatus);

  ASSERT_TRUE(!!statusCallback);
  statusCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::IN_PROGRESS)});
}






TEST(ProblemManagerManualRetryTest, NoRetryRetrying) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  auto e = exception_ptr();
  try {
    throw NetworkException("something bad");
  } catch (NetworkException& ne) {
    e = current_exception();
  }
  ASSERT_NE(exception_ptr(), e);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).Times(2).WillRepeatedly(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);

  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->error(e);
  auto spi = sp->status();
  EXPECT_EQ(submittedstates::FAILED, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  submitCallback.reset();

  sp->retry();
  sp->retry();
  sp->retry();
  sp->retry();
  spi = sp->status();
  EXPECT_EQ(submittedstates::RETRYING, spi.state);
  EXPECT_EQ(submittedstates::SUBMITTING, spi.lastGoodState);
  EXPECT_EQ(errortypes::NETWORK, spi.error.type);
  EXPECT_TRUE(!!submitCallback);
}
TEST(ProblemManagerManualRetryTest, NoRetryDoneSuccess) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeProblemInfo(problemId, "", remotestatuses::COMPLETED)});
  auto spi = sp->status();
  EXPECT_EQ(submittedstates::DONE, spi.state);
  EXPECT_EQ(submittedstates::DONE, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::COMPLETED, spi.remoteStatus);

  // retry
  submitCallback.reset();
  sp->retry();
  spi = sp->status();
  EXPECT_EQ(submittedstates::DONE, spi.state);
  EXPECT_EQ(submittedstates::DONE, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::COMPLETED, spi.remoteStatus);
  ASSERT_FALSE(!!submitCallback);
}



TEST(ProblemManagerManualRetryTest, NoRetryDoneFailure) {
  auto problemId = string("the-id");
  auto problem = Problem("solver-a", "blarg", json::Null(), o);

  StatusSapiCallbackPtr submitCallback;

  auto mockSapiService = make_shared<MockSapiService>();
  EXPECT_CALL(*mockSapiService, fetchSolversImpl(_)).Times(0);
  EXPECT_CALL(*mockSapiService, submitProblemsImpl(_, _)).WillOnce(SaveArg<1>(&submitCallback));
  EXPECT_CALL(*mockSapiService, multiProblemStatusImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, fetchAnswerImpl(_, _)).Times(0);
  EXPECT_CALL(*mockSapiService, cancelProblemsImpl(_, _)).Times(0);

  auto mockRetryService = make_shared<NiceMock<MockRetryTimerService>>();
  auto problemManager = makeProblemManager(mockSapiService, make_shared<MockAnswerService>(), mockRetryService,
    dummyRetryTiming, minLimits);

  // submit
  auto sp = problemManager->submitProblem(problem.solver(), problem.type(), problem.data(), problem.params());
  ASSERT_TRUE(!!submitCallback);
  submitCallback->complete(vector<RemoteProblemInfo>{makeFailedProblemInfo(problemId, "", "uh oh")});
  auto spi = sp->status();
  EXPECT_EQ(submittedstates::DONE, spi.state);
  EXPECT_EQ(submittedstates::DONE, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::FAILED, spi.remoteStatus);
  EXPECT_EQ(errortypes::SOLVE, spi.error.type);

  // retry
  submitCallback.reset();
  sp->retry();
  spi = sp->status();
  EXPECT_EQ(submittedstates::DONE, spi.state);
  EXPECT_EQ(submittedstates::DONE, spi.lastGoodState);
  EXPECT_EQ(remotestatuses::FAILED, spi.remoteStatus);
  EXPECT_EQ(errortypes::SOLVE, spi.error.type);
  ASSERT_FALSE(!!submitCallback);
}
