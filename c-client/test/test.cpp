//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <ostream>

#include <json.hpp>

using std::ostream;

namespace {
struct JsonEquals : boost::static_visitor<bool> {
  template<typename A, typename B>
  bool operator()(const A&, const B&) const { return false; }
  template<typename A>
  bool operator()(const A& a, const A& b) const { return a == b; }
};
} // namespace {anonymous}

namespace json {

bool operator==(const Value &a, const Value &b) {
  return boost::apply_visitor(JsonEquals(), a.variant(), b.variant());
}

ostream& operator<<(ostream& o, const Value& v) {
  return o << jsonToString(v);
}

} // namespace json
