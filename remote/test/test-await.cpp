//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <exception>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include <boost/noncopyable.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <problem.hpp>

using std::bind;
using std::condition_variable;
using std::lock_guard;
using std::make_shared;
using std::mutex;
using std::numeric_limits;
using std::shared_ptr;
using std::string;
using std::thread;
using std::tuple;
using std::unique_lock;
using std::vector;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::microseconds;

using testing::_;
using testing::StrictMock;
using testing::Invoke;

using sapiremote::SubmittedProblemObserverPtr;
using sapiremote::SubmittedProblem;
using sapiremote::SubmittedProblemPtr;
using sapiremote::AnswerCallbackPtr;
using sapiremote::awaitSubmission;
using sapiremote::awaitCompletion;

namespace {

class MockSubmittedProblem : public SubmittedProblem {
public:
  MOCK_CONST_METHOD0(problemIdImpl, string());
  MOCK_CONST_METHOD0(doneImpl, bool());
  MOCK_CONST_METHOD0(statusImpl, sapiremote::SubmittedProblemInfo());
  MOCK_CONST_METHOD0(answerImpl, tuple<string, json::Value>());
  MOCK_CONST_METHOD1(answerImpl, void(AnswerCallbackPtr));
  MOCK_METHOD0(cancelImpl, void());
  MOCK_METHOD0(retryImpl, void());
  MOCK_METHOD1(addSubmittedProblemObserverImpl, void(const SubmittedProblemObserverPtr& observer));
};

double now() {
  return 1e-6 * duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

void NotifySubmitted(SubmittedProblemObserverPtr observer) { observer->notifySubmitted(); }
void NotifyDone(SubmittedProblemObserverPtr observer) { observer->notifyDone(); }
void NotifyError(SubmittedProblemObserverPtr observer) { observer->notifyError(); }


class DelayedNotifierImpl : boost::noncopyable {
private:
  mutex mutex_;
  condition_variable cv_;
  bool start_;
  thread th_;
  SubmittedProblemObserverPtr obs_;

  void thfunc(double delayS) {
    {
      unique_lock<mutex> l(mutex_);
      while (!start_) cv_.wait(l);
    }
    std::this_thread::sleep_for(microseconds(static_cast<microseconds::rep>(delayS * 1e6)));
    obs_->notifyDone();
  }

public:
  DelayedNotifierImpl(double delayS) : start_(false), th_(bind(&DelayedNotifierImpl::thfunc, this, delayS)) {}
  ~DelayedNotifierImpl() { if (th_.joinable()) th_.join(); }
  void start(SubmittedProblemObserverPtr obs) {
    lock_guard<mutex> l(mutex_);
    obs_ = obs;
    start_ = true;
    cv_.notify_all();
  }
};

class DelayedNotifier {
private:
  shared_ptr<DelayedNotifierImpl> dn_;

public:
  DelayedNotifier(double delayS) : dn_(make_shared<DelayedNotifierImpl>(delayS)) {}
  void operator()(SubmittedProblemObserverPtr obs) { dn_->start(obs); }
};


} // namespace {anonymous}

TEST(AwaitTest, submissionTimeout) {
  vector<SubmittedProblemPtr> problems;
  for (auto i = 0; i < 5; ++i) {
    auto mp = make_shared<StrictMock<MockSubmittedProblem>>();
    problems.push_back(mp);
    EXPECT_CALL(*mp, addSubmittedProblemObserverImpl(_)).Times(1);
  }

  auto start = now();
  EXPECT_FALSE(awaitSubmission(problems, 0.1));
  auto end = now();
  EXPECT_GE(end - start, 0.1);
}



TEST(AwaitTest, submissionImmediate) {
  vector<SubmittedProblemPtr> problems;
  for (auto i = 0; i < 5; ++i) {
    auto mp = make_shared<StrictMock<MockSubmittedProblem>>();
    problems.push_back(mp);
    EXPECT_CALL(*mp, addSubmittedProblemObserverImpl(_)).WillOnce(Invoke(NotifySubmitted));
  }
  for (auto i = 0; i < 5; ++i) {
    auto mp = make_shared<StrictMock<MockSubmittedProblem>>();
    problems.push_back(mp);
    EXPECT_CALL(*mp, addSubmittedProblemObserverImpl(_)).WillOnce(Invoke(NotifyError));
  }

  auto start = now();
  EXPECT_TRUE(awaitSubmission(problems, 2.0));
  auto end = now();
  EXPECT_LT(end - start, 0.05);
}



TEST(AwaitTest, completionTimeout) {
  vector<SubmittedProblemPtr> problems;
  for (auto i = 0; i < 5; ++i) {
    auto mp = make_shared<StrictMock<MockSubmittedProblem>>();
    problems.push_back(mp);
    EXPECT_CALL(*mp, addSubmittedProblemObserverImpl(_)).Times(1);
  }

  auto start = now();
  EXPECT_FALSE(awaitCompletion(problems, 1, 0.1));
  auto end = now();
  EXPECT_GE(end - start, 0.1);
}



TEST(AwaitTest, completionImmediate) {
  vector<SubmittedProblemPtr> problems;
  for (auto i = 0; i < 5; ++i) {
    auto mp = make_shared<StrictMock<MockSubmittedProblem>>();
    problems.push_back(mp);
    EXPECT_CALL(*mp, addSubmittedProblemObserverImpl(_)).WillOnce(Invoke(NotifyDone));
  }
  for (auto i = 0; i < 5; ++i) {
    auto mp = make_shared<StrictMock<MockSubmittedProblem>>();
    problems.push_back(mp);
    EXPECT_CALL(*mp, addSubmittedProblemObserverImpl(_)).WillOnce(Invoke(NotifyError));
  }

  auto start = now();
  EXPECT_TRUE(awaitCompletion(problems, static_cast<int>(problems.size()), 2.0));
  auto end = now();
  EXPECT_LT(end - start, 0.05);
}



TEST(AwaitTest, SC90) {
  DelayedNotifier delayedNotifer(0.1);
  auto mp = make_shared<StrictMock<MockSubmittedProblem>>();
  EXPECT_CALL(*mp, addSubmittedProblemObserverImpl(_)).WillOnce(Invoke(delayedNotifer));
  auto problems = vector<SubmittedProblemPtr>(1, mp);

  auto start = now();
  EXPECT_TRUE(awaitCompletion(problems, 1, numeric_limits<double>::infinity()));
  auto end = now();
  EXPECT_GE(end - start, 0.1);
}
