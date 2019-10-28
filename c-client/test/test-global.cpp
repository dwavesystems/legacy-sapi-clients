//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <problem.hpp>
#include <threadpool.hpp>
#include <http-service.hpp>
#include <sapi-service.hpp>
#include <retry-service.hpp>
#include <problem-manager.hpp>
#include <coding.hpp>

#include <dwave_sapi.h>

using std::make_shared;
using std::string;
using std::unique_ptr;
using std::vector;

using sapiremote::http::HttpService;
using sapiremote::http::HttpHeaders;
using sapiremote::http::Proxy;
using sapiremote::http::HttpCallbackPtr;
using sapiremote::ThreadPool;
using sapiremote::RetryTimerService;
using sapiremote::RetryTimerPtr;
using sapiremote::RetryNotifiableWeakPtr;
using sapiremote::RetryTiming;


namespace sapiremote {
extern char const * const userAgent = "sapi-remote/test";

namespace {

class DummyHttpService : public HttpService {
private:
  virtual void asyncGetImpl(const std::string&, const HttpHeaders&, const Proxy&, HttpCallbackPtr) {}
  virtual void asyncPostImpl(const std::string&, const HttpHeaders&, std::string&, const Proxy&, HttpCallbackPtr) {}
  virtual void asyncDeleteImpl(const std::string&, const HttpHeaders&, std::string&, const Proxy&, HttpCallbackPtr) {}
  virtual void shutdownImpl() {}
};

class DummyThreadPool : public ThreadPool {
private:
  virtual void shutdownImpl() {}
  virtual void postImpl(std::function<void()>) {}
};

class DummyRetryTimerService : public RetryTimerService {
private:
  virtual void shutdownImpl() {}
  virtual RetryTimerPtr createRetryTimerImpl(const RetryNotifiableWeakPtr&, const RetryTiming&) {
    return RetryTimerPtr();
  }
};

class DummyProblemManager : public ProblemManager {
private:
  virtual SubmittedProblemPtr submitProblemImpl(string&, string&, json::Value&, json::Object&) {
    return SubmittedProblemPtr();
  }
  virtual SubmittedProblemPtr addProblemImpl(const string&) { return SubmittedProblemPtr(); }
  virtual SolverMap fetchSolversImpl() { return SolverMap(); }
};

} // namespace {anonymous}

namespace http {
HttpServicePtr makeHttpService(int) { return make_shared<DummyHttpService>(); }
} // namespace sapiremote::http

const RetryTiming& defaultRetryTiming() {
  static const auto t = RetryTiming{-1, -1, -1.0f};
  return t;
}

ThreadPoolPtr makeThreadPool(int) { return make_shared<DummyThreadPool>(); }
RetryTimerServicePtr makeRetryTimerService() { return make_shared<DummyRetryTimerService>(); }
SapiServicePtr makeSapiService(http::HttpServicePtr, string, string, http::Proxy) { return SapiServicePtr(); }
AnswerServicePtr makeAnswerService(ThreadPoolPtr) { return AnswerServicePtr(); }

ProblemManagerPtr makeProblemManager(SapiServicePtr, AnswerServicePtr, RetryTimerServicePtr,
    const RetryTiming&, const ProblemManagerLimits&) { return make_shared<DummyProblemManager>(); }

} // namespace sapiremote


TEST(GlobalTest, Count) {
  sapi_Connection* null = 0;
  EXPECT_EQ(null, sapi_localConnection());
  sapi_globalInit();
  EXPECT_NE(null, sapi_localConnection());
  sapi_globalCleanup();
  EXPECT_EQ(null, sapi_localConnection());

  sapi_globalInit();
  sapi_globalInit();
  EXPECT_NE(null, sapi_localConnection());
  sapi_globalCleanup();
  EXPECT_NE(null, sapi_localConnection());
  sapi_globalCleanup();
  EXPECT_EQ(null, sapi_localConnection());

  sapi_globalCleanup();
  EXPECT_EQ(null, sapi_localConnection());
  sapi_globalInit();
  EXPECT_EQ(null, sapi_localConnection());
  sapi_globalInit();
  EXPECT_NE(null, sapi_localConnection());
  sapi_globalCleanup();
  EXPECT_EQ(null, sapi_localConnection());
}

TEST(GlobalTest, RemoteNoInit) {
  sapi_Connection* conn;
  EXPECT_EQ(SAPI_ERR_NO_INIT, sapi_remoteConnection("", "", 0, &conn, 0));
}
