//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef PROBLEM_MANAGER_HPP_INCLUDED
#define PROBLEM_MANAGER_HPP_INCLUDED

#include <memory>
#include <string>
#include <stdexcept>

#include "answer-service.hpp"
#include "sapi-service.hpp"
#include "retry-service.hpp"
#include "problem.hpp"
#include "solver.hpp"

namespace sapiremote {

class ProblemManager {
private:
  virtual SubmittedProblemPtr submitProblemImpl(
      std::string& solver,
      std::string& problemType,
      json::Value& problemData,
      json::Object& problemParams) = 0;
  virtual SubmittedProblemPtr addProblemImpl(const std::string& id) = 0;
  virtual SolverMap fetchSolversImpl() = 0;

public:
  virtual ~ProblemManager() {}

  SubmittedProblemPtr submitProblem(
      std::string solver,
      std::string problemType,
      json::Value problemData,
      json::Object problemParams) {
    return submitProblemImpl(solver, problemType, problemData, problemParams);
  }

  SubmittedProblemPtr addProblem(const std::string& id) {
    return addProblemImpl(id);
  }

  SolverMap fetchSolvers() { return fetchSolversImpl(); }
};
typedef std::shared_ptr<ProblemManager> ProblemManagerPtr;

struct ProblemManagerLimits {
  int maxProblemsPerSubmission;
  int maxIdsPerStatusQuery;
  int maxActiveRequests;
};

ProblemManagerPtr makeProblemManager(
    SapiServicePtr sapiService,
    AnswerServicePtr answerService,
    RetryTimerServicePtr retryService,
    const RetryTiming& retryTiming,
    const ProblemManagerLimits& limits);

} // namespace sapiremote

#endif
