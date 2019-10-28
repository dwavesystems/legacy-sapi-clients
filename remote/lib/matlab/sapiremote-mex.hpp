//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef SAPIREMOTE_MEX_HPP_INCLUDED
#define SAPIREMOTE_MEX_HPP_INCLUDED

#include <mex.h>

#include <map>
#include <string>
#include <vector>

#include <problem-manager.hpp>
#include <json.hpp>

#include "mxfreedeleter.hpp"
#include "json-to-matlab.hpp"

extern const char* mexName;

namespace err_id {
namespace internal {
extern const char* numArgs;
extern const char* numOut;
extern const char* argType;
} // namespace err_id::internal

extern const char* argType;
extern const char* badHandle;
extern const char* badConnection;
extern const char* badProblem;
extern const char* solveError;
extern const char* asyncNotDone;
extern const char* authError;
extern const char* networkError;
} // namespace err_id

namespace subfunctions {
void done(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
void answer(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
void awaitSubmission(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
void awaitCompletion(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
void cancel(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
void retry(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
void problemId(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
void status(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
void solvers(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
void encodeQp(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
void submitProblem(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
void addProblem(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
void decodeQp(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
} // namespace subfunctions

void failBadHandle();

struct ConnectionInfo {
  std::string url;
  std::string token;
  sapiremote::http::Proxy proxy;
};

class Connection {
private:
  sapiremote::ProblemManagerPtr problemManager_;
  mutable sapiremote::SolverMap solvers_;
  mutable bool haveSolvers_;

public:
  Connection(sapiremote::ProblemManagerPtr problemManager) : problemManager_(problemManager), haveSolvers_(false) {}

  const sapiremote::SolverMap& solvers() const {
    if (!haveSolvers_) {
      solvers_ = problemManager_->fetchSolvers();
      haveSolvers_ = true;
    }
    return solvers_;
  }

  sapiremote::SolverPtr solver(const std::string& id) const {
    const auto& s = solvers();
    auto iter = s.find(id);
    if (iter == s.end()) failBadHandle();
    return iter->second;
  }

  void fetchSolvers() {
    solvers_ = problemManager_->fetchSolvers();
    haveSolvers_ = true;
  }

  const sapiremote::ProblemManagerPtr& problemManager() const { return problemManager_; }
};
typedef std::shared_ptr<Connection> ConnectionPtr;

ConnectionPtr getConnection(const mxArray* handle);
sapiremote::ProblemManagerPtr makeProblemManager(ConnectionInfo conninfo);
sapiremote::SolverPtr getSolver(const mxArray* handle);

std::string uniqueId();

void exitFn();

#endif
