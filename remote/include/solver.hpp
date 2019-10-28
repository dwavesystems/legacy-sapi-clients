//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef SOLVER_HPP_INCLUDED
#define SOLVER_HPP_INCLUDED

#include <memory>
#include <string>
#include <utility>

#include "problem.hpp"
#include "types.hpp"
#include "json.hpp"
#include "coding.hpp"

namespace sapiremote {

class Solver {
private:
  const std::string id_;
  const json::Object properties_;
  const std::unique_ptr<QpSolverInfo> qpInfo_;

  virtual SubmittedProblemPtr submitProblemImpl(
      std::string& type,
      json::Value& problem,
      json::Object& params) const = 0;
public:
  Solver(std::string id, json::Object properties) :
      id_(std::move(id)),
      properties_(std::move(properties)),
      qpInfo_(extractQpSolverInfo(properties_)) {}
  virtual ~Solver() {}
  const std::string& id() const { return id_; }
  const json::Object& properties() const { return properties_; }
  const std::unique_ptr<QpSolverInfo>& qpInfo() const { return qpInfo_; }
  SubmittedProblemPtr submitProblem(
      std::string type,
      json::Value problem,
      json::Object params) const {
    return submitProblemImpl(type, problem, params);
  }
};

} // namespace sapiremote

#endif
