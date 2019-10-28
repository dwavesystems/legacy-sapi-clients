//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <answer-service.hpp>
#include <threadpool.hpp>
#include <problem.hpp>
#include <json.hpp>

using std::bind;
using std::exception_ptr;
using std::make_shared;
using std::string;

using sapiremote::ThreadPoolPtr;
using sapiremote::AnswerService;
using sapiremote::AnswerCallback;
using sapiremote::AnswerCallbackPtr;
using sapiremote::SubmittedProblemObserver;
using sapiremote::SubmittedProblemObserverPtr;

namespace {

class AnswerServiceImpl : public AnswerService {
private:
  ThreadPoolPtr threadPool_;

  virtual void postDoneImpl(SubmittedProblemObserverPtr observer) {
    threadPool_->post(bind(&SubmittedProblemObserver::notifyDone, observer));
  }

  virtual void postSubmittedImpl(SubmittedProblemObserverPtr observer) {
    threadPool_->post(bind(&SubmittedProblemObserver::notifySubmitted, observer));
  }

  virtual void postErrorImpl(SubmittedProblemObserverPtr observer) {
    threadPool_->post(bind(&SubmittedProblemObserver::notifyError, observer));
  }

  virtual void postAnswerImpl(AnswerCallbackPtr callback, string& type, json::Value& ans) {
    threadPool_->post(bind(&AnswerCallback::answer, callback, std::move(type), std::move(ans)));
  }

  virtual void postAnswerErrorImpl(AnswerCallbackPtr callback, std::exception_ptr e) {
    threadPool_->post(bind(&AnswerCallback::error, callback, e));
  }

public:
  AnswerServiceImpl(ThreadPoolPtr threadPool) : threadPool_(threadPool) {}
};

} // namespace {anonymous}

namespace sapiremote {

AnswerServicePtr makeAnswerService(ThreadPoolPtr threadPool) {
  return make_shared<AnswerServiceImpl>(threadPool);
}

} // namespace sapiremote
