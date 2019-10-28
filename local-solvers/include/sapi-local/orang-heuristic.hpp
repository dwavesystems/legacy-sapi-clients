//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef SAPILOCAL_HEURISTIC_SOLVERS_HPP_INCLUDED
#define SAPILOCAL_HEURISTIC_SOLVERS_HPP_INCLUDED

#include "sapi-local/problems.hpp"

namespace sapilocal {

struct OrangHeuristicParams
{
	int iterationLimit;
	double timeLimitSeconds;
	int maxComplexity;
	int noProgressLimit;
	int numPerturbedCopies;
	double minBitFlipProb;
	double maxBitFlipProb;
	int numVariables;
	unsigned int rngSeed;
	bool useSeed;
};

IsingResult isingHeuristic(ProblemType problemType, const SparseMatrix& problem, const OrangHeuristicParams& params);

} // namespace sapilocal

#endif
