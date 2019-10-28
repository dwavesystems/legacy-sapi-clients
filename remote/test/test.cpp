//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <string>

#include <sapi-service.hpp>
#include <json.hpp>

#include "test.hpp"

using std::string;

using sapiremote::SolverInfo;
using sapiremote::RemoteProblemInfo;

SolverInfo makeSolverInfo(string id, json::Object properties) {
  SolverInfo solverInfo;
  solverInfo.id = std::move(id);
  solverInfo.properties = std::move(properties);
  return solverInfo;
}

RemoteProblemInfo makeProblemInfo(string id, string type, sapiremote::remotestatuses::Type status) {
  RemoteProblemInfo pi;
  pi.id = std::move(id);
  pi.type = std::move(type);
  pi.status = status;
  return pi;
}

RemoteProblemInfo makeFailedProblemInfo(string id, string type, string errMsg) {
  RemoteProblemInfo pi;
  pi.id = std::move(id);
  pi.type = std::move(type);
  pi.status = sapiremote::remotestatuses::FAILED;
  pi.errorMessage = std::move(errMsg);
  return pi;
}

RemoteProblemInfo makeProblemInfo(
  string id, string type, sapiremote::remotestatuses::Type status, string submittedOn) {

  auto pi = makeProblemInfo(id, type, status);
  pi.submittedOn = submittedOn;
  return pi;
}

RemoteProblemInfo makeProblemInfo(
  string id, string type, sapiremote::remotestatuses::Type status, string submittedOn, string solvedOn) {

  auto pi = makeProblemInfo(id, type, status, submittedOn);
  pi.solvedOn = solvedOn;
  return pi;
}

namespace sapiremote {
extern char const * const userAgent = "sapi-remote/test";

bool operator==(const SolverInfo& a, const SolverInfo& b) {
  return a.id == b.id && a.properties == b.properties;
}

bool operator==(const RemoteProblemInfo& a, const RemoteProblemInfo& b) {
  return a.id == b.id && a.status == b.status && a.type == b.type && a.submittedOn == b.submittedOn
      && a.solvedOn == b.solvedOn;
}

bool operator==(const Problem& a, const Problem& b) {
  return a.solver() == b.solver()
      && a.type() == b.type()
      && a.data() == b.data()
      && a.params() == b.params();
}

void PrintTo(const RemoteProblemInfo& pi, std::ostream* os) {
  *os << "RemoteProblemInfo(id=" << pi.id << ", type=" << pi.type << ", status=" << pi.status
      << ", submittedOn=" << pi.submittedOn << ", solvedOn=" << pi.solvedOn << ")";
}
} // namespace sapiremote

namespace {
struct JsonEquals : boost::static_visitor<bool> {
  template<typename A, typename B>
  bool operator()(const A&, const B&) const { return false; }
  template<typename A>
  bool operator()(const A& a, const A& b) const { return a == b; }
};
} // namespace {anonymous}

namespace json {

bool operator==(const Value& a, const Value& b) {
  return boost::apply_visitor(JsonEquals(), a.variant(), b.variant());
}

void PrintTo(const Value& a, std::ostream* out) {
  *out << jsonToString(a);
}

} // namespace json
