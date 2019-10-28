//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <string>
#include <utility>

#include <problem-manager.hpp>
#include <sapi-service.hpp>
#include <answer-service.hpp>
#include <http-service.hpp>
#include <threadpool.hpp>

using std::string;

using sapiremote::ProblemManagerLimits;
using sapiremote::ProblemManagerPtr;
using sapiremote::makeProblemManager;
using sapiremote::makeSapiService;
using sapiremote::AnswerServicePtr;
using sapiremote::makeAnswerService;
using sapiremote::makeThreadPool;
using sapiremote::RetryTimerServicePtr;
using sapiremote::defaultRetryTiming;
using sapiremote::makeRetryTimerService;
using sapiremote::http::HttpServicePtr;
using sapiremote::http::makeHttpService;
using sapiremote::http::Proxy;

namespace {

const ProblemManagerLimits limits = {
    20,  // maxProblemsPerSubmission
    100, // maxIdsPerStatusQuery
    6    // maxActiveRequests
};

HttpServicePtr getHttpService() {
  static auto service = makeHttpService(2);
  return service;
}

AnswerServicePtr getAnswerService() {
  static auto service = makeAnswerService(makeThreadPool(2));
  return service;
}

RetryTimerServicePtr getRetryService() {
  static auto service = makeRetryTimerService();
  return service;
}

} // namespace {anonymous}

ProblemManagerPtr createProblemManager(string& url, string& token, Proxy& proxy) {
  auto sapiService = makeSapiService(getHttpService(), std::move(url), std::move(token), std::move(proxy));
  return makeProblemManager(sapiService, getAnswerService(), getRetryService(), defaultRetryTiming(), limits);
}
