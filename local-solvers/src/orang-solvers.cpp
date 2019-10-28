//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <algorithm>
#include <chrono>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <utility>

#include <iostream>

#include <boost/random.hpp>
#include <boost/iterator/indirect_iterator.hpp>

#include <orang/orang.h>

#include <sapi-local/sapi-local.hpp>

using std::binary_search;
using std::make_pair;
using std::map;
using std::max;
using std::min;
using std::numeric_limits;
using std::ostringstream;
using std::pair;
using std::string;
using std::vector;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

using boost::make_indirect_iterator;

using orang::Var;
using orang::VarVector;
using orang::DomIndexVector;

using sapilocal::ProblemType;
using sapilocal::MatrixEntry;
using sapilocal::SparseMatrix;
using sapilocal::OrangStructure;
using sapilocal::OrangSampleParams;
using sapilocal::PROBLEM_ISING;
using sapilocal::PROBLEM_QUBO;
using sapilocal::UnsupportedProblemTypeException;
using sapilocal::InvalidParameterException;

namespace {

class Str {
private:
  ostringstream sstr_;
public:
  template<typename T>
  Str& operator<<(const T& t) { sstr_ << t; return *this; }
  operator string() const { return sstr_.str(); }
};

class InvalidVariableException : public sapilocal::InvalidProblemException {
public:
  InvalidVariableException(int v) : InvalidProblemException(Str() << "Invalid variable: " << v) {}
};

class InvalidVariablePairException : public sapilocal::InvalidProblemException {
public:
  InvalidVariablePairException(int v1, int v2) :
    InvalidProblemException(Str() << "Invalid variable pair: (" << v1 << "," << v2 << ")") {}
};

class InvalidConfiguration : public InvalidParameterException {
public:
  InvalidConfiguration(const string& msg) : InvalidParameterException("Invalid configuration: " + msg) {}
};

typedef orang::Table<double> TableType;
typedef TableType::smartptr TablePtr;
typedef vector<TablePtr> TablePtrVector;

class Rng {
private:
  boost::mt19937 engine_;
  boost::uniform_01<double> dist_;
public:
  Rng(int seed) : engine_(seed) {}
  double operator()() { return dist_(engine_); }
};

typedef orang::Task<orang::LogSumProductOperations<Rng> > SampleTask;
typedef orang::Task<orang::MinOperations<double, orang::Plus<double> > > OptimizeTask;

const signed char isingDomain[2] = { -1, 1 };
const signed char quboDomain[2] = { 0, 1 };

struct OrangProblem {
  TablePtrVector tables;
  VarVector usedVariables;
};

struct StateAndEnergy {
  const DomIndexVector* state;
  double energy;
};

StateAndEnergy makeStateAndEnergy(const DomIndexVector* state, double energy) {
  StateAndEnergy se = {state, energy};
  return se;
}

struct SaeComp {
  bool operator()(const StateAndEnergy& a, const StateAndEnergy& b) const {
    return a.energy < b.energy || a.energy == b.energy && *a.state < *b.state;
  }
};

unsigned int randomSeed(const OrangSampleParams& params) {
  if (params.useSeed) {
    return params.randomSeed;
  } else {
    return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
  }
}

void validateNumReads(int numReads) {
  if (numReads < 0) throw InvalidParameterException("number of reads must be non-negative");
}

void validateMaxAnswers(int maxAnswers) {
  if (maxAnswers < 0) throw InvalidParameterException("maximum number of answers must be non-negative");
}

void validateBeta(double beta) {
  if (!(beta >= 0.0 && beta < numeric_limits<double>::infinity())) {
    throw InvalidParameterException("beta must be finite and non-negative");
  }
}

OrangStructure validateAndNormalizeStructure(OrangStructure s) {
  if (s.numVars < 0) throw InvalidConfiguration("number of variables must be non-negative");

  sort(s.activeVars.begin(), s.activeVars.end());
  s.activeVars.erase(unique(s.activeVars.begin(), s.activeVars.end()), s.activeVars.end());
  if (!s.activeVars.empty()) {
    if (s.activeVars.front() < 0) throw InvalidConfiguration("active variable indices must be non-negative");
    if (s.activeVars.back() >= s.numVars) {
      throw InvalidConfiguration("active variable indices must be less than the number of variables");
    }
  }

  BOOST_FOREACH( auto& p, s.activeVarPairs ) {
    if (p.first == p.second) throw InvalidConfiguration("active variable pairs must be distinct");
    if (!binary_search(s.activeVars.begin(), s.activeVars.end(), p.first)
        || !binary_search(s.activeVars.begin(), s.activeVars.end(), p.first)) {
      throw InvalidConfiguration("active variables pairs must consist of active variables");
    }
    if (p.first > p.second) std::swap(p.first, p.second);
  }
  sort(s.activeVarPairs.begin(), s.activeVarPairs.end());
  s.activeVarPairs.erase(unique(s.activeVarPairs.begin(), s.activeVarPairs.end()), s.activeVarPairs.end());

  auto sortedVarOrder = s.varOrder;
  sort(sortedVarOrder.begin(), sortedVarOrder.end());
  if (sortedVarOrder != s.activeVars) {
    throw InvalidConfiguration("variable elimination order must consist precisely of active variables");
  }

  return s;
}

void validateVar(int v, const vector<int>& vars) {
  if (!binary_search(vars.begin(), vars.end(), v)) throw InvalidVariableException(v);
}

void validateVarPair(int v1, int v2, const vector<pair<int, int> >& varPairs) {
  if (!binary_search(varPairs.begin(), varPairs.end(), make_pair(min(v1, v2), max(v1, v2)))) {
    throw InvalidVariablePairException(v1, v2);
  }
}

TablePtr createTable1(Var qi, double v, bool ising) {
  static const DomIndexVector domSizes1(1, 2);
  VarVector vars1(1, qi);
  TablePtr tablePtr( new TableType(vars1, domSizes1) );

  if (ising) {
    (*tablePtr)[0] = -v;
    (*tablePtr)[1] = v;
  } else {
    (*tablePtr)[0] = 0;
    (*tablePtr)[1] = v;
  }

  return tablePtr;
}

TablePtr createTable2(Var qi, Var qj, double v, bool ising) {
  static const DomIndexVector domSizes2(2, 2);
  VarVector vars2(2);
  vars2[0] = min(qi, qj);
  vars2[1] = max(qi, qj);
  TablePtr tablePtr( new TableType(vars2, domSizes2) );

  if (ising) {
    (*tablePtr)[0] = v;
    (*tablePtr)[1] = -v;
    (*tablePtr)[2] = -v;
    (*tablePtr)[3] = v;
  } else {
    (*tablePtr)[0] = 0;
    (*tablePtr)[1] = 0;
    (*tablePtr)[2] = 0;
    (*tablePtr)[3] = v;
  }

  return tablePtr;
}

OrangProblem convertProblem(
    ProblemType problemType,
    SparseMatrix problem,
    const OrangStructure& s,
    double beta = -1.0) {

  bool ising;
  switch (problemType) {
    case PROBLEM_ISING: ising = true; break;
    case PROBLEM_QUBO: ising = false; break;
    default: throw UnsupportedProblemTypeException();
  }

  OrangProblem p;
  p.tables.reserve(problem.size());
  BOOST_FOREACH( const MatrixEntry& e, problem ) {
    if (e.i == e.j) {
      validateVar(e.i, s.activeVars);
      p.tables.push_back(createTable1(e.i, -beta * e.value, ising));
      p.usedVariables.push_back(e.i);
    } else {
      validateVarPair(e.i, e.j, s.activeVarPairs);
      p.tables.push_back(createTable2(e.i, e.j, -beta * e.value, ising));
      p.usedVariables.push_back(e.i);
      p.usedVariables.push_back(e.j);
    }
  }

  sort(p.usedVariables.begin(), p.usedVariables.end());
  p.usedVariables.erase(unique(p.usedVariables.begin(), p.usedVariables.end()), p.usedVariables.end());
  return p;
}

double calculateSampleEnergy(const TablePtrVector& tables, const DomIndexVector& state, double beta) {
  double e = 0.0;
  BOOST_FOREACH( const TablePtr& tp, tables ) {
    size_t ti = 0;
    BOOST_FOREACH( const orang::TableVar& var, tp->vars() ) {
      ti += var.stepSize * state[var.index];
    }
    e += (*tp)[ti];
  }
  return -e / beta;
}

} // namespace {anonymous}



namespace sapilocal {

IsingResult orangSample(ProblemType problemType, const SparseMatrix& problem, const OrangSampleParams& params) {
  validateNumReads(params.numReads);
  validateMaxAnswers(params.maxAnswers);
  validateBeta(params.beta);
  auto os = validateAndNormalizeStructure(params.s);
  auto op = convertProblem(problemType, problem, os, params.beta);
  VarVector varOrder(os.varOrder.begin(), os.varOrder.end());

  Rng rng(randomSeed(params));

  SampleTask task(make_indirect_iterator(op.tables.begin()), make_indirect_iterator(op.tables.end()),
    rng, os.numVars);
  orang::TreeDecomp decomp(task.graph(), varOrder, task.domSizes());
  orang::BucketTree<SampleTask> bucketTree(task, decomp, DomIndexVector(os.numVars), true, false);

  IsingResult result;
  auto domain = problemType == PROBLEM_ISING ? isingDomain : quboDomain;

  if (params.answerHistogram) {
    typedef map<DomIndexVector, int> StateCount;
    StateCount stateCount;
    for (unsigned int i = 0; i < params.numReads; ++i) {
      ++stateCount[bucketTree.solve()];
    }

    vector<StateAndEnergy> stateEnergies;
    stateEnergies.reserve(stateCount.size());
    BOOST_FOREACH( const StateCount::value_type& sp, stateCount ) {
      stateEnergies.push_back(
        makeStateAndEnergy(&sp.first, calculateSampleEnergy(op.tables, sp.first, params.beta)));
    }
    sort(stateEnergies.begin(), stateEnergies.end(), SaeComp());
    if (stateEnergies.size() > static_cast<size_t>(params.maxAnswers)) stateEnergies.resize(params.maxAnswers);
    size_t numSolutions = stateEnergies.size();

    result.solutions.assign(numSolutions * os.numVars, IsingResult::unusedVariable);
    result.energies.reserve(numSolutions);
    result.numOccurrences.reserve(numSolutions);
    size_t sBase = 0;

    BOOST_FOREACH( const StateAndEnergy& sae, stateEnergies ) {
      result.energies.push_back(sae.energy);
      result.numOccurrences.push_back(stateCount[*sae.state]);
      BOOST_FOREACH( Var v, op.usedVariables ) {
        result.solutions[sBase + v] = domain[(*sae.state)[v]];
      }
      sBase += os.numVars;
    }

  } else {
    auto numSamples = min(params.numReads, params.maxAnswers);
    result.solutions.assign(numSamples * os.numVars, IsingResult::unusedVariable);
    result.energies.reserve(numSamples);
    size_t sBase = 0;

    for (auto i = 0; i < numSamples; ++i) {
      DomIndexVector state = bucketTree.solve();
      result.energies.push_back(calculateSampleEnergy(op.tables, state, params.beta));
      BOOST_FOREACH( Var v, op.usedVariables ) {
        result.solutions[sBase + v] = domain[state[v]];
      }
      sBase += os.numVars;
    }
  }

  return result;
}

IsingResult orangOptimize(ProblemType problemType, const SparseMatrix& problem, const OrangOptimizeParams& params) {
  try {
    validateNumReads(params.numReads);
    validateMaxAnswers(params.maxAnswers);
    auto os = validateAndNormalizeStructure(params.s);
    auto op = convertProblem(problemType, problem, os);
    VarVector varOrder(os.varOrder.begin(), os.varOrder.end());

    auto maxSolutions = min(params.numReads, params.maxAnswers);
    if (maxSolutions == 0) return IsingResult();

    OptimizeTask task(make_indirect_iterator(op.tables.begin()), make_indirect_iterator(op.tables.end()),
      maxSolutions, os.numVars);
    orang::TreeDecomp decomp(task.graph(), varOrder, task.domSizes());
    orang::BucketTree<OptimizeTask> bucketTree(task, decomp, DomIndexVector(os.numVars), true, false);

    auto sol = bucketTree.solve();
    const auto& solutions = sol.solutions();
    auto numSolutions = solutions.size();

    IsingResult result;
    result.solutions.assign(numSolutions * os.numVars, IsingResult::unusedVariable);
    result.energies.reserve(numSolutions);
    size_t sBase = 0;
    auto domain = problemType == PROBLEM_ISING ? isingDomain : quboDomain;

    BOOST_FOREACH( const auto& s, solutions ) {
      result.energies.push_back(bucketTree.problemValue() + s.value);
      BOOST_FOREACH( auto v, os.activeVars ) {
        result.solutions[sBase + v] = domain[s.solution[v]];
      }
      sBase += os.numVars;
    }

    if (params.answerHistogram) {
      result.numOccurrences.assign(numSolutions, 1);
      if (params.numReads > numSolutions) {
        result.numOccurrences.front() = params.numReads - numSolutions + 1;
      }
    }

    return result;
  } catch (orang::Exception& e) {
    throw SolveException(string("Internal error: ") + e.what());
  }
}

} // namespace sapilocal
