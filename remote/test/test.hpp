//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef TEST_TEST_HPP_INCLUDED
#define TEST_TEST_HPP_INCLUDED

#include <exception>
#include <string>
#include <ostream>

#include <gmock/gmock.h>

#include <sapi-service.hpp>
#include <json.hpp>

template <typename E>
class ExceptionPtrMatcher : public testing::MatcherInterface<std::exception_ptr> {
private:
  std::string typeName_;

public:
  ExceptionPtrMatcher(std::string typeName) : typeName_(typeName) {}

  virtual bool MatchAndExplain(std::exception_ptr ep, testing::MatchResultListener* listener) const {
    try {
      std::rethrow_exception(ep);
    } catch (E&) {
      return true;
    } catch (std::exception&) {
    }
    return false;
  }

  virtual void DescribeTo(std::ostream* os) const { *os << "is a thrown " << typeName_; }
  virtual void DescribeNegationTo(std::ostream* os) const { *os << "is not a thrown " << typeName_; }
};

template<typename Ex>
testing::Matcher<std::exception_ptr> Thrown0(const char* typeName) {
  return testing::Matcher<std::exception_ptr>(new ExceptionPtrMatcher<Ex>(typeName));
}

#define Thrown(ExType) Thrown0<ExType>(#ExType)

sapiremote::SolverInfo makeSolverInfo(std::string id, json::Object properties);
sapiremote::RemoteProblemInfo makeProblemInfo(
  std::string id, std::string type, sapiremote::remotestatuses::Type status);
sapiremote::RemoteProblemInfo makeProblemInfo(
  std::string id, std::string type, sapiremote::remotestatuses::Type status,
  std::string submittedOn);
sapiremote::RemoteProblemInfo makeProblemInfo(
  std::string id, std::string type, sapiremote::remotestatuses::Type status,
  std::string submittedOn, std::string solvedOn);
sapiremote::RemoteProblemInfo makeFailedProblemInfo(std::string id, std::string type, std::string errMsg);

namespace sapiremote {
bool operator==(const SolverInfo& a, const SolverInfo& b);
bool operator==(const RemoteProblemInfo& a, const RemoteProblemInfo& b);
bool operator==(const Problem& a, const Problem& b);
void PrintTo(const RemoteProblemInfo& pi, std::ostream* os);
} // namespace sapiremote

namespace json {
bool operator==(const Value& a, const Value& b);
inline bool operator!=(const Value& a, const Value& b) { return !(a == b); }
void PrintTo(const Value& a, std::ostream* out);
} // namespace json

#endif
