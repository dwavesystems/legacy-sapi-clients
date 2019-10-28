//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <exception>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include <boost/foreach.hpp>

#include <problem.hpp>
#include <solver.hpp>
#include <problem-manager.hpp>
#include <json.hpp>
#include <exceptions.hpp>

using std::bad_alloc;
using std::exception_ptr;
using std::current_exception;
using std::rethrow_exception;
using std::make_shared;
using std::string;
using std::tuple;

using sapiremote::SubmittedProblem;
using sapiremote::SubmittedProblemPtr;
using sapiremote::SubmittedProblemObserverPtr;
using sapiremote::AnswerCallbackPtr;
using sapiremote::Solver;
using sapiremote::ProblemManager;
using sapiremote::ProblemManagerPtr;
using sapiremote::SolverMap;
using sapiremote::SubmittedProblemInfo;
using sapiremote::InternalException;
using sapiremote::ProblemCancelledException;
using sapiremote::NoAnswerException;
using sapiremote::SolveException;
using sapiremote::NetworkException;
using sapiremote::CommunicationException;
using sapiremote::AuthenticationException;
using sapiremote::Error;
using sapiremote::http::Proxy;

namespace submittedstates = sapiremote::submittedstates;
namespace remotestatuses = sapiremote::remotestatuses;
namespace errortypes = sapiremote::errortypes;

namespace sapiremote {

extern char const * const userAgent = "sapi-remote/test";

} // namespace sapiremote

namespace {

submittedstates::Type decodeSubmittedState(const string& str) {
  if (str == "SUBMITTING") return submittedstates::SUBMITTING;
  else if (str == "SUBMITTED") return submittedstates::SUBMITTED;
  else if (str == "DONE") return submittedstates::DONE;
  else if (str == "RETRYING") return submittedstates::RETRYING;
  else if (str == "FAILED") return submittedstates::FAILED;
  else throw std::invalid_argument(str);
}

remotestatuses::Type decodeRemoteStatus(const string& str) {
  if (str == "PENDING") return remotestatuses::PENDING;
  else if (str == "IN_PROGRESS") return remotestatuses::IN_PROGRESS;
  else if (str == "COMPLETED") return remotestatuses::COMPLETED;
  else if (str == "FAILED") return remotestatuses::FAILED;
  else if (str == "CANCELED") return remotestatuses::CANCELED;
  else if (str == "UNKNOWN") return remotestatuses::UNKNOWN;
  else throw std::invalid_argument(str);
}

errortypes::Type decodeErrorType(const string& str) {
  if (str == "NETWORK") return errortypes::NETWORK;
  else if (str == "PROTOCOL") return errortypes::PROTOCOL;
  else if (str == "AUTH") return errortypes::AUTH;
  else if (str == "SOLVE") return errortypes::SOLVE;
  else if (str == "MEMORY") return errortypes::MEMORY;
  else if (str == "INTERNAL") return errortypes::INTERNAL;
  else throw std::invalid_argument(str);
}

class TestSubmittedProblem : public SubmittedProblem {
private:
  string problemId_;
  string submittedOn_;
  string solvedOn_;
  tuple<string, json::Value> answer_;
  submittedstates::Type state_;
  submittedstates::Type lastGoodState_;
  remotestatuses::Type remoteStatus_;
  Error error_;
  exception_ptr ex_;

  virtual string problemIdImpl() const { return problemId_; }
  virtual bool doneImpl() const { return state_ == submittedstates::DONE; }

  virtual SubmittedProblemInfo statusImpl() const {
    SubmittedProblemInfo p;
    p.problemId = problemId_;
    p.state = state_;
    p.lastGoodState = lastGoodState_;
    p.remoteStatus = remoteStatus_;
    p.error = error_;
    p.submittedOn = submittedOn_;
    p.solvedOn = solvedOn_;
    return p;
  }

  virtual void answerImpl(AnswerCallbackPtr) const { throw InternalException("not implemented"); }

  virtual tuple<string, json::Value> answerImpl() const {
    if (state_ != submittedstates::DONE) {
      throw NoAnswerException();
    } else if (remoteStatus_ == remotestatuses::COMPLETED) {
      return answer_;
    } else {
      rethrow_exception(ex_);
    }
  }

  virtual void cancelImpl() {
    if (done()) return;
    try {
      throw ProblemCancelledException();
    } catch (ProblemCancelledException& e) {
      state_ = lastGoodState_ = submittedstates::DONE;
      remoteStatus_ = remotestatuses::CANCELED;
      error_.type = errortypes::SOLVE;
      error_.message = e.what();
      ex_ = current_exception();
    }
  }

  virtual void retryImpl() {
    if (state_ == submittedstates::FAILED
        && lastGoodState_ != submittedstates::DONE
        && error_.type != errortypes::SOLVE) {

      state_ = submittedstates::RETRYING;
    }
  }

  virtual void addSubmittedProblemObserverImpl(const SubmittedProblemObserverPtr& observer) {
    if (!problemId_.empty()) observer->notifySubmitted();
    if (done() && remoteStatus_ == remotestatuses::COMPLETED) observer->notifyDone();
    if (done() && remoteStatus_ != remotestatuses::COMPLETED) observer->notifyError();
  }

  void setErrorState(exception_ptr e) {
    try {
      rethrow_exception(e);
    } catch (bad_alloc&) {
      state_ = submittedstates::FAILED;
      error_.type = errortypes::MEMORY;
      error_.message.clear();
      ex_ = current_exception();
    } catch (NetworkException& ex) {
      state_ = submittedstates::FAILED;
      error_.type = errortypes::NETWORK;
      error_.message = ex.what();
    } catch (CommunicationException& ex) {
      state_ = submittedstates::FAILED;
      error_.type = errortypes::PROTOCOL;
      error_.message = ex.what();
    } catch (AuthenticationException& ex) {
      state_ = submittedstates::FAILED;
      error_.type = errortypes::AUTH;
      error_.message = ex.what();
    } catch (SolveException& ex) {
      state_ = lastGoodState_ = submittedstates::DONE;
      error_.type = errortypes::SOLVE;
      error_.message = ex.what();
    } catch (std::exception& ex) {
      state_ = submittedstates::FAILED;
      error_.type = errortypes::INTERNAL;
      error_.message = ex.what();
    } catch (...) {
      state_ = submittedstates::FAILED;
      error_.type = errortypes::INTERNAL;
      error_.message = "unknown error";
    }
  }

public:
  TestSubmittedProblem() : state_(submittedstates::SUBMITTING), lastGoodState_(submittedstates::SUBMITTING),
      remoteStatus_(remotestatuses::PENDING), error_(Error{errortypes::INTERNAL, string()}) {}
  TestSubmittedProblem(string problemId) : problemId_(problemId), state_(submittedstates::SUBMITTED),
      lastGoodState_(submittedstates::SUBMITTED), remoteStatus_(remotestatuses::PENDING),
      error_(Error{errortypes::INTERNAL, string()}) {}
  TestSubmittedProblem(string problemId, tuple<string, json::Value> answer) :
      problemId_(problemId), answer_(answer), state_(submittedstates::DONE), lastGoodState_(submittedstates::DONE),
      remoteStatus_(remotestatuses::COMPLETED), error_(Error{errortypes::INTERNAL, string()}) {}

  TestSubmittedProblem(string problemId, exception_ptr e) :
      problemId_(problemId), lastGoodState_(submittedstates::SUBMITTED),
      remoteStatus_(remotestatuses::PENDING), ex_(e) { setErrorState(e); }

  TestSubmittedProblem(SubmittedProblemInfo info) :
      problemId_(std::move(info.problemId)), submittedOn_(info.submittedOn), solvedOn_(info.solvedOn),
      state_(info.state), lastGoodState_(info.lastGoodState), remoteStatus_(info.remoteStatus),
      error_(std::move(info.error)) {
    try {
      throw 0;
    } catch (...) {
      ex_ = current_exception();
    }
  }
};

class TestSolver : public Solver {
private:
  virtual SubmittedProblemPtr submitProblemImpl(
      string& type, json::Value& data, json::Object& params) const {

    auto problemIdIter = params.find("problem_id");
    auto problemId = (problemIdIter != params.end() && problemIdIter->second.isString()) ?
                     problemIdIter->second.getString() : "";

    auto errorIter = params.find("error");
    bool hasError = errorIter != params.end()
                    && errorIter->second.isString();

    auto answerIter = params.find("answer");
    bool setAnswer = answerIter != params.end()
                     && answerIter->second.isBool()
                     && answerIter->second.getBool();

    if (hasError) {
      try {
        throw SolveException(errorIter->second.getString());
      } catch (...) {
        return make_shared<TestSubmittedProblem>(problemId, current_exception());
      }
    } else if (setAnswer) {
      auto answer = json::Object();
      answer["type"] = type;
      answer["data"] = data;
      answer["params"] = params;
      return make_shared<TestSubmittedProblem>(problemId,
          tuple<string, json::Value>(type, std::move(answer)));
    } else {
      return make_shared<TestSubmittedProblem>(problemId);
    }
  }
public:
  TestSolver(string id, json::Object properties) : Solver(std::move(id), std::move(properties)) {}
};

class EchoSolver : public Solver {
private:
  virtual SubmittedProblemPtr submitProblemImpl(
      string& type, json::Value& data, json::Object& params) const {

    auto& answer = data.getObject();
    BOOST_FOREACH( const auto& e, params ) {
      answer.insert(e);
    }

    return make_shared<TestSubmittedProblem>("echo",
        tuple<string, json::Value>(type, std::move(answer)));
  }
public:
  EchoSolver(string id, json::Object properties) : Solver(std::move(id), std::move(properties)) {}
};

class StatusSolver : public Solver {
private:
  virtual SubmittedProblemPtr submitProblemImpl(
      string& type, json::Value& data, json::Object& params) const {

      SubmittedProblemInfo info;
      info.problemId = params.at("problem_id").getString();
      info.state = decodeSubmittedState(params.at("state").getString());
      info.lastGoodState = decodeSubmittedState(params.at("last_good_state").getString());
      info.remoteStatus = decodeRemoteStatus(params.at("remote_status").getString());
      info.error.type = decodeErrorType(params.at("error_type").getString());
      info.error.message = params.at("error_message").getString();
      info.submittedOn = params.at("submitted_on").getString();
      info.solvedOn = params.at("solved_on").getString();
      return make_shared<TestSubmittedProblem>(std::move(info));
  }
public:
  StatusSolver(string id) : Solver(std::move(id), json::Object()) {}
};

class TestProblemManager : public ProblemManager {
private:
  string url_;
  string token_;
  Proxy proxy_;
  SolverMap solvers_;

  virtual SubmittedProblemPtr submitProblemImpl(string&, string&, json::Value&, json::Object&) {
    throw InternalException("not implemented");
  }

  virtual SubmittedProblemPtr addProblemImpl(const string& id) {
    return make_shared<TestSubmittedProblem>(id);
  }

  virtual SolverMap fetchSolversImpl() { return solvers_; }

public:
  TestProblemManager(string url, string token, Proxy proxy) :
      url_(url), token_(token), proxy_(proxy) {

    auto testProps = json::Object{};
    testProps["qubits"] = json::Array{ 1, 4, 5 };
    auto couplers = json::Array{};
    couplers.push_back(json::Array{1, 4});
    couplers.push_back(json::Array{4, 5});
    testProps["couplers"] = std::move(couplers);
    auto testSolver = make_shared<TestSolver>("test", testProps);
    solvers_.insert(make_pair(testSolver->id(), testSolver));

    auto echoSolver = make_shared<EchoSolver>("echo", json::Object());
    solvers_.insert(make_pair(echoSolver->id(), echoSolver));

    json::Object configProps;
    configProps["url"] = url_;
    configProps["token"] = token_;
    configProps["proxy"] = proxy_.enabled() ? json::Value(proxy_.url()) : json::Null();
    auto configSolver = make_shared<TestSolver>("config", std::move(configProps));
    solvers_.insert(make_pair(configSolver->id(), configSolver));

    auto statusSolver = make_shared<StatusSolver>("status");
    solvers_.insert(make_pair(statusSolver->id(), statusSolver));
  }
};

} // namespace {anonymous}

namespace testimpl {

ProblemManagerPtr createProblemManager(string url, string token, Proxy proxy) {
  return make_shared<TestProblemManager>(std::move(url), std::move(token), std::move(proxy));
}

} // namespace testimpl
