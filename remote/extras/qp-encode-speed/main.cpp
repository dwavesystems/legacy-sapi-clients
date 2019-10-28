//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <streambuf>
#include <chrono>

#include <boost/foreach.hpp>

#include <json.hpp>
#include <coding.hpp>
#include <solver.hpp>

using std::cout;
using std::ifstream;
using std::istreambuf_iterator;
using std::make_shared;
using std::string;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

using sapiremote::QpProblem;
using sapiremote::QpProblemEntry;

class DummySolver : public sapiremote::Solver {
  virtual sapiremote::SubmittedProblemPtr submitProblemImpl(std::string&, json::Value&, json::Object&) const {
    throw std::runtime_error("not implemented");
  }
public:
  DummySolver(json::Object props) : Solver("dummy", std::move(props)) {}
};

int main(int argc, char* argv[]) {

  for (auto i = 1; i < argc; ++i) {
    ifstream file(argv[i]);
    auto s = string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
    auto solver = make_shared<DummySolver>(json::stringToJson(s).getObject());
    {
      auto t0 = system_clock::now();
      for (auto j = 0; j < 1000; ++j) {
        auto v = sapiremote::extractQpSolverInfo(solver->properties());
      }
      auto t1 = system_clock::now();
      cout << argv[i] << " (extract info): " << duration_cast<milliseconds>(t1 - t0).count() << "\n";
    }
    {
      const auto& v = solver->qpInfo();
      auto p = QpProblem{};
      BOOST_FOREACH( auto q, v->qubits ) {
        p.push_back(QpProblemEntry{q, q, -1.0});
      }
      BOOST_FOREACH( auto c, v->couplers ) {
        p.push_back(QpProblemEntry{c.first, c.second, -1.0});
      }

      auto t0 = system_clock::now();
      for (auto j = 0; j < 1000; ++j) {
        auto e = sapiremote::encodeQpProblem(solver, p);
      }
      auto t1 = system_clock::now();
      cout << argv[i] << " (encode): " << duration_cast<milliseconds>(t1 - t0).count() << "\n";

    }
  }
  return 0;
}
