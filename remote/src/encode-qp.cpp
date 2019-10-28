//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <algorithm>
#include <exception>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>

#include <exceptions.hpp>
#include <base64.hpp>
#include <json.hpp>
#include <solver.hpp>
#include <coding.hpp>

using std::make_pair;
using std::numeric_limits;
using std::pair;
using std::partition;
using std::sort;
using std::to_string;
using std::unique;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

using boost::numeric_cast;

using sapiremote::EncodingException;
using sapiremote::encodeBase64;
using sapiremote::QpProblemEntry;
using sapiremote::QpProblem;

namespace {

class BadQpInfo : public std::exception {
public:
  const char* what() throw() { return "BadQpInfo"; }
};

class UnsupportedSolverException : public EncodingException {
public:
  UnsupportedSolverException() : EncodingException("solver does not support qp encoding") {}
};

class BadQubitException : public EncodingException {
public:
  BadQubitException(long long q) : EncodingException("invalid qubit " + to_string(q)) {}
};

class BadCouplerException : public EncodingException {
public:
  BadCouplerException(long long q1, long long q2) :
      EncodingException("invalid coupler (" + to_string(q1) + "," + to_string(q2) + ")") {}
};

namespace propkeys {
const auto qubits = "qubits";
const auto couplers = "couplers";
} // namespace {anoymous}::propkeys

namespace probkeys {
const auto format = "format";
const auto lin = "lin";
const auto quad = "quad";
} // namespace {anoymous}::probkeys

typedef vector<int> QubitVector;
typedef pair<int, int> Coupler;
typedef vector<Coupler> CouplerVector;
typedef unordered_map<int, int> QubitIndexMap;
typedef unordered_map<Coupler, int, boost::hash<Coupler>> CouplerIndexMap;


QubitVector extractQubits(const json::Object& props) {
  auto propQubits = props.at(propkeys::qubits).getArray();
  QubitVector qubits;
  qubits.reserve(propQubits.size());
  BOOST_FOREACH( const auto& v, propQubits ) {
    auto q = numeric_cast<int>(v.getInteger());
    if (q < 0) throw BadQpInfo();
    qubits.push_back(q);
  }
  return qubits;
}

CouplerVector extractCouplers(const json::Object& props, const QubitIndexMap& qubits) {
  auto propCouplers = props.at(propkeys::couplers).getArray();
  CouplerVector couplers;
  couplers.reserve(propCouplers.size());
  BOOST_FOREACH( const auto& v, propCouplers ) {
    const auto& c = v.getArray();
    if (c.size() != 2) throw BadQpInfo();
    auto q1 = numeric_cast<int>(c[0].getInteger());
    auto q2 = numeric_cast<int>(c[1].getInteger());
    if (q1 == q2 || qubits.find(q1) == qubits.end() || qubits.find(q2) == qubits.end()) {
      throw BadQpInfo();
    }
    if (q2 < q1) std::swap(q1, q2);
    couplers.push_back(make_pair(q1, q2));
  }
  return couplers;
}

unordered_set<int> extractUsedQubits(const vector<QpProblemEntry>& problem, const QubitIndexMap& qubitIndices) {
  unordered_set<int> usedQubits;
  BOOST_FOREACH( const auto& e, problem ) {
    if (e.i == e.j) {
      if (qubitIndices.find(e.i) == qubitIndices.end()) throw BadQubitException(e.i);
      usedQubits.insert(e.i);
    } else {
      if (qubitIndices.find(e.i) == qubitIndices.end() || qubitIndices.find(e.j) == qubitIndices.end()) {
        throw BadCouplerException(e.i, e.j);
      }
      usedQubits.insert(e.i);
      usedQubits.insert(e.j);
    }
  }
  return usedQubits;
}

bool isLinearCoeff(const QpProblemEntry& e) {
  return e.i == e.j;
}

} // namespace {anonymous}

namespace sapiremote {

unique_ptr<QpSolverInfo> extractQpSolverInfo(const json::Object& props) {
  try {
    auto qpi = unique_ptr<QpSolverInfo>(new QpSolverInfo);

    qpi->qubits = extractQubits(props);
    qpi->qubitIndices = QubitIndexMap{};
    auto qubitsSize = qpi->qubits.size();
    for (auto i = 0u; i < qubitsSize; ++i) {
      qpi->qubitIndices[qpi->qubits[i]] = i;
    }
    qpi->couplers = extractCouplers(props, qpi->qubitIndices);

    return qpi;

  } catch (BadQpInfo&) {
  } catch (std::out_of_range&) {
  } catch (boost::bad_numeric_cast&) {
  } catch (json::TypeException&) {
  }
  return unique_ptr<QpSolverInfo>();
}

json::Value encodeQpProblem(SolverPtr solver, QpProblem problem) {
  const auto& qpi = solver->qpInfo();
  if (!qpi) throw UnsupportedSolverException();

  auto linEnd = partition(problem.begin(), problem.end(), isLinearCoeff);
  auto usedQubits = extractUsedQubits(problem, qpi->qubitIndices);
  auto lin = vector<double>(qpi->qubits.size(), numeric_limits<double>::quiet_NaN());
  BOOST_FOREACH( auto q, usedQubits ) {
    lin[qpi->qubitIndices[q]] = 0.0;
  }
  BOOST_FOREACH( const auto& e, make_pair(problem.begin(), linEnd)) {
    lin[qpi->qubitIndices[e.i]] += e.value;
  }

  auto couplerIndices = CouplerIndexMap{};
  int ci = 0;
  BOOST_FOREACH( auto c, qpi->couplers ) {
    if (usedQubits.count(c.first) > 0 && usedQubits.count(c.second) > 0) {
      couplerIndices[c] = ci;
      ++ci;
    }
  }

  auto quad = vector<double>(couplerIndices.size(), 0.0);
  BOOST_FOREACH( const auto& e, make_pair(linEnd, problem.end()) ) {
    auto c = make_pair(std::min(e.i, e.j), std::max(e.i, e.j));
    auto iter = couplerIndices.find(c);
    if (iter == couplerIndices.end()) throw BadCouplerException(e.i, e.j);
    quad[iter->second] += e.value;
  }

  return json::Object{
    {probkeys::format, "qp"},
    {probkeys::lin, encodeBase64(lin)},
    {probkeys::quad, encodeBase64(quad)}
  };
}

} // namespace sapiremote
