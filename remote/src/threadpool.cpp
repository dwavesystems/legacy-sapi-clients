//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

#include <boost/noncopyable.hpp>

#include <threadpool.hpp>
#include <exceptions.hpp>

using std::bind;
using std::function;
using std::thread;
using std::mutex;
using std::condition_variable;
using std::vector;
using std::queue;
using std::unique_lock;
using std::invalid_argument;
using std::make_shared;

using sapiremote::ServiceShutdownException;
using sapiremote::ThreadPool;

namespace {

class AutoJoinThread : boost::noncopyable {
private:
  thread t_;

public:
  AutoJoinThread(AutoJoinThread&& other) : t_(std::move(other.t_)) {}
  AutoJoinThread& operator=(AutoJoinThread&& other) { t_ = std::move(other.t_); return *this; }

  AutoJoinThread(function<void()> threadFn) : t_(threadFn) {}
  ~AutoJoinThread() { if (t_.joinable()) t_.join(); }
};

class ThreadPoolImpl : public ThreadPool {
private:
  queue<function<void()>> workQueue_;
  vector<AutoJoinThread> threads_;
  mutex mutex_;
  condition_variable cv_;
  bool running_;

  void threadFn() {
    for (;;) {
      unique_lock<mutex> lock(mutex_);
      while (running_  && workQueue_.empty()) cv_.wait(lock);
      if (!running_) break;

      auto work = workQueue_.front();
      workQueue_.pop();
      lock.unlock();

      try {
        work();
      } catch (...) {
        // eat exceptions
      }
    }
  }

  virtual void shutdownImpl() {
    unique_lock<mutex> lock(mutex_);
    running_ = false;
    cv_.notify_all();
    lock.unlock();
    threads_.clear();
  }

  virtual void postImpl(function<void()> f) {
    unique_lock<mutex> lock(mutex_);
    if (!running_) throw ServiceShutdownException();
    workQueue_.push(std::move(f));
    cv_.notify_one();
  }

public:
  ThreadPoolImpl(int threads) : running_(true) {
    if (threads < 1) throw std::invalid_argument("Number of threads must be positive");
    threads_.reserve(threads);
    for (auto i = 0; i < threads; ++i) {
      threads_.push_back(AutoJoinThread(bind(&ThreadPoolImpl::threadFn, this)));
    }
  }

  ~ThreadPoolImpl() { shutdown(); }
};

} // namespace {anonymous}

namespace sapiremote {

ThreadPoolPtr makeThreadPool(int threads) {
  return make_shared<ThreadPoolImpl>(threads);
}

} // namespace sapiremote
