//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>

#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/foreach.hpp>
#include <boost/system/error_code.hpp>

#include <retry-service.hpp>
#include <exceptions.hpp>

using std::bind;
using std::enable_shared_from_this;
using std::lock_guard;
using std::make_shared;
using std::mutex;
using std::ref;
using std::shared_ptr;
using std::thread;
using std::unique_ptr;
using std::unordered_map;
using std::weak_ptr;
using namespace std::placeholders;

using boost::asio::io_service;
using boost::asio::deadline_timer;
using boost::posix_time::milliseconds;
using boost::system::error_code;

using sapiremote::RetryTiming;
using sapiremote::RetryNotifiableWeakPtr;
using sapiremote::RetryTimer;
using sapiremote::RetryTimerPtr;
using sapiremote::RetryTimerService;
using sapiremote::RetryTimerServicePtr;
using sapiremote::ServiceShutdownException;

namespace {

class RetryTimerServiceImpl;
typedef shared_ptr<RetryTimerServiceImpl> RetryTimerServiceImplPtr;

class RetryTimerImpl : public RetryTimer {
private:
  RetryTimerServiceImplPtr rts_;
  const RetryTiming timing_;
  const RetryNotifiableWeakPtr target_;
  unique_ptr<deadline_timer> timer_;
  mutex mutex_;
  int nextDelayMs_;
  bool waiting_;
  bool fail_;
  bool failOnExpiry_;

  virtual RetryTimer::RetryAction retryImpl() {
    lock_guard<mutex> l(mutex_);
    if (!timer_) return RetryTimer::SHUTDOWN;
    if (!waiting_) {
      timer_->expires_from_now(milliseconds(nextDelayMs_));
      timer_->async_wait(bind(&RetryTimerImpl::timerExpired, this, _1));

      waiting_ = true;
      if (nextDelayMs_ >= timing_.maxDelayMs) failOnExpiry_ = true;
      auto nextDelayMs = nextDelayMs_ * timing_.delayScale;
      nextDelayMs_ = nextDelayMs > timing_.maxDelayMs ? timing_.maxDelayMs : static_cast<int>(nextDelayMs);
    }
    return fail_ ? RetryTimer::FAIL : RetryTimer::RETRY;
  }

  virtual void successImpl() {
    lock_guard<mutex> l(mutex_);
    waiting_ = false;
    fail_ = false;
    failOnExpiry_ = false;
    nextDelayMs_ = timing_.initDelayMs;
    if (timer_) timer_->cancel();
  }

  void timerExpired(const error_code& ec) {
    if (!ec) {
      {
        lock_guard<mutex> l(mutex_);
        waiting_ = false;
        fail_ = failOnExpiry_;
      }
      auto lt = target_.lock();
      if (lt) lt->notify();
    }
  }

public:
  RetryTimerImpl(RetryTimerServiceImplPtr rts, unique_ptr<deadline_timer> timer, RetryNotifiableWeakPtr target,
      const RetryTiming& timing) : rts_(rts), timing_(timing), target_(target), timer_(std::move(timer)),
          nextDelayMs_(timing.initDelayMs), waiting_(false), fail_(false), failOnExpiry_(false) {

    if (timing_.initDelayMs < 1) throw std::invalid_argument("initDelayMs must be positive");
    if (timing_.delayScale < 1.0f) throw std::invalid_argument("delayScale must be >=1.0f");
    if (timing_.maxDelayMs < timing_.initDelayMs) {
      throw std::invalid_argument("maxDelayMs must be at least initDelayMs");
    }
  }

  ~RetryTimerImpl();

  void shutdown() {
    lock_guard<mutex> l(mutex_);
    timer_.reset();
    rts_.reset();
  }
};
typedef weak_ptr<RetryTimerImpl> RetryTimerImplWeakPtr;

class IoServiceThread : boost::noncopyable {
private:
  thread t_;

  static void run(io_service& ioService) { ioService.run(); }

public:
  IoServiceThread() {}
  IoServiceThread(IoServiceThread&& other) : t_(std::move(other.t_)) {}
  IoServiceThread& operator=(IoServiceThread&& other) { t_ = std::move(other.t_); return *this; }
  explicit IoServiceThread(io_service& ioService) : t_(run, ref(ioService)) {}
  ~IoServiceThread() { join(); }

  void join() { if (t_.joinable()) t_.join(); }
};


class RetryTimerServiceImpl : public enable_shared_from_this<RetryTimerServiceImpl>, public RetryTimerService {
private:
  unique_ptr<io_service> ioService_;
  IoServiceThread thread_;
  unique_ptr<io_service::work> work_;
  unordered_map<RetryTimerImpl*, RetryTimerImplWeakPtr> timers_;
  mutex mutex_;
  bool running_;

  virtual void shutdownImpl() {
    {
      lock_guard<mutex> l(mutex_);
      running_ = false;
    }
    work_.reset();
    BOOST_FOREACH( const auto& t, timers_ ) {
      auto lt = t.second.lock();
      if (lt) lt->shutdown();
    }
    thread_.join();
    ioService_.reset();
  }

  virtual RetryTimerPtr createRetryTimerImpl(const RetryNotifiableWeakPtr& rn, const RetryTiming& timing) {
    lock_guard<mutex> l(mutex_);
    if (!running_) throw ServiceShutdownException();
    auto dt = unique_ptr<deadline_timer>(new deadline_timer(*ioService_));
    auto timer = make_shared<RetryTimerImpl>(shared_from_this(), std::move(dt), rn, timing);
    timers_[timer.get()] = timer;
    return timer;
  }

public:
  RetryTimerServiceImpl() : ioService_(new io_service), work_(new io_service::work(*ioService_)), running_(true) {
    thread_ = IoServiceThread(*ioService_);
  }

  ~RetryTimerServiceImpl() { shutdown(); }

  void removeTimer(RetryTimerImpl* timer) {
    lock_guard<mutex> l(mutex_);
    if (running_) timers_.erase(timer);
  }

  static RetryTimerServicePtr create() {
    return make_shared<RetryTimerServiceImpl>();
  }
};


RetryTimerImpl::~RetryTimerImpl() {
  if (rts_) rts_->removeTimer(this);
}

} // namespace {anonymous}


namespace sapiremote {

const RetryTiming& defaultRetryTiming() {
  static RetryTiming timing = { 10, 10000, 10.0f };
  return timing;
}

RetryTimerServicePtr makeRetryTimerService() {
  return RetryTimerServiceImpl::create();
}

} // namespace sapiremote
