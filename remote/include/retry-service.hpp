//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef TIMER_SERVICE_HPP_INCLUDED
#define TIMER_SERVICE_HPP_INCLUDED

#include <memory>

namespace sapiremote {

struct RetryTiming {
  int initDelayMs;
  int maxDelayMs;
  float delayScale;
};

class RetryTimer {
public:
  enum RetryAction { RETRY, FAIL, SHUTDOWN };
private:
  virtual RetryAction retryImpl() = 0;
  virtual void successImpl() = 0;
public:
  virtual ~RetryTimer() {}
  RetryAction retry() { return retryImpl(); }
  void success() { successImpl(); }
};
typedef std::shared_ptr<RetryTimer> RetryTimerPtr;

class RetryNotifiable {
private:
  virtual void notifyImpl() = 0;
public:
  virtual ~RetryNotifiable() {}
  void notify() { try{ notifyImpl(); } catch (...) {} }
};
typedef std::weak_ptr<RetryNotifiable> RetryNotifiableWeakPtr;

class RetryTimerService {
private:
  virtual void shutdownImpl() = 0;
  virtual RetryTimerPtr createRetryTimerImpl(const RetryNotifiableWeakPtr& rn, const RetryTiming& timing) = 0;
public:
  virtual ~RetryTimerService() {}
  void shutdown() { shutdownImpl(); }
  RetryTimerPtr createRetryTimer(const RetryNotifiableWeakPtr& rn, const RetryTiming& timing) {
    return createRetryTimerImpl(rn, timing);
  }
};
typedef std::shared_ptr<RetryTimerService> RetryTimerServicePtr;

const RetryTiming& defaultRetryTiming();
RetryTimerServicePtr makeRetryTimerService();

} // namespace sapiremote

#endif
