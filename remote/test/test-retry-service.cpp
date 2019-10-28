//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>

#include <gtest/gtest.h>

#include <exceptions.hpp>
#include <retry-service.hpp>

#include "test.hpp"

#if __GNUC__ == 4 && __GNUC_MINOR__ <= 6 && !defined(__clang__)
#define steady_clock monotonic_clock
#endif

using std::bind;
using std::condition_variable;
using std::lock_guard;
using std::make_shared;
using std::mutex;
using std::unique_lock;
using std::chrono::steady_clock;
using std::chrono::milliseconds;

using sapiremote::RetryTiming;
using sapiremote::RetryNotifiable;
using sapiremote::RetryTimer;
using sapiremote::RetryTimerService;
using sapiremote::makeRetryTimerService;
using sapiremote::ServiceShutdownException;
using sapiremote::defaultRetryTiming;

namespace {

class Event : public RetryNotifiable {
private:
  mutable condition_variable cv_;
  mutable mutex mutex_;
  bool fired_;

  virtual void notifyImpl() {
    {
      lock_guard<mutex> l(mutex_);
      fired_ = true;
    }
    cv_.notify_all();
  }

public:
  Event() : fired_(false) {}

  bool wait(int millis) const {
    unique_lock<mutex> l(mutex_);
    cv_.wait_for(l, milliseconds(millis), bind(&Event::fired_unlocked, this));
    return fired_;
  }

  void reset() {
    lock_guard<mutex> l(mutex_);
    fired_ = false;
  }

  bool fired_unlocked() const {
    return fired_;
  }
};

} // namespace {anonymous}

TEST(RetryServiceTest, ValidDefault) {
  auto rts = makeRetryTimerService();
  auto rn = make_shared<Event>();
  rts->createRetryTimer(rn, defaultRetryTiming());
}

TEST(RetryServiceTest, InvalidParams) {
  auto rts = makeRetryTimerService();
  auto rn = make_shared<Event>();

  EXPECT_THROW(rts->createRetryTimer(rn, {0, 10000, 2.0f}), std::invalid_argument);
  EXPECT_THROW(rts->createRetryTimer(rn, {10, 10000, 0.0f}), std::invalid_argument);
  EXPECT_THROW(rts->createRetryTimer(rn, {1000, 10, 2.0f}), std::invalid_argument);
}

TEST(RetryServiceTest, ShutdownTimerCreation) {
  auto rts = makeRetryTimerService();
  rts->shutdown();
  auto rn = make_shared<Event>();
  EXPECT_THROW(rts->createRetryTimer(rn, {1, 10, 2.0f}), ServiceShutdownException);
}

TEST(RetryServiceTest, ShutdownTimerRetry) {
  auto rts = makeRetryTimerService();
  auto rn = make_shared<Event>();
  auto timing = RetryTiming{1, 2, 3.0f};
  auto timer = rts->createRetryTimer(rn, timing);
  rts->shutdown();
  EXPECT_EQ(RetryTimer::SHUTDOWN, timer->retry());
}

TEST(RetryServiceTest, SufficientDelay) {
  auto rts = makeRetryTimerService();
  auto event = make_shared<Event>();
  auto timing = RetryTiming{100, 100000, 1e3f};
  auto timer = rts->createRetryTimer(event, timing);

  auto startTime = steady_clock::now();
  EXPECT_EQ(RetryTimer::RETRY, timer->retry());
  EXPECT_TRUE(event->wait(5000));
  auto endTime = steady_clock::now();
  EXPECT_LE(milliseconds(100), endTime - startTime);
}

TEST(RetryServiceTest, RetryFails) {
  auto rts = makeRetryTimerService();
  auto event = make_shared<Event>();
  auto timing = RetryTiming{20, 40, 1.5f}; // 20 30 40
  auto timer = rts->createRetryTimer(event, timing);

  EXPECT_EQ(RetryTimer::RETRY, timer->retry());
  EXPECT_TRUE(event->wait(5000));
  event->reset();

  EXPECT_EQ(RetryTimer::RETRY, timer->retry());
  EXPECT_TRUE(event->wait(5000));
  event->reset();

  EXPECT_EQ(RetryTimer::RETRY, timer->retry());
  EXPECT_TRUE(event->wait(5000));
  event->reset();

  EXPECT_EQ(RetryTimer::FAIL, timer->retry());
  EXPECT_TRUE(event->wait(5000));
  event->reset();
  EXPECT_EQ(RetryTimer::FAIL, timer->retry());
}

TEST(RetryServiceTest, RetryIgnoreWhileWaiting) {
  auto rts = makeRetryTimerService();
  auto event = make_shared<Event>();
  auto timing = RetryTiming{100, 150, 2.0f}; // 100 150
  auto timer = rts->createRetryTimer(event, timing);

  EXPECT_EQ(RetryTimer::RETRY, timer->retry());
  EXPECT_EQ(RetryTimer::RETRY, timer->retry());
  EXPECT_EQ(RetryTimer::RETRY, timer->retry());
  EXPECT_TRUE(event->wait(5000));
  event->reset();

  EXPECT_EQ(RetryTimer::RETRY, timer->retry());
  EXPECT_EQ(RetryTimer::RETRY, timer->retry());
  EXPECT_EQ(RetryTimer::RETRY, timer->retry());
  EXPECT_TRUE(event->wait(5000));
  event->reset();

  EXPECT_EQ(RetryTimer::FAIL, timer->retry());
  EXPECT_TRUE(event->wait(5000));
  event->reset();
  EXPECT_EQ(RetryTimer::FAIL, timer->retry());
}

TEST(RetryServiceTest, Success) {
  auto rts = makeRetryTimerService();
  auto event = make_shared<Event>();
  auto timing = RetryTiming{10, 10000, 1e3f}; // 10 10000
  auto timer = rts->createRetryTimer(event, timing);

  EXPECT_EQ(RetryTimer::RETRY, timer->retry());
  EXPECT_TRUE(event->wait(5000));
  event->reset();
  timer->success();

  EXPECT_EQ(RetryTimer::RETRY, timer->retry());
  EXPECT_TRUE(event->wait(5000));
  event->reset();
}
