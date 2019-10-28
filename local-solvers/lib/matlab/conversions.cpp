//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cmath>
#include <limits>
#include <memory>
#include <string>

#include <mex.h>

#include <sapi-local/sapi-local.hpp>

#include "conversions.hpp"
#include "errid.hpp"

using std::floor;
using std::fmod;
using std::numeric_limits;
using std::string;
using std::unique_ptr;

using sapilocal::ProblemType;
using sapilocal::SparseMatrix;
using sapilocal::makeMatrixEntry;
using sapilocal::OrangSampleParams;
using sapilocal::OrangOptimizeParams;
using sapilocal::OrangHeuristicParams;
using sapilocal::IsingResult;
using sapilocal::OrangStructure;

namespace {

namespace problemtypes {
auto ising = "ising";
auto qubo = "qubo";
} // namespace {anonymous}::problemtypes

namespace fields {
auto numVars = "num_vars";
auto activeVars = "active_vars";
auto activeVarPairs = "active_var_pairs";
auto varOrder = "var_order";
auto numReads = "num_reads";
auto maxAnswers = "max_answers";
auto answerHistogram = "answer_histogram";
auto beta = "beta";
auto randomSeed = "random_seed";

auto iterationLimit = "iteration_limit";
auto maxBitFlipProb = "max_bit_flip_prob";
auto maxComplexity = "max_local_complexity";
auto minBitFlipProb = "min_bit_flip_prob";
auto noProgressLimit = "local_stuck_limit";
auto numPerturbedCopies = "num_perturbed_copies";
auto numVariables = "num_variables";
auto timeLimitSeconds = "time_limit_seconds";

auto energies = "energies";
auto solutions = "solutions";
auto numOccurrences = "num_occurrences";
} // namespace {anonymous}::fields

namespace fieldarrays {
const char* isingResultHist[] = { fields::energies, fields::solutions, fields::numOccurrences };
const char* isingResultNoHist[] = { fields::energies, fields::solutions };
} // namespace {anonymous}::fieldarrays

struct MxFreeDeleter {
  void operator()(void* a) { mxFree(a); }
};

double getDoubleField(const mxArray* array, const char* field) {
  auto fieldArray = mxGetField(array, 0, field);
  if (!fieldArray) mexErrMsgIdAndTxt(errid::missingField, "missing '%s' parameter", field);
  if (!mxIsDouble(fieldArray) || mxIsComplex(fieldArray) || mxGetNumberOfElements(fieldArray) != 1) {
    mexErrMsgIdAndTxt(errid::argType, "'%s' parameter must be a scalar double", field);
  }
  return mxGetScalar(fieldArray);
}

int getIntField(const mxArray* array, const char* field) {
  auto d = getDoubleField(array, field);
  if (d != floor(d)) mexErrMsgIdAndTxt(errid::argValue, "'%s' parameter must be integer-valued", field);
  if (d > numeric_limits<int>::max() || d < numeric_limits<int>::min()) {
    mexErrMsgIdAndTxt(errid::argValue, "'%s' parameter out of range", field);
  }
  return static_cast<int>(d);
}

bool getBoolField(const mxArray* array, const char* field) {
  auto fieldArray = mxGetField(array, 0, field);
  if (!fieldArray) mexErrMsgIdAndTxt(errid::missingField, "missing '%s' parameter", field);
  if (!mxIsLogicalScalar(fieldArray)) {
    mexErrMsgIdAndTxt(errid::argType, "'%s' parameter must be a scalar logical", field);
  }
  return mxIsLogicalScalarTrue(fieldArray);
}

OrangStructure::VarVector getVarVectorField(const mxArray* array, const char* field) {
  auto fieldArray = mxGetField(array, 0, field);
  if (!fieldArray) mexErrMsgIdAndTxt(errid::missingField, "missing '%s' parameter", field);
  if (!mxIsDouble(fieldArray) || mxIsComplex(fieldArray)
      || (!mxIsEmpty(fieldArray) && mxGetM(fieldArray) != 1 && mxGetN(fieldArray) != 1)) {
    mexErrMsgIdAndTxt(errid::argType, "'%s' parameter must be a double vector", field);
  }

  auto size = mxGetNumberOfElements(fieldArray);
  OrangStructure::VarVector v;
  v.reserve(size);

  auto pr = mxGetPr(fieldArray);
  while (size-- > 0) {
    auto d = *pr++;
    if (d != floor(d)) mexErrMsgIdAndTxt(errid::argValue, "'%s' parameter entries must be integer-valued", field);
    if (d > numeric_limits<int>::max() || d < numeric_limits<int>::min()) {
      mexErrMsgIdAndTxt(errid::argValue, "'%s' parameter entry out of range", field);
    }
    v.push_back(static_cast<OrangStructure::Var>(d));
  }
  return v;
}

OrangStructure::VarPairVector getVarPairVectorField(const mxArray* array, const char* field) {
  auto fieldArray = mxGetField(array, 0, field);
  if (!fieldArray) mexErrMsgIdAndTxt(errid::missingField, "missing '%s' parameter", field);
  if (!mxIsDouble(fieldArray) || mxIsComplex(fieldArray) || (mxGetM(fieldArray) != 2 && !mxIsEmpty(fieldArray))) {
    mexErrMsgIdAndTxt(errid::argType, "'%s' parameter must be a 2xN double matrix", field);
  }

  auto size = mxGetN(fieldArray);
  OrangStructure::VarPairVector v;
  v.reserve(size);

  auto pr = mxGetPr(fieldArray);
  while (size-- > 0) {
    auto d = *pr++;
    if (d != floor(d)) mexErrMsgIdAndTxt(errid::argValue, "'%s' parameter entries must be integer-valued", field);
    if (d > numeric_limits<int>::max() || d < numeric_limits<int>::min()) {
      mexErrMsgIdAndTxt(errid::argValue, "'%s' parameter entry out of range", field);
    }
    auto v1 = static_cast<OrangStructure::Var>(d);

    d = *pr++;
    if (d != floor(d)) mexErrMsgIdAndTxt(errid::argValue, "'%s' parameter entries must be integer-valued", field);
    if (d > numeric_limits<int>::max() || d < numeric_limits<int>::min()) {
      mexErrMsgIdAndTxt(errid::argValue, "'%s' parameter entry out of range", field);
    }
    auto v2 = static_cast<OrangStructure::Var>(d);

    v.push_back(OrangStructure::VarPair(v1, v2));
  }
  return v;
}

OrangStructure convertOrangStructure(const mxArray* structureArray) {
  OrangStructure s;
  s.numVars = getIntField(structureArray, fields::numVars);
  s.activeVars = getVarVectorField(structureArray, fields::activeVars);
  s.activeVarPairs = getVarPairVectorField(structureArray, fields::activeVarPairs);
  s.varOrder = getVarVectorField(structureArray, fields::varOrder);
  return s;
}

} // namespace {anonymous}

ProblemType convertProblemType(const mxArray* ptArray) {
  if (!mxIsChar(ptArray)) mexErrMsgIdAndTxt(errid::argType, "problem type must be string");
  auto ptPtr = unique_ptr<char, MxFreeDeleter>(mxArrayToString(ptArray));
  if (!ptPtr) mexErrMsgIdAndTxt(errid::noMem, "out of memory");
  auto problemType = string(ptPtr.get());
  if (problemType == problemtypes::ising) {
    return sapilocal::PROBLEM_ISING;
  } else if (problemType == problemtypes::qubo) {
    return sapilocal::PROBLEM_QUBO;
  } else {
    mexErrMsgIdAndTxt(errid::problemType, "unknown problem type");
  }
}

SparseMatrix convertSparseMatrix(const mxArray* problemArray) {
  if (!mxIsDouble(problemArray) || mxIsComplex(problemArray) || mxGetNumberOfDimensions(problemArray) > 2) {
    mexErrMsgIdAndTxt(errid::argType, "problem must be a real matrix of doubles");
  }

  auto rowsU = mxGetM(problemArray);
  auto colsU = mxGetN(problemArray);
  if (rowsU > numeric_limits<int>::max() || colsU > numeric_limits<int>::max()) {
    mexErrMsgIdAndTxt(errid::problem, "problem dimensions too large");
  }

  SparseMatrix matrix;
  auto rows = static_cast<int>(rowsU);
  auto cols = static_cast<int>(colsU);
  auto pr = mxGetPr(problemArray);

  if (mxIsSparse(problemArray)) {
    auto jc = mxGetJc(problemArray);
    auto ir = mxGetIr(problemArray);
    for (auto c = 0; c < cols; ++c) {
      for (auto i = jc[c]; i < jc[c + 1]; ++i) {
        matrix.push_back(makeMatrixEntry(static_cast<int>(*ir++), c, *pr++));
      }
    }
    return matrix;

  } else {
    for (auto c = 0; c < cols; ++c) {
      for (auto r = 0; r < rows; ++r) {
        if (*pr != 0.0) matrix.push_back(makeMatrixEntry(r, c, *pr));
        ++pr;
      }
    }
    return matrix;
  }
}

OrangSampleParams convertOrangSampleParams(const mxArray* ospArray) {
  OrangSampleParams params;
  params.s = convertOrangStructure(ospArray);
  params.numReads = getIntField(ospArray, fields::numReads);
  params.maxAnswers = getIntField(ospArray, fields::maxAnswers);
  params.answerHistogram = getBoolField(ospArray, fields::answerHistogram);
  params.beta = getDoubleField(ospArray, fields::beta);

  auto rngSeedField = mxGetField(ospArray, 0, fields::randomSeed);
  if (rngSeedField && !mxIsEmpty(rngSeedField)) {
    params.useSeed = true;
    params.randomSeed = static_cast<unsigned int>(
      fmod(getDoubleField(ospArray, fields::randomSeed), numeric_limits<unsigned int>::max()));
  } else {
    params.useSeed = false;
  }

  return params;
}

OrangOptimizeParams convertOrangOptimizeParams(const mxArray* ospArray) {
  OrangOptimizeParams params;
  params.s = convertOrangStructure(ospArray);
  params.numReads = getIntField(ospArray, fields::numReads);
  params.maxAnswers = getIntField(ospArray, fields::maxAnswers);
  params.answerHistogram = getBoolField(ospArray, fields::answerHistogram);
  return params;
}

sapilocal::OrangHeuristicParams convertOrangHeuristicParams(const mxArray* ospArray)
{
	OrangHeuristicParams params;
	params.iterationLimit = getIntField(ospArray, fields::iterationLimit);
	params.maxBitFlipProb = getDoubleField(ospArray, fields::maxBitFlipProb);
	params.maxComplexity = getIntField(ospArray, fields::maxComplexity);
	params.minBitFlipProb = getDoubleField(ospArray, fields::minBitFlipProb);
	params.noProgressLimit = getIntField(ospArray, fields::noProgressLimit);
	params.numPerturbedCopies = getIntField(ospArray, fields::numPerturbedCopies);
	params.numVariables = getIntField(ospArray, fields::numVariables);

	auto rngSeedField = mxGetField(ospArray, 0, fields::randomSeed);
	if (rngSeedField && !mxIsEmpty(rngSeedField))
	{
		params.useSeed = true;
		params.rngSeed = static_cast<unsigned int>(fmod(getDoubleField(ospArray, fields::randomSeed), numeric_limits<unsigned int>::max()));
	}
	else
		params.useSeed = false;
	
	params.timeLimitSeconds = getDoubleField(ospArray, fields::timeLimitSeconds);

	return params;
}


mxArray* convertIsingResult(const IsingResult& result) {
  auto hist = result.energies.size() == result.numOccurrences.size();
  auto resultArray = mxCreateStructMatrix(1, 1, hist ? 3 : 2,
   hist ? fieldarrays::isingResultHist : fieldarrays::isingResultNoHist);

  auto numSolutions = result.energies.size();
  auto solutionSize = result.solutions.size() / numSolutions;

  auto energiesArray = mxCreateDoubleMatrix(1, numSolutions, mxREAL);
  copy(result.energies.begin(), result.energies.end(), mxGetPr(energiesArray));
  mxSetField(resultArray, 0, fields::energies, energiesArray);

  auto solutionsArray = mxCreateDoubleMatrix(solutionSize, numSolutions, mxREAL);
  copy(result.solutions.begin(), result.solutions.end(), mxGetPr(solutionsArray));
  mxSetField(resultArray, 0, fields::solutions, solutionsArray);

  if (hist) {
    auto numOccArray = mxCreateDoubleMatrix(1, numSolutions, mxREAL);
    copy(result.numOccurrences.begin(), result.numOccurrences.end(), mxGetPr(numOccArray));
    mxSetField(resultArray, 0, fields::numOccurrences, numOccArray);
  }

  return resultArray;
}
