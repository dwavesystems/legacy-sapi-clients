//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <exceptions.hpp>
#include <http-service.hpp>
#include <sapi-service.hpp>

#include "test.hpp"
#include "json-builder.hpp"

using std::exception_ptr;
using std::make_shared;
using std::rethrow_exception;
using std::string;
using std::unique_ptr;
using std::vector;

using testing::SaveArg;
using testing::Eq;
using testing::ElementsAre;
using testing::AtLeast;
using testing::Invoke;
using testing::_;

using sapiremote::http::HttpService;
using sapiremote::http::HttpHeaders;
using sapiremote::http::Proxy;
using sapiremote::http::HttpCallbackPtr;

using sapiremote::SapiService;
using sapiremote::SolversSapiCallback;
using sapiremote::SolversSapiCallbackPtr;
using sapiremote::StatusSapiCallback;
using sapiremote::StatusSapiCallbackPtr;
using sapiremote::CancelSapiCallback;
using sapiremote::FetchAnswerSapiCallback;
using sapiremote::FetchAnswerSapiCallbackPtr;
using sapiremote::SolverInfo;
using sapiremote::RemoteProblemInfo;
using sapiremote::makeSapiService;
using sapiremote::SolveException;
using sapiremote::ProblemCancelledException;
using sapiremote::NoAnswerException;
using sapiremote::AuthenticationException;
using sapiremote::Problem;

namespace remotestatuses = sapiremote::remotestatuses;

namespace {

auto a = jsonArray();
auto o = jsonObject();

class MockHttpService : public HttpService {
public:
  MOCK_METHOD4(asyncGetImpl,
      void(const string& url, const HttpHeaders& headers, const Proxy& proxy, HttpCallbackPtr callback));
  MOCK_METHOD5(asyncPostImpl, void(
      const string& url, const HttpHeaders& headers, string& data, const Proxy& proxy, HttpCallbackPtr callback));
  MOCK_METHOD5(asyncDeleteImpl, void(
      const string& url, const HttpHeaders& headers, string& data, const Proxy& proxy, HttpCallbackPtr callback));
  MOCK_METHOD0(shutdownImpl, void());
};

class MockSolverSapiCallback : public SolversSapiCallback {
public:
  MOCK_METHOD1(completeImpl, void(vector<SolverInfo>&));
  MOCK_METHOD1(errorImpl, void(std::exception_ptr));
};

class MockStatusSapiCallback : public StatusSapiCallback {
public:
  MOCK_METHOD1(completeImpl, void(vector<RemoteProblemInfo>&));
  MOCK_METHOD1(errorImpl, void(std::exception_ptr));
};

class MockCancelSapiCallback : public CancelSapiCallback {
public:
  MOCK_METHOD0(completeImpl, void());
  MOCK_METHOD1(errorImpl, void(std::exception_ptr));
};

class MockFetchAnswerSapiCallback : public FetchAnswerSapiCallback {
public:
  MOCK_METHOD2(completeImpl, void(string&, json::Value&));
  MOCK_METHOD1(errorImpl, void(std::exception_ptr));
};

MATCHER_P(HasAuthToken, token, "") {
  auto iter = arg.find("X-Auth-Token");
  return iter != arg.end() && iter->second == token;
}

MATCHER_P(UsesProxy, proxy, "") {
  return arg.enabled() == proxy.enabled() && (!arg.enabled() || arg.url() == proxy.url());
}

MATCHER_P(EqJson, matchJson, "") {
  try {
    auto argJson = json::stringToJson(arg);
    return argJson == matchJson;
  } catch (json::ParseException&) {
    return false;
  }
}

const auto solversPath = "solvers/remote/";
const auto problemsPath = "problems/";

string problemStatusToString(remotestatuses::Type s) {
  switch (s) {
    case remotestatuses::PENDING: return "PENDING";
    case remotestatuses::IN_PROGRESS: return "IN_PROGRESS";
    case remotestatuses::COMPLETED: return "COMPLETED";
    case remotestatuses::FAILED: return "FAILED";
    case remotestatuses::CANCELED: return "CANCELED";
    default: throw std::invalid_argument("unknown problem status");
  }
}

} // namespace {anonymous}

namespace std {
void PrintTo(exception_ptr ep, ostream* os) {
  if (ep == exception_ptr()) {
    *os << "(empty exception_ptr)";
    return;
  }
  try {
    std::rethrow_exception(ep);
  } catch (std::exception& e) {
    *os << e.what();
  } catch (...) {
    *os << "(unknown exception type)";
  }
}
} // namespace std

namespace sapiremote {
namespace http {
void PrintTo(const Proxy& proxy, std::ostream* os) {
  if (proxy.enabled()) {
    *os << "Proxy(\"" << proxy.url() << "\")";
  } else {
    *os << "Proxy()";
  }
}
} // namespace sapiremote::http
} // namespace sapiremote



TEST(SapiServiceTest, fetchSolvers) {
  const auto baseUrl = string("test://test/");

  HttpCallbackPtr httpCallback;

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);
  EXPECT_CALL(*mockHttpService, asyncGetImpl(Eq(baseUrl + solversPath), _, _, _)).WillOnce(SaveArg<3>(&httpCallback));

  auto expectedSolver1 = makeSolverInfo("solver1", (o, "useful", false));
  auto expectedSolver2 = makeSolverInfo("solver2", (o, "stuff", (a, 1, "two", json::Null())));

  auto mockSolverSapiCallback = make_shared<MockSolverSapiCallback>();
  EXPECT_CALL(*mockSolverSapiCallback, errorImpl(_)).Times(0);
  EXPECT_CALL(*mockSolverSapiCallback, completeImpl(ElementsAre(expectedSolver1, expectedSolver2))).Times(1);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  sapiService->fetchSolvers(mockSolverSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  auto solverData = (a,
      (o, "id", expectedSolver1.id, "properties", expectedSolver1.properties),
      (o, "id", expectedSolver2.id, "properties", expectedSolver2.properties)).value();
  httpCallback->complete(200, make_shared<string>(json::jsonToString(solverData)));
}



TEST(SapiServiceTest, fetchSolversAuthFailure) {
  const auto baseUrl = string("test://test/");

  HttpCallbackPtr httpCallback;

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);
  EXPECT_CALL(*mockHttpService, asyncGetImpl(Eq(baseUrl + solversPath), _, _, _)).WillOnce(SaveArg<3>(&httpCallback));

  auto mockSolverSapiCallback = make_shared<MockSolverSapiCallback>();
  EXPECT_CALL(*mockSolverSapiCallback, errorImpl(Thrown(AuthenticationException))).Times(1);
  EXPECT_CALL(*mockSolverSapiCallback, completeImpl(_)).Times(0);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  sapiService->fetchSolvers(mockSolverSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  httpCallback->complete(401, make_shared<string>());
}



TEST(SapiServiceTest, submitProblem) {
  const auto baseUrl = string("test://test/");

  HttpCallbackPtr httpCallback;
  auto solver = "solver-123";
  auto problemType = "magic";
  auto problemData = (a, "things", json::Null(), 456).array();
  auto problemParams = (o, "good", false).object();
  auto postData = (a, (o, "solver", solver, "type", problemType, "data", problemData, "params", problemParams)).value();

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncGetImpl(_, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);
  EXPECT_CALL(*mockHttpService, asyncPostImpl(Eq(baseUrl + problemsPath), _, EqJson(postData), _, _))
      .WillOnce(SaveArg<4>(&httpCallback));

  RemoteProblemInfo pi = makeProblemInfo("problem-002", problemType, remotestatuses::IN_PROGRESS);
  vector<RemoteProblemInfo> pis(1, pi);
  auto statusData = (a, (o, "id", pi.id, "type", pi.type, "status", problemStatusToString(pi.status))).array();

  auto mockStatusSapiCallback = make_shared<MockStatusSapiCallback>();
  EXPECT_CALL(*mockStatusSapiCallback, errorImpl(_)).Times(0);
  EXPECT_CALL(*mockStatusSapiCallback, completeImpl(pis)).Times(1);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  vector<Problem> problems;
  problems.push_back(Problem(solver, problemType, problemData, problemParams));
  sapiService->submitProblems(problems, mockStatusSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  httpCallback->complete(200, make_shared<string>(json::jsonToString(statusData)));
}



TEST(SapiServiceTest, submitMultipleProblems) {
  const auto baseUrl = string("test://test/");

  vector<Problem> problems;
  problems.push_back(Problem("solver1", "a", "data", (o, "param", 4).object()));
  problems.push_back(Problem("solver1", "b", "different data", json::Object()));
  problems.push_back(Problem("solver5", "a", (a, 1, 2).array(), (o, "hello", true).object()));
  problems.push_back(Problem("solver4", "zxcv", 100, (o, "hello", false).object()));

  json::Array postData;
  BOOST_FOREACH( const auto& p, problems ) {
    postData.push_back((o, "solver", p.solver(), "type", p.type(), "data", p.data(), "params", p.params()).value());
  }

  HttpCallbackPtr httpCallback;
  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncGetImpl(_, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);
  EXPECT_CALL(*mockHttpService, asyncPostImpl(Eq(baseUrl + problemsPath), _, EqJson(postData), _, _))
      .WillOnce(SaveArg<4>(&httpCallback));

  vector<RemoteProblemInfo> pis;
  pis.push_back(makeProblemInfo("1", "a", remotestatuses::COMPLETED));
  pis.push_back(makeProblemInfo("2", "b", remotestatuses::FAILED));
  pis.push_back(makeProblemInfo("3", "a", remotestatuses::PENDING));
  pis.push_back(makeProblemInfo("5", "zxcv", remotestatuses::IN_PROGRESS));
  json::Array statusData;
  BOOST_FOREACH( const auto& pi, pis ) {
    statusData.push_back((o, "id", pi.id, "type", pi.type, "status", problemStatusToString(pi.status)).value());
  }

  auto mockStatusSapiCallback = make_shared<MockStatusSapiCallback>();
  EXPECT_CALL(*mockStatusSapiCallback, errorImpl(_)).Times(0);
  EXPECT_CALL(*mockStatusSapiCallback, completeImpl(pis)).Times(1);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  sapiService->submitProblems(problems, mockStatusSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  httpCallback->complete(200, make_shared<string>(json::jsonToString(statusData)));
}



TEST(SapiServiceTest, submitProblemAuthFailure) {
  const auto baseUrl = string("test://test/");

  HttpCallbackPtr httpCallback;
  auto solver = "solver-123";
  auto problemType = "magic";
  auto problemData = (a, "things", json::Null(), 456).array();
  auto problemParams = (o, "good", false).object();

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncGetImpl(_, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, _, _)).WillOnce(SaveArg<4>(&httpCallback));

  auto mockStatusSapiCallback = make_shared<MockStatusSapiCallback>();
  EXPECT_CALL(*mockStatusSapiCallback, errorImpl(Thrown(AuthenticationException))).Times(1);
  EXPECT_CALL(*mockStatusSapiCallback, completeImpl(_)).Times(0);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  vector<Problem> problems;
  problems.push_back(Problem(solver, problemType, problemData, problemParams));
  sapiService->submitProblems(problems, mockStatusSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  httpCallback->complete(401, make_shared<string>());
}



TEST(SapiServiceTest, multiProblemStatus) {
  const auto baseUrl = string("test://test/");

  HttpCallbackPtr httpCallback;

  vector<RemoteProblemInfo> expectedProblemInfos;
  expectedProblemInfos.push_back(makeProblemInfo("p1", "hello", remotestatuses::COMPLETED, "abc", "def"));
  expectedProblemInfos.push_back(makeProblemInfo("p2", "blarg", remotestatuses::PENDING, "1111"));
  expectedProblemInfos.push_back(makeProblemInfo("", "", remotestatuses::FAILED));
  expectedProblemInfos.push_back(makeProblemInfo("p2", "blarg", remotestatuses::IN_PROGRESS, "a day ago"));
  expectedProblemInfos.push_back(makeProblemInfo("p100", "asdf", remotestatuses::CANCELED, "", "weird"));
  expectedProblemInfos.push_back(makeProblemInfo("p101", "jkl;", remotestatuses::CANCELED, "qwerty", "uiop"));
  expectedProblemInfos.push_back(makeProblemInfo("p234", "zxcv", remotestatuses::FAILED));

  auto statusUrl = baseUrl + problemsPath + "?id=p1,p2,p3,p2,p100,p101,p234";
  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncGetImpl(statusUrl, _, _, _)).WillOnce(SaveArg<3>(&httpCallback));
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);

  vector<string> problemIds;
  problemIds.push_back(expectedProblemInfos[0].id);
  problemIds.push_back(expectedProblemInfos[1].id);
  problemIds.push_back("p3");
  problemIds.push_back(expectedProblemInfos[3].id);
  problemIds.push_back(expectedProblemInfos[4].id);
  problemIds.push_back(expectedProblemInfos[5].id);
  problemIds.push_back(expectedProblemInfos[6].id);

  auto mockStatusSapiCallback = make_shared<MockStatusSapiCallback>();
  EXPECT_CALL(*mockStatusSapiCallback, errorImpl(_)).Times(0);
  EXPECT_CALL(*mockStatusSapiCallback, completeImpl(expectedProblemInfos)).Times(1);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  sapiService->multiProblemStatus(problemIds, mockStatusSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  auto statusData = (a,
      (o, "id", "p1", "type", "hello", "status", "COMPLETED", "submitted_on", "abc", "solved_on", "def"),
      (o, "id", "p2", "type", "blarg", "status", "PENDING", "submitted_on", "1111"),
      json::Null(),
      (o, "id", "p2", "type", "blarg", "status", "IN_PROGRESS", "submitted_on", "a day ago"),
      (o, "id", "p100", "type", "asdf", "status", "CANCELED", "solved_on", "weird"),
      (o, "id", "p101", "type", "jkl;", "status", "CANCELLED", "submitted_on", "qwerty", "solved_on", "uiop"),
      (o, "id", "p234", "type", "zxcv", "status", "FAILED")).array();
  httpCallback->complete(200, make_shared<string>(json::jsonToString(statusData)));
}



TEST(SapiServiceTest, multiProblemStatusAuthFailure) {
  const auto baseUrl = string("test://test/");

  HttpCallbackPtr httpCallback;

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncGetImpl(_, _, _, _)).WillOnce(SaveArg<3>(&httpCallback));
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);

  auto mockStatusSapiCallback = make_shared<MockStatusSapiCallback>();
  EXPECT_CALL(*mockStatusSapiCallback, errorImpl(Thrown(AuthenticationException))).Times(1);
  EXPECT_CALL(*mockStatusSapiCallback, completeImpl(_)).Times(0);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  sapiService->multiProblemStatus(vector<string>{"p123"}, mockStatusSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  httpCallback->complete(401, make_shared<string>());
}



TEST(SapiServiceTest, fetchAnswer) {
  const auto baseUrl = string("test://test/");
  auto problemType = string("magic");
  const auto problemId = "12345";
  const auto answerUrl = baseUrl + problemsPath + problemId + "/";

  HttpCallbackPtr httpCallback;

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncGetImpl(answerUrl, _, _, _)).WillOnce(SaveArg<3>(&httpCallback));
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);

  auto expectedAnswer = (a, (o, "stuff", (a, 1, 2, false), json::Null())).value();
  auto answerData = (o, "status", "COMPLETED", "type", problemType, "answer", expectedAnswer).value();

  auto mockFetchAnswerSapiCallback = make_shared<MockFetchAnswerSapiCallback>();
  EXPECT_CALL(*mockFetchAnswerSapiCallback, errorImpl(_)).Times(0);
  EXPECT_CALL(*mockFetchAnswerSapiCallback, completeImpl(problemType, expectedAnswer)).Times(1);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  sapiService->fetchAnswer(problemId, mockFetchAnswerSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  httpCallback->complete(200, make_shared<string>(json::jsonToString(answerData)));
}



TEST(SapiServiceTest, fetchAnswerUnavailable) {
  const auto baseUrl = string("test://test/");
  const auto problemId = "12345";
  const auto answerUrl = baseUrl + problemsPath + problemId + "/";

  HttpCallbackPtr httpCallback;

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncGetImpl(answerUrl, _, _, _)).WillOnce(SaveArg<3>(&httpCallback));
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);

  auto mockFetchAnswerSapiCallback = make_shared<MockFetchAnswerSapiCallback>();
  EXPECT_CALL(*mockFetchAnswerSapiCallback, errorImpl(Thrown(NoAnswerException))).Times(1);
  EXPECT_CALL(*mockFetchAnswerSapiCallback, completeImpl(_, _)).Times(0);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  sapiService->fetchAnswer(problemId, mockFetchAnswerSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  auto answerData = (o, "status", "IN_PROGRESS").value();
  httpCallback->complete(200, make_shared<string>(json::jsonToString(answerData)));
}



TEST(SapiServiceTest, fetchAnswerFailed) {
  const auto baseUrl = string("test://test/");
  const auto problemId = "12345";
  const auto answerUrl = baseUrl + problemsPath + problemId + "/";

  HttpCallbackPtr httpCallback;

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncGetImpl(answerUrl, _, _, _)).WillOnce(SaveArg<3>(&httpCallback));
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);

  auto mockFetchAnswerSapiCallback = make_shared<MockFetchAnswerSapiCallback>();
  EXPECT_CALL(*mockFetchAnswerSapiCallback, errorImpl(Thrown(SolveException))).Times(1);
  EXPECT_CALL(*mockFetchAnswerSapiCallback, completeImpl(_, _)).Times(0);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  sapiService->fetchAnswer(problemId, mockFetchAnswerSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  auto answerData = (o, "status", "FAILED").value();
  httpCallback->complete(200, make_shared<string>(json::jsonToString(answerData)));
}



TEST(SapiServiceTest, fetchAnswerCancelled) {
  const auto baseUrl = string("test://test/");
  const auto problemId = "12345";
  const auto answerUrl = baseUrl + problemsPath + problemId + "/";

  HttpCallbackPtr httpCallback;

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncGetImpl(answerUrl, _, _, _)).WillOnce(SaveArg<3>(&httpCallback));
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);

  auto mockFetchAnswerSapiCallback = make_shared<MockFetchAnswerSapiCallback>();
  EXPECT_CALL(*mockFetchAnswerSapiCallback, errorImpl(Thrown(ProblemCancelledException))).Times(1);
  EXPECT_CALL(*mockFetchAnswerSapiCallback, completeImpl(_, _)).Times(0);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  sapiService->fetchAnswer(problemId, mockFetchAnswerSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  auto answerData = (o, "status", "CANCELED").value();
  httpCallback->complete(200, make_shared<string>(json::jsonToString(answerData)));
}



TEST(SapiServiceTest, fetchAnswerCancelledTwoElls) {
  const auto baseUrl = string("test://test/");
  const auto problemId = "12345";
  const auto answerUrl = baseUrl + problemsPath + problemId + "/";

  HttpCallbackPtr httpCallback;

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncGetImpl(answerUrl, _, _, _)).WillOnce(SaveArg<3>(&httpCallback));
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);

  auto mockFetchAnswerSapiCallback = make_shared<MockFetchAnswerSapiCallback>();
  EXPECT_CALL(*mockFetchAnswerSapiCallback, errorImpl(Thrown(ProblemCancelledException))).Times(1);
  EXPECT_CALL(*mockFetchAnswerSapiCallback, completeImpl(_, _)).Times(0);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  sapiService->fetchAnswer(problemId, mockFetchAnswerSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  auto answerData = (o, "status", "CANCELLED").value();
  httpCallback->complete(200, make_shared<string>(json::jsonToString(answerData)));
}



TEST(SapiServiceTest, fetchAnswerFailedErrorMessage) {
  const auto baseUrl = string("test://test/");
  const auto problemId = "12345";
  const auto answerUrl = baseUrl + problemsPath + problemId + "/";
  auto errMsg = "uh oh!";

  HttpCallbackPtr httpCallback;
  exception_ptr ex;

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncGetImpl(answerUrl, _, _, _)).WillOnce(SaveArg<3>(&httpCallback));
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);

  auto mockFetchAnswerSapiCallback = make_shared<MockFetchAnswerSapiCallback>();
  EXPECT_CALL(*mockFetchAnswerSapiCallback, errorImpl(Thrown(SolveException))).WillOnce(SaveArg<0>(&ex));
  EXPECT_CALL(*mockFetchAnswerSapiCallback, completeImpl(_, _)).Times(0);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  sapiService->fetchAnswer(problemId, mockFetchAnswerSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  auto answerData = (o, "status", "FAILED", "error_message", errMsg).value();
  httpCallback->complete(200, make_shared<string>(json::jsonToString(answerData)));

  try {
    rethrow_exception(ex);
  } catch (SolveException& e) {
    EXPECT_NE(string::npos, string(e.what()).find(errMsg));
  }
}



TEST(SapiServiceTest, fetchAnswerAuthFailure) {
  const auto baseUrl = string("test://test/");

  HttpCallbackPtr httpCallback;

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, asyncGetImpl(_, _, _, _)).WillOnce(SaveArg<3>(&httpCallback));
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);

  auto mockFetchAnswerSapiCallback = make_shared<MockFetchAnswerSapiCallback>();
  EXPECT_CALL(*mockFetchAnswerSapiCallback, errorImpl(Thrown(AuthenticationException))).Times(1);
  EXPECT_CALL(*mockFetchAnswerSapiCallback, completeImpl(_, _)).Times(0);

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", Proxy());
  sapiService->fetchAnswer("123", mockFetchAnswerSapiCallback);

  ASSERT_TRUE(!!httpCallback);
  httpCallback->complete(401, make_shared<string>());
}



TEST(SapiServiceTest, useProxy) {
  const auto baseUrl = string("test://test/");
  const auto proxy = Proxy("proxy://something");
  const auto problemId = "123";
  const auto problems = vector<Problem>(1);

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);
  EXPECT_CALL(*mockHttpService, asyncGetImpl(_, _, UsesProxy(proxy), _)).Times(AtLeast(0));
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, UsesProxy(proxy), _)).Times(AtLeast(0));
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, UsesProxy(proxy), _)).Times(AtLeast(0));

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", proxy);
  sapiService->fetchSolvers(make_shared<MockSolverSapiCallback>());
  sapiService->submitProblems(problems, make_shared<MockStatusSapiCallback>());
  sapiService->multiProblemStatus(vector<string>(1, problemId), make_shared<MockStatusSapiCallback>());
  sapiService->fetchAnswer("", make_shared<MockFetchAnswerSapiCallback>());
  sapiService->cancelProblems(vector<string>(1, ""), make_shared<MockCancelSapiCallback>());
}



TEST(SapiServiceTest, noProxy) {
  const auto baseUrl = string("test://test/");
  const auto proxy = Proxy();
  const auto problemId = "123";
  const auto problems = vector<Problem>(1);

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);
  EXPECT_CALL(*mockHttpService, asyncGetImpl(_, _, UsesProxy(proxy), _)).Times(AtLeast(0));
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, _, _, UsesProxy(proxy), _)).Times(AtLeast(0));
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, _, _, UsesProxy(proxy), _)).Times(AtLeast(0));

  auto sapiService = makeSapiService(mockHttpService, baseUrl, "", proxy);
  sapiService->fetchSolvers(make_shared<MockSolverSapiCallback>());
  sapiService->submitProblems(problems, make_shared<MockStatusSapiCallback>());
  sapiService->multiProblemStatus(vector<string>(1, problemId), make_shared<MockStatusSapiCallback>());
  sapiService->fetchAnswer("", make_shared<MockFetchAnswerSapiCallback>());
  sapiService->cancelProblems(vector<string>(1, ""), make_shared<MockCancelSapiCallback>());
}



TEST(SapiServiceTest, token) {
  const auto baseUrl = string("test://test/");
  const auto proxy = Proxy();
  const auto token = "qwerty";
  const auto problemId = "123";
  const auto problems = vector<Problem>(1);

  auto mockHttpService = make_shared<MockHttpService>();
  EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);
  EXPECT_CALL(*mockHttpService, asyncGetImpl(_, HasAuthToken(token), _, _)).Times(AtLeast(0));
  EXPECT_CALL(*mockHttpService, asyncPostImpl(_, HasAuthToken(token), _, _, _)).Times(AtLeast(0));
  EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, HasAuthToken(token), _, _, _)).Times(AtLeast(0));

  auto sapiService = makeSapiService(mockHttpService, baseUrl, token, proxy);
  sapiService->fetchSolvers(make_shared<MockSolverSapiCallback>());
  sapiService->submitProblems(problems, make_shared<MockStatusSapiCallback>());
  sapiService->multiProblemStatus(vector<string>(1, problemId), make_shared<MockStatusSapiCallback>());
  sapiService->fetchAnswer("", make_shared<MockFetchAnswerSapiCallback>());
  sapiService->cancelProblems(vector<string>(1, ""), make_shared<MockCancelSapiCallback>());
}



TEST(SapiServiceTest, emptyToken) { // SC-187
  const auto baseUrl = string("test://test/");
  const auto proxy = Proxy();
  const auto tokens = vector<string>{" ", "\t", "\n", "\r", "\v", "\f", " \t\n\r\v\f"};
  const auto strippedToken = "";
  const auto problemId = "123";
  const auto problems = vector<Problem>(1);

  BOOST_FOREACH( const auto& token, tokens ) {
    auto mockHttpService = make_shared<MockHttpService>();
    EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);
    EXPECT_CALL(*mockHttpService, asyncGetImpl(_, HasAuthToken(strippedToken), _, _)).Times(AtLeast(0));
    EXPECT_CALL(*mockHttpService, asyncPostImpl(_, HasAuthToken(strippedToken), _, _, _)).Times(AtLeast(0));
    EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, HasAuthToken(strippedToken), _, _, _)).Times(AtLeast(0));

    auto sapiService = makeSapiService(mockHttpService, baseUrl, token, proxy);
    sapiService->fetchSolvers(make_shared<MockSolverSapiCallback>());
    sapiService->submitProblems(problems, make_shared<MockStatusSapiCallback>());
    sapiService->multiProblemStatus(vector<string>(1, problemId), make_shared<MockStatusSapiCallback>());
    sapiService->fetchAnswer("", make_shared<MockFetchAnswerSapiCallback>());
    sapiService->cancelProblems(vector<string>(1, ""), make_shared<MockCancelSapiCallback>());
  }
}



TEST(SapiServiceTest, tokenStripWs) { // SC-187
  const auto baseUrl = string("test://test/");
  const auto proxy = Proxy();
  const auto tokens = vector<string>{
    " qwerty", "\tqwerty", "\nqwerty", "\rqwerty", "\vqwerty", "\fqwerty",
    "qwerty ", "qwerty\t", "qwerty\n", "qwerty\r", "qwerty\v", "qwerty\f",
    " qwerty ", " \t\n\r\v\fqwerty   ", "\t\t\tqwerty\n\r\r\n", "\v\v\vqwerty", "qwerty\n\r"
  };
  const auto strippedToken = "qwerty";
  const auto problemId = "123";
  const auto problems = vector<Problem>(1);

  BOOST_FOREACH( const auto& token, tokens ) {
    auto mockHttpService = make_shared<MockHttpService>();
    EXPECT_CALL(*mockHttpService, shutdownImpl()).Times(0);
    EXPECT_CALL(*mockHttpService, asyncGetImpl(_, HasAuthToken(strippedToken), _, _)).Times(AtLeast(0));
    EXPECT_CALL(*mockHttpService, asyncPostImpl(_, HasAuthToken(strippedToken), _, _, _)).Times(AtLeast(0));
    EXPECT_CALL(*mockHttpService, asyncDeleteImpl(_, HasAuthToken(strippedToken), _, _, _)).Times(AtLeast(0));

    auto sapiService = makeSapiService(mockHttpService, baseUrl, token, proxy);
    sapiService->fetchSolvers(make_shared<MockSolverSapiCallback>());
    sapiService->submitProblems(problems, make_shared<MockStatusSapiCallback>());
    sapiService->multiProblemStatus(vector<string>(1, problemId), make_shared<MockStatusSapiCallback>());
    sapiService->fetchAnswer("", make_shared<MockFetchAnswerSapiCallback>());
    sapiService->cancelProblems(vector<string>(1, ""), make_shared<MockCancelSapiCallback>());
  }
}



TEST(SapiServiceTest, tokenInvalidChars) { // SC-187
  const auto baseUrl = string("test://test/");
  const auto proxy = Proxy();
  const auto tokens = vector<string>{
    "a b", "a\nb", " a b ", " a b", "a b ", "\1", "a\tb",
  };

  BOOST_FOREACH( const auto& token, tokens ) {
    auto mockHttpService = make_shared<MockHttpService>();
    EXPECT_THROW(makeSapiService(mockHttpService, baseUrl, token, proxy), AuthenticationException);
  }
}
