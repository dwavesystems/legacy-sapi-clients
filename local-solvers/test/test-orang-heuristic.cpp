//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <boost/assign/list_of.hpp>

#include <gtest/gtest.h>

#include <sapi-local/sapi-local.hpp>

using boost::assign::list_of;

using sapilocal::SparseMatrix;
using sapilocal::makeMatrixEntry;
using sapilocal::IsingResult;

sapilocal::OrangHeuristicParams defaultParams()
{
	sapilocal::OrangHeuristicParams params;

	params.iterationLimit = 10;
	params.maxBitFlipProb = 1.0 / 8.0;
	params.maxComplexity = 9;
	params.minBitFlipProb = 1.0 / 32.0;
	params.noProgressLimit = 8;
	params.numPerturbedCopies = 4;
	params.numVariables = 0;
	params.timeLimitSeconds = 5.0;	
	params.useSeed = false;

	return params;
}

TEST(TestOrangHeuristic, empty_problem) {
  SparseMatrix problem;
  sapilocal::OrangHeuristicParams params = defaultParams();
  IsingResult result = sapilocal::isingHeuristic(sapilocal::PROBLEM_ISING, problem, params);
  IsingResult expectedResult;
  expectedResult.energies.assign(1, 0.0);
  EXPECT_EQ(result.solutions, expectedResult.solutions);
  EXPECT_EQ(result.energies, expectedResult.energies);
}

TEST(TestOrangHeuristic, easy_problem) {
  SparseMatrix problem = list_of
      (makeMatrixEntry( 0,  0, -1))
      (makeMatrixEntry( 0,  1,  3))
      (makeMatrixEntry( 0,  5, -3))
      (makeMatrixEntry( 1,  1,  5))
      (makeMatrixEntry( 1,  2, -4))
      (makeMatrixEntry( 1,  6,  8))
      (makeMatrixEntry( 2,  2,  2))
      (makeMatrixEntry( 2,  3,  7))
      (makeMatrixEntry( 2,  7,  6))
      (makeMatrixEntry( 3,  3,  5))
      (makeMatrixEntry( 3,  4, -1))
      (makeMatrixEntry( 3,  8,  3))
      (makeMatrixEntry( 4,  4,  2))
      (makeMatrixEntry( 4,  9, -9))
      (makeMatrixEntry( 5,  5, -2))
      (makeMatrixEntry( 5,  6,  5))
      (makeMatrixEntry( 6,  6,  7))
      (makeMatrixEntry( 6,  7, -7))
      (makeMatrixEntry( 7,  7,  9))
      (makeMatrixEntry( 7,  8, -4))
      (makeMatrixEntry( 8,  8,  6))
      (makeMatrixEntry( 8,  9, -8))
      (makeMatrixEntry( 9,  9,  8));

  sapilocal::OrangHeuristicParams params = defaultParams();
  params.timeLimitSeconds = 1.0;
  params.iterationLimit = 0;
  params.maxComplexity = 3;
  IsingResult result = sapilocal::isingHeuristic(sapilocal::PROBLEM_ISING, problem, params);

  IsingResult expectedResult;
  expectedResult.energies.assign(1, -89.0);
  signed char expectedSolution[] = {1, 1, 1, -1, -1, 1, -1, -1, -1, -1};
  expectedResult.solutions.assign(expectedSolution, expectedSolution + sizeof(expectedSolution) / sizeof(signed char));
  
  EXPECT_EQ(result.solutions, expectedResult.solutions);
  EXPECT_EQ(result.energies, expectedResult.energies);
}

TEST(TestOrangHeuristic, unused_variables) {
  SparseMatrix problem = list_of
      (makeMatrixEntry(1, 1, -1))
      (makeMatrixEntry(2, 2, 1))
      (makeMatrixEntry(3, 3, -1))
      (makeMatrixEntry(5, 5, -1))
      (makeMatrixEntry(8, 8, 1));

  sapilocal::OrangHeuristicParams params = defaultParams();
  params.timeLimitSeconds = 1.0;
  params.iterationLimit = 0;
  IsingResult result = sapilocal::isingHeuristic(sapilocal::PROBLEM_ISING, problem, params);

  signed char u = IsingResult::unusedVariable;
  IsingResult expectedResult;
  expectedResult.energies.assign(1, -5.0);
  signed char expectedSolution[] = {u, 1, -1, 1, u, 1, u, u, -1};
  expectedResult.solutions.assign(expectedSolution, expectedSolution + sizeof(expectedSolution) / sizeof(signed char));

  EXPECT_EQ(result.solutions, expectedResult.solutions);
  EXPECT_EQ(result.energies, expectedResult.energies);
}

TEST(TestOrangHeuristic, min_variables) {
  SparseMatrix problem = list_of(makeMatrixEntry(0, 0, -1));

  sapilocal::OrangHeuristicParams params = defaultParams();
  params.timeLimitSeconds = 1.0;
  params.iterationLimit = 0;
  params.numVariables = 8;
  IsingResult result = sapilocal::isingHeuristic(sapilocal::PROBLEM_ISING, problem, params);

  signed char u = IsingResult::unusedVariable;
  IsingResult expectedResult;
  expectedResult.energies.assign(1, -1.0);
  signed char expectedSolution[] = {1, u, u, u, u, u, u, u};
  expectedResult.solutions.assign(expectedSolution, expectedSolution + sizeof(expectedSolution) / sizeof(signed char));

  EXPECT_EQ(result.solutions, expectedResult.solutions);
  EXPECT_EQ(result.energies, expectedResult.energies);
}
