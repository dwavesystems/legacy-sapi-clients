//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef ANSWER_SERVICE_HPP_INCLUDED
#define ANSWER_SERVICE_HPP_INCLUDED

#include <exception>
#include <memory>

#include "problem.hpp"
#include "json.hpp"
#include "threadpool.hpp"

namespace sapiremote {

class AnswerService {
private:
  virtual void postDoneImpl(SubmittedProblemObserverPtr observer) = 0;
  virtual void postSubmittedImpl(SubmittedProblemObserverPtr observer) = 0;
  virtual void postErrorImpl(SubmittedProblemObserverPtr observer) = 0;

  virtual void postAnswerImpl(AnswerCallbackPtr callback, std::string& type, json::Value& ans) = 0;
  virtual void postAnswerErrorImpl(AnswerCallbackPtr callback, std::exception_ptr e) = 0;

public:
  virtual ~AnswerService() {}

  void postDone(SubmittedProblemObserverPtr observer) {
    try {
      postDoneImpl(observer);
    } catch (...) {
      postError(observer);
    }
  }
  void postSubmitted(SubmittedProblemObserverPtr observer) {
    try {
      postSubmittedImpl(observer);
    } catch (...) {
      postError(observer);
    }
  }
  void postError(SubmittedProblemObserverPtr observer) {
    try {
      postErrorImpl(observer);
    } catch (...) {
      observer->notifyError(); // doesn't throw
    }
  }

  void postAnswer(AnswerCallbackPtr callback, std::string type, json::Value ans) {
    try {
      postAnswerImpl(callback, type, ans);
    } catch (...) {
      postAnswerError(callback, std::current_exception());
    }
  }
  void postAnswerError(AnswerCallbackPtr callback, std::exception_ptr e) {
    try {
      postAnswerErrorImpl(callback, e);
    } catch (...) {
      callback->error(std::current_exception()); // doesn't throw
    }
  }
};
typedef std::shared_ptr<AnswerService> AnswerServicePtr;

AnswerServicePtr makeAnswerService(ThreadPoolPtr threadPool);

} // namespace sapiremote

#endif
