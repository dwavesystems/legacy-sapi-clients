//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <memory>
#include <mutex>
#include <stdexcept>

#include <boost/noncopyable.hpp>

#include <retry-service.hpp>

#include "dwave_sapi.h"
#include "internal.hpp"
#include <local.hpp>

using std::lock_guard;
using std::make_shared;
using std::mutex;
using std::unique_ptr;

using sapi::NotInitializedException;

namespace {

const sapiremote::ProblemManagerLimits limits = {
  20,  // maxProblemsPerSubmission
  100, // maxIdsPerStatusQuery
  6    // maxActiveProblemSubmissions
};

class GlobalState : boost::noncopyable {
private:
  sapiremote::http::HttpServicePtr httpService_;
  sapiremote::ThreadPoolPtr answerThreadPool_;
  sapiremote::RetryTimerServicePtr retryService_;
  sapi::LocalConnection localConnection_;

public:
  GlobalState() :
      httpService_(sapiremote::http::makeHttpService(2)),
      answerThreadPool_(sapiremote::makeThreadPool(2)),
      retryService_(sapiremote::makeRetryTimerService()) {}

  ~GlobalState() {
    httpService_->shutdown();
    answerThreadPool_->shutdown();
    retryService_->shutdown();
  }

  const sapiremote::http::HttpServicePtr& httpService() const { return httpService_; }
  const sapiremote::ThreadPoolPtr& answerThreadPool() const { return answerThreadPool_; }
  const sapiremote::RetryTimerServicePtr& retryService() const { return retryService_; }
  sapi_Connection* localConnection() { return &localConnection_; }
};

class GlobalStateMangager : boost::noncopyable {
private:
  unique_ptr<GlobalState> gs_;
  int count_;
  mutex mutex_;

public:
  GlobalStateMangager() : gs_(), count_(0) {}

  void init() {
    lock_guard<mutex> l(mutex_);
    if (count_ == 0) gs_.reset(new GlobalState);
    ++count_;
  }

  void cleanup() {
    lock_guard<mutex> l(mutex_);
    --count_;
    if (count_ == 0) gs_.reset();
  }

  sapi_Connection* localConnection() {
    lock_guard<mutex> l(mutex_);
    if (!gs_) throw NotInitializedException();
    return gs_->localConnection();
  }

  sapiremote::ProblemManagerPtr makeProblemManager(const char* url, const char* token, const char* proxy) {
    lock_guard<mutex> l(mutex_);
    if (!gs_) throw NotInitializedException();

    auto srp = proxy ? sapiremote::http::Proxy(proxy) : sapiremote::http::Proxy();
    auto sapiService = sapiremote::makeSapiService(gs_->httpService(), url, token, srp);
    auto answerService = sapiremote::makeAnswerService(gs_->answerThreadPool());
    return sapiremote::makeProblemManager(sapiService, answerService, gs_->retryService(),
      sapiremote::defaultRetryTiming(), limits);
  }
};

GlobalStateMangager& gsm() {
  static GlobalStateMangager theGsm;
  return theGsm;
}

} // namespace {anonymous}


namespace sapi {

sapiremote::ProblemManagerPtr makeProblemManager(const char* url, const char* token, const char* proxy) {
  return gsm().makeProblemManager(url, token, proxy);
}

} // namespace sapi

DWAVE_SAPI sapi_Code sapi_globalInit() {
  try {
    gsm().init();
    return SAPI_OK;
  } catch (...) {
    return SAPI_ERR_OUT_OF_MEMORY;
  }
}

DWAVE_SAPI void sapi_globalCleanup() {
  gsm().cleanup();
}

DWAVE_SAPI sapi_Connection* sapi_localConnection() {
  try {
    return gsm().localConnection();
  } catch (...) {
    return 0;
  }
}
