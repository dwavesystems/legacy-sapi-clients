//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <limits>
#include <numeric>
#include <vector>
#include <utility>

#include <boost/foreach.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/assign/list_of.hpp>

#include <gtest/gtest.h>

#include <sapi-local/sapi-local.hpp>

using std::accumulate;
using std::make_pair;
using std::numeric_limits;
using std::vector;

using namespace boost::assign;

using sapilocal::PROBLEM_ISING;
using sapilocal::PROBLEM_QUBO;
using sapilocal::SparseMatrix;
using sapilocal::OrangOptimizeParams;
using sapilocal::IsingResult;
using sapilocal::orangOptimize;
using sapilocal::makeMatrixEntry;
using sapilocal::UnsupportedProblemTypeException;
using sapilocal::InvalidParameterException;
using sapilocal::InvalidProblemException;

TEST(TestOrangOptimize, noReads) {
  OrangOptimizeParams params;
  params.s.numVars = 0;
  params.numReads = 0;
  params.maxAnswers = 0;
  params.answerHistogram = true;
  SparseMatrix problem;
  IsingResult r = orangOptimize(PROBLEM_ISING, problem, params);
  EXPECT_TRUE(r.energies.empty());
  EXPECT_TRUE(r.solutions.empty());
  EXPECT_TRUE(r.numOccurrences.empty());
}

TEST(TestOrangOptimize, emptyProblemNoHist) {
  OrangOptimizeParams params;
  params.s.numVars = 5;
  params.numReads = 3;
  params.maxAnswers = 3;
  params.answerHistogram = false;
  SparseMatrix problem;
  IsingResult r = orangOptimize(PROBLEM_ISING, problem, params);
  EXPECT_EQ(r.energies, vector<double>(1, 0.0));
  EXPECT_EQ(r.solutions, vector<signed char>(params.s.numVars, IsingResult::unusedVariable));
  EXPECT_TRUE(r.numOccurrences.empty());
}

TEST(TestOrangOptimize, emptyProblemHist) {
  OrangOptimizeParams params;
  params.s.numVars = 7;
  params.numReads = 1000;
  params.maxAnswers = 1;
  params.answerHistogram = true;
  SparseMatrix problem;
  IsingResult r = orangOptimize(PROBLEM_ISING, problem, params);
  EXPECT_EQ(r.energies, vector<double>(1, 0.0));
  EXPECT_EQ(r.solutions, vector<signed char>(params.s.numVars, IsingResult::unusedVariable));
  EXPECT_EQ(r.numOccurrences, vector<int>(1, params.numReads));
}

TEST(TestOrangOptimize, solutionSizeNoHist) {
  OrangOptimizeParams params;
  params.s.numVars = 23;
  params.s.activeVars += 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 22;
  params.s.varOrder = params.s.activeVars;
  params.answerHistogram = false;

  SparseMatrix problem;
  problem.push_back(makeMatrixEntry(0, 0, 1.0));
  problem.push_back(makeMatrixEntry(1, 1, 1.0));
  problem.push_back(makeMatrixEntry(2, 2, 1.0));
  problem.push_back(makeMatrixEntry(3, 3, 1.0));
  problem.push_back(makeMatrixEntry(4, 4, 1.0));
  problem.push_back(makeMatrixEntry(5, 5, 1.0));
  problem.push_back(makeMatrixEntry(6, 6, 1.0));
  problem.push_back(makeMatrixEntry(7, 7, 1.0));
  problem.push_back(makeMatrixEntry(8, 8, 1.0));
  problem.push_back(makeMatrixEntry(9, 9, 1.0));
  problem.push_back(makeMatrixEntry(22, 22, 1.0));

  params.numReads = 1000;
  params.maxAnswers = 10;
  auto r1 = orangOptimize(PROBLEM_ISING, problem, params);
  EXPECT_EQ(r1.energies.size(), 10);
  EXPECT_EQ(r1.solutions.size(), 10 * params.s.numVars);

  params.numReads = 33;
  params.maxAnswers = 1000;
  auto r2 = orangOptimize(PROBLEM_ISING, problem, params);
  EXPECT_EQ(r2.energies.size(), 33);
  EXPECT_EQ(r2.solutions.size(), 33 * params.s.numVars);
}

TEST(TestOrangOptimize, solutionSizeHist) {
  OrangOptimizeParams params;
  params.numReads = 10000;
  params.maxAnswers = 3;
  params.s.numVars = 4;
  params.s.activeVars += 0, 1, 2, 3;
  push_back(params.s.activeVarPairs)(0, 1)(1, 2)(2, 3);
  params.s.varOrder = params.s.activeVars;
  params.answerHistogram = true;

  SparseMatrix problem;
  problem.push_back(makeMatrixEntry(0, 0, -1));
  problem.push_back(makeMatrixEntry(0, 1, -1));
  problem.push_back(makeMatrixEntry(1, 2, -1));
  problem.push_back(makeMatrixEntry(2, 3, -1));

  auto r = orangOptimize(PROBLEM_ISING, problem, params);
  EXPECT_EQ(r.energies.size(), params.maxAnswers);
  EXPECT_EQ(r.solutions.size(), params.maxAnswers * params.s.numVars);
  EXPECT_LE(accumulate(r.numOccurrences.begin(), r.numOccurrences.end(), 0), params.numReads);
}

TEST(TestOrangOptimize, exactSolutionIsingNoHist) {
  OrangOptimizeParams params;
  params.s.numVars = 6;
  params.s.activeVars += 0, 1, 2, 3, 4, 5;
  push_back(params.s.activeVarPairs)(0, 1)(0, 3)(1, 2)(1, 4)(2, 5)(3, 4)(4, 5);
  params.s.varOrder += 0, 3, 1, 4, 2, 5;
  params.numReads = 10;
  params.maxAnswers = 10;
  params.answerHistogram = false;

  SparseMatrix problem;
  problem.push_back(makeMatrixEntry(0, 0, 4.0));
  problem.push_back(makeMatrixEntry(0, 1, -3.0));
  problem.push_back(makeMatrixEntry(0, 3, -5.0));
  problem.push_back(makeMatrixEntry(1, 1, -4.0));
  problem.push_back(makeMatrixEntry(1, 2, -2.0));
  problem.push_back(makeMatrixEntry(1, 4, -5.0));
  problem.push_back(makeMatrixEntry(2, 2, 3.0));
  problem.push_back(makeMatrixEntry(2, 5, 3.0));
  problem.push_back(makeMatrixEntry(3, 3, -10.0));
  problem.push_back(makeMatrixEntry(3, 4, -3.0));
  problem.push_back(makeMatrixEntry(4, 4, 7.0));
  problem.push_back(makeMatrixEntry(4, 5, 5.0));
  problem.push_back(makeMatrixEntry(5, 5, 6.0));

  auto r = orangOptimize(PROBLEM_ISING, problem, params);
  EXPECT_EQ(list_of(-32.0)(-28.0)(-24.0)(-24.0)(-20.0)(-20.0)(-20.0)(-20.0)(-20.0)(-20.0), r.energies);
  EXPECT_EQ(list_of<signed char>
    (+1)(+1)(+1)(+1)(+1)(-1)
    (+1)(+1)(-1)(+1)(+1)(-1)
    (-1)(-1)(-1)(+1)(-1)(+1)
    (-1)(+1)(+1)(+1)(+1)(-1)
    (-1)(-1)(-1)(-1)(-1)(+1)
    (-1)(-1)(-1)(+1)(-1)(-1)
    (-1)(+1)(-1)(+1)(+1)(-1)
    (+1)(-1)(-1)(+1)(-1)(+1)
    (+1)(+1)(-1)(+1)(-1)(+1)
    (+1)(+1)(+1)(+1)(-1)(-1), r.solutions);
  EXPECT_TRUE(r.numOccurrences.empty());
}

TEST(TestOrangOptimize, exactSolutionIsingHist) {
  OrangOptimizeParams params;
  params.s.numVars = 10;
  params.s.activeVars += 0, 1, 2, 3, 4, 5, 6, 7, 8, 9;
  push_back(params.s.activeVarPairs)(0, 9)(1, 9)(2, 9)(3, 9)(4, 9)(5, 9)(6, 9)(7, 9)(8, 9);
  params.s.varOrder += 8, 7, 6, 5, 4, 3, 2, 1, 0, 9;
  params.numReads = 10000;
  params.maxAnswers = 2;
  params.answerHistogram = true;

  SparseMatrix problem;
  problem.push_back(makeMatrixEntry(0, 0, -10.0));
  problem.push_back(makeMatrixEntry(0, 9, -6.0));
  problem.push_back(makeMatrixEntry(1, 1, 8.0));
  problem.push_back(makeMatrixEntry(1, 9, -10.0));
  problem.push_back(makeMatrixEntry(2, 2, 9.0));
  problem.push_back(makeMatrixEntry(2, 9, -7.0));
  problem.push_back(makeMatrixEntry(3, 3, 5.0));
  problem.push_back(makeMatrixEntry(3, 9, 10.0));
  problem.push_back(makeMatrixEntry(4, 4, 9.0));
  problem.push_back(makeMatrixEntry(4, 9, -3.0));
  problem.push_back(makeMatrixEntry(5, 5, -1.0));
  problem.push_back(makeMatrixEntry(5, 9, -7.0));
  problem.push_back(makeMatrixEntry(6, 6, 7.0));
  problem.push_back(makeMatrixEntry(6, 9, -3.0));
  problem.push_back(makeMatrixEntry(7, 7, 4.0));
  problem.push_back(makeMatrixEntry(7, 9, -7.0));
  problem.push_back(makeMatrixEntry(8, 8, 5.0));
  problem.push_back(makeMatrixEntry(8, 9, -7.0));
  problem.push_back(makeMatrixEntry(9, 9, -1.0));

  auto r = orangOptimize(PROBLEM_ISING, problem, params);
  EXPECT_EQ(list_of(-93.0)(-85.0), r.energies);
  EXPECT_EQ(list_of<signed char>
    (+1)(-1)(-1)(+1)(-1)(-1)(-1)(-1)(-1)(-1)
    (-1)(-1)(-1)(+1)(-1)(-1)(-1)(-1)(-1)(-1), r.solutions);
  EXPECT_EQ(list_of(9999)(1), r.numOccurrences);
}

TEST(TestOrangOptimize, exactSolutionQuboNoHist) {
  OrangOptimizeParams params;
  params.s.numVars = 8;
  params.s.activeVars += 0, 1, 2, 3, 4, 5, 6, 7;
  push_back(params.s.activeVarPairs)(0, 1)(0, 7)(1, 2)(2, 3)(3, 4)(4, 5)(5, 6)(6, 7);
  params.s.varOrder += 4, 5, 6, 7, 0, 1, 2, 3;
  params.numReads = 5;
  params.maxAnswers = 5;
  params.answerHistogram = false;

  SparseMatrix problem;
  problem.push_back(makeMatrixEntry(0, 0, 5.0));
  problem.push_back(makeMatrixEntry(0, 1, -1.0));
  problem.push_back(makeMatrixEntry(0, 7, 1.0));
  problem.push_back(makeMatrixEntry(1, 1, 2.0));
  problem.push_back(makeMatrixEntry(1, 2, 3.0));
  problem.push_back(makeMatrixEntry(2, 2, 3.0));
  problem.push_back(makeMatrixEntry(2, 3, -4.0));
  problem.push_back(makeMatrixEntry(3, 3, -3.0));
  problem.push_back(makeMatrixEntry(3, 4, -4.0));
  problem.push_back(makeMatrixEntry(4, 4, -5.0));
  problem.push_back(makeMatrixEntry(4, 5, 4.0));
  problem.push_back(makeMatrixEntry(5, 5, 4.0));
  problem.push_back(makeMatrixEntry(5, 6, -5.0));
  problem.push_back(makeMatrixEntry(6, 6, -4.0));
  problem.push_back(makeMatrixEntry(6, 7, 1.0));
  problem.push_back(makeMatrixEntry(7, 7, -4.0));

  auto r = orangOptimize(PROBLEM_QUBO, problem, params);
  EXPECT_EQ(list_of(-20.0)(-19.0)(-17.0)(-17.0)(-17.0), r.energies);
  EXPECT_EQ(list_of<signed char>
    (0)(0)(1)(1)(1)(0)(1)(1)
    (0)(0)(0)(1)(1)(0)(1)(1)
    (0)(0)(1)(1)(1)(0)(0)(1)
    (0)(0)(1)(1)(1)(0)(1)(0)
    (0)(0)(1)(1)(1)(1)(1)(1), r.solutions);
  EXPECT_TRUE(r.numOccurrences.empty());
}

TEST(TestOrangOptimize, exactSolutionQuboHist) {
  OrangOptimizeParams params;
  params.s.numVars = 4;
  params.s.activeVars += 0, 1, 2, 3;
  push_back(params.s.activeVarPairs)(0, 1)(0, 2)(0, 3)(1, 2)(1, 3)(2, 3);
  params.s.varOrder += 0, 3, 1, 2;
  params.numReads = 12345;
  params.maxAnswers = 3;
  params.answerHistogram = true;

  SparseMatrix problem;
  problem.push_back(makeMatrixEntry(0, 0, 9.0));
  problem.push_back(makeMatrixEntry(0, 1, -3.0));
  problem.push_back(makeMatrixEntry(0, 2, -4.0));
  problem.push_back(makeMatrixEntry(0, 3, -5.0));
  problem.push_back(makeMatrixEntry(1, 1, -3.0));
  problem.push_back(makeMatrixEntry(1, 2, 6.0));
  problem.push_back(makeMatrixEntry(1, 3, 6.0));
  problem.push_back(makeMatrixEntry(2, 2, 7.0));
  problem.push_back(makeMatrixEntry(2, 3, 5.0));
  problem.push_back(makeMatrixEntry(3, 3, 1.0));

  auto r = orangOptimize(PROBLEM_QUBO, problem, params);
  EXPECT_EQ(list_of(-3.0)(0.0)(1.0), r.energies);
  EXPECT_EQ(list_of<signed char>(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(1), r.solutions);
  EXPECT_EQ(list_of(12343)(1)(1), r.numOccurrences);
}

TEST(TestOrangOptimize, sloppyOrangStructure) {
  OrangOptimizeParams params;
  params.s.numVars = 20;
  params.s.activeVars += 2, 5, 3, 11, 19, 13, 2, 2, 17, 7, 19;
  push_back(params.s.activeVarPairs)(2, 5)(3, 2)(7, 5)(2, 3)(13, 11)(17, 11)(19, 11)(2, 11)(7, 5);
  params.s.varOrder += 2, 3, 5, 7, 11, 13, 17, 19;
  params.numReads = 0;
  params.maxAnswers = 0;
  params.answerHistogram = true;

  SparseMatrix problem;
  problem.push_back(makeMatrixEntry(2, 2, 1.0));
  problem.push_back(makeMatrixEntry(3, 3, 1.0));
  problem.push_back(makeMatrixEntry(5, 5, 1.0));
  problem.push_back(makeMatrixEntry(7, 7, 1.0));
  problem.push_back(makeMatrixEntry(11, 11, 1.0));
  problem.push_back(makeMatrixEntry(13, 13, 1.0));
  problem.push_back(makeMatrixEntry(17, 17, 1.0));
  problem.push_back(makeMatrixEntry(19, 19, 1.0));
  problem.push_back(makeMatrixEntry(2, 3, 1.0));
  problem.push_back(makeMatrixEntry(2, 5, 1.0));
  problem.push_back(makeMatrixEntry(2, 11, 1.0));
  problem.push_back(makeMatrixEntry(5, 7, 1.0));
  problem.push_back(makeMatrixEntry(11, 13, 1.0));
  problem.push_back(makeMatrixEntry(11, 17, 1.0));
  problem.push_back(makeMatrixEntry(11, 19, 1.0));

  orangOptimize(PROBLEM_ISING, problem, params);
}

TEST(TestOrangOptimize, varPairIndexOrder) {
  OrangOptimizeParams params;
  params.s.numVars = 2;
  params.s.activeVars += 0, 1;
  push_back(params.s.activeVarPairs)(0, 1);
  params.s.varOrder += 0, 1;
  params.numReads = 0;
  params.maxAnswers = 0;
  params.answerHistogram = true;

  SparseMatrix problem;
  problem.push_back(makeMatrixEntry(1, 0, -1));

  orangOptimize(PROBLEM_ISING, problem, params);
}

TEST(TestOrangOptimize, badProblemType) {
  OrangOptimizeParams params;
  params.s.numVars = 0;
  params.numReads = 0;
  params.maxAnswers = 0;
  params.answerHistogram = true;

  EXPECT_THROW(orangOptimize(static_cast<sapilocal::ProblemType>(-999), SparseMatrix(), params),
    UnsupportedProblemTypeException);
}

TEST(TestOrangOptimize, badNumVars) {
  OrangOptimizeParams params;
  params.s.numVars = -1;
  params.numReads = 0;
  params.maxAnswers = 0;
  params.answerHistogram = true;

  EXPECT_THROW(orangOptimize(PROBLEM_ISING, SparseMatrix(), params), InvalidParameterException);
}

TEST(TestOrangOptimize, badActiveVars) {
  OrangOptimizeParams params;
  params.s.numVars = 100;
  params.s.activeVars += 45, -1, 54;
  params.s.varOrder = params.s.activeVars;
  params.numReads = 0;
  params.maxAnswers = 0;
  params.answerHistogram = true;
  EXPECT_THROW(orangOptimize(PROBLEM_ISING, SparseMatrix(), params), InvalidParameterException);

  params.s.activeVars.clear();
  params.s.activeVars += 1, 10, 100;
  params.s.varOrder = params.s.activeVars;
  EXPECT_THROW(orangOptimize(PROBLEM_ISING, SparseMatrix(), params), InvalidParameterException);
}

TEST(TestOrangOptimize, badActiveVarPairs) {
  OrangOptimizeParams params;
  params.s.numVars = 10;
  params.s.activeVars += 1, 4, 7;
  push_back(params.s.activeVarPairs)(0, 1);
  params.s.varOrder = params.s.activeVars;
  params.numReads = 0;
  params.maxAnswers = 0;
  params.answerHistogram = true;
  EXPECT_THROW(orangOptimize(PROBLEM_ISING, SparseMatrix(), params), InvalidParameterException);

  params.s.activeVarPairs.clear();
  push_back(params.s.activeVarPairs)(4, 4);
  EXPECT_THROW(orangOptimize(PROBLEM_ISING, SparseMatrix(), params), InvalidParameterException);
}

TEST(TestOrangOptimize, badVarOrder) {
  OrangOptimizeParams params;
  params.s.numVars = 6;
  params.s.activeVars += 0, 2, 3, 5;
  params.s.varOrder += 0, 1, 2, 3, 5;
  params.numReads = 0;
  params.maxAnswers = 0;
  params.answerHistogram = true;
  EXPECT_THROW(orangOptimize(PROBLEM_ISING, SparseMatrix(), params), InvalidParameterException);

  params.s.varOrder.clear();
  params.s.varOrder += 0, 3, 5;
  EXPECT_THROW(orangOptimize(PROBLEM_ISING, SparseMatrix(), params), InvalidParameterException);

  params.s.varOrder.clear();
  params.s.varOrder += 0, 1, 3, 5;
  EXPECT_THROW(orangOptimize(PROBLEM_ISING, SparseMatrix(), params), InvalidParameterException);

  params.s.varOrder.clear();
  params.s.varOrder += 0, 3, 2, 3, 5;
  EXPECT_THROW(orangOptimize(PROBLEM_ISING, SparseMatrix(), params), InvalidParameterException);
}

TEST(TestOrangOptimize, badNumReads) {
  OrangOptimizeParams params;
  params.s.numVars = 0;
  params.numReads = -1;
  params.maxAnswers = 0;
  params.answerHistogram = true;

  EXPECT_THROW(orangOptimize(PROBLEM_ISING, SparseMatrix(), params), InvalidParameterException);
}

TEST(TestOrangOptimize, badMaxAnswers) {
  OrangOptimizeParams params;
  params.s.numVars = 0;
  params.numReads = 0;
  params.maxAnswers = -1;
  params.answerHistogram = true;

  EXPECT_THROW(orangOptimize(PROBLEM_ISING, SparseMatrix(), params), InvalidParameterException);
}

TEST(TestOrangOptimize, badProblem) {
  OrangOptimizeParams params;
  params.s.numVars = 10;
  params.s.activeVars += 3, 4, 5, 6, 7;
  push_back(params.s.activeVarPairs)(3, 4)(4, 5)(5, 6)(6, 7);
  params.s.varOrder = params.s.activeVars;
  params.numReads = 10;
  params.maxAnswers = 1;

  SparseMatrix invalidVar;
  invalidVar.push_back(makeMatrixEntry(1, 1, 1.0));
  EXPECT_THROW(orangOptimize(PROBLEM_ISING, invalidVar, params), InvalidProblemException);

  SparseMatrix invalidVarPair;
  invalidVarPair.push_back(makeMatrixEntry(3, 5, 1.0));
  EXPECT_THROW(orangOptimize(PROBLEM_ISING, invalidVarPair, params), InvalidProblemException);
}
