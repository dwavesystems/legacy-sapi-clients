//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef PYTHON_API_HPP_INCLUDED
#define PYTHON_API_HPP_INCLUDED

#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <problem-manager.hpp>
#include <solver.hpp>
#include <coding.hpp>
#include <json.hpp>

#include "internals.hpp"

class SubmittedProblem {
private:
  sapiremote::SubmittedProblemPtr sp_;

public:
  SubmittedProblem(sapiremote::SubmittedProblemPtr sp) : sp_(sp) {}

  sapiremote::SubmittedProblemPtr sp() const { return sp_; }

  void cancel() { sp_->cancel(); }
  void retry() { sp_->retry(); }
  std::string problem_id() { return sp_->problemId(); }
  sapiremote::SubmittedProblemInfo status() { return sp_->status(); }
  std::tuple<std::string, json::Value> answer() { return sp_->answer(); }
  bool done() { return sp_->done(); }
};

class Solver {
  sapiremote::SolverPtr solver_;

public:
  Solver() {}
  Solver(sapiremote::SolverPtr solver) : solver_(solver) {}
  sapiremote::SolverPtr solver() const { return solver_; }
  const json::Object& properties() const { return solver_->properties(); }

  SubmittedProblem submit(std::string& type, json::Value& problem, json::Object& params) {
    return solver_->submitProblem(std::move(type), std::move(problem), std::move(params));
  }
};

class Connection {
private:
  sapiremote::ProblemManagerPtr problemManager_;
  std::map<std::string, Solver> solvers_;

public:
  Connection(std::string url, std::string token, sapiremote::http::Proxy proxy = sapiremote::http::Proxy()) :
      problemManager_(createProblemManager(url, token, proxy)),
      solvers_(fetchSolvers(problemManager_)) {}
  const std::map<std::string, Solver>& solvers() const { return solvers_; }
  SubmittedProblem add_problem(std::string& id) { return problemManager_->addProblem(std::move(id)); }
};

bool await_completion(const std::vector<SubmittedProblem>& problems, int min_done, double timeout);

json::Value encode_qp_problem(const Solver& solver, sapiremote::QpProblem& problem);
std::tuple<json::Object, sapiremote::QpAnswer> decode_qp_answer(
    const std::string& problemType, json::Object& answer);

#endif
