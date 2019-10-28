//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef PROBLEM_HPP_INCLUDED
#define PROBLEM_HPP_INCLUDED

#include <exception>
#include <memory>
#include <string>
#include <tuple>

#include "types.hpp"
#include "json.hpp"

namespace sapiremote {

class SubmittedProblemObserver {
private:
  virtual void notifySubmittedImpl() = 0;
  virtual void notifyDoneImpl() = 0;
  virtual void notifyErrorImpl() = 0;
public:
  virtual ~SubmittedProblemObserver() {}

  void notifySubmitted() {
    try {
      notifySubmittedImpl();
    } catch (...) {
      // eat errors
    }
  }

  void notifyDone() {
    try {
      notifyDoneImpl();
    } catch (...) {
      // eat errors
    }
  }

  void notifyError() {
    try {
      notifyErrorImpl();
    } catch (...) {
      // eat errors
    }
  }
};
typedef std::shared_ptr<SubmittedProblemObserver> SubmittedProblemObserverPtr;

class AnswerCallback {
private:
  virtual void answerImpl(std::string& type, json::Value& ans) = 0;
  virtual void errorImpl(std::exception_ptr e) = 0;
public:
  virtual ~AnswerCallback() {}
  void answer(std::string type, json::Value ans) {
    try {
      answerImpl(type, ans);
    } catch (...) {
      error(std::current_exception());
    }
  }
  void error(std::exception_ptr e) {
    try {
      errorImpl(e);
    } catch (...) {
      // Ignore error.  Nowhere to send it.
    }
  }
};
typedef std::shared_ptr<AnswerCallback> AnswerCallbackPtr;

class SubmittedProblem {
private:
  virtual std::string problemIdImpl() const = 0;
  virtual bool doneImpl() const = 0;
  virtual SubmittedProblemInfo statusImpl() const = 0;
  virtual std::tuple<std::string, json::Value> answerImpl() const = 0;
  virtual void answerImpl(AnswerCallbackPtr callback) const = 0;
  virtual void cancelImpl() = 0;
  virtual void retryImpl() = 0;
  virtual void addSubmittedProblemObserverImpl(const SubmittedProblemObserverPtr& observer) = 0;
public:
  virtual ~SubmittedProblem() {}
  std::string problemId() const { return problemIdImpl(); }
  bool done() const { return doneImpl(); }
  SubmittedProblemInfo status() const { return statusImpl(); }
  std::tuple<std::string, json::Value> answer() const { return answerImpl(); }
  void answer(AnswerCallbackPtr callback) const { answerImpl(callback); }
  void cancel() { cancelImpl(); }
  void retry() { retryImpl(); }
  void addSubmittedProblemObserver(const SubmittedProblemObserverPtr& observer) {
    addSubmittedProblemObserverImpl(observer);
  }
};
typedef std::shared_ptr<SubmittedProblem> SubmittedProblemPtr;

bool awaitSubmission(const std::vector<SubmittedProblemPtr>& problems, double timeoutS);
bool awaitCompletion(const std::vector<SubmittedProblemPtr>& problems, int mindone, double timeoutS);

} // namespace sapiremote

#endif
