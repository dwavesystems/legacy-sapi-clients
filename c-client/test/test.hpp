//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef SAPI_C_CLIENT_TEST_HPP
#define SAPI_C_CLIENT_TEST_HPP

#include <ostream>

#include <dwave_sapi.h>

inline bool operator==(const sapi_Coupler& a, const sapi_Coupler& b) {
  return a.q1 == b.q1 && a.q2 == b.q2;
}

inline bool operator==(const sapi_ProblemEntry& a, const sapi_ProblemEntry& b) {
  return a.i == b.i && a.j == b.j && a.value == b.value;
}

namespace json {
bool operator==(const Value& a, const Value& b);
inline bool operator!=(const Value& a, const Value& b) { return !(a == b); }
std::ostream& operator<<(std::ostream& o, const Value& v);
} // namespace json

class GlobalState {
public:
  GlobalState() { if (sapi_globalInit() != SAPI_OK) throw std::runtime_error("sapi_globalInit failed"); }
  ~GlobalState() { sapi_globalCleanup(); }
};

#endif
