//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <http-service.hpp>
#include <sapi-service.hpp>
#include <retry-service.hpp>
#include <problem-manager.hpp>
#include <json.hpp>

namespace sapiremote {

extern char const * const userAgent = "sapi-remote/spam";

} // namespace sapiremote

using std::cerr;
using std::cout;
using std::string;
using std::vector;

using sapiremote::ProblemManagerLimits;
using sapiremote::ProblemManagerPtr;
using sapiremote::makeProblemManager;
using sapiremote::makeSapiService;
using sapiremote::AnswerServicePtr;
using sapiremote::makeAnswerService;
using sapiremote::makeThreadPool;
using sapiremote::http::HttpServicePtr;
using sapiremote::http::makeHttpService;
using sapiremote::http::Proxy;
using sapiremote::SolverPtr;
using sapiremote::QpProblem;
using sapiremote::QpProblemEntry;
using sapiremote::extractQpSolverInfo;
using sapiremote::encodeQpProblem;
using sapiremote::SubmittedProblemPtr;
using sapiremote::awaitCompletion;
using sapiremote::defaultRetryTiming;
using sapiremote::RetryTiming;
using sapiremote::makeRetryTimerService;

const ProblemManagerLimits limits = {
    20,  // maxProblemsPerSubmission
    100, // maxIdsPerStatusQuery
    6    // maxActiveRequests
};

const RetryTiming retryTiming = { 1, 16, 2.0f };

ProblemManagerPtr createProblemManager(string url, string token) {
  auto sapiService = makeSapiService(makeHttpService(2), std::move(url), std::move(token), Proxy{});
  auto answerService = makeAnswerService(makeThreadPool(2));
  auto retryService = makeRetryTimerService();
  return makeProblemManager(sapiService, answerService, retryService, retryTiming, limits);
}

json::Value createProblem(const SolverPtr& solver) {
  auto qpSolverInfo = extractQpSolverInfo(solver->properties());
  auto problem = QpProblem{};
  for (auto it = qpSolverInfo->qubits.begin(); it != qpSolverInfo->qubits.end(); ++it) {
    problem.push_back(QpProblemEntry{*it, *it, -1.0});
  }
  return encodeQpProblem(solver, problem);
}

int main(int argc, char* argv[]) {

  if (argc < 3) {
    cerr << "Missing URL and token arguments\n";
    return 1;
  }
  auto problemManager = createProblemManager(argv[1], argv[2]);

  auto solvers = problemManager->fetchSolvers();
  if (argc < 4) {
    for (auto it = solvers.begin(); it != solvers.end(); ++it) {
      cout << it->first << "\n";
    }
    return 0;
  }

  auto sit = solvers.find(argv[3]);
  if (sit == solvers.end()) {
    cerr << "No such solver\n";
    return 1;
  }

  auto solver = sit->second;
  auto problem = createProblem(solver);
  auto params = json::Object{{"num_reads", 1000}};

  auto submittedProblems = vector<SubmittedProblemPtr>();
  for (auto i = 0; i < 100; ++i) {
    submittedProblems.push_back(solver->submitProblem("ising", problem, params));
  }


  auto completedProblems = vector<SubmittedProblemPtr>();
  auto remainingProblems = vector<SubmittedProblemPtr>();
  while (!submittedProblems.empty()) {
    awaitCompletion(submittedProblems, submittedProblems.size(), 10.0);
    completedProblems.clear();
    for (auto it = submittedProblems.begin(); it != submittedProblems.end(); ++it) {
      if ((*it)->done()) {
        completedProblems.push_back(*it);
      } else {
        remainingProblems.push_back(*it);
      }
    }
    cout << (100.0 * completedProblems.size() / submittedProblems.size()) << "%\n";
    submittedProblems = std::move(remainingProblems);
    for (auto it = completedProblems.begin(); it != completedProblems.end(); ++it) {
      try {
        (*it)->answer();
        cout << ".";
      } catch (std::exception& e) {
        cout << "\n" << e.what() << "\n";
      }
    }
    cout << "\n";
  }


  return 0;
}
