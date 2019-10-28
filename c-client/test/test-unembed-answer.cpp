//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <dwave_sapi.h>

using std::vector;


TEST(UnembedAnswerTest, Trivial) {
  const auto solutions = vector<int>{};
  const auto embeddings = sapi_Embeddings{0, 0};
  const auto problem = sapi_Problem{0, 0};
  auto newSolutions = vector<int>{};
  size_t numNewSolutions;

  EXPECT_EQ(SAPI_OK, sapi_unembedAnswer(solutions.data(), 0, 0, &embeddings, SAPI_BROKEN_CHAINS_DISCARD,
    0, newSolutions.data(), &numNewSolutions, 0));
  EXPECT_EQ(SAPI_OK, sapi_unembedAnswer(solutions.data(), 0, 0, &embeddings, SAPI_BROKEN_CHAINS_VOTE,
    0, newSolutions.data(), &numNewSolutions, 0));
  EXPECT_EQ(SAPI_OK, sapi_unembedAnswer(solutions.data(), 0, 0, &embeddings, SAPI_BROKEN_CHAINS_WEIGHTED_RANDOM,
    0, newSolutions.data(), &numNewSolutions, 0));
  EXPECT_EQ(SAPI_OK, sapi_unembedAnswer(solutions.data(), 0, 0, &embeddings, SAPI_BROKEN_CHAINS_MINIMIZE_ENERGY,
    &problem, newSolutions.data(), &numNewSolutions, 0));
}



TEST(UnembedAnswerTest, Discard) {
  const auto solutions = vector<int>{
    +1, +1, +1, +1, +3, +1, +1,
    +1, +1, -1, +1, +1, -1, +1,
    +1, +1, -1, +1, +1, -1, -1,
    +1, -1, -1, +1, +1, -1, -1,
    +1, -1, -1, -1, +1, -1, -1,
    -1, -1, -1, -1, 33, -1, -1};
  const auto solutionLen = 7;
  const auto numSolutions = 6;

  auto embeddingsData = vector<int>{1, 2, 0, 2, -1, 0, 0};
  auto embeddings = sapi_Embeddings{embeddingsData.data(), embeddingsData.size()};
  auto newSolutions = vector<int>(3 * numSolutions, -999);
  size_t numNewSolutions = 999;

  const auto expectedSolutions = vector<int>{
      +1,   +1,   +1,
      -1,   +1,   +1,
      -1,   +1,   -1,
      -1,   -1,   -1,
    -999, -999, -999,
    -999, -999, -999
  };

  ASSERT_EQ(SAPI_OK, sapi_unembedAnswer(solutions.data(), solutionLen, numSolutions, &embeddings,
    SAPI_BROKEN_CHAINS_DISCARD, 0, newSolutions.data(), &numNewSolutions, 0));

  EXPECT_EQ(4, numNewSolutions);
  EXPECT_EQ(expectedSolutions, newSolutions);
}



TEST(UnembedAnswerTest, DiscardAll) {
  const auto solutions = vector<int>{-1, 1, -1, 1, 1, -1};
  const auto solutionLen = 2;
  const auto numSolutions = 3;

  auto embeddingsData = vector<int>{0, 0};
  auto embeddings = sapi_Embeddings{embeddingsData.data(), embeddingsData.size()};
  auto newSolutions = vector<int>(numSolutions, -999);
  size_t numNewSolutions = 999;

  ASSERT_EQ(SAPI_OK, sapi_unembedAnswer(solutions.data(), solutionLen, numSolutions, &embeddings,
    SAPI_BROKEN_CHAINS_DISCARD, 0, newSolutions.data(), &numNewSolutions, 0));

  EXPECT_EQ(0, numNewSolutions);
}



TEST(UnembedAnswerTest, VoteNoTies) {
  const auto solutions = vector<int>{
    3, +1, +1, -1, -1, -1,
    3, -1, -1, -1, +1, +1,
    3, -1, -1, +1, +1, -1,
    3, +1, +1, -1, +1, -1
  };
  const auto solutionLen = 6;
  const auto numSolutions = 4;

  auto embeddingsData = vector<int>{-1, 0, 0, 1, 1, 1};
  auto embeddings = sapi_Embeddings{embeddingsData.data(), embeddingsData.size()};
  auto newSolutions = vector<int>(2 * numSolutions, -999);
  size_t numNewSolutions = 999;

  const auto expectedSolutions = vector<int>{1, -1,   -1, 1,   -1, 1,   1, -1};

  ASSERT_EQ(SAPI_OK, sapi_unembedAnswer(solutions.data(), solutionLen, numSolutions, &embeddings,
    SAPI_BROKEN_CHAINS_VOTE, 0, newSolutions.data(), &numNewSolutions, 0));

  EXPECT_EQ(numSolutions, numNewSolutions);
  EXPECT_EQ(expectedSolutions, newSolutions);
}



TEST(UnembedAnswerTest, VoteTies) {
  const auto solutions = vector<int>{
    +1, +1, -1, 3, -1,
    -1, +1, +1, 3, -1,
    +1, -1, -1, 3, +1,
    -1, -1, +1, 3, +1
  };
  const auto solutionLen = 5;
  const auto numSolutions = 4;

  auto embeddingsData = vector<int>{0, 1, 0, -1, 1};
  auto embeddings = sapi_Embeddings{embeddingsData.data(), embeddingsData.size()};
  auto newSolutions = vector<int>(2 * numSolutions, -999);
  size_t numNewSolutions = 999;

  ASSERT_EQ(SAPI_OK, sapi_unembedAnswer(solutions.data(), solutionLen, numSolutions, &embeddings,
    SAPI_BROKEN_CHAINS_VOTE, 0, newSolutions.data(), &numNewSolutions, 0));

  EXPECT_EQ(numSolutions, numNewSolutions);
  for (auto i = 0u; i < newSolutions.size(); ++i) {
    EXPECT_TRUE(newSolutions[i] == 1 || newSolutions[i] == -1);
  }
}



TEST(UnembedAnswerTest, WeightedRandom) {
  const int runs = 100;
  const auto solutions = vector<int>{
    +1, -1, -1, +1, -1, -1, -1, -1, 3, 3, -1,
    -1, +1, +1, +1, -1, -1, -1, -1, 3, 3, -1
  };
  const auto solutionLen = 11;
  const auto numSolutions = 2;

  auto embeddingsData = vector<int>{0, 1, 1, 0, 1, 1, 1, 1, -1, -1, 1};
  auto embeddings = sapi_Embeddings{embeddingsData.data(), embeddingsData.size()};
  auto newSolutions = vector<int>(2 * numSolutions, -999);

  auto totals = vector<int>(newSolutions.size());
  for (auto r = 0; r < runs; ++r) {
    size_t numNewSolutions = 999;
    ASSERT_EQ(SAPI_OK, sapi_unembedAnswer(solutions.data(), solutionLen, numSolutions, &embeddings,
      SAPI_BROKEN_CHAINS_WEIGHTED_RANDOM, 0, newSolutions.data(), &numNewSolutions, 0));
    EXPECT_EQ(numSolutions, numNewSolutions);
    for (auto i = 0u; i < totals.size(); ++i) {
      totals[i] += newSolutions[i];
    }
    // this is annoying but helps the (time-based) RNG seeding on Windows
    std::this_thread::sleep_for(std::chrono::nanoseconds(1));
  }

  EXPECT_EQ(runs, totals[0]);
  EXPECT_EQ(-runs, totals[1]);
  // don't check the acutal probabilities--too unreliable and slow
  EXPECT_LT(-runs, totals[2]);
  EXPECT_GT(runs, totals[2]);
  EXPECT_LT(-runs, totals[3]);
  EXPECT_GT(runs, totals[3]);
}



TEST(UnembedAnswerTest, MinimizeEnergy) {
  const auto solutions = vector<int>{
    -1, -1, -1, -1, -1, -1, +1, +1, +1, 3, +1,
    +1, +1, +1, +1, +1, -1, +1, -1, -1, 3, -1,
    +1, +1, -1, +1, -1, -1, -1, -1, -1, 3, -1
  };
  const auto solutionLen = 11;
  const auto numSolutions = 3;

  auto embeddingsData = vector<int>{0, 1, 2, 3, 4, 0, 1, 2, 3, -1, 4};
  auto embeddings = sapi_Embeddings{embeddingsData.data(), embeddingsData.size()};

  auto problemData = vector<sapi_ProblemEntry>{
    {0, 1, -1.0}, {0, 2, 2.0}, {0, 3, 3.0}, {0, 4, -1.0}, {2, 1, -1.0}, {1, 3, 3.0},
    {3, 1, -1.0}, {1, 4, -1.0}, {2, 3, 1.0}, {4, 2, -1.0}, {2, 4, -1.0}, {3, 4, 1.0},
  };
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto newSolutions = vector<int>(5 * numSolutions, -999);
  size_t numNewSolutions = 999;

  const auto expectedSolutions = vector<int>{
    -1, -1, -1, +1, -1,
    +1, +1, +1, -1, +1,
    -1, -1, -1, +1, -1
  };

  ASSERT_EQ(SAPI_OK, sapi_unembedAnswer(solutions.data(), solutionLen, numSolutions, &embeddings,
    SAPI_BROKEN_CHAINS_MINIMIZE_ENERGY, &problem, newSolutions.data(), &numNewSolutions, 0));

  EXPECT_EQ(numSolutions, numNewSolutions);
  EXPECT_EQ(expectedSolutions, newSolutions);
}



TEST(UnembedAnswerTest, MinimizeEnergyEasy) {
  const auto solutions = vector<int>{
    -1, -1, +1, 3, -1, -1, -1,
    -1, +1, -1, 3, +1, +1, +1
  };
  const auto solutionLen = 7;
  const auto numSolutions = 2;

  auto embeddingsData = vector<int>{0, 0, 1, -1, 2, 2, 2};
  auto embeddings = sapi_Embeddings{embeddingsData.data(), embeddingsData.size()};

  auto problemData = vector<sapi_ProblemEntry>{{0, 0, -1.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto newSolutions = vector<int>(3 * numSolutions, -999);
  size_t numNewSolutions = 999;

  const auto expectedSolutions = vector<int>{-1, +1, -1,   +1, -1, +1};

  ASSERT_EQ(SAPI_OK, sapi_unembedAnswer(solutions.data(), solutionLen, numSolutions, &embeddings,
    SAPI_BROKEN_CHAINS_MINIMIZE_ENERGY, &problem, newSolutions.data(), &numNewSolutions, 0));

  EXPECT_EQ(numSolutions, numNewSolutions);
  EXPECT_EQ(expectedSolutions, newSolutions);
}



TEST(UnembedAnswerTest, MinimizeEnergyTooManyVars) {
  const auto solutions = vector<int>{1, 1};
  const auto solutionLen = 2;
  const auto numSolutions = 1;

  auto embeddingsData = vector<int>{0, 1};
  auto embeddings = sapi_Embeddings{embeddingsData.data(), embeddingsData.size()};

  auto problemData = vector<sapi_ProblemEntry>{{0, 3, -1.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto newSolutions = vector<int>(numSolutions, -999);
  size_t numNewSolutions = 999;

  ASSERT_EQ(SAPI_ERR_INVALID_PARAMETER, sapi_unembedAnswer(solutions.data(), solutionLen, numSolutions, &embeddings,
    SAPI_BROKEN_CHAINS_MINIMIZE_ENERGY, &problem, newSolutions.data(), &numNewSolutions, 0));
}



TEST(UnembedAnswerTest, MinimizeEnergySmallProbLargeEmb) {
  const auto solutions = vector<int>{1, -1, 1, -1, 1, -1, 1, -1};
  const auto solutionLen = 8;
  const auto numSolutions = 1;

  auto embeddingsData = vector<int>{0, 0, 1, 1, 2, 2, 3, 3};
  auto embeddings = sapi_Embeddings{embeddingsData.data(), embeddingsData.size()};

  auto problemData = vector<sapi_ProblemEntry>{{0, 1, -1.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto newSolutions = vector<int>(4 * numSolutions, -999);
  size_t numNewSolutions = 999;

  ASSERT_EQ(SAPI_OK, sapi_unembedAnswer(solutions.data(), solutionLen, numSolutions, &embeddings,
    SAPI_BROKEN_CHAINS_MINIMIZE_ENERGY, &problem, newSolutions.data(), &numNewSolutions, 0));

  EXPECT_EQ(numSolutions, numNewSolutions);
  for (auto i = 0u; i < newSolutions.size(); ++i) {
    EXPECT_TRUE(newSolutions[i] == 1 || newSolutions[i] == -1);
  }
}
