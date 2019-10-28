//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef SAPI_SERVICE_HPP_INCLUDED
#define SAPI_SERVICE_HPP_INCLUDED

#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "http-service.hpp"
#include "types.hpp"
#include "json.hpp"

namespace sapiremote {

class SapiCallback {
private:
  virtual void errorImpl(std::exception_ptr e) = 0;

public:
  virtual ~SapiCallback() {}
  void error(std::exception_ptr e) {
    // Quash errors.  Service can't do anything about them.
    try { errorImpl(e); } catch (...) {}
  }
};

class SolversSapiCallback : public SapiCallback {
private:
  virtual void completeImpl(std::vector<SolverInfo>& solverInfo) = 0;
public:
  void complete(std::vector<SolverInfo> solverInfo) {
    try {
      completeImpl(solverInfo);
    } catch (...) {
      error(std::current_exception());
    }
  }
};
typedef std::shared_ptr<SolversSapiCallback> SolversSapiCallbackPtr;

class StatusSapiCallback : public SapiCallback {
private:
  virtual void completeImpl(std::vector<RemoteProblemInfo>& problemInfo) = 0;
public:
  void complete(std::vector<RemoteProblemInfo> problemInfo) {
    try {
      completeImpl(problemInfo);
    } catch (...) {
      error(std::current_exception());
    }
  }
};
typedef std::shared_ptr<StatusSapiCallback> StatusSapiCallbackPtr;

class CancelSapiCallback : public SapiCallback {
private:
  virtual void completeImpl() = 0;
public:
  void complete() {
    try {
      completeImpl();
    } catch (...) {
      error(std::current_exception());
    }
  }
};
typedef std::shared_ptr<CancelSapiCallback> CancelSapiCallbackPtr;

class FetchAnswerSapiCallback : public SapiCallback {
private:
  virtual void completeImpl(std::string& type, json::Value& answer) = 0;
public:
  void complete(std::string type, json::Value answer) {
    try {
      completeImpl(type, answer);
    } catch (...) {
      error(std::current_exception());
    }
  }
};
typedef std::shared_ptr<FetchAnswerSapiCallback> FetchAnswerSapiCallbackPtr;

class Problem {
private:
  std::string solver_;
  std::string type_;
  json::Value data_;
  json::Object params_;

public:
  Problem() {}

  Problem(std::string solver, std::string type, json::Value data, json::Object params) :
    solver_(std::move(solver)),
    type_(std::move(type)),
    data_(std::move(data)),
    params_(std::move(params)) {}

  Problem(const Problem& other) :
    solver_(other.solver_),
    type_(other.type_),
    data_(other.data_),
    params_(other.params_) {}

  Problem(Problem&& other) :
    solver_(std::move(other.solver_)),
    type_(std::move(other.type_)),
    data_(std::move(other.data_)),
    params_(std::move(other.params_)) {}

  Problem& operator=(const Problem& other) {
    solver_ = other.solver_;
    type_ = other.type_;
    data_ = other.data_;
    params_ = other.params_;
    return *this;
  }

  Problem& operator=(Problem&& other) {
    solver_ = std::move(other.solver_);
    type_ = std::move(other.type_);
    data_ = std::move(other.data_);
    params_ = std::move(other.params_);
    return *this;
  }

  const std::string& solver() const { return solver_; }
  const std::string& type() const { return type_; }
  const json::Value& data() const { return data_; }
  const json::Object& params() const { return params_; }
  std::string& solver() { return solver_; }
  std::string& type() { return type_; }
  json::Value& data() { return data_; }
  json::Object& params() { return params_; }
};

class SapiService {
private:
  virtual void fetchSolversImpl(SolversSapiCallbackPtr callback) = 0;
  virtual void submitProblemsImpl(std::vector<Problem>& problems, StatusSapiCallbackPtr callback) = 0;
  virtual void multiProblemStatusImpl(const std::vector<std::string>& ids, StatusSapiCallbackPtr callback) = 0;
  virtual void fetchAnswerImpl(const std::string& id, FetchAnswerSapiCallbackPtr callback) = 0;
  virtual void cancelProblemsImpl(const std::vector<std::string>& ids, CancelSapiCallbackPtr callback) = 0;

public:
  virtual ~SapiService() {}
  void fetchSolvers(SolversSapiCallbackPtr callback) { fetchSolversImpl(callback); }
  void submitProblems(std::vector<Problem> problems, StatusSapiCallbackPtr callback) {
    submitProblemsImpl(problems, callback);
  }
  void multiProblemStatus(const std::vector<std::string>& ids, StatusSapiCallbackPtr callback) {
    multiProblemStatusImpl(ids, callback);
  }
  void fetchAnswer(const std::string& id, FetchAnswerSapiCallbackPtr callback) {
    fetchAnswerImpl(id, callback);
  }
  void cancelProblems(const std::vector<std::string>& ids, CancelSapiCallbackPtr callback) {
    cancelProblemsImpl(ids, callback);
  }
};
typedef std::shared_ptr<SapiService> SapiServicePtr;

SapiServicePtr makeSapiService(
    http::HttpServicePtr httpService,
    std::string baseUrl,
    std::string token,
    http::Proxy proxy);

} // namespace sapiremote

#endif
