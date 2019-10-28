//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include "find_embedding.hpp"

#include <map>
#include <stack>
#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

#include "test.hpp"

using std::make_pair;
using std::map;
using std::pair;
using std::set;
using std::shared_ptr;
using std::vector;
using std::stack;
using std::unordered_map;

using testing::_;
using testing::AllOf;
using testing::EndsWith;
using testing::NiceMock;
using testing::Return;
using testing::StartsWith;

using test::AssertValidFindEmbeddingResult;

namespace
{

std::map<std::pair<int, int>, int> getChimeraAdjacency(int M, int N, int L)
{
	std::map<std::pair<int, int>, int> ret;

	int m = M;
	int n = N;
	int k = L;

	// vertical edges:
	for (int j = 0; j < n; ++j)
	{
		int start = k * 2 * j;
		for (int i = 0; i < m - 1; ++i)
		{
			for (int t = 0; t < k; ++t)
			{
				int fm = start + t;
				int to = start + t + n * k * 2;
				ret.insert(std::make_pair(std::make_pair(fm, to), 1));
				ret.insert(std::make_pair(std::make_pair(to, fm), 1));
			}
			start = start + n * k * 2;
		}
	}

	// horizontal edges:
	for (int i = 0; i < m; ++i)
	{
		int start = k * (2 * n * i + 1);
		for (int j = 0; j < n - 1; ++j)
		{
			for (int t = 0; t < k; ++t)
			{
				int fm = start + t;
				int to = start + t + k * 2;
				ret.insert(std::make_pair(std::make_pair(fm, to), 1));
				ret.insert(std::make_pair(std::make_pair(to, fm), 1));
			}
			start = start + k * 2;
		}
	}

	// inside-cell edges:
	for (int i = 0; i < m; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			int add = (i * n + j) * k * 2;
			for (int t = 0; t < k; ++t)
				for (int u = k; u < 2 * k; ++u)
				{
					ret.insert(std::make_pair(std::make_pair(t + add, u + add), 1));
					ret.insert(std::make_pair(std::make_pair(u + add, t + add), 1));
				}
		}
	}

	return ret;
}


class MockLocalInteraction : public find_embedding_::LocalInteraction
{
public:
	MOCK_CONST_METHOD1(displayOutputImpl, void(const std::string&));
	MOCK_CONST_METHOD0(cancelledImpl, bool());

	MockLocalInteraction()
	{
		ON_CALL(*this, displayOutputImpl(_)).WillByDefault(Return());
		ON_CALL(*this, cancelledImpl()).WillByDefault(Return(false));
	}
};
} // anonymous namespace

TEST(FindEmbeddingTest, InvalidParameters)
{
	int problemSize = 20;
	std::map<std::pair<int, int>, int> problemMap;
	for (int i = 0; i < problemSize; ++i)
		for (int j = 0; j < problemSize; ++j)
			problemMap.insert(std::make_pair(std::make_pair(i, j), 1));

	compressed_matrix::CompressedMatrix<int> problem(problemSize, problemSize, problemMap);

	int M = 5;
	int N = 5;
	int L = 4;

	std::map<std::pair<int, int>, int> AMap = getChimeraAdjacency(M, N, L);
	int ASize = M * N * L * 2;
	compressed_matrix::CompressedMatrix<int> A(ASize, ASize, AMap);

	find_embedding_::FindEmbeddingExternalParams defaultFindEmbeddingExternalParams;
	defaultFindEmbeddingExternalParams.localInteractionPtr.reset(new NiceMock<MockLocalInteraction>);

	find_embedding_::FindEmbeddingExternalParams feExternalParams;


	// Q
	feExternalParams = defaultFindEmbeddingExternalParams;
	compressed_matrix::CompressedMatrix<int> problemSizeNotCorrect_1(problemSize, problemSize + 1, problemMap);
	EXPECT_THROW(find_embedding_::findEmbedding(problemSizeNotCorrect_1, A, feExternalParams), find_embedding_::FindEmbeddingException);

	compressed_matrix::CompressedMatrix<int> problemSizeNotCorrect_2(problemSize + 1, problemSize, problemMap);
	EXPECT_THROW(find_embedding_::findEmbedding(problemSizeNotCorrect_2, A, feExternalParams), find_embedding_::FindEmbeddingException);
	

	// A
	compressed_matrix::CompressedMatrix<int> ASizeNotCorrect_1(ASize, ASize + 1, AMap);
	EXPECT_THROW(find_embedding_::findEmbedding(problem, ASizeNotCorrect_1, feExternalParams), find_embedding_::FindEmbeddingException);

	compressed_matrix::CompressedMatrix<int> ASizeNotCorrect_2(ASize + 1, ASize, AMap);
	EXPECT_THROW(find_embedding_::findEmbedding(problem, ASizeNotCorrect_2, feExternalParams), find_embedding_::FindEmbeddingException);
	

	// localInteractionPtr
	feExternalParams = defaultFindEmbeddingExternalParams;
	feExternalParams.localInteractionPtr.reset();
	EXPECT_THROW(find_embedding_::findEmbedding(problem, A, feExternalParams), find_embedding_::FindEmbeddingException);


	// max_no_improvement
	feExternalParams = defaultFindEmbeddingExternalParams;
	feExternalParams.maxNoImprovement = -1;
	EXPECT_THROW(find_embedding_::findEmbedding(problem, A, feExternalParams), find_embedding_::FindEmbeddingException);


	// timeout
	feExternalParams = defaultFindEmbeddingExternalParams;
	feExternalParams.timeout = -1.0;
	EXPECT_THROW(find_embedding_::findEmbedding(problem, A, feExternalParams), find_embedding_::FindEmbeddingException);


	// tries
	feExternalParams = defaultFindEmbeddingExternalParams;
	feExternalParams.tries = -1;
	EXPECT_THROW(find_embedding_::findEmbedding(problem, A, feExternalParams), find_embedding_::FindEmbeddingException);


	// verbose
	feExternalParams = defaultFindEmbeddingExternalParams;
	feExternalParams.verbose = -1;
	EXPECT_THROW(find_embedding_::findEmbedding(problem, A, feExternalParams), find_embedding_::FindEmbeddingException);
	
	feExternalParams = defaultFindEmbeddingExternalParams;
	feExternalParams.verbose = 2;
	EXPECT_THROW(find_embedding_::findEmbedding(problem, A, feExternalParams), find_embedding_::FindEmbeddingException);
}

TEST(FindEmbeddingTest, EmptyQ)
{
	compressed_matrix::CompressedMatrix<int> problem;

	int M = 5;
	int N = 5;
	int L = 4;

	std::map<std::pair<int, int>, int> AMap = getChimeraAdjacency(M, N, L);
	int ASize = M * N * L * 2;
	compressed_matrix::CompressedMatrix<int> A(ASize, ASize, AMap);

	find_embedding_::FindEmbeddingExternalParams feExternalParams;
	feExternalParams.localInteractionPtr.reset(new NiceMock<MockLocalInteraction>);

	std::vector<std::vector<int> > embeddings = find_embedding_::findEmbedding(problem, A, feExternalParams);
	EXPECT_EQ(0, embeddings.size());
}

TEST(FindEmbeddingTest, EmptyA)
{
	int problemSize = 20;
	std::map<std::pair<int, int>, int> problemMap;
	for (int i = 0; i < problemSize; ++i)
		for (int j = 0; j < problemSize; ++j)
			problemMap.insert(std::make_pair(std::make_pair(i, j), 1));

	compressed_matrix::CompressedMatrix<int> problem(problemSize, problemSize, problemMap);

	compressed_matrix::CompressedMatrix<int> A;

	find_embedding_::FindEmbeddingExternalParams feExternalParams;
	feExternalParams.localInteractionPtr.reset(new NiceMock<MockLocalInteraction>);

	std::vector<std::vector<int> > embeddings = find_embedding_::findEmbedding(problem, A, feExternalParams);
	EXPECT_EQ(0, embeddings.size());
}

TEST(FindEmbeddingTest, EmptyQAndA)
{
	compressed_matrix::CompressedMatrix<int> problem;
	compressed_matrix::CompressedMatrix<int> A;

	find_embedding_::FindEmbeddingExternalParams feExternalParams;
	feExternalParams.localInteractionPtr.reset(new NiceMock<MockLocalInteraction>);

	std::vector<std::vector<int> > embeddings = find_embedding_::findEmbedding(problem, A, feExternalParams);
	EXPECT_EQ(0, embeddings.size());
}

TEST(FindEmbeddingTest, NoSolution)
{
	int problemSize = 3;
	std::map<std::pair<int, int>, int> problemMap;
	problemMap.insert(std::make_pair(std::make_pair(0, 1), 1));
	problemMap.insert(std::make_pair(std::make_pair(0, 2), 1));
	problemMap.insert(std::make_pair(std::make_pair(1, 2), 1));


	compressed_matrix::CompressedMatrix<int> problem(problemSize, problemSize, problemMap);

	std::map<std::pair<int, int>, int> AMap;
	int answerSize = 3;

	AMap.insert(std::make_pair(std::make_pair(0, 1), 1));
	AMap.insert(std::make_pair(std::make_pair(1, 2), 1));
	compressed_matrix::CompressedMatrix<int> A(answerSize, answerSize, AMap);


	find_embedding_::FindEmbeddingExternalParams feExternalParams;
	feExternalParams.localInteractionPtr.reset(new NiceMock<MockLocalInteraction>);

	std::vector<std::vector<int> > embeddings = find_embedding_::findEmbedding(problem, A, feExternalParams);

	EXPECT_EQ(0, embeddings.size());
}

//K9 into C2
TEST(FindEmbeddingTest, K9IntoC2)
{
	//creating a K9 complete graph
	int problemSize = 9;
	std::map<std::pair<int, int>, int> problemMap;
	for (int i = 0; i < problemSize; i++) {
		for (int j = 0; j < problemSize; j++) {
			if (i != j) {
				problemMap.insert(std::make_pair(std::make_pair(i, j), 1));
			}
		}
	}
	compressed_matrix::CompressedMatrix<int> problem(problemSize, problemSize, problemMap);

	//creating a C2 graph
	int M = 2;
	int N = 2;
	int L = 4;
	std::map<std::pair<int, int>, int> AMap = getChimeraAdjacency(M, N, L);
	int ASize = M * N * L * 2;
	compressed_matrix::CompressedMatrix<int> A(ASize, ASize, AMap);

	find_embedding_::FindEmbeddingExternalParams feExternalParams;
	feExternalParams.localInteractionPtr.reset(new NiceMock<MockLocalInteraction>);

	vector<vector<int> > embeddings = find_embedding_::findEmbedding(problem, A, feExternalParams);
	EXPECT_EQ(problemSize, embeddings.size());

	EXPECT_TRUE(AssertValidFindEmbeddingResult(problem, A, embeddings));
}

TEST(FindEmbeddingTest, QSimpleUnConnectedGraph)
{
	int problemSize = 3;
	std::map<std::pair<int, int>, int> problemMap;
	problemMap.insert(std::make_pair(std::make_pair(0, 0), 0));
	problemMap.insert(std::make_pair(std::make_pair(1, 1), 0));
	problemMap.insert(std::make_pair(std::make_pair(2, 2), 0));

	compressed_matrix::CompressedMatrix<int> problem(problemSize, problemSize, problemMap);

	std::map<std::pair<int, int>, int> AMap;
	int answerSize = 4;
	AMap.insert(std::make_pair(std::make_pair(0, 1), 1));
	AMap.insert(std::make_pair(std::make_pair(0, 2), 1));
	AMap.insert(std::make_pair(std::make_pair(1, 2), 1));
	AMap.insert(std::make_pair(std::make_pair(2, 3), 1));
	AMap.insert(std::make_pair(std::make_pair(3, 0), 1));
	AMap.insert(std::make_pair(std::make_pair(3, 1), 1));

	compressed_matrix::CompressedMatrix<int> A(answerSize, answerSize, AMap);

	find_embedding_::FindEmbeddingExternalParams feExternalParams;
	feExternalParams.localInteractionPtr.reset(new NiceMock<MockLocalInteraction>);

	vector<vector<int> > embeddings = find_embedding_::findEmbedding(problem, A, feExternalParams);
	EXPECT_EQ(3, embeddings.size());

	EXPECT_TRUE(AssertValidFindEmbeddingResult(problem, A, embeddings));
}

TEST(FindEmbeddingTest, NoEdgesAnywhere) {
	int problemSize = 3;
	std::map<std::pair<int, int>, int> problemMap;
	problemMap.insert(std::make_pair(std::make_pair(0, 0), 0));
	problemMap.insert(std::make_pair(std::make_pair(1, 1), 0));
	problemMap.insert(std::make_pair(std::make_pair(2, 2), 0));

	compressed_matrix::CompressedMatrix<int> problem(problemSize, problemSize, problemMap);

	std::map<std::pair<int, int>, int> AMap;
	int answerSize = 3;
	AMap.insert(std::make_pair(std::make_pair(0, 0), 1));
	AMap.insert(std::make_pair(std::make_pair(1, 1), 1));
	AMap.insert(std::make_pair(std::make_pair(2, 2), 1));
	compressed_matrix::CompressedMatrix<int> A(answerSize, answerSize, AMap);

	find_embedding_::FindEmbeddingExternalParams feExternalParams;
	feExternalParams.localInteractionPtr.reset(new NiceMock<MockLocalInteraction>);

	vector<vector<int> > embeddings = find_embedding_::findEmbedding(problem, A, feExternalParams);
	EXPECT_EQ(3, embeddings.size());

	EXPECT_TRUE(AssertValidFindEmbeddingResult(problem, A, embeddings));
}

TEST(FindEmbeddingTest, InvalidEmbeddings)
{
	int problemSize = 6;
	std::map<std::pair<int, int>, int> problemMap;
	problemMap.insert(std::make_pair(std::make_pair(0, 1), 0));
	problemMap.insert(std::make_pair(std::make_pair(1, 2), 0));
	problemMap.insert(std::make_pair(std::make_pair(2, 0), 0));
	problemMap.insert(std::make_pair(std::make_pair(5, 4), 0));

	compressed_matrix::CompressedMatrix<int> problem(problemSize, problemSize, problemMap);

	std::map<std::pair<int, int>, int> AMap;
	int answerSize = 4;
	AMap.insert(std::make_pair(std::make_pair(0, 1), 1));
	AMap.insert(std::make_pair(std::make_pair(0, 2), 1));
	AMap.insert(std::make_pair(std::make_pair(1, 2), 1));
	AMap.insert(std::make_pair(std::make_pair(2, 3), 1));
	AMap.insert(std::make_pair(std::make_pair(3, 0), 1));
	AMap.insert(std::make_pair(std::make_pair(3, 1), 1));

	compressed_matrix::CompressedMatrix<int> A(answerSize, answerSize, AMap);

	find_embedding_::FindEmbeddingExternalParams feExternalParams;
	feExternalParams.localInteractionPtr.reset(new NiceMock<MockLocalInteraction>);

	vector<vector<int> > embeddings = find_embedding_::findEmbedding(problem, A, feExternalParams);
	EXPECT_EQ(0, embeddings.size());
}

TEST(FindEmbeddingTest, SAPI2010) {
  map<pair<int, int>, int> qMap;
  qMap.insert(make_pair(make_pair(1, 5), 1));
  compressed_matrix::CompressedMatrix<int> qCm(6, 6, qMap);

  map<pair<int, int>, int> aMap;
  aMap.insert(make_pair(make_pair(0, 1), 1));
  aMap.insert(make_pair(make_pair(1, 2), 1));
  aMap.insert(make_pair(make_pair(2, 3), 1));
  aMap.insert(make_pair(make_pair(3, 4), 1));
  aMap.insert(make_pair(make_pair(4, 0), 1));
  aMap.insert(make_pair(make_pair(1, 4), 1));
  compressed_matrix::CompressedMatrix<int> aCm(5, 5, aMap);

  find_embedding_::FindEmbeddingExternalParams params;
  params.localInteractionPtr.reset(new NiceMock<MockLocalInteraction>);

  vector<vector<int> > embs = find_embedding_::findEmbedding(qCm, aCm, params);
  EXPECT_TRUE(embs.empty());
}

TEST(FindEmbeddingTest, VerboseEarlyBail) {
	map<pair<int, int>, int> qMap;
	qMap.insert(make_pair(make_pair(0, 1), 1));
	qMap.insert(make_pair(make_pair(0, 2), 1));
	qMap.insert(make_pair(make_pair(1, 2), 1));
	compressed_matrix::CompressedMatrix<int> qCm(3, 3, qMap);

	map<pair<int, int>, int> aMap;
	aMap.insert(make_pair(make_pair(2, 3), 1));
	compressed_matrix::CompressedMatrix<int> aCm(4, 4, aMap);

	auto li = shared_ptr<MockLocalInteraction>(new MockLocalInteraction);
	EXPECT_CALL(*li, displayOutputImpl(AllOf(
		StartsWith("Failed to find embedding. Current component has "),
		EndsWith("\n")
	))).Times(1);

	find_embedding_::FindEmbeddingExternalParams params;
	params.verbose = 1;
	params.localInteractionPtr = li;

	vector<vector<int> > embs = find_embedding_::findEmbedding(qCm, aCm, params);
	EXPECT_TRUE(embs.empty());
}
