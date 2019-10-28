//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef SAPI_IMPL_HPP_INCLUDED
#define SAPI_IMPL_HPP_INCLUDED

#include <memory>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>

#include <problem.hpp>

#include "dwave_sapi.h"
#include "internal.hpp"


struct sapi_SubmittedProblem {
private:
  virtual sapiremote::SubmittedProblemPtr remoteSubmittedProblemImpl() const = 0;
  virtual void cancelImpl() = 0;
  virtual bool doneImpl() const = 0;
  virtual sapi::IsingResultPtr resultImpl() const = 0;

public:
  virtual ~sapi_SubmittedProblem() {}

  sapiremote::SubmittedProblemPtr remoteSubmittedProblem() const { return remoteSubmittedProblemImpl(); }
  void cancel() { cancelImpl(); }
  bool done() const { return doneImpl(); }
  sapi::IsingResultPtr result() const { return resultImpl(); }
};


struct sapi_Solver {
private:
  virtual const sapi_SolverProperties* propertiesImpl() const = 0;
  virtual sapi::IsingResultPtr solveImpl(
      sapi_ProblemType type, const sapi_Problem *problem,
      const sapi_SolverParameters *params) const  = 0;
  virtual sapi::SubmittedProblemPtr submitImpl(
      sapi_ProblemType type, const sapi_Problem *problem,
      const sapi_SolverParameters *params) const = 0;

public:
  virtual ~sapi_Solver() {}

  const sapi_SolverProperties* properties() const { return propertiesImpl(); }

  sapi::IsingResultPtr solve(
      sapi_ProblemType type,
      const sapi_Problem *problem,
      const sapi_SolverParameters *params) const {
    return solveImpl(type, problem, params);
  }

  sapi::SubmittedProblemPtr submit(
      sapi_ProblemType type,
      const sapi_Problem *problem,
      const sapi_SolverParameters *params) const {
    return submitImpl(type, problem, params);
  }
};


struct sapi_Connection : boost::noncopyable {
private:
  const sapi::SolverMap solvers_;
  const std::vector<const char*> solverNames_;

public:
  sapi_Connection(sapi::SolverMap solvers);
  virtual ~sapi_Connection() {}

  sapi_Solver* getSolver(const std::string& solverName) const;
  const char* const* solverNames() const { return solverNames_.data(); }
};

#endif
