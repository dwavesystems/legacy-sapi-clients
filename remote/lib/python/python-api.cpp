//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>

#include <problem.hpp>
#include <coding.hpp>

#include "python-api.hpp"

using std::map;
using std::string;
using std::tuple;
using std::vector;

using sapiremote::ProblemManagerPtr;
using sapiremote::QpProblem;
using sapiremote::QpAnswer;
using sapiremote::awaitCompletion;
using sapiremote::encodeQpProblem;
using sapiremote::decodeQpAnswer;

map<string, Solver> fetchSolvers(ProblemManagerPtr problemManager) {
  auto solvers = problemManager->fetchSolvers();
  return map<string, Solver>(solvers.begin(), solvers.end());
}

bool await_completion(const vector<SubmittedProblem>& problems, int min_done, double timeout) {
  vector<sapiremote::SubmittedProblemPtr> sps;
  sps.reserve(problems.size());
  BOOST_FOREACH(const auto& p, problems) {
    sps.push_back(p.sp());
  }
  return awaitCompletion(sps, min_done, timeout);
}

json::Value encode_qp_problem(const Solver& solver, QpProblem& problem) {
  return encodeQpProblem(solver.solver(), std::move(problem));
}

tuple<json::Object, QpAnswer> decode_qp_answer(const string& problemType, json::Object& answer) {
  auto qpAnswer = decodeQpAnswer(problemType, answer);
  return make_tuple(std::move(answer), std::move(qpAnswer));
}
