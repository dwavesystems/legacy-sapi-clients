//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef TYPES_HPP_INCLUDED
#define TYPES_HPP_INCLUDED

#include <memory>
#include <string>
#include <unordered_map>
#include "json.hpp"

namespace sapiremote {

namespace remotestatuses {
enum Type { PENDING, IN_PROGRESS, COMPLETED, FAILED, CANCELED, UNKNOWN };
} // namespace sapiremote::remotestatus

namespace submittedstates {
enum Type { SUBMITTING, SUBMITTED, DONE, RETRYING, FAILED };
} // namespace sapiremote::submittedstate

namespace errortypes {
enum Type { NETWORK, PROTOCOL, AUTH, SOLVE, MEMORY, INTERNAL };
} // namespace sapiremote::errortypes

struct Error {
  errortypes::Type type;
  std::string message;
};

struct RemoteProblemInfo {
  remotestatuses::Type status;
  std::string id;
  std::string submittedOn;
  std::string solvedOn;
  std::string type;
  std::string errorMessage;
};

struct SolverInfo {
  std::string id;
  json::Object properties;
};

struct SubmittedProblemInfo {
  std::string problemId;
  std::string submittedOn;
  std::string solvedOn;
  submittedstates::Type state;
  submittedstates::Type lastGoodState; // last non-FAILED/RETRYING state when state == FAILED || state == RETRYING
  remotestatuses::Type remoteStatus;
  Error error; // only valid when state == FAILED || state == RETRYING || (state == DONE && remoteStatus != COMPLETED)
};

class Solver;
typedef std::shared_ptr<Solver> SolverPtr;
typedef std::unordered_map<std::string, SolverPtr> SolverMap;

template<typename T>
std::string toString(T t);

} // namespace sapiremote

#endif
