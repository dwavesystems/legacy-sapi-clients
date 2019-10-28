//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef REMOTE_HPP_INCLUDED
#define REMOTE_HPP_INCLUDED

#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <json.hpp>

#include <problem.hpp>
#include <problem-manager.hpp>
#include <solver.hpp>

#include "sapi-impl.hpp"
#include "internal.hpp"

namespace sapi {

IsingResultPtr decodeRemoteIsingResult(const std::tuple<std::string, json::Value>& result);


class RemoteSupportedProblemTypesProperty {
private:
  bool valid_;
  std::vector<std::string> probTypes_;
  std::vector<const char*> probTypesCStr_;
  sapi_SupportedProblemTypeProperty prop_;
public:
  RemoteSupportedProblemTypesProperty(const json::Object& jsonProps);
  const sapi_SupportedProblemTypeProperty* prop() const { return valid_ ? &prop_ : 0; }
};


class RemoteQuantumSolverProperties {
private:
  bool valid_;
  std::vector<int> qubits_;
  std::vector<sapi_Coupler> couplers_;
  sapi_QuantumSolverProperties props_;
public:
  RemoteQuantumSolverProperties(const json::Object& jsonProps);
  const sapi_QuantumSolverProperties* props() const { return valid_ ? &props_ : 0; }
};


class RemoteIsingRangeProperties {
private:
  bool valid_;
  sapi_IsingRangeProperties props_;
public:
  RemoteIsingRangeProperties(const json::Object& jsonProps);
  const sapi_IsingRangeProperties* props() const { return valid_ ? &props_ : 0; }
};


class RemoteAnnealOffsetProperties {
private:
  bool valid_;
  std::vector<sapi_AnnealOffsetRange> ranges_;
  sapi_AnnealOffsetProperties props_;
public:
  RemoteAnnealOffsetProperties(const json::Object& jsonProps);
  const sapi_AnnealOffsetProperties* props() const { return valid_ ? &props_ : 0; }
};


class RemoteAnnealScheduleProperties {
private:
  bool valid_;
  sapi_AnnealScheduleProperties props_;
public:
  RemoteAnnealScheduleProperties(const json::Object& jsonProps);
  const sapi_AnnealScheduleProperties* props() const { return valid_ ? &props_ : 0; }
};


class RemoteParametersProperty {
private:
  bool valid_;
  std::vector<std::string> params_;
  std::vector<const char*> paramsCStr_;
  sapi_ParametersProperty prop_;
public:
  RemoteParametersProperty(const json::Object& jsonProps);
  const sapi_ParametersProperty* prop() const { return valid_ ? &prop_ : 0; }
};


class RemoteVirtualGraphProperties {
private:
  bool valid_;
  std::vector<std::string> params_;
  std::vector<const char*> paramsCStr_;
  sapi_VirtualGraphProperties props_;
public:
  RemoteVirtualGraphProperties(const json::Object& jsonProps);
  const sapi_VirtualGraphProperties* prop() const { return valid_ ? &props_ : 0; }
};


class RemoteSolverProperties {
private:
  const RemoteSupportedProblemTypesProperty sptProp_;
  const RemoteQuantumSolverProperties qsProps_;
  const RemoteIsingRangeProperties irProps_;
  const RemoteAnnealOffsetProperties aoProps_;
  const RemoteAnnealScheduleProperties asProps_;
  const RemoteParametersProperty pProp_;
  const RemoteVirtualGraphProperties vgProps_;
  const sapi_SolverProperties props_;

public:
  RemoteSolverProperties(const json::Object& jsonProps) :
      sptProp_(RemoteSupportedProblemTypesProperty(jsonProps)),
      qsProps_(RemoteQuantumSolverProperties(jsonProps)),
      irProps_(RemoteIsingRangeProperties(jsonProps)),
      aoProps_(RemoteAnnealOffsetProperties(jsonProps)),
      asProps_(RemoteAnnealScheduleProperties(jsonProps)),
      pProp_(RemoteParametersProperty(jsonProps)),
      vgProps_(RemoteVirtualGraphProperties(jsonProps)),
      props_(sapi_SolverProperties{sptProp_.prop(), qsProps_.props(), irProps_.props(), aoProps_.props(),
        asProps_.props(), pProp_.prop(), vgProps_.prop()}) {}

  const sapi_SolverProperties* props() const { return &props_; }
};


class RemoteParameterValidator {
private:
  std::unordered_set<std::string> validParamNames_;
  bool hasValidParamNames_;

public:
  void operator()(const json::Object& params) const;
  RemoteParameterValidator(const json::Object& solverProps);
};


class RemoteSubmittedProblem : public sapi_SubmittedProblem {
private:
  sapiremote::SubmittedProblemPtr rsp_;

  virtual sapiremote::SubmittedProblemPtr remoteSubmittedProblemImpl() const { return rsp_; }
  virtual void cancelImpl() { rsp_->cancel(); }
  virtual bool doneImpl() const { return rsp_->done(); }
  virtual IsingResultPtr resultImpl() const { return decodeRemoteIsingResult(rsp_->answer()); }

public:
  RemoteSubmittedProblem(const sapiremote::SubmittedProblemPtr& rsp) : rsp_(rsp) {}
};


class RemoteSolver : public sapi_Solver {
private:
  sapiremote::SolverPtr rsolver_;
  RemoteSolverProperties props_;
  RemoteParameterValidator validateParamNames_;

  virtual const sapi_SolverProperties* propertiesImpl() const { return props_.props(); }
  virtual IsingResultPtr solveImpl(
      sapi_ProblemType type, const sapi_Problem *problem,
      const sapi_SolverParameters *params) const;
  virtual SubmittedProblemPtr submitImpl(
      sapi_ProblemType type, const sapi_Problem *problem,
      const sapi_SolverParameters *params) const;

public:
  RemoteSolver(const sapiremote::SolverPtr& rsolver) :
    rsolver_(rsolver), props_(rsolver_->properties()), validateParamNames_(rsolver->properties()) {}
};


class RemoteConnection : public sapi_Connection {
private:
  sapiremote::ProblemManagerPtr problemManager_;

public:
  RemoteConnection(const sapiremote::ProblemManagerPtr& problemManager);
};


IsingResultPtr decodeRemoteIsingResult(const std::tuple<std::string, json::Value>& result);
json::Object quantumParametersToJson(const sapi_QuantumSolverParameters& params);

} // namespace sapi

#endif
