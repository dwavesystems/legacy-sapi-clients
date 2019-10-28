//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <new>
#include <queue>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/foreach.hpp>

#include <problem-manager.hpp>
#include <answer-service.hpp>
#include <sapi-service.hpp>
#include <retry-service.hpp>
#include <solver.hpp>
#include <json.hpp>
#include <exceptions.hpp>

using std::exception_ptr;
using std::current_exception;
using std::rethrow_exception;
using std::bad_alloc;
using std::shared_ptr;
using std::weak_ptr;
using std::unique_ptr;
using std::make_shared;
using std::enable_shared_from_this;
using std::make_move_iterator;
using std::deque;
using std::queue;
using std::string;
using std::vector;
using std::tuple;
using std::make_tuple;
using std::find;
using std::sort;
using std::unique;
using std::mutex;
using std::condition_variable;
using std::unique_lock;
using std::lock_guard;

using sapiremote::AnswerCallback;
using sapiremote::AnswerCallbackPtr;
using sapiremote::ProblemManagerLimits;
using sapiremote::ProblemManager;
using sapiremote::ProblemManagerPtr;
using sapiremote::Problem;
using sapiremote::SubmittedProblemPtr;
using sapiremote::SubmittedProblem;
using sapiremote::SubmittedProblemObserver;
using sapiremote::SubmittedProblemObserverPtr;
using sapiremote::AnswerServicePtr;
using sapiremote::RetryTiming;
using sapiremote::RetryTimer;
using sapiremote::RetryTimerPtr;
using sapiremote::RetryNotifiable;
using sapiremote::RetryTimerServicePtr;
using sapiremote::SolversSapiCallback;
using sapiremote::StatusSapiCallback;
using sapiremote::CancelSapiCallback;
using sapiremote::FetchAnswerSapiCallback;
using sapiremote::SolverMap;
using sapiremote::SapiServicePtr;
using sapiremote::RemoteProblemInfo;
using sapiremote::SubmittedProblemInfo;
using sapiremote::SolverInfo;
using sapiremote::Solver;
using sapiremote::SolverPtr;
using sapiremote::Error;
using sapiremote::NetworkException;
using sapiremote::CommunicationException;
using sapiremote::AuthenticationException;
using sapiremote::ProblemCancelledException;
using sapiremote::SolveException;
using sapiremote::NoAnswerException;
using sapiremote::TooManyProblemIdsException;

namespace remotestatuses = sapiremote::remotestatuses;
namespace submittedstates = sapiremote::submittedstates;
namespace errortypes = sapiremote::errortypes;

namespace {

class ProblemManagerImpl;
typedef shared_ptr<ProblemManagerImpl> ProblemManagerImplPtr;

class RetryNotifiableImpl;



//=========================================================================================================
//
// Request types
//

namespace request {
enum Type { SUBMIT, STATUS, CANCEL, ANSWER };
} // namespace {anonymous}::request



//=========================================================================================================
//
// Synchronous answer retrieval support
//

class AnswerCallbackImpl : public AnswerCallback {
private:
  mutex mutex_;
  condition_variable cv_;

  tuple<string, json::Value> result_;
  exception_ptr ex_;
  bool done_;
  bool success_;

  virtual void answerImpl(string& type, json::Value& ans) {
    lock_guard<mutex> l(mutex_);
    result_ = make_tuple(std::move(type), std::move(ans));
    done_ = true;
    success_ = true;
    cv_.notify_all();
  }

  virtual void errorImpl(exception_ptr e) {
    lock_guard<mutex> l(mutex_);
    ex_ = e;
    done_ = true;
    success_ = false;
    cv_.notify_all();
  }

public:
  AnswerCallbackImpl() : done_(false), success_(false) {}

  tuple<string, json::Value> getAnswer() {
    unique_lock<mutex> lock(mutex_);
    while (!done_) cv_.wait(lock);

    if (!success_) rethrow_exception(ex_);
    return std::move(result_);
  }
};



//=========================================================================================================
//
// Remote SubmittedProblem implementation
//

class SubmittedProblemImpl : public SubmittedProblem, public enable_shared_from_this<SubmittedProblemImpl> {
private:
  ProblemManagerImplPtr rpm_;
  AnswerServicePtr answerService_;

  mutable mutex mutex_;

  Problem problem_;

  string problemId_;
  string submittedOn_;
  string solvedOn_;
  string errorMessage_;
  submittedstates::Type state_;
  submittedstates::Type lastGoodState_;
  remotestatuses::Type remoteStatus_;
  Error error_;
  exception_ptr ex_;
  bool cancelled_;

  vector<weak_ptr<SubmittedProblemObserver>> observers_;

  vector<SubmittedProblemObserverPtr> liveObservers();

  // SubmittedProblem implementation
  virtual string problemIdImpl() const;
  virtual bool doneImpl() const;
  virtual SubmittedProblemInfo statusImpl() const;
  virtual tuple<string, json::Value> answerImpl() const;
  virtual void answerImpl(AnswerCallbackPtr callback) const;
  virtual void cancelImpl();
  virtual void retryImpl();
  virtual void addSubmittedProblemObserverImpl(const SubmittedProblemObserverPtr& observer);

public:
  SubmittedProblemImpl(ProblemManagerImplPtr rpm, AnswerServicePtr answerService, Problem problem);
  SubmittedProblemImpl(ProblemManagerImplPtr rpm, AnswerServicePtr answerService, string problemId);

  Problem problem() const;
  bool cancelled() const;
  void resetCancelled();
  void setProblemId(std::string id);
  bool updateStatus(RemoteProblemInfo rpi);
  void setError(std::exception_ptr e, bool retry);
};

typedef shared_ptr<SubmittedProblemImpl> SubmittedProblemImplPtr;
typedef weak_ptr<SubmittedProblemImpl> SubmittedProblemImplWeakPtr;
typedef vector<SubmittedProblemImplWeakPtr> SubmittedProblemImplWeakVector;



//=========================================================================================================
//
// Helper function to fail several submitted problems at once
//

template<typename Iter>
void failSubmittedProblems(Iter begin, Iter end, exception_ptr e, bool retry) {
  for (; begin != end; ++begin) {
    SubmittedProblemImplPtr lsp = begin->lock();
    if (lsp) lsp->setError(e, retry);
  }
}



//=========================================================================================================
//
// Solver List
//

class SolversCallback : public SolversSapiCallback {
private:
  vector<SolverInfo> solverInfo_;
  exception_ptr ex_;
  mutex mutex_;
  condition_variable cv_;
  bool done_;
  bool success_;

  virtual void completeImpl(vector<SolverInfo>& solverInfo) {
    lock_guard<mutex> lock(mutex_);
    solverInfo_.swap(solverInfo);
    done_ = true;
    success_ = true;
    cv_.notify_all();
  }

  virtual void errorImpl(exception_ptr e) {
    lock_guard<mutex> lock(mutex_);
    ex_ = e;
    done_ = true;
    success_ = false;
    cv_.notify_all();
  }

public:
  SolversCallback() : done_(false), success_(false) {}
  const vector<SolverInfo>& getSolvers() {
    unique_lock<mutex> lock(mutex_);
    while (!done_) cv_.wait(lock);

    if (!success_) rethrow_exception(ex_);
    return solverInfo_;
  }
};

//=========================================================================================================
//
// ProblemManagerImpl
//

class ProblemManagerImpl : public ProblemManager, public enable_shared_from_this<ProblemManagerImpl> {
private:
  friend class RetryNotifiableImpl;
  typedef tuple<string, AnswerCallbackPtr> PendingAnswerFetch;
  enum RetryState { NO_RETRY, WAITING_TO_RETRY, RETRY_NOW };

  SapiServicePtr sapiService_;
  AnswerServicePtr answerService_;

  // request management
  mutex requestMutex_;
  deque<request::Type> requestQueue_;
  int availableRequests_;
  shared_ptr<RetryNotifiableImpl> retryNotifiable_;
  RetryTimerPtr retryTimer_;
  RetryState retryState_;

  // problem submission
  mutex unsubmittedProblemsMutex_;
  deque<SubmittedProblemImplWeakPtr> unsubmittedProblems_;
  int maxProblemsPerSubmission_;

  // problem status querying
  mutex activeProblemMutex_;
  deque<SubmittedProblemImplWeakPtr> activeProblems_;
  int maxIdsPerStatusQuery_;

  // problem cancellation
  mutex cancelMutex_;
  vector<string> cancelIds_;

  // answer fetching
  mutex pendingFetchesMutex_;
  queue<PendingAnswerFetch> pendingFetches_;

  // ProblemManager implementation
  virtual SubmittedProblemPtr submitProblemImpl(
      string& solver,
      string& problemType,
      json::Value& problemData,
      json::Object& problemParams);
  virtual SubmittedProblemPtr addProblemImpl(const std::string& id);
  virtual SolverMap fetchSolversImpl();

  //------
  void retryNotification();
  void retrySubmit(const SubmittedProblemImplWeakVector& problems);
  void queueActiveProblems(const SubmittedProblemImplWeakVector& problems, bool atFront);
  void retryCancel(vector<string> ids);

  void failUnsubmittedProblems(exception_ptr e);
  void failActiveProblems(exception_ptr e);
  void failPendingFetches(exception_ptr e);

  bool checkCancelled(SubmittedProblemImplPtr problem);

  void pushRequest(request::Type r);
  void pushSubmitRequest();
  void pushStatusRequest();
  void pushCancelRequest();
  void pushAnswerRequest();
  void processRequestQueue();

  // functions that actually make SAPI requests
  // these are called by processRequestQueue
  // they must not call processRequestQueue
  // they can push into requestQueue_
  // return true iff request sent
  bool sendSubmitRequest();
  bool sendStatusRequest();
  bool sendCancelRequest();
  bool sendAnswerRequest();

  bool reduceMaxIds();
  void requestComplete();
  bool retryFailedRequest();
  void stopRetrying();

  ProblemManagerImpl(
      SapiServicePtr sapiService,
      AnswerServicePtr answerCallbackService,
      RetryTimerServicePtr retryService,
      const RetryTiming& retryTiming,
      const ProblemManagerLimits& limits);

public:
  static ProblemManagerImplPtr create(
    SapiServicePtr sapiService,
    AnswerServicePtr answerCallbackService,
    RetryTimerServicePtr retryService,
    const RetryTiming& retryTiming,
    const ProblemManagerLimits& limits);

  void statusComplete(
      const SubmittedProblemImplWeakVector& problems,
      vector<RemoteProblemInfo> problemInfo,
      bool submit);
  void statusFailed(const SubmittedProblemImplWeakVector& problem, bool submit, exception_ptr e);
  void fetchAnswerComplete(AnswerCallbackPtr callback, string type, json::Value answer);
  void fetchAnswerFailed(string problemId, AnswerCallbackPtr callback, exception_ptr e);
  void cancelComplete();
  void cancelFailed(exception_ptr e, vector<string> ids);

  void addSubmittedProblem(const SubmittedProblemImplWeakPtr& sp, submittedstates::Type state);

  void fetchAnswer(string id, AnswerCallbackPtr callback);
};



//=========================================================================================================
//
// Solver Implementation
//

class SolverImpl : public Solver {
private:
  ProblemManagerPtr problemManager_;

  virtual SubmittedProblemPtr submitProblemImpl(
        string& type,
        json::Value& problem,
        json::Object& params) const {
    return problemManager_->submitProblem(id(), std::move(type), std::move(problem), std::move(params));
  }

public:
  SolverImpl(string id, json::Object properties, ProblemManagerPtr problemManager) :
    Solver(std::move(id), std::move(properties)), problemManager_(problemManager) {}
};



//=========================================================================================================
//
// Retry notification receiver
//

class RetryNotifiableImpl : public RetryNotifiable {
private:
  ProblemManagerImpl* pm_; // raw pointer since ProblemManagerImpl owns this
  virtual void notifyImpl() { pm_->retryNotification(); }
public:
  RetryNotifiableImpl(ProblemManagerImpl* pm) : pm_(pm) {}
};



//=========================================================================================================
//
// Problem status callback from either submission or status polling
//

class StatusCallback : public StatusSapiCallback, boost::noncopyable {
private:
  ProblemManagerImplPtr rpm_;
  SubmittedProblemImplWeakVector problems_;
  bool submit_;

  virtual void completeImpl(vector<RemoteProblemInfo>& problemInfo) {
    rpm_->statusComplete(problems_, std::move(problemInfo), submit_);
  }

  virtual void errorImpl(exception_ptr e) { rpm_->statusFailed(problems_, submit_, e); }

public:
  StatusCallback(ProblemManagerImplPtr rpm, request::Type mode) :
      rpm_(rpm), submit_(mode == request::SUBMIT) {
    switch (mode) {
      case request::SUBMIT:
      case request::STATUS:
        break;
      default:
        throw std::invalid_argument("invalid StatusCallback request type");
    }
  }
  void setProblems(SubmittedProblemImplWeakVector problems) { problems_ = std::move(problems); }
};
typedef shared_ptr<StatusCallback> StatusCallbackPtr;



//=========================================================================================================
//
// Cancel problems
//

class CancelCallback : public CancelSapiCallback {
private:
  ProblemManagerImplPtr rpm_;
  vector<string> ids_;

  virtual void completeImpl() { rpm_->cancelComplete(); }
  virtual void errorImpl(exception_ptr e) { rpm_->cancelFailed(e, std::move(ids_)); }

public:
  CancelCallback(ProblemManagerImplPtr rpm) : rpm_(rpm) {}
  void setIds(vector<string> ids) { ids_ = std::move(ids); }
};



//=========================================================================================================
//
// Fetch answer
//

class FetchAnswerCallback : public FetchAnswerSapiCallback, boost::noncopyable {
private:
  ProblemManagerImplPtr rpm_;
  AnswerCallbackPtr callback_;
  string id_;

  virtual void completeImpl(string& type, json::Value& answer) {
    rpm_->fetchAnswerComplete(std::move(callback_), std::move(type), std::move(answer));
  }

  virtual void errorImpl(exception_ptr e) { rpm_->fetchAnswerFailed(std::move(id_), std::move(callback_), e); }

public:
  FetchAnswerCallback(ProblemManagerImplPtr rpm, AnswerCallbackPtr callback, string id) :
      rpm_(rpm), callback_(callback), id_(std::move(id)) {}
};



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//   S u b m i t t e d P r o b l e m I m p l   M E M B E R   F U N C T I O N S
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

vector<SubmittedProblemObserverPtr> SubmittedProblemImpl::liveObservers() {
  vector<SubmittedProblemObserverPtr> obs;
  auto replace = false;
  BOOST_FOREACH( const auto& o, observers_ ) {
    auto lo = o.lock();
    if (lo) {
      obs.push_back(lo);
    } else {
      replace = true;
    }
  }

  if (replace) observers_.assign(obs.begin(), obs.end());
  return obs;
}

string SubmittedProblemImpl::problemIdImpl() const {
  lock_guard<mutex> l(mutex_);
  return problemId_;
}

bool SubmittedProblemImpl::doneImpl() const {
  lock_guard<mutex> l(mutex_);
  return state_ == submittedstates::DONE || state_ == submittedstates::FAILED;
}

SubmittedProblemInfo SubmittedProblemImpl::statusImpl() const {
  lock_guard<mutex> l(mutex_);
  SubmittedProblemInfo p;
  p.problemId = problemId_;
  p.submittedOn = submittedOn_;
  p.solvedOn = solvedOn_;
  p.state = state_;
  p.lastGoodState = lastGoodState_;
  p.remoteStatus = remoteStatus_;
  p.error = error_;
  return p;
}

std::tuple<std::string, json::Value> SubmittedProblemImpl::answerImpl() const {
  auto callback = make_shared<AnswerCallbackImpl>();
  answer(callback);
  return callback->getAnswer();
}

void SubmittedProblemImpl::answerImpl(AnswerCallbackPtr callback) const {
  string id;
  try {
    lock_guard<mutex> l(mutex_);
    switch (state_) {
      case submittedstates::SUBMITTING:
      case submittedstates::SUBMITTED:
      case submittedstates::RETRYING:
        throw NoAnswerException();

      case submittedstates::FAILED:
        rethrow_exception(ex_);

      case submittedstates::DONE:
        if (remoteStatus_ != remotestatuses::COMPLETED) rethrow_exception(ex_);
    }

    id = problemId_;
  } catch (...) {
    answerService_->postAnswerError(callback, current_exception());
    return;
  }

  rpm_->fetchAnswer(std::move(id), callback);
}

void SubmittedProblemImpl::cancelImpl() {
  lock_guard<mutex> l(mutex_);
  cancelled_ = true;
}

void SubmittedProblemImpl::retryImpl() {
  submittedstates::Type lastGoodState;
  {
    lock_guard<mutex> l(mutex_);
    if (state_ != submittedstates::FAILED) return;
    if (lastGoodState_ == submittedstates::DONE) return;
    if (error_.type == errortypes::SOLVE) return;
    state_ = submittedstates::RETRYING;
    lastGoodState = lastGoodState_;
  }

  try {
    rpm_->addSubmittedProblem(shared_from_this(), lastGoodState);
  } catch (...) {
    setError(current_exception(), false);
  }
}

void SubmittedProblemImpl::addSubmittedProblemObserverImpl(const SubmittedProblemObserverPtr& observer) {
  bool submitted;
  bool done;
  bool success;

  {
    lock_guard<mutex> l(mutex_);
    observers_.push_back(observer);
    submitted = !problemId_.empty();
    done = state_ == submittedstates::DONE || state_ == submittedstates::FAILED;
    success = done && remoteStatus_ == remotestatuses::COMPLETED;
  }

  if (submitted) answerService_->postSubmitted(observer);
  if (done) {
    if (success) {
      answerService_->postDone(observer);
    } else {
      answerService_->postError(observer);
    }
  }
}

SubmittedProblemImpl::SubmittedProblemImpl(
    ProblemManagerImplPtr rpm,
    AnswerServicePtr answerService,
    Problem problem) :
  rpm_(rpm),
  answerService_(answerService),
  problem_(std::move(problem)),
  state_(submittedstates::SUBMITTING),
  lastGoodState_(submittedstates::SUBMITTING),
  remoteStatus_(remotestatuses::UNKNOWN),
  error_(Error{errortypes::INTERNAL, string()}),
  cancelled_(false) {}

SubmittedProblemImpl::SubmittedProblemImpl(
    ProblemManagerImplPtr rpm,
    AnswerServicePtr answerService,
    string problemId) :
  rpm_(rpm),
  answerService_(answerService),
  problemId_(problemId),
  state_(submittedstates::SUBMITTED),
  lastGoodState_(submittedstates::SUBMITTED),
  remoteStatus_(remotestatuses::UNKNOWN),
  error_(Error{errortypes::INTERNAL, string()}),
  cancelled_(false) {}

Problem SubmittedProblemImpl::problem() const {
  lock_guard<mutex> l(mutex_);
  return problem_;
}

bool SubmittedProblemImpl::cancelled() const {
  lock_guard<mutex> l(mutex_);
  return cancelled_;
}

void SubmittedProblemImpl::resetCancelled() {
  lock_guard<mutex> l(mutex_);
  cancelled_ = false;
}

void SubmittedProblemImpl::setProblemId(std::string id) {
  vector<SubmittedProblemObserverPtr> obs;
  {
    lock_guard<mutex> l(mutex_);
    problemId_ = std::move(id);
    obs = liveObservers();
  }

  BOOST_FOREACH( auto& o, obs ) {
    answerService_->postSubmitted(o);
  }
}

bool SubmittedProblemImpl::updateStatus(RemoteProblemInfo rpi) {
  try {
    vector<SubmittedProblemObserverPtr> obs;
    bool done = true;
    {
      lock_guard<mutex> l(mutex_);
      if (!rpi.submittedOn.empty()) submittedOn_ = rpi.submittedOn;
      if (!rpi.solvedOn.empty()) solvedOn_ = rpi.solvedOn;

      remoteStatus_ = rpi.status;
      switch (remoteStatus_) {
        case remotestatuses::PENDING:
        case remotestatuses::IN_PROGRESS:
          done = false;
          state_ = lastGoodState_ = submittedstates::SUBMITTED;
          break;

        case remotestatuses::COMPLETED:
          state_ = lastGoodState_ = submittedstates::DONE;
          break;

        case remotestatuses::CANCELED:
          throw ProblemCancelledException();
          break;

        case remotestatuses::FAILED:
          throw SolveException(rpi.errorMessage);
          break;

        default:
          break;
      }

      if (done) obs = liveObservers();
    }

    BOOST_FOREACH( auto& o, obs ) {
      answerService_->postDone(o);
    }

    problem_ = Problem();
    return done;

  } catch (...) {
    setError(current_exception(), false);
    return true;
  }
}

void SubmittedProblemImpl::setError(std::exception_ptr e, bool retry) {
  vector<SubmittedProblemObserverPtr> obs;
  {
    lock_guard<mutex> l(mutex_);
    ex_ = e;
    try {
      try {
        rethrow_exception(e);
      } catch (bad_alloc&) {
        throw;
      } catch (NetworkException& ex) {
        state_ = retry ? submittedstates::RETRYING : submittedstates::FAILED;
        error_.type = errortypes::NETWORK;
        error_.message = ex.what();
      } catch (CommunicationException& ex) {
        state_ = retry ? submittedstates::RETRYING : submittedstates::FAILED;
        error_.type = errortypes::PROTOCOL;
        error_.message = ex.what();
      } catch (AuthenticationException& ex) {
        state_ = retry ? submittedstates::RETRYING : submittedstates::FAILED;
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
    } catch (bad_alloc&) {
      state_ = submittedstates::FAILED;
      error_.type = errortypes::MEMORY;
      error_.message.clear();
      ex_ = current_exception();
    }

    if (!retry) obs = liveObservers();
  }

  BOOST_FOREACH( auto& o, obs ) {
    answerService_->postError(o);
  }
}



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//   P r o b l e m M a n a g e r I m p l   M E M B E R   F U N C T I O N S
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void ProblemManagerImpl::retryNotification() {
  {
    lock_guard<mutex> l(requestMutex_);
    retryState_ = RETRY_NOW;
  }
  processRequestQueue();
}

void ProblemManagerImpl::retrySubmit(const SubmittedProblemImplWeakVector& problems) {
  bool anyActive = false;
  try {
    lock_guard<mutex> lock(unsubmittedProblemsMutex_);
    unsubmittedProblems_.insert(unsubmittedProblems_.begin(), problems.begin(), problems.end());
    anyActive = !unsubmittedProblems_.empty();
  } catch (...) {
    failSubmittedProblems(problems.begin(), problems.end(), current_exception(), false);
  }

  if (anyActive) pushSubmitRequest();
  processRequestQueue();
}

void ProblemManagerImpl::retryCancel(vector<string> ids) {
  bool anyToCancel = false;
  try {
    lock_guard<mutex> lock(cancelMutex_);
    cancelIds_.insert(cancelIds_.end(), make_move_iterator(ids.begin()), make_move_iterator(ids.end()));
    anyToCancel = !cancelIds_.empty();
  } catch (...) {
    // ignore
  }

  if (anyToCancel) pushCancelRequest();
  processRequestQueue();
}

void ProblemManagerImpl::failUnsubmittedProblems(exception_ptr e) {
  deque<SubmittedProblemImplWeakPtr> upLocal;
  {
    lock_guard<mutex> lock(unsubmittedProblemsMutex_);
    upLocal = std::move(unsubmittedProblems_);
  }
  failSubmittedProblems(upLocal.begin(), upLocal.end(), e, false);
}

void ProblemManagerImpl::failActiveProblems(exception_ptr e) {
  deque<SubmittedProblemImplWeakPtr> apLocal;
  {
    lock_guard<mutex> lock(activeProblemMutex_);
    apLocal = std::move(activeProblems_);
  }
  failSubmittedProblems(apLocal.begin(), apLocal.end(), e, false);
}

void ProblemManagerImpl::failPendingFetches(exception_ptr e) {
  queue<PendingAnswerFetch> pfLocal;
  {
    lock_guard<mutex> lock(pendingFetchesMutex_);
    pfLocal = std::move(pendingFetches_);
  }
  while (!pfLocal.empty()) {
    answerService_->postAnswerError(std::get<1>(pfLocal.front()), e);
    pfLocal.pop();
  }
}

bool ProblemManagerImpl::checkCancelled(SubmittedProblemImplPtr problem) {
  try {
    if (problem->cancelled()) {
      lock_guard<mutex> l(cancelMutex_);
      cancelIds_.push_back(problem->problemId());
      problem->resetCancelled();
      return true;
    }
  } catch (...) {
    // ignore -- cancelled status not reset
  }
  return false;
}

void ProblemManagerImpl::pushRequest(request::Type r) {
  lock_guard<mutex> l(requestMutex_);
  if (find(requestQueue_.begin(), requestQueue_.end(), r) == requestQueue_.end()) {
    requestQueue_.push_back(r);
  }
}

void ProblemManagerImpl::pushSubmitRequest() {
  try {
    pushRequest(request::SUBMIT);
  } catch(...) {
    failUnsubmittedProblems(current_exception());
  }
}

void ProblemManagerImpl::pushStatusRequest() {
  try {
    pushRequest(request::STATUS);
  } catch(...) {
    failActiveProblems(current_exception());
  }
}

void ProblemManagerImpl::pushCancelRequest() {
  try {
    pushRequest(request::CANCEL);
  } catch (...) {
    // ignore
  }
}

void ProblemManagerImpl::pushAnswerRequest() {
  try {
    pushRequest(request::ANSWER);
  } catch (...) {
    failPendingFetches(current_exception());
  }
}

void ProblemManagerImpl::processRequestQueue() {
  unique_lock<mutex> lock(requestMutex_);
  if (retryState_ == WAITING_TO_RETRY) return;

  while (availableRequests_ > 0 && !requestQueue_.empty()) {
    --availableRequests_;
    auto requestType = requestQueue_.front();
    requestQueue_.pop_front();

    auto done = false;
    lock.unlock();
    switch (requestType) {
      case request::SUBMIT:
        done = sendSubmitRequest();
        break;

      case request::STATUS:
        done = sendStatusRequest();
        break;

      case request::CANCEL:
        done = sendCancelRequest();
        break;

      case request::ANSWER:
        done = sendAnswerRequest();
        break;

      default:
        break;
    }
    lock.lock();
    if (done) {
      // return to waiting state only if request actually sent
      if (retryState_ == RETRY_NOW) retryState_ = WAITING_TO_RETRY;
    } else {
      ++availableRequests_;
    }
  }
}

bool ProblemManagerImpl::sendSubmitRequest() {
  auto quota = maxProblemsPerSubmission_;
  StatusCallbackPtr callback;
  vector<Problem> problems;
  SubmittedProblemImplWeakVector submittedProblems;

  try {
    callback = make_shared<StatusCallback>(shared_from_this(), request::SUBMIT);
    problems.reserve(quota);
    submittedProblems.reserve(quota);
  } catch (...) {
    failUnsubmittedProblems(current_exception());
    return false;
  }

  // nothing throws from here on
  bool moreUnsubmitted;
  {
    lock_guard<mutex> l(unsubmittedProblemsMutex_);
    auto subEnd = unsubmittedProblems_.begin();
    const auto upEnd = unsubmittedProblems_.end();

    while (quota > 0 && subEnd != upEnd) {
      auto lsp = subEnd->lock();
      if (lsp) {
        problems.push_back(lsp->problem());
        submittedProblems.push_back(lsp);
        --quota;
      }
      ++subEnd;
    }
    unsubmittedProblems_.erase(unsubmittedProblems_.begin(), subEnd);
    moreUnsubmitted = !unsubmittedProblems_.empty();
  }

  if (moreUnsubmitted) pushSubmitRequest();

  if (submittedProblems.empty()) {
    return false;
  } else {
    callback->setProblems(std::move(submittedProblems));
    sapiService_->submitProblems(std::move(problems), callback);
    return true;
  }
}

bool ProblemManagerImpl::sendStatusRequest() {
  auto quota = maxIdsPerStatusQuery_;
  StatusCallbackPtr callback;
  vector<string> ids;
  SubmittedProblemImplWeakVector problems;

  try {
    callback = make_shared<StatusCallback>(shared_from_this(), request::STATUS);
    ids.reserve(quota);
    problems.reserve(quota);
  } catch (...) {
    failActiveProblems(current_exception());
    return false;
  }

  // nothing throws from here on
  bool moreActive;
  {
    lock_guard<mutex> l(activeProblemMutex_);
    auto apIter = activeProblems_.begin();
    const auto apEnd = activeProblems_.end();

    while (quota > 0 && apIter != apEnd) {
      auto lap = apIter->lock();
      if (lap) {
        ids.push_back(lap->problemId());
        problems.push_back(lap);
        --quota;
      }
      ++apIter;
    }
    activeProblems_.erase(activeProblems_.begin(), apIter);
    moreActive = !activeProblems_.empty();
  }

  if (moreActive) pushStatusRequest();

  if (problems.empty()) {
    return false;
  } else {
    callback->setProblems(std::move(problems));
    sapiService_->multiProblemStatus(std::move(ids), callback);
    return true;
  }
}

bool ProblemManagerImpl::sendCancelRequest() {
  try {
    auto callback = make_shared<CancelCallback>(shared_from_this());
    vector<string> ids;
    {
      lock_guard<mutex> l(cancelMutex_);
      ids = std::move(cancelIds_);
    }
    sort(ids.begin(), ids.end());
    ids.erase(unique(ids.begin(), ids.end()), ids.end());
    callback->setIds(ids);
    sapiService_->cancelProblems(ids, callback);
    return true;
  } catch (...) {
    // ignore
  }
  return false;
}

bool ProblemManagerImpl::sendAnswerRequest() {
  PendingAnswerFetch nextFetch;
  bool moreRequests;
  {
    lock_guard<mutex> l(pendingFetchesMutex_);
    if (pendingFetches_.empty()) return false;

    nextFetch = std::move(pendingFetches_.front());
    pendingFetches_.pop();
    moreRequests = !pendingFetches_.empty();
  }
  auto& id = std::get<0>(nextFetch);
  auto& answerCallback = std::get<1>(nextFetch);

  if (moreRequests) pushAnswerRequest();

  try {
    auto callback = make_shared<FetchAnswerCallback>(shared_from_this(), answerCallback, id);
    sapiService_->fetchAnswer(id, callback);
    return true;
  } catch (...) {
    answerService_->postAnswerError(answerCallback, current_exception());
    return false;
  }
}

SubmittedProblemPtr ProblemManagerImpl::submitProblemImpl(
    string& solver,
    string& type,
    json::Value& problemData,
    json::Object& params) {

  auto problem = Problem(std::move(solver), std::move(type), std::move(problemData), std::move(params));
  auto submittedProblem = make_shared<SubmittedProblemImpl>(shared_from_this(), answerService_, std::move(problem));

  {
    lock_guard<mutex> lock(unsubmittedProblemsMutex_);
    unsubmittedProblems_.push_back(submittedProblem);
  }

  pushSubmitRequest();
  processRequestQueue();
  return submittedProblem;
}

SubmittedProblemPtr ProblemManagerImpl::addProblemImpl(const string& id) {
  auto submittedProblem = make_shared<SubmittedProblemImpl>(shared_from_this(), answerService_, id);
  submittedProblem->setProblemId(id);

  {
    lock_guard<mutex> lock(activeProblemMutex_);
    activeProblems_.push_back(submittedProblem);
  }

  pushStatusRequest();
  processRequestQueue();
  return submittedProblem;
}

ProblemManagerImpl::ProblemManagerImpl(
    SapiServicePtr sapiService,
    AnswerServicePtr answerService,
    RetryTimerServicePtr retryService,
    const RetryTiming& retryTiming,
    const ProblemManagerLimits& limits) :
      sapiService_(sapiService),
      answerService_(std::move(answerService)),
      availableRequests_(limits.maxActiveRequests),
      retryNotifiable_(make_shared<RetryNotifiableImpl>(this)),
      retryTimer_(retryService->createRetryTimer(retryNotifiable_, retryTiming)),
      retryState_(NO_RETRY),
      maxProblemsPerSubmission_(limits.maxProblemsPerSubmission),
      maxIdsPerStatusQuery_(limits.maxIdsPerStatusQuery) {}

ProblemManagerImplPtr ProblemManagerImpl::create(
    SapiServicePtr sapiService,
    AnswerServicePtr answerService,
    RetryTimerServicePtr retryService,
    const RetryTiming& retryTiming,
    const ProblemManagerLimits& limits) {

#define CHECK_LIMIT(l) do { if(l < 1) throw std::invalid_argument(#l); } while (false)
  CHECK_LIMIT(limits.maxProblemsPerSubmission);
  CHECK_LIMIT(limits.maxIdsPerStatusQuery);
  CHECK_LIMIT(limits.maxActiveRequests);
#undef CHECK_LIMIT
  auto pmi = shared_ptr<ProblemManagerImpl>(
    new ProblemManagerImpl(sapiService, answerService, retryService, retryTiming, limits));
  return pmi;
}

bool ProblemManagerImpl::reduceMaxIds() {
  lock_guard<mutex> lock(activeProblemMutex_);
  if (maxIdsPerStatusQuery_ < 2) return false;
  maxIdsPerStatusQuery_ = static_cast<int>(maxIdsPerStatusQuery_ * 0.7);
  return true;
}

void ProblemManagerImpl::requestComplete() {
  {
    lock_guard<mutex> l(requestMutex_);
    ++availableRequests_;
  }
  processRequestQueue();
}

bool ProblemManagerImpl::retryFailedRequest() {
  lock_guard<mutex> l(requestMutex_);
  switch (retryTimer_->retry()) {
    case RetryTimer::RETRY:
      if (retryState_ != RETRY_NOW) retryState_ = WAITING_TO_RETRY;
      return true;

    case RetryTimer::FAIL:
      if (retryState_ != RETRY_NOW) retryState_ = WAITING_TO_RETRY;
      return false;

    default:
      retryState_ = NO_RETRY;
      return false;
  }
}

void ProblemManagerImpl::stopRetrying() {
  lock_guard<mutex> l(requestMutex_);
  if (retryState_ != NO_RETRY) {
    retryTimer_->success();
    retryState_ = NO_RETRY;
  }
}

void ProblemManagerImpl::queueActiveProblems(const SubmittedProblemImplWeakVector& problems, bool atFront) {
  bool anyCancelled = false;
  BOOST_FOREACH( auto& p, problems ) {
    auto lp = p.lock();
    if (lp && checkCancelled(lp)) anyCancelled = true;
  }
  if (anyCancelled) pushCancelRequest();

  try {
    lock_guard<mutex> lock(activeProblemMutex_);
    activeProblems_.insert(atFront ? activeProblems_.begin() : activeProblems_.end(),
        problems.begin(), problems.end());
    if (!activeProblems_.empty()) pushStatusRequest();
  } catch (...) {
    failSubmittedProblems(problems.begin(), problems.end(), current_exception(), false);
  }
}

void ProblemManagerImpl::statusComplete(
    const SubmittedProblemImplWeakVector& problems,
    vector<RemoteProblemInfo> problemInfo,
    bool submit) {

  stopRetrying();
  SubmittedProblemImplWeakVector stillActive;
  try {
    auto numProblems = problems.size();
    if (problemInfo.size() != numProblems) throw std::runtime_error("problem status size mismatch");

    stillActive.reserve(numProblems);

    for (size_t i = 0; i < numProblems; ++i) {
      auto lp = problems[i].lock();
      if (lp) {
        if (submit) lp->setProblemId(std::move(problemInfo[i].id));
        if (!lp->updateStatus(std::move(problemInfo[i]))) stillActive.push_back(lp);
      }
    }

  } catch (...) {
    statusFailed(problems, submit, current_exception());
  }

  if (!stillActive.empty()) {
    queueActiveProblems(stillActive, false);
  }
  requestComplete();
}

void ProblemManagerImpl::statusFailed(const SubmittedProblemImplWeakVector& problems, bool submit, exception_ptr e) {

  auto nonNetworkFailure = true;
  try {
    rethrow_exception(e);

  } catch (NetworkException&) {
    nonNetworkFailure = false;
    auto retry = retryFailedRequest();
    failSubmittedProblems(problems.begin(), problems.end(), e, retry);
    if (retry) {
      if (submit) {
        retrySubmit(problems);
      } else {
        queueActiveProblems(problems, true);
      }
    }

  } catch (TooManyProblemIdsException&) {
    if (reduceMaxIds()) {
      queueActiveProblems(problems, true);
    } else {
      failSubmittedProblems(problems.begin(), problems.end(), e, false);
    }

  } catch (...) {
    failSubmittedProblems(problems.begin(), problems.end(), e, false);
  }

  if (nonNetworkFailure) stopRetrying();
  requestComplete();
}

void ProblemManagerImpl::fetchAnswerComplete(AnswerCallbackPtr callback, string type, json::Value answer) {
  stopRetrying();
  answerService_->postAnswer(callback, std::move(type), std::move(answer));
  requestComplete();
}

void ProblemManagerImpl::fetchAnswerFailed(string problemId, AnswerCallbackPtr callback, exception_ptr e) {
  try {
    rethrow_exception(e);

  } catch (NetworkException&) {
    auto retry = retryFailedRequest();
    if (retry) {
      fetchAnswer(std::move(problemId), callback);
    } else {
      answerService_->postAnswerError(callback, e);
    }

  } catch (...) {
    stopRetrying();
    answerService_->postAnswerError(callback, e);
  }

  requestComplete();
}

void ProblemManagerImpl::cancelComplete() {
  stopRetrying();
  requestComplete();
}

void ProblemManagerImpl::cancelFailed(exception_ptr e, vector<string> ids) {
  try {
    rethrow_exception(e);

  } catch (NetworkException&) {
    auto retry = retryFailedRequest();
    if (retry) retryCancel(std::move(ids));

  } catch (...) {
    stopRetrying();
  }

  requestComplete();
}

void ProblemManagerImpl::addSubmittedProblem(const SubmittedProblemImplWeakPtr& sp, submittedstates::Type state) {
  if (state == submittedstates::SUBMITTING) {
    {
      lock_guard<mutex> l(unsubmittedProblemsMutex_);
      unsubmittedProblems_.push_back(sp);
    }
    pushSubmitRequest();
  } else {
    {
      lock_guard<mutex> l(activeProblemMutex_);
      activeProblems_.push_back(sp);
    }
    pushStatusRequest();
  }
  processRequestQueue();
}


void ProblemManagerImpl::fetchAnswer(string id, AnswerCallbackPtr callback) {
  {
    lock_guard<mutex> lock(pendingFetchesMutex_);
    pendingFetches_.push(make_tuple(std::move(id), callback));
  }

  pushAnswerRequest();
  processRequestQueue();
}

SolverMap ProblemManagerImpl::fetchSolversImpl() {
  auto cb = make_shared<SolversCallback>();
  sapiService_->fetchSolvers(cb);
  auto solverInfos = cb->getSolvers();

  SolverMap solvers;
  BOOST_FOREACH( const auto& solverInfo, solverInfos ) {
    solvers[solverInfo.id] = make_shared<SolverImpl>(
        solverInfo.id, std::move(solverInfo.properties), shared_from_this());
  }

  return solvers;
}

} // namespace {anonymous}



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//   P U B L I C   I N T E R F A C E   I M P L E M E N T A T I O N
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

namespace sapiremote {
ProblemManagerPtr makeProblemManager(
    SapiServicePtr sapiService,
    AnswerServicePtr answerService,
    RetryTimerServicePtr retryService,
    const RetryTiming& retryTiming,
    const ProblemManagerLimits& limits) {

  return ProblemManagerImpl::create(sapiService, answerService, retryService, retryTiming, limits);
}

} // namespace sapiremote
