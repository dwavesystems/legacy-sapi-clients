//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include "gtest/gtest.h"
#include "fix_variables.hpp"

TEST(FixVariablesTest, InvalidInput)
{
	// Q
	compressed_matrix::CompressedMatrix<double> invalidQ(5, 6);
	EXPECT_THROW(fix_variables_::fixQuboVariables(invalidQ, 1), fix_variables_::FixVariablesException);
	EXPECT_THROW(fix_variables_::fixQuboVariables(invalidQ, 2), fix_variables_::FixVariablesException);

	// method
	compressed_matrix::CompressedMatrix<double> Q(6, 6);
	EXPECT_THROW(fix_variables_::fixQuboVariables(Q, -1), fix_variables_::FixVariablesException);
	EXPECT_THROW(fix_variables_::fixQuboVariables(Q, 0), fix_variables_::FixVariablesException);
	EXPECT_THROW(fix_variables_::fixQuboVariables(Q, 3), fix_variables_::FixVariablesException);
}

TEST(FixVariablesTest, ValidInput)
{
	compressed_matrix::CompressedMatrix<double> Q(3, 3);
	Q(0, 0) = 1000;
	Q(0, 1) = -1100;
	Q(0, 2) = 6;
	Q(1, 0) = -1000;
	Q(1, 1) = 600;
	Q(1, 2) = 8000;
	Q(2, 0) = -200;
	Q(2, 1) = 9;
	Q(2, 2) = -2000;

	fix_variables_::FixVariablesResult result = fix_variables_::fixQuboVariables(Q, 1);
	EXPECT_EQ(result.fixedVars[0].first, 3);
	EXPECT_EQ(result.fixedVars[0].second, 1);
	EXPECT_EQ(result.fixedVars[1].first, 1);
	EXPECT_EQ(result.fixedVars[1].second, 0);
	EXPECT_EQ(result.fixedVars[2].first, 2);
	EXPECT_EQ(result.fixedVars[2].second, 0);
	EXPECT_EQ(result.newQ.nnz(), 0);
	EXPECT_EQ(result.offset, -2000);

	result = fix_variables_::fixQuboVariables(Q, 2);
	EXPECT_EQ(result.fixedVars[0].first, 3);
	EXPECT_EQ(result.fixedVars[0].second, 1);
	EXPECT_EQ(result.fixedVars[1].first, 1);
	EXPECT_EQ(result.fixedVars[1].second, 0);
	EXPECT_EQ(result.fixedVars[2].first, 2);
	EXPECT_EQ(result.fixedVars[2].second, 0);
	EXPECT_EQ(result.newQ.nnz(), 0);
	EXPECT_EQ(result.offset, -2000);

	// before adding codes in the fix_variables.cpp to remove unused variables from ret.fixedVars, for empty Q, method 1 will fix all the
	// unused variables in empty Q; method 2 will fix no unused variables in empty Q.
	// tried non-empty Q with some usused variables, method 1 will fix all the unused variables, method 2 will fix no unused variables.
	/*
	compressed_matrix::CompressedMatrix<double> Q2(128, 128);
	fix_variables_::FixVariablesResult result2 = fix_variables_::fixQuboVariables(Q2, 1);
	EXPECT_EQ(result2.fixedVars.size(), 128);
	fix_variables_::FixVariablesResult result3 = fix_variables_::fixQuboVariables(Q2, 2);
	EXPECT_EQ(result3.fixedVars.size(), 0); //result is the same as matlab code fixQUBOVariables.m for method 2
	*/
}

TEST(FixVariablesTest, EmptyQ)
{
	for (int sz = 0; sz <= 6; ++sz)
	{
		compressed_matrix::CompressedMatrix<double> Q(sz, sz);
		fix_variables_::FixVariablesResult result = fix_variables_::fixQuboVariables(Q, 1);
		EXPECT_EQ(result.fixedVars.size(), 0);
		EXPECT_EQ(result.newQ.numRows(), sz);
		EXPECT_EQ(result.newQ.numCols(), sz);
		EXPECT_EQ(result.newQ.nnz(), 0);
		EXPECT_EQ(result.offset, 0.0);
	}

	for (int sz = 0; sz <= 6; ++sz)
	{
		std::vector<int> rowOffsets(sz + 1, 0);
		std::vector<int> colIndices;
		std::vector<double> values;
		compressed_matrix::CompressedMatrix<double> Q(sz, sz, rowOffsets, colIndices, values);
		fix_variables_::FixVariablesResult result = fix_variables_::fixQuboVariables(Q, 1);
		EXPECT_EQ(result.fixedVars.size(), 0);
		EXPECT_EQ(result.newQ.numRows(), sz);
		EXPECT_EQ(result.newQ.numCols(), sz);
		EXPECT_EQ(result.newQ.nnz(), 0);
		EXPECT_EQ(result.offset, 0.0);
	}
}


TEST(FixVariablesTest, BugSAPI1311) {
  compressed_matrix::CompressedMatrix<double> Q(2, 2);
  Q(0, 0) = 2.2;
  Q(0, 1) = -4.0;
  Q(1, 1) = 2.0;

  auto result1 = fix_variables_::fixQuboVariables(Q, 1);
  EXPECT_EQ(0, result1.newQ.nnz());
  EXPECT_EQ(0, result1.offset);
  ASSERT_EQ(2, result1.fixedVars.size());
  EXPECT_LE(1, result1.fixedVars[0].first);
  EXPECT_GE(2, result1.fixedVars[0].first);
  EXPECT_LE(1, result1.fixedVars[1].first);
  EXPECT_GE(2, result1.fixedVars[1].first);
  EXPECT_NE(result1.fixedVars[0].first, result1.fixedVars[1].first);
  EXPECT_EQ(0, result1.fixedVars[0].second);
  EXPECT_EQ(0, result1.fixedVars[1].second);

  auto result2 = fix_variables_::fixQuboVariables(Q, 2);
  EXPECT_EQ(0, result2.newQ.nnz());
  EXPECT_EQ(0, result2.offset);
  ASSERT_EQ(2, result2.fixedVars.size());
  EXPECT_LE(1, result2.fixedVars[0].first);
  EXPECT_GE(2, result2.fixedVars[0].first);
  EXPECT_LE(1, result2.fixedVars[1].first);
  EXPECT_GE(2, result2.fixedVars[1].first);
  EXPECT_NE(result2.fixedVars[0].first, result2.fixedVars[1].first);
  EXPECT_EQ(0, result2.fixedVars[0].second);
  EXPECT_EQ(0, result2.fixedVars[1].second);
}
