//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cstring>
#include <exception>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>

#include <problem.hpp>

#include <dwave_sapi.h>
#include <sapi-impl.hpp>

using std::current_exception;
using std::numeric_limits;
using std::string;
using std::vector;

using sapi::handleException;
using sapi::SolverMap;

namespace remotestatuses = sapiremote::remotestatuses;
namespace submittedstates = sapiremote::submittedstates;
namespace errortypes = sapiremote::errortypes;

namespace {

vector<const char*> extractSolverNames(const SolverMap& solvers) {
  auto names = vector<const char*>();
  BOOST_FOREACH( const auto& e, solvers ) {
    names.push_back(e.first.c_str());
  }
  names.push_back(0);
  return names;
}

} // namespace {anonymous}

sapi_Connection::sapi_Connection(SolverMap solvers) :
    solvers_(std::move(solvers)), solverNames_(extractSolverNames(solvers_)) {}

sapi_Solver* sapi_Connection::getSolver(const string& solverName) const {
  auto iter = solvers_.find(solverName);
  if (iter == solvers_.end()) {
    return 0;
  } else {
    return iter->second.get();
  }
}


DWAVE_SAPI const char** sapi_listSolvers(const sapi_Connection* connection) {
  return const_cast<const char**>(connection->solverNames());
}


DWAVE_SAPI sapi_Solver* sapi_getSolver(const sapi_Connection* connection, const char* solverName) {
  return connection->getSolver(solverName);
}


DWAVE_SAPI const sapi_SolverProperties* sapi_getSolverProperties(const sapi_Solver* solver) {
  return solver->properties();
}


DWAVE_SAPI sapi_Code sapi_solveIsing(
    const sapi_Solver* solver,
    const sapi_Problem* problem,
    const sapi_SolverParameters* params,
    sapi_IsingResult** result,
    char* err_msg) {

  try {
    *result = solver->solve(SAPI_PROBLEM_TYPE_ISING, problem, params).release();
    return SAPI_OK;

  } catch (...) {
    return handleException(current_exception(), err_msg);
  }
}


DWAVE_SAPI sapi_Code sapi_solveQubo(
    const sapi_Solver* solver,
    const sapi_Problem* problem,
    const sapi_SolverParameters* params,
    sapi_IsingResult** result,
    char* err_msg) {

  try {
    *result = solver->solve(SAPI_PROBLEM_TYPE_QUBO, problem, params).release();
    return SAPI_OK;

  } catch (...) {
    return handleException(current_exception(), err_msg);
  }
}


DWAVE_SAPI sapi_Code sapi_asyncSolveIsing(
    const sapi_Solver* solver,
    const sapi_Problem* problem,
    const sapi_SolverParameters* params,
    sapi_SubmittedProblem** submittedProblem,
    char* err_msg) {

  try {
    *submittedProblem = solver->submit(SAPI_PROBLEM_TYPE_ISING, problem, params).release();
    return SAPI_OK;

  } catch (...) {
    return handleException(current_exception(), err_msg);
  }
}


DWAVE_SAPI sapi_Code sapi_asyncSolveQubo(
    const sapi_Solver* solver,
    const sapi_Problem* problem,
    const sapi_SolverParameters* params,
    sapi_SubmittedProblem** submittedProblem,
    char* err_msg) {

  try {
    auto sp = solver->submit(SAPI_PROBLEM_TYPE_QUBO, problem, params);
    *submittedProblem = sp.release();
    return SAPI_OK;

  } catch (...) {
    return handleException(current_exception(), err_msg);
  }
}


DWAVE_SAPI int sapi_awaitCompletion(
    const sapi_SubmittedProblem** submittedProblems,
    size_t numSubmittedProblems,
    size_t minDone0,
    double timeout) {

  try {
    if (minDone0 > numSubmittedProblems) minDone0 = numSubmittedProblems;
    if (minDone0 > numeric_limits<int>::max()) minDone0 = numeric_limits<int>::max();
    auto minDone = static_cast<int>(minDone0);

    auto remoteProblems = vector<sapiremote::SubmittedProblemPtr>();
    for (size_t i = 0; i < numSubmittedProblems; ++i) {
      auto rp = submittedProblems[i]->remoteSubmittedProblem();
      if (rp) {
        remoteProblems.push_back(rp);
      } else {
        --minDone;
      }
    }

    if (remoteProblems.empty() || minDone <= 0) return true;
    return sapiremote::awaitCompletion(remoteProblems, minDone, timeout);

  } catch (...) {
    return 0;
  }
}


DWAVE_SAPI void sapi_cancelSubmittedProblem(sapi_SubmittedProblem* submittedProblem) {
  submittedProblem->cancel();
}


DWAVE_SAPI int sapi_asyncDone(const sapi_SubmittedProblem* submittedProblem) {
  return submittedProblem->done();
}


DWAVE_SAPI sapi_Code sapi_asyncResult(
    const sapi_SubmittedProblem* submittedProblem,
    sapi_IsingResult** result,
    char* err_msg) {

  try {
    *result = submittedProblem->result().release();
    return SAPI_OK;
  } catch (...) {
    return handleException(current_exception(), err_msg);
  }
}


DWAVE_SAPI void sapi_asyncRetry(const sapi_SubmittedProblem* submittedProblem) {
  try {
    auto rsp = submittedProblem->remoteSubmittedProblem();
    if (rsp) rsp->retry();
  } catch (...) {}
}


DWAVE_SAPI sapi_Code sapi_asyncStatus(const sapi_SubmittedProblem* submitted_problem, sapi_ProblemStatus *status) {
  try {
    auto rsp = submitted_problem->remoteSubmittedProblem();
    if (!rsp) {
      std::memset(status->problem_id, 0, sizeof(status->problem_id));
      std::memset(status->time_received, 0, sizeof(status->time_received));
      std::memset(status->time_solved, 0, sizeof(status->time_solved));
      status->state = SAPI_STATE_DONE;
      status->last_good_state = SAPI_STATE_DONE;
      status->remote_status = SAPI_STATUS_COMPLETED;
      status->error_code = SAPI_OK;
      std::memset(status->error_message, 0, sizeof(status->error_message));
      return SAPI_OK;
    }

    auto rstatus = rsp->status();
    std::memset(status->problem_id, 0, sizeof(status->problem_id));
    std::strncpy(status->problem_id, rstatus.problemId.c_str(), sizeof(status->problem_id) - 1);
    std::memset(status->time_received, 0, sizeof(status->time_received));
    std::strncpy(status->time_received, rstatus.submittedOn.c_str(), sizeof(status->time_received) - 1);
    std::memset(status->time_solved, 0, sizeof(status->time_solved));
    std::strncpy(status->time_solved, rstatus.solvedOn.c_str(), sizeof(status->time_solved) - 1);

    switch (rstatus.state) {
      case submittedstates::SUBMITTING: status->state = SAPI_STATE_SUBMITTING; break;
      case submittedstates::SUBMITTED: status->state = SAPI_STATE_SUBMITTED; break;
      case submittedstates::DONE: status->state = SAPI_STATE_DONE; break;
      case submittedstates::RETRYING: status->state = SAPI_STATE_RETRYING; break;
      default: status->state = SAPI_STATE_FAILED; break;
    }

    switch (rstatus.lastGoodState) {
      case submittedstates::SUBMITTED: status->last_good_state = SAPI_STATE_SUBMITTED; break;
      case submittedstates::DONE: status->last_good_state = SAPI_STATE_DONE; break;
      default: status->last_good_state = SAPI_STATE_SUBMITTING; break;
    }

    switch (rstatus.remoteStatus) {
      case remotestatuses::PENDING: status->remote_status = SAPI_STATUS_PENDING; break;
      case remotestatuses::IN_PROGRESS: status->remote_status = SAPI_STATUS_IN_PROGRESS; break;
      case remotestatuses::COMPLETED: status->remote_status = SAPI_STATUS_COMPLETED; break;
      case remotestatuses::FAILED: status->remote_status = SAPI_STATUS_FAILED; break;
      case remotestatuses::CANCELED: status->remote_status = SAPI_STATUS_CANCELED; break;
      default: status->remote_status = SAPI_STATUS_UNKNOWN;
    }

    std::memset(status->error_message, 0, sizeof(status->error_message));
    status->error_code = SAPI_OK;
    if (rstatus.state == submittedstates::FAILED || rstatus.state == submittedstates::RETRYING
        || (rstatus.state == submittedstates::DONE && rstatus.remoteStatus != remotestatuses::COMPLETED)) {
      switch (rstatus.error.type) {
        case errortypes::AUTH: status->error_code = SAPI_ERR_AUTHENTICATION; break;
        case errortypes::MEMORY: status->error_code = SAPI_ERR_OUT_OF_MEMORY; break;
        case errortypes::NETWORK: status->error_code = SAPI_ERR_NETWORK; break;
        case errortypes::PROTOCOL: status->error_code = SAPI_ERR_COMMUNICATION; break;
        case errortypes::SOLVE: status->error_code = SAPI_ERR_SOLVE_FAILED; break;
        default: status->error_code = SAPI_ERR_SOLVE_FAILED; break;
      }

      std::strncpy(status->error_message, rstatus.error.message.c_str(), sizeof(status->error_message) - 1);
    }

    return SAPI_OK;

  } catch (...) {
    // assume it's out of memory
    return SAPI_ERR_OUT_OF_MEMORY;
  }
}
