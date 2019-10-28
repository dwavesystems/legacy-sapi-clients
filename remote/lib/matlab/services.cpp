//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <mex.h>

#include <http-service.hpp>
#include <sapi-service.hpp>
#include <retry-service.hpp>
#include <problem-manager.hpp>
#include <answer-service.hpp>
#include <solver.hpp>

#include "sapiremote-mex.hpp"

using std::make_shared;
using std::ostringstream;
using std::string;
using std::unique_ptr;
using std::unordered_map;

using sapiremote::makeThreadPool;
using sapiremote::ThreadPoolPtr;
using sapiremote::http::HttpServicePtr;
using sapiremote::http::makeHttpService;
using sapiremote::AnswerServicePtr;
using sapiremote::RetryTimerServicePtr;
using sapiremote::defaultRetryTiming;
using sapiremote::makeRetryTimerService;
using sapiremote::ProblemManagerLimits;
using sapiremote::ProblemManagerPtr;
using sapiremote::makeSapiService;
using sapiremote::makeProblemManager;

namespace {

const ProblemManagerLimits limits = {
    20,  // maxProblemsPerSubmission
    100, // maxIdsPerStatusQuery
    6    // maxActiveProblemSubmissions
};


ThreadPoolPtr getCallbackThreadPool() {
  static ThreadPoolPtr threadPool;
  if (!threadPool) threadPool = makeThreadPool(2);
  return threadPool;
}

HttpServicePtr getHttpService() {
  static HttpServicePtr httpService;
  if (!httpService) httpService = makeHttpService(2);
  return httpService;
}

AnswerServicePtr getAnswerService() {
  static AnswerServicePtr answerService;
  if (!answerService) answerService = makeAnswerService(getCallbackThreadPool());
  return answerService;
}

RetryTimerServicePtr getRetryService() {
  static RetryTimerServicePtr retryService;
  if (!retryService) retryService = makeRetryTimerService();
  return retryService;
}

} // namespace {anonymous}

ProblemManagerPtr makeProblemManager(ConnectionInfo conninfo) {
  auto sapiService = makeSapiService(getHttpService(),
      std::move(conninfo.url), std::move(conninfo.token), std::move(conninfo.proxy));
  return makeProblemManager(sapiService, getAnswerService(), getRetryService(), defaultRetryTiming(), limits);
}

void exitFn() {
  try {
    auto httpService = getHttpService();
    httpService->shutdown();
  } catch (...) {}

  try {
    auto callbackThreadPool = getCallbackThreadPool();
    callbackThreadPool->shutdown();
  } catch (...) {}

  try {
    auto retryService = getRetryService();
    retryService->shutdown();
  } catch (...) {}
}

