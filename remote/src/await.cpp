//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <chrono>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <new>
#include <stdexcept>
#include <vector>

#include <boost/foreach.hpp>

#include <problem.hpp>

using std::condition_variable;
using std::lock_guard;
using std::make_shared;
using std::mutex;
using std::numeric_limits;
using std::shared_ptr;
using std::unique_lock;
using std::vector;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::hours;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;

using sapiremote::SubmittedProblemObserver;
using sapiremote::SubmittedProblemPtr;

#if __GNUC__ == 4 && __GNUC_MINOR__ <= 4 && !defined(__clang__)
#define SAPIREMOTE_WAIT_TIMEOUT (false)
#else
#define SAPIREMOTE_WAIT_TIMEOUT (std::cv_status::timeout)
#endif

namespace {

system_clock::time_point maxTime() {
  // This is ugly.
  // Not using system_clock::time_point::max() because of bad behaviour in MSVC 2013: system_clock tick length
  // is 100ns but condition_variable::wait_until adds system_clock::duration and nanoseconds values, which
  // causes an overflow with large end-times.
  // Also subtract an hour since condition_variable::wait_until in MSVC 2013 calls system_clock::now() after
  // computing relative time, potentially causing overflow with maximum end time.
  static auto t = system_clock::time_point(duration_cast<system_clock::duration>(nanoseconds::max())) - hours(1);
  return t;
}

system_clock::time_point computeEndTime(double timeoutS) {
  auto now = system_clock::now();
  auto endTime = maxTime();
  auto timeoutMs = ceil(timeoutS * 1e3);
  if (duration_cast<milliseconds>(endTime - now).count() > timeoutMs) {
    endTime = now + milliseconds(static_cast<milliseconds::rep>(timeoutMs));
  }
  return endTime;
}

class EventCounter {
private:
  mutex mutex_;
  condition_variable cv_;
  system_clock::time_point endTime_;
  int remaining_;

public:
  EventCounter(int remaining, system_clock::time_point endTime) : remaining_(remaining), endTime_(endTime) {}

  void notify() {
    lock_guard<mutex> l(mutex_);
    --remaining_;
    cv_.notify_all();
  }

  bool wait() {
    unique_lock<mutex> lock(mutex_);
    while (remaining_ > 0) {
      if (cv_.wait_until(lock, endTime_) == SAPIREMOTE_WAIT_TIMEOUT) return false;
    }
    return true;
  }
};
typedef shared_ptr<EventCounter> EventCounterPtr;

class EventNotifier {
private:
  mutex mutex_;
  EventCounterPtr eventCounter_;
  bool done_;

public:
  EventNotifier(EventCounterPtr eventCounter) : eventCounter_(eventCounter), done_(false) {}

  void notify() {
    unique_lock<mutex> l(mutex_);
    if (!done_) {
      done_ = true;
      l.unlock();
      eventCounter_->notify();
    }
  }
};

class SubmissionObserver : public SubmittedProblemObserver, public EventNotifier {
private:
  virtual void notifySubmittedImpl() { notify(); }
  virtual void notifyDoneImpl() {}
  virtual void notifyErrorImpl() { notify(); }

public:
  SubmissionObserver(EventCounterPtr eventCounter) : EventNotifier(eventCounter) {}
};

class CompletionObserver : public SubmittedProblemObserver, public EventNotifier {
private:
  virtual void notifySubmittedImpl() {}
  virtual void notifyDoneImpl() { notify(); }
  virtual void notifyErrorImpl() { notify(); }

public:
  CompletionObserver(EventCounterPtr eventCounter) : EventNotifier(eventCounter) {}
};

template<typename T>
bool awaitEvents(const vector<SubmittedProblemPtr>& problems, int minEvents, double timeoutS) {
  if (minEvents > 0 && static_cast<unsigned int>(minEvents) > problems.size()) {
    minEvents = static_cast<int>(problems.size());
  }
  auto eventCounter = make_shared<EventCounter>(minEvents, computeEndTime(timeoutS));
  vector<shared_ptr<T>> observers;
  BOOST_FOREACH( auto& p, problems ) {
    observers.push_back(make_shared<T>(eventCounter));
    p->addSubmittedProblemObserver(observers.back());
  }
  return eventCounter->wait();
}

} // namespace {anonymous}

namespace sapiremote {

bool awaitSubmission(const vector<SubmittedProblemPtr>& problems, double timeoutS) {
  if (problems.size() > numeric_limits<int>::max()) throw std::invalid_argument("sapiremote::awaitSubmission");
  return awaitEvents<SubmissionObserver>(problems, static_cast<int>(problems.size()), timeoutS);
}

bool awaitCompletion(const vector<SubmittedProblemPtr>& problems, int mindone, double timeoutS) {
  return awaitEvents<CompletionObserver>(problems, mindone, timeoutS);
}

} // namespace sapiremote
