//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cmath>
#include <cstddef>
#include <ctime>
#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/date_time.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/random.hpp>

#include <orang/orang.h>

#include <sapi-local/sapi-local.hpp>

using std::size_t;
using std::abs;
using std::max;
using std::min;
using std::numeric_limits;
using std::string;
using std::time;
using std::vector;

using boost::bind;
using boost::make_indirect_iterator;
using boost::posix_time::ptime;
using boost::posix_time::microsec_clock;
using boost::posix_time::time_duration;
using boost::posix_time::milliseconds;
using boost::posix_time::pos_infin;

using orang::Var;
using orang::VarVector;
using orang::DomIndex;
using orang::DomIndexVector;
using orang::Table;
using orang::TableVar;
using orang::TreeDecomp;
using orang::BucketTree;
using orang::Task;
using orang::MinOperations;
using orang::Plus;
using orang::greedyVarOrder;
using orang::greedyvarorder::MIN_FILL;

using sapilocal::IsingResult;
using sapilocal::MatrixEntry;
using sapilocal::SparseMatrix;

using sapilocal::InvalidParameterException;

namespace {

typedef boost::uniform_01<boost::mt19937> Rng;

typedef Table<double> TableType;
typedef TableType::const_smartptr TablePtr;
typedef Task<MinOperations<double, Plus<double> > > TaskType;

const double eps = 1e-12;

struct Params {
  int iterationLimit;
  ptime endTime;
  int maxComplexity;
  int noProgressLimit;
  int numPerturbedCopies;
  double minBitFlipProb;
  double maxBitFlipProb;
};

struct LocalSearchParams {
  int maxComplexity;
  int noProgressLimit;
  ptime endTime;
};

struct LocalSearchResult {
  double energy;
  DomIndexVector solution;
  bool exact;
};

bool timedOut(const ptime& endTime) { return microsec_clock::local_time() > endTime; }

bool noticablyLess(double x, double y) { return y - x > abs(y) * eps; }

ptime computeEndTime(double timeLimitSeconds) {
  ptime endTime = microsec_clock::local_time();
  if (timeLimitSeconds == numeric_limits<double>::infinity()) {
    endTime += time_duration(pos_infin);
  } else {
    endTime += milliseconds(static_cast<boost::uint64_t>(timeLimitSeconds * 1000));
  }
  return endTime;
}

TableType createHTable(Var i, double v) {
  static const DomIndexVector domSizes(1, 2);
  TableType t(VarVector(1, i), domSizes);
  t[0] = -v;
  t[1] = v;
  return t;
}

TableType createJTable(Var i, Var j, double v) {
  static const DomIndexVector domSizes(2, 2);
  VarVector scope(2, 0);
  scope[0] = min(i, j);
  scope[1] = max(i, j);
  TableType t(scope, domSizes);
  t[0] = v;
  t[1] = -v;
  t[2] = -v;
  t[3] = v;
  return t;
}

TaskType buildProblem(const SparseMatrix& problemMatrix, Var minVars) {
  vector<TableType> tables;
  tables.reserve(problemMatrix.size());

  BOOST_FOREACH( const MatrixEntry& e, problemMatrix ) {
    Var i = e.i;
    Var j = e.j;
    if (i == j) {
      tables.push_back(createHTable(i, e.value));
    } else {
      tables.push_back(createJTable(i, j, e.value));
    }
  }

  return TaskType(tables.begin(), tables.end(), 1, minVars);
}

double calculateEnergy(const TaskType& problem, const DomIndexVector& solution) {
  double energy = 0.0;
  BOOST_FOREACH( const TablePtr& tp, problem.tables() ) {
    size_t index = 0;
    BOOST_FOREACH( const TableVar& var, tp->vars() ) {
      index += solution[var.index] * var.stepSize;
    }
    energy += (*tp)[index];
  }
  return energy;
}

IsingResult convertLocalSearchResult(const TaskType& problem, const LocalSearchResult& lsResult) {
  IsingResult result;
  result.energies.assign(1, lsResult.energy);

  Var numVars = problem.numVars();
  result.solutions.assign(numVars, IsingResult::unusedVariable);
  for (Var n = 0; n < numVars; ++n) {
    if (problem.domSize(n) > 1) result.solutions[n] = lsResult.solution[n] == 1 ? 1 : -1;
  }

  return result;
}

LocalSearchResult localSearch(
    const TaskType& problem,
    const LocalSearchParams& params,
    DomIndexVector initSolution,
    Rng& rng) {
  LocalSearchResult result;
  result.solution = initSolution;
  result.energy = calculateEnergy(problem, result.solution);
  result.exact = false;

  vector<int> clampRanks(problem.numVars(), 0);
  int noProgress = 0;

  while (!timedOut(params.endTime) && noProgress <= params.noProgressLimit && !result.exact) {
    VarVector varOrder = greedyVarOrder(problem, params.maxComplexity, clampRanks, MIN_FILL, rng);
    TreeDecomp decomp(problem.graph(), varOrder, problem.domSizes());
    BucketTree<TaskType> bucketTree(problem, decomp, result.solution, true, false);
    DomIndexVector newSolution = bucketTree.solve().solutions().begin()->solution;

    if (varOrder.size() == problem.numVars()) {
      result.solution.swap(newSolution);
      result.energy = bucketTree.problemValue();
      result.exact = true;
    } else {
      BOOST_FOREACH( int& c, clampRanks ) {
        ++c;
      }
      BOOST_FOREACH( DomIndex v, varOrder ) {
        --clampRanks[v];
      }

      double newEnergy = calculateEnergy(problem, newSolution);
      if (noticablyLess(newEnergy, result.energy)) {
        result.energy = newEnergy;
        result.solution.swap(newSolution);
        noProgress = 0;
      } else {
        ++noProgress;
      }
    }
  }

  return result;
}

LocalSearchResult isingHeuristic(const TaskType& problem, const Params& params, Rng& rng) {
  LocalSearchParams lsp;
  lsp.endTime = params.endTime;
  lsp.maxComplexity = params.maxComplexity;
  lsp.noProgressLimit = params.noProgressLimit;

  DomIndexVector initSolution;
  size_t numVars = problem.numVars();
  initSolution.reserve(numVars);
  for (size_t n = 0; n < numVars; ++n) initSolution.push_back(rng() < 0.5 ? 0 : 1);

  LocalSearchResult r = localSearch(problem, lsp, initSolution, rng);
  LocalSearchResult best;
  best.solution = r.solution;
  best.energy = r.energy;
  best.exact = r.exact;

  vector<double> bitflipProbs;
  bitflipProbs.reserve(params.numPerturbedCopies);
  bitflipProbs.push_back(params.minBitFlipProb);
  if (params.numPerturbedCopies > 1) {
    double delta = (params.maxBitFlipProb - params.minBitFlipProb) / (params.numPerturbedCopies - 1);
    for (int n = 1; n < params.numPerturbedCopies; ++n) {
      bitflipProbs.push_back(params.minBitFlipProb + n * delta);
    }
  }

  DomIndexVector iterBestSolution = best.solution;
  double iterBestEnergy = best.energy;

  for (int iter = 0;
      iter < params.iterationLimit && !timedOut(params.endTime) && !best.exact;
      ++iter) {

    initSolution.swap(iterBestSolution);
    iterBestEnergy = numeric_limits<double>::infinity();

    for (int c = 0; c < params.numPerturbedCopies && !timedOut(params.endTime) && !best.exact; ++c) {
      for (size_t n = 0; n < numVars; ++n) {
        r.solution[n] = rng() < bitflipProbs[c] ? 1 - initSolution[n] : initSolution[n];
      }

      r = localSearch(problem, lsp, r.solution, rng);

      best.exact = r.exact;
      if (noticablyLess(r.energy, iterBestEnergy)) {
        iterBestEnergy = r.energy;
        iterBestSolution.swap(r.solution);
      }
    }

    if (noticablyLess(iterBestEnergy, best.energy)) {
      best.energy = iterBestEnergy;
      best.solution = iterBestSolution;
    }
  }

  return best;
}


SparseMatrix qubo_to_ising(const SparseMatrix& quboProblem, double& offset)
{
	SparseMatrix isingProblem;
	offset = 0.0;
	std::map<std::pair<int, int>, double> isingProblemMap;
	for (size_t i = 0; i < quboProblem.size(); ++i)
	{
		int r = quboProblem[i].i;
		int c = quboProblem[i].j;
		double v = quboProblem[i].value;

		if (r == c)
		{
			double hv = v * 0.5;
			isingProblemMap[std::make_pair(r, r)] += hv;
			offset += hv;
		}
		else
		{
			double jv = v * 0.25;
			isingProblemMap[std::make_pair(r, r)] += jv;
			isingProblemMap[std::make_pair(c, c)] += jv;
			isingProblemMap[std::make_pair(r, c)] += jv;
			offset += jv;
		}
	}

	for (std::map<std::pair<int, int>, double>::const_iterator it = isingProblemMap.begin(), end = isingProblemMap.end(); it != end; ++it)
		isingProblem.push_back(sapilocal::makeMatrixEntry(it->first.first, it->first.second, it->second));

	return isingProblem;
}


void adjust_answer(IsingResult& result, int from, int to, double offset)
{
	for (size_t i = 0; i < result.solutions.size(); ++i)
	{
		if (result.solutions[i] == from)
			result.solutions[i] = to;
	}

	for (size_t i = 0; i < result.energies.size(); ++i)
		result.energies[i] += offset;
}

void validateIterationLimit(int iterationLimit)
{
	if (!(iterationLimit >= 0))
		throw InvalidParameterException("iteration_limit must be >= 0");
}

void validateMinMaxBitFlipProb(double minBitFlipProb, double maxBitFlipProb)
{
	if (!(minBitFlipProb >= 0.0 && minBitFlipProb <= 1.0))
		throw InvalidParameterException("min_bit_flip_prob must be [0.0, 1.0]");

	if (!(maxBitFlipProb >= 0.0 && maxBitFlipProb <= 1.0))
		throw InvalidParameterException("max_bit_flip_prob must be [0.0, 1.0]");

	if (!(minBitFlipProb <= maxBitFlipProb))
		throw InvalidParameterException("min_bit_flip_prob must be <= max_bit_flip_prob");
}

void validateMaxComplexity(int maxComplexity)
{
	if (!(maxComplexity > 0))
		throw InvalidParameterException("max_local_complexity must be > 0");
}

void validateNoProgressLimit(int noProgressLimit)
{
	if (!(noProgressLimit > 0))
		throw InvalidParameterException("local_stuck_limit must be > 0");
}

void validateNumPerturbedCopies(int numPerturbedCopies)
{
	if (!(numPerturbedCopies > 0))
		throw InvalidParameterException("num_perturbed_copies must be > 0");
}

void validateNumVariables(int numVariables)
{
	if (!(numVariables >= 0))
		throw InvalidParameterException("num_variables must be > 0");
}

void validateTimeLimitSeconds(double timeLimitSeconds)
{
	if (!(timeLimitSeconds >= 0.0))
		throw InvalidParameterException("time_limit_seconds must be > 0");
}

} // namespace {anonymous}

namespace sapilocal {

IsingResult isingHeuristic(ProblemType problemType, const SparseMatrix& problem, const OrangHeuristicParams& params)
{
	validateIterationLimit(params.iterationLimit);
	validateMinMaxBitFlipProb(params.minBitFlipProb, params.maxBitFlipProb);
	validateMaxComplexity(params.maxComplexity);
	validateNoProgressLimit(params.noProgressLimit);
	validateNumPerturbedCopies(params.numPerturbedCopies);
	validateNumVariables(params.numVariables);
	validateTimeLimitSeconds(params.timeLimitSeconds);

	Var minVars = params.numVariables;
	SparseMatrix isingProblem;
	double offset;
	TaskType task = buildProblem(problemType == sapilocal::PROBLEM_QUBO ? qubo_to_ising(problem, offset) : problem, minVars);
	
	Params hParams;
	hParams.endTime = computeEndTime(params.timeLimitSeconds);
	hParams.iterationLimit = params.iterationLimit;
	hParams.maxBitFlipProb = params.maxBitFlipProb;
	hParams.maxComplexity = params.maxComplexity;
	hParams.minBitFlipProb = params.minBitFlipProb;
	hParams.noProgressLimit = params.noProgressLimit;
	hParams.numPerturbedCopies = params.numPerturbedCopies;
	
	unsigned int rngSeed;
	if (params.useSeed)
		rngSeed = params.rngSeed;
	else
		rngSeed = static_cast<unsigned int>(time(0));
	Rng rng((boost::mt19937(rngSeed)));
	
	IsingResult result = convertLocalSearchResult(task, isingHeuristic(task, hParams, rng));
	if (problemType == sapilocal::PROBLEM_QUBO)
		adjust_answer(result, -1, 0, offset);

	return result;
}

} // namespace sapilocal
