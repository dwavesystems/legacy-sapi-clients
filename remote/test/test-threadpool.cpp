//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <mutex>
#include <condition_variable>
#include <utility>
#include <chrono>

#include <gtest/gtest.h>

#include <exceptions.hpp>
#include <threadpool.hpp>

using std::condition_variable;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::ref;
using std::chrono::milliseconds;

using sapiremote::makeThreadPool;

namespace {

class Work {
  mutable mutex mutex_;
  mutable condition_variable cv_;

  bool wait_;
  bool notified_;
  bool done_;

public:
  Work() : wait_(false), notified_(false), done_(false) {}

  Work& awaitNotification(bool wait = true) {
    lock_guard<mutex> l(mutex_);
    wait_ = wait;
    return *this;
  }

  void notify() {
    lock_guard<mutex> l(mutex_);
    notified_ = true;
    cv_.notify_all();
  }

  void operator()() {
    unique_lock<mutex> l(mutex_);
    if (wait_) {
      while (!notified_) cv_.wait(l);
    }
    done_ = true;
    cv_.notify_all();
  }

  void wait(int millis) {
    unique_lock<mutex> l(mutex_);
    cv_.wait_for(l, milliseconds(millis), std::bind(&Work::done_unlocked, this));
  }

  bool done() const {
    lock_guard<mutex> l(mutex_);
    return done_;
  }

  bool done_unlocked() const {
    return done_;
  }
};

} // namespace {anonymous}

TEST(ThreadPoolTest, post) {
  auto threadPool = makeThreadPool(1);
  Work w;
  threadPool->post(ref(w));
  w.wait(10);
  EXPECT_TRUE(w.done());
}

TEST(ThreadPoolTest, postBlock) {
  auto threadPool = makeThreadPool(1);
  Work wBlock, w2;
  wBlock.awaitNotification();
  threadPool->post(ref(wBlock));
  threadPool->post(ref(w2));
  w2.wait(50);
  EXPECT_FALSE(wBlock.done());
  EXPECT_FALSE(w2.done());
  wBlock.notify();
  w2.wait(100);
  EXPECT_TRUE(wBlock.done());
  EXPECT_TRUE(w2.done());
}

TEST(ThreadPoolTest, postConcurrent) {
  auto threadPool = makeThreadPool(2);
  Work wBlock, w2;
  wBlock.awaitNotification();
  threadPool->post(ref(wBlock));
  threadPool->post(ref(w2));
  w2.wait(100);
  EXPECT_FALSE(wBlock.done());
  EXPECT_TRUE(w2.done());
  wBlock.notify();
  wBlock.wait(100);
  EXPECT_TRUE(wBlock.done());
}

TEST(ThreadPoolTest, shutdown) {
  auto threadPool = makeThreadPool(1);
  threadPool->shutdown();
  Work w;
  EXPECT_THROW(threadPool->post(ref(w)), sapiremote::ServiceShutdownException);
}

TEST(ThreadPoolTest, invalidNumThreads) {
  EXPECT_THROW(makeThreadPool(0), std::invalid_argument);
  EXPECT_THROW(makeThreadPool(-1), std::invalid_argument);
}
