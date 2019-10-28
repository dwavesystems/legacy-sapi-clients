//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <map>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <dwave_sapi.h>

using std::pair;
using std::make_pair;
using std::map;
using std::vector;


TEST(EmbedProblemTest, UnusableVar) {
  auto problem = sapi_Problem{0, 0};

  auto embeddingData = vector<int>{0, 0};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adj = sapi_Problem{0, 0};

  sapi_EmbedProblemResult* r;

  EXPECT_EQ(SAPI_ERR_INVALID_PARAMETER, sapi_embedProblem(&problem, &embeddings, &adj, false, false, 0, &r, 0));
}


TEST(EmbedProblemTest, BadChain) {
  auto problemData = vector<sapi_ProblemEntry>{{0, 1, 1.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto embeddingData = vector<int>{0, 1, 0};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{{0, 1, 0.0}, {1, 2, 0.0}};
  auto adj = sapi_Problem{adjData.data(), adjData.size()};

  sapi_EmbedProblemResult* r;

  EXPECT_EQ(SAPI_ERR_INVALID_PARAMETER, sapi_embedProblem(&problem, &embeddings, &adj, false, false, 0, &r, 0));
}


TEST(EmbedProblemTest, Nonadjacent) {
  auto problemData = vector<sapi_ProblemEntry>{{0, 1, 1.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto embeddingData = vector<int>{0, 0, -1, 1, 1};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{{0, 1, 0.0}, {1, 2, 0.0}, {2, 3, 0.0}, {3, 4, 0.0}};
  auto adj = sapi_Problem{adjData.data(), adjData.size()};

  sapi_EmbedProblemResult* r;

  EXPECT_EQ(SAPI_ERR_INVALID_PARAMETER, sapi_embedProblem(&problem, &embeddings, &adj, false, false, 0, &r, 0));
}


TEST(EmbedProblemTest, HIndexTooLarge) {
  auto problemData = vector<sapi_ProblemEntry>{{0, 0, 1.0}, {1, 1, 2.0}, {2, 2, 3.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto embeddingData = vector<int>{0, 0, 1};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{{0, 1, 0.0}, {1, 2, 0.0}};
  auto adj = sapi_Problem{adjData.data(), adjData.size()};

  sapi_EmbedProblemResult* r;

  EXPECT_EQ(SAPI_ERR_INVALID_PARAMETER, sapi_embedProblem(&problem, &embeddings, &adj, false, false, 0, &r, 0));
}


TEST(EmbedProblemTest, JIndexTooLarge) {
  auto problemData = vector<sapi_ProblemEntry>{{0, 1, -1.0}, {1, 2, -2.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto embeddingData = vector<int>{0, 0, 1};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{{0, 1, 0.0}, {1, 2, 0.0}};
  auto adj = sapi_Problem{adjData.data(), adjData.size()};

  sapi_EmbedProblemResult* r;

  EXPECT_EQ(SAPI_ERR_INVALID_PARAMETER, sapi_embedProblem(&problem, &embeddings, &adj, false, false, 0, &r, 0));
}


TEST(EmbedProblemTest, BadHJRange) {
  auto problemData = vector<sapi_ProblemEntry>{{0, 1, -1.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto embeddingData = vector<int>{0, 1};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{{0, 1, 0.0}};
  auto adj = sapi_Problem{adjData.data(), adjData.size()};

  sapi_IsingRangeProperties ranges;
  sapi_EmbedProblemResult* r;

  ranges.h_min = 0;
  ranges.h_max = 1;
  ranges.j_min = -1;
  ranges.j_max = 1;
  EXPECT_EQ(SAPI_ERR_INVALID_PARAMETER, sapi_embedProblem(&problem, &embeddings, &adj, false, true, &ranges, &r, 0));

  ranges.h_min = -1;
  ranges.h_max = 0;
  ranges.j_min = -1;
  ranges.j_max = 1;
  EXPECT_EQ(SAPI_ERR_INVALID_PARAMETER, sapi_embedProblem(&problem, &embeddings, &adj, false, true, &ranges, &r, 0));

  ranges.h_min = -1;
  ranges.h_max = 1;
  ranges.j_min = 0;
  ranges.j_max = 1;
  EXPECT_EQ(SAPI_ERR_INVALID_PARAMETER, sapi_embedProblem(&problem, &embeddings, &adj, false, true, &ranges, &r, 0));

  ranges.h_min = -1;
  ranges.h_max = 1;
  ranges.j_min = -1;
  ranges.j_max = 0;
  EXPECT_EQ(SAPI_ERR_INVALID_PARAMETER, sapi_embedProblem(&problem, &embeddings, &adj, false, true, &ranges, &r, 0));
}


TEST(EmbedProblemTest, Trivial) {
  auto problem = sapi_Problem{0, 0};
  auto embeddings = sapi_Embeddings{0, 0};
  auto adj = sapi_Problem{0, 0};

  sapi_EmbedProblemResult* r;

  ASSERT_EQ(SAPI_OK, sapi_embedProblem(&problem, &embeddings, &adj, false, false, 0, &r, 0));
  EXPECT_EQ(r->problem.len, 0);
  EXPECT_EQ(r->jc.len, 0);
  EXPECT_EQ(r->embeddings.len, 0);
  sapi_freeEmbedProblemResult(r);

  ASSERT_EQ(SAPI_OK, sapi_embedProblem(&problem, &embeddings, &adj, true, false, 0, &r, 0));
  EXPECT_EQ(r->problem.len, 0);
  EXPECT_EQ(r->jc.len, 0);
  EXPECT_EQ(r->embeddings.len, 0);
  sapi_freeEmbedProblemResult(r);

  ASSERT_EQ(SAPI_OK, sapi_embedProblem(&problem, &embeddings, &adj, false, true, 0, &r, 0));
  EXPECT_EQ(r->problem.len, 0);
  EXPECT_EQ(r->jc.len, 0);
  EXPECT_EQ(r->embeddings.len, 0);
  sapi_freeEmbedProblemResult(r);

  ASSERT_EQ(SAPI_OK, sapi_embedProblem(&problem, &embeddings, &adj, true, true, 0, &r, 0));
  EXPECT_EQ(r->problem.len, 0);
  EXPECT_EQ(r->jc.len, 0);
  EXPECT_EQ(r->embeddings.len, 0);
  sapi_freeEmbedProblemResult(r);
}


TEST(EmbedProblemTest, EmptyProblemNonemptyEmbedding) {
  auto problem = sapi_Problem{0, 0};

  auto embeddingData = vector<int>{0, 0};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{{0, 1, 0.0}};
  auto adj = sapi_Problem{adjData.data(), adjData.size()};

  sapi_EmbedProblemResult* r;

  ASSERT_EQ(SAPI_OK, sapi_embedProblem(&problem, &embeddings, &adj, false, false, 0, &r, 0));
  EXPECT_EQ(r->problem.len, 0);

  ASSERT_EQ(1, r->jc.len);
  EXPECT_EQ(0, r->jc.elements[0].i);
  EXPECT_EQ(1, r->jc.elements[0].j);
  EXPECT_EQ(-1.0, r->jc.elements[0].value);

  auto embVec = vector<int>(r->embeddings.elements, r->embeddings.elements + r->embeddings.len);
  EXPECT_EQ(embeddingData, embVec);

  sapi_freeEmbedProblemResult(r);
}


TEST(EmbedProblemTest, Typical) {
  auto problemData = vector<sapi_ProblemEntry>{
    {0, 0, 1.0}, {1, 1, 10.0}, {0, 1, 15.0}, {2, 1, -8.0}, {0, 2, 5.0}, {2, 0, -2.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto embeddingData = vector<int>{2, 0, 1, 1};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{
    {0, 1, 0.0}, {1, 2, 0.0}, {2, 3, 0.0}, {3, 0, 0.0}, {2, 0, 0.0}};
  auto adj = sapi_Problem{adjData.data(), adjData.size()};

  sapi_EmbedProblemResult* r;

  auto expectedProblem = map<pair<int, int>, double>{
    {make_pair(1, 1), 1.0}, {make_pair(2, 2), 5.0}, {make_pair(3, 3), 5.0},
    {make_pair(0, 1), 3.0}, {make_pair(0, 2), -4.0}, {make_pair(0, 3), -4.0}, {make_pair(1, 2), 15.0}
  };

  ASSERT_EQ(SAPI_OK, sapi_embedProblem(&problem, &embeddings, &adj, false, false, 0, &r, 0));

  auto embProblem = map<pair<int, int>, double>{};
  for (size_t i = 0; i < r->problem.len; ++i) {
    auto pe = r->problem.elements + i;
    embProblem[make_pair(pe->i, pe->j)] = pe->value;
  }
  EXPECT_EQ(expectedProblem, embProblem);

  ASSERT_EQ(1, r->jc.len);
  EXPECT_EQ(2, r->jc.elements->i);
  EXPECT_EQ(3, r->jc.elements->j);
  EXPECT_EQ(-1.0, r->jc.elements->value);

  auto embVec = vector<int>(r->embeddings.elements, r->embeddings.elements + r->embeddings.len);
  EXPECT_EQ(embeddingData, embVec);

  sapi_freeEmbedProblemResult(r);
}


TEST(EmbedProblemTest, Clean) {
  auto problemData = vector<sapi_ProblemEntry>{
    {0, 0, -2.0}, {1, 1, 4.0}, {2, 2, -5.0}, {3, 3, 14.0},
    {0, 1, 2.0}, {2, 1, -3.0}, {2, 3, -18.0}, {3, 0, -7.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto embeddingData = vector<int>{2, 1, 2, 3, 0, 3, 3, 0, 3, 1};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{
    {0, 2, 0.0}, {2, 9, 0.0}, {9, 1, 0.0}, {1, 7, 0.0}, {7, 4, 0.0},
    {4, 3, 0.0}, {3, 5, 0.0}, {5, 2, 0.0}, {0, 7, 0.0}, {4, 9, 0.0},
    {3, 2, 0.0}, {5, 8, 0.0}, {8, 6, 0.0}, {6, 1, 0.0}, {8, 9, 0.0}
  };
  auto adj = sapi_Problem{adjData.data(), adjData.size()};

  sapi_EmbedProblemResult* r;

  auto expectedProblem = map<pair<int, int>, double>{
    {make_pair(1, 1), 2}, {make_pair(2, 2), -5}, {make_pair(3, 3), 7}, {make_pair(4, 4), -1},
    {make_pair(5, 5), 7}, {make_pair(7, 7), -1}, {make_pair(9, 9), 2}, {make_pair(1, 7), 1},
    {make_pair(4, 9), 1}, {make_pair(2, 9), -3}, {make_pair(2, 3), -9}, {make_pair(2, 5), -9},
    {make_pair(3, 4), -7}
  };
  auto expectedJc = map<pair<int, int>, double>{
    {make_pair(4, 7), -1}, {make_pair(1, 9), -1}, {make_pair(3, 5), -1}
  };
  auto expectedEmb = vector<int>{-1, 1, 2, 3, 0, 3, -1, 0, -1, 1};

  ASSERT_EQ(SAPI_OK, sapi_embedProblem(&problem, &embeddings, &adj, true, false, 0, &r, 0));

  auto embProblem = map<pair<int, int>, double>{};
  for (size_t i = 0; i < r->problem.len; ++i) {
    auto pe = r->problem.elements + i;
    embProblem[make_pair(pe->i, pe->j)] = pe->value;
  }
  EXPECT_EQ(expectedProblem, embProblem);

  auto embJc = map<pair<int, int>, double>{};
  for (size_t i = 0; i < r->jc.len; ++i) {
    auto pe = r->jc.elements + i;
    embJc[make_pair(pe->i, pe->j)] = pe->value;
  }
  EXPECT_EQ(expectedJc, embJc);

  auto embVec = vector<int>(r->embeddings.elements, r->embeddings.elements + r->embeddings.len);
  EXPECT_EQ(expectedEmb, embVec);

  sapi_freeEmbedProblemResult(r);
}


TEST(EmbedProblemTest, CleanUnUsedVariables) {
  auto problemData = vector<sapi_ProblemEntry>{
    {2, 2, -1.0}, {6, 6, 3.0}, {7, 7, -2.0},
    {1, 2, 1.0}, {1, 3, 2.0}, {2, 7, 1.0}, {2, 3, -2.0}, {3, 7, -1.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};
  auto embeddingData = vector<int>{4, 4, 4, 3, 3, 1, 1, 1, 6, 6, 0, 0, 0, 2, 2, 2, 5, 5, 5, 7, 7, 6, 6};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{
    {0, 1, 0.0}, {0, 2, 0.0}, {1, 2, 0.0}, {2, 3, 0.0}, {3, 4, 0.0}, {4, 5, 0.0}, {5, 6, 0.0}, {6, 7, 0.0},
    {7, 10, 0.0}, {10, 11, 0.0}, {11, 12, 0.0}, {12, 10, 0.0}, {10, 9, 0.0}, {9, 8, 0.0}, {8, 21, 0.0}, {21, 22, 0.0},
    {22, 8, 0.0}, {7, 14, 0.0}, {14, 15, 0.0}, {14, 13, 0.0}, {14, 16, 0.0}, {16, 17, 0.0}, {17, 18, 0.0}, {18, 16, 0.0},
    {15, 3, 0.0}, {15, 19, 0.0}, {19, 20, 0.0}, {20, 3, 0.0}
  };
  auto adj = sapi_Problem{adjData.data(), adjData.size()};
  sapi_EmbedProblemResult* r;

  auto expectedProblem = map<pair<int, int>, double>{
    {make_pair(3, 15), -2}, {make_pair(3, 20), -1}, {make_pair(4, 5), 2}, {make_pair(7, 14), 1},
    {make_pair(8, 8), 1}, {make_pair(14, 14), -0.5}, {make_pair(15, 15), -0.5}, {make_pair(15, 19), 1},
    {make_pair(19, 19), -1}, {make_pair(20, 20), -1}, {make_pair(21, 21), 1}, {make_pair(22, 22), 1}
  };

  auto expectedJc = map<pair<int, int>, double>{
    {make_pair(3, 4), -1}, {make_pair(5, 6), -1}, {make_pair(6, 7), -1}, {make_pair(14, 15), -1},
    {make_pair(19, 20), -1}, {make_pair(8, 21), -1}, {make_pair(8, 22), -1}, {make_pair(21, 22), -1}
  };
  auto expectedEmb = vector<int>{4, -1, -1, 3, 3, 1, 1, 1, 6, -1, 0, -1, -1, -1, 2, 2, 5, -1, -1, 7, 7, 6, 6};
  EXPECT_EQ(SAPI_OK, sapi_embedProblem(&problem, &embeddings, &adj, true, false, 0, &r, 0));

  auto embProblem = map<pair<int, int>, double>{};
  for (size_t i = 0; i < r->problem.len; ++i) {
    auto pe = r->problem.elements + i;
    embProblem[make_pair(pe->i, pe->j)] = pe->value;
  }
  EXPECT_EQ(expectedProblem, embProblem);

  auto embJc = map<pair<int, int>, double>{};
  for (size_t i = 0; i < r->jc.len; ++i) {
    auto pe = r->jc.elements + i;
    embJc[make_pair(pe->i, pe->j)] = pe->value;
  }
  EXPECT_EQ(expectedJc, embJc);

  auto embVec = vector<int>(r->embeddings.elements, r->embeddings.elements + r->embeddings.len);
  EXPECT_EQ(expectedEmb, embVec);

  sapi_freeEmbedProblemResult(r);
}


TEST(EmbedProblemTest, CleanAllUnused) {
  auto problemData = vector<sapi_ProblemEntry>{};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};
  auto embeddingData = vector<int>{0, 0, 0};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{{0, 1, 0.0}, {1, 2, 0.0}, {2, 0, 0.0}};
  auto adj = sapi_Problem{adjData.data(), adjData.size()};
  sapi_EmbedProblemResult* r;

  ASSERT_EQ(SAPI_OK, sapi_embedProblem(&problem, &embeddings, &adj, true, false, 0, &r, 0));
  EXPECT_EQ(0, r->problem.len);
  EXPECT_EQ(0, r->jc.len);

  auto expectedEmb = vector<int>{0, -1, -1};
  auto embVec = vector<int>(r->embeddings.elements, r->embeddings.elements + r->embeddings.len);
  EXPECT_EQ(expectedEmb, embVec);

  sapi_freeEmbedProblemResult(r);
}


TEST(EmbedProblemTest, CleanDisconnected) {
  auto problemData = vector<sapi_ProblemEntry>{{0, 0, 7.0}, {1, 1, -5.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto embeddingData = vector<int>{1, 1, 1, -1, 0, 0};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{
    {0, 1, 0.0}, {1, 2, 0.0}, {2, 4, 0.0}, {4, 5, 0.0}, {5, 0, 0.0}
  };
  auto adj = sapi_Problem{adjData.data(), adjData.size()};

  sapi_EmbedProblemResult* r;
  ASSERT_EQ(SAPI_OK, sapi_embedProblem(&problem, &embeddings, &adj, true, false, 0, &r, 0));

  EXPECT_EQ(0, r->jc.len);

  auto embCount = map<int, int>{};
  auto e0i = -1;
  auto e1i = -1;
  for (size_t i = 0; i < r->embeddings.len; ++i) {
    ++embCount[r->embeddings.elements[i]];
    switch (r->embeddings.elements[i]) {
      case 0: e0i = i; break;
      case 1: e1i = i; break;
    }
  }
  auto expectedEmbCount = map<int, int>{{-1, 4}, {0, 1}, {1, 1}};
  EXPECT_EQ(expectedEmbCount, embCount);
  EXPECT_TRUE(e0i == 4 || e0i == 5);
  EXPECT_TRUE(e1i == 0 || e1i == 1 || e1i == 2);

  auto embProblem = map<pair<int, int>, double>{};
  for (size_t i = 0; i < r->problem.len; ++i) {
    auto pe = r->problem.elements + i;
    embProblem[make_pair(pe->i, pe->j)] = pe->value;
  }
  auto expectedProblem = map<pair<int, int>, double>{{make_pair(e0i, e0i), 7.0}, {make_pair(e1i, e1i), -5.0}};
  EXPECT_EQ(expectedProblem, embProblem);

  sapi_freeEmbedProblemResult(r);
}


TEST(EmbedProblemTest, SmearJPos) {
  auto problemData = vector<sapi_ProblemEntry>{
    {0, 0, -6.0}, {1, 1, -2.0}, {2, 2, 8.0},
    {0, 1, 3.0}, {1, 2, 1.0}, {2, 0, -6.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto embeddingData = vector<int>{0, 1, 2};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{
    {0, 1, 0.0}, {0, 2, 0.0}, {1, 2, 0.0}, {3, 0, 0.0}, {4, 3, 0.0}, {5, 1, 0.0}, {5, 6, 0.0},
    {2, 7, 0.0}, {2, 8, 0.0}, {7, 8, 0.0}, {7, 9, 0.0}, {8, 9, 0.0}, {0, 7, 0.0}};
  auto adj = sapi_Problem{adjData.data(), adjData.size()};

  auto ranges = sapi_IsingRangeProperties{-4, 1, -6, 3};

  sapi_EmbedProblemResult* r;

  auto expectedProblem = map<pair<int, int>, double>{
    {make_pair(0, 0), -3.0}, {make_pair(1, 1), -2.0}, {make_pair(2, 2), 2.0}, {make_pair(3, 3), -3.0},
    {make_pair(7, 7), 2.0}, {make_pair(8, 8), 2.0}, {make_pair(9, 9), 2.0}, {make_pair(0, 1), 3.0},
    {make_pair(1, 2), 1.0}, {make_pair(0, 2), -3.0}, {make_pair(0, 7), -3.0}
  };

  auto expectedJc = map<pair<int, int>, double>{
    {make_pair(0, 3), -1.0}, {make_pair(2, 7), -1.0}, {make_pair(2, 8), -1.0},
    {make_pair(7, 8), -1.0}, {make_pair(7, 9), -1.0}, {make_pair(8, 9), -1.0},
  };

  auto expectedEmb = vector<int>{0, 1, 2, 0, -1, -1, -1, 2, 2, 2};

  ASSERT_EQ(SAPI_OK, sapi_embedProblem(&problem, &embeddings, &adj, false, true, &ranges, &r, 0));

  auto embProblem = map<pair<int, int>, double>{};
  for (size_t i = 0; i < r->problem.len; ++i) {
    auto pe = r->problem.elements + i;
    embProblem[make_pair(pe->i, pe->j)] = pe->value;
  }
  EXPECT_EQ(expectedProblem, embProblem);

  auto embJc = map<pair<int, int>, double>{};
  for (size_t i = 0; i < r->jc.len; ++i) {
    auto pe = r->jc.elements + i;
    embJc[make_pair(pe->i, pe->j)] = pe->value;
  }
  EXPECT_EQ(expectedJc, embJc);

  auto embVec = vector<int>(r->embeddings.elements, r->embeddings.elements + r->embeddings.len);
  EXPECT_EQ(expectedEmb, embVec);

  sapi_freeEmbedProblemResult(r);
}


TEST(EmbedProblemTest, SmearJNeg) {
  auto problemData = vector<sapi_ProblemEntry>{
    {0, 0, 80.0}, {1, 1, 100.0}, {2, 2, -12.0},
    {0, 1, -60.0}, {1, 2, 8.0}, {3, 2, 1.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};

  auto embeddingData = vector<int>{0, 1, 2, 3};
  auto embeddings = sapi_Embeddings{embeddingData.data(), embeddingData.size()};

  auto adjData = vector<sapi_ProblemEntry>{
    {0, 1, 0.0}, {1, 2, 0.0}, {2, 3, 0.0}, {0, 4, 0.0}, {0, 5, 0.0}, {0, 6, 0.0},
    {1, 4, 0.0}, {1, 5, 0.0}, {1, 6, 0.0}, {2, 4, 0.0}, {2, 5, 0.0}, {2, 6, 0.0}
  };
  auto adj = sapi_Problem{adjData.data(), adjData.size()};

  auto ranges = sapi_IsingRangeProperties{-1, 1, -10, 12};

  sapi_EmbedProblemResult* r;

  auto expectedProblem = map<pair<int, int>, double>{
    {make_pair(0, 0), 80.0}, {make_pair(1, 1), 25.0}, {make_pair(2, 2), -12.0}, {make_pair(4, 4), 25.0},
    {make_pair(5, 5), 25.0}, {make_pair(6, 6), 25.0}, {make_pair(0, 1), -15.0}, {make_pair(0, 4), -15.0},
    {make_pair(0, 5), -15.0}, {make_pair(0, 6), -15.0}, {make_pair(1, 2), 2.0}, {make_pair(2, 4), 2.0},
    {make_pair(2, 5), 2.0}, {make_pair(2, 6), 2.0}, {make_pair(2, 3), 1.0}
  };

  auto expectedJc = map<pair<int, int>, double>{
    {make_pair(1, 4), -1.0}, {make_pair(1, 5), -1.0}, {make_pair(1, 6), -1.0}
  };

  auto expectedEmb = vector<int>{0, 1, 2, 3, 1, 1, 1};

  ASSERT_EQ(SAPI_OK, sapi_embedProblem(&problem, &embeddings, &adj, false, true, &ranges, &r, 0));

  auto embProblem = map<pair<int, int>, double>{};
  for (size_t i = 0; i < r->problem.len; ++i) {
    auto pe = r->problem.elements + i;
    embProblem[make_pair(pe->i, pe->j)] = pe->value;
  }
  EXPECT_EQ(expectedProblem, embProblem);

  auto embJc = map<pair<int, int>, double>{};
  for (size_t i = 0; i < r->jc.len; ++i) {
    auto pe = r->jc.elements + i;
    embJc[make_pair(pe->i, pe->j)] = pe->value;
  }
  EXPECT_EQ(expectedJc, embJc);

  auto embVec = vector<int>(r->embeddings.elements, r->embeddings.elements + r->embeddings.len);
  EXPECT_EQ(expectedEmb, embVec);

  sapi_freeEmbedProblemResult(r);
}
