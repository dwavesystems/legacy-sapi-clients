//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cstddef>
#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include "test.hpp"

using std::size_t;
using std::find;
using std::make_pair;
using std::map;
using std::pair;
using std::queue;
using std::set;
using std::vector;

using testing::AssertionResult;
using testing::AssertionSuccess;
using testing::AssertionFailure;

using compressed_matrix::CompressedMatrix;

namespace {

bool inducesConnected(const CompressedMatrix<int>& graph, const vector<int>& vertices) {
  if (vertices.empty()) return true;

  map<int, vector<int> > nbrs;
  map<int, bool> seen;

  const vector<int>& ro = graph.rowOffsets();
  const vector<int>& ci = graph.colIndices();
  BOOST_FOREACH( int v, vertices ) {
    if (v >= graph.numRows()) return false;
    seen[v] = false;
    for (int i = ro[v]; i < ro[v + 1]; ++i) {
      if (find(vertices.begin(), vertices.end(), ci[i]) != vertices.end()) {
        nbrs[v].push_back(ci[i]);
        nbrs[ci[i]].push_back(v);
      }
    }
  }

  queue<int> visit;
  visit.push(vertices[0]);
  seen[vertices[0]] = true;
  size_t seenCount = 1;

  while (!visit.empty()) {
    int v = visit.front();
    visit.pop();
    BOOST_FOREACH( int u, nbrs[v] ) {
      if (!seen[u]) {
        seen[u] = true;
        ++seenCount;
        visit.push(u);
      }
    }
  }

  return seenCount == vertices.size();
}

} // namespace {anonymous}

namespace test {

AssertionResult AssertValidFindEmbeddingResult(
    const CompressedMatrix<int>& sourceGraph,
    const CompressedMatrix<int>& targetGraph,
    const vector<vector<int> >& embedding) {

  int sourceVarsI = std::max(sourceGraph.numRows(), sourceGraph.numCols());
  if (sourceVarsI < 0) {
    return AssertionFailure() << "negative source dimensions!";
  }
  size_t sourceVars = static_cast<size_t>(sourceVarsI);

  if (sourceVars > 0 && embedding.empty()) {
    return AssertionSuccess() << "no embedding found";
  }

  if (sourceVars != embedding.size()) {
    return AssertionFailure() << "source size (" << sourceVars << ") does not match embeddings size ("
      << embedding.size() << ")";
  }

  vector<int> embeddedFrom(targetGraph.numRows(), -1);

  for (size_t sv = 0; sv < embedding.size(); ++sv) {
    if (embedding[sv].empty()) return AssertionFailure() << "source vertex " << sv << " has empty embedding";

    BOOST_FOREACH( int tv, embedding[sv] ) {
      if (tv < 0 || tv >= targetGraph.numRows()) {
        return AssertionFailure() << "source vertex " << sv << " maps to invalid target vertex " << tv;
      }
      if (embeddedFrom[tv] == sv) {
        return AssertionFailure() << "target vertex " << tv << " appears multiple times for source vertex " << sv;
      } else if (embeddedFrom[tv] != -1) {
        return AssertionFailure() << "target vertex " << tv << " used by source vertices "
          << embeddedFrom[tv] << " and " << sv;
      }
      embeddedFrom[tv] = sv;
    }

    if (!inducesConnected(targetGraph, embedding[sv])) {
      return AssertionFailure() << "embedding of source vertex " << sv << " does not induce a connected graph";
    }
  }

  set<pair<int, int> > sourceAdj;
  for (CompressedMatrix<int>::const_iterator iter = sourceGraph.begin(); iter != sourceGraph.end(); ++iter) {
    sourceAdj.insert(make_pair(iter.row(), iter.col()));
    sourceAdj.insert(make_pair(iter.col(), iter.row()));
  }

  set<pair<int, int> > targetAdj;
  for (CompressedMatrix<int>::const_iterator iter = targetGraph.begin(); iter != targetGraph.end(); ++iter) {
    targetAdj.insert(make_pair(iter.row(), iter.col()));
    targetAdj.insert(make_pair(iter.col(), iter.row()));
  }

  for (size_t su = 0; su < embedding.size(); ++su) {
    for (size_t sv = su + 1; sv < embedding.size(); ++sv) {
      if (sourceAdj.find(make_pair(su, sv)) == sourceAdj.end()) continue;
      bool found = false;
      BOOST_FOREACH( int tu, embedding[su] ) {
        BOOST_FOREACH( int tv, embedding[sv]) {
          if (targetAdj.find(make_pair(tu, tv)) != targetAdj.end()) {
            found = true;
            break;
          }
        }
        if (found) break;
      }

      if (!found) {
        return AssertionFailure() << "adjacent source vertices " << su << " and " << sv
          << " are not adjacent after embedding";
      }
    }
  }

  return AssertionSuccess();
}

} // namespace test


namespace {

TEST(EmbeddingValidationTest, NonemptySourceEmptyEmbedding) {
  map<pair<int, int>, int> sourceAdj;
  sourceAdj[make_pair(0, 1)] = 1;
  sourceAdj[make_pair(0, 2)] = 1;
  sourceAdj[make_pair(1, 2)] = 1;
  CompressedMatrix<int> sourceGraph(3, 3, sourceAdj);

  map<pair<int, int>, int> targetAdj;
  targetAdj[make_pair(0, 1)] = 1;
  CompressedMatrix<int> targetGraph(2, 2, targetAdj);

  vector<vector<int> > embedding(0);

  EXPECT_TRUE(test::AssertValidFindEmbeddingResult(sourceGraph, targetGraph, embedding));
}

TEST(EmbeddingValidationTest, Typical) {
  // K_9
  map<pair<int, int>, int> sourceAdj;
  for (int i = 0; i < 9; ++i) {
    for (int j = i + 1; j < 9; ++j) {
      sourceAdj[make_pair(i, j)] = 1;
    }
  }
  CompressedMatrix<int> sourceGraph(9, 9, sourceAdj);

  // C2
  map<pair<int, int>, int> targetAdj;
  for (int r = 0; r < 32; r += 16) {
    for (int c = 0; c < 16; c += 8) {
      for (int i = 0; i < 4; ++i) {
        for (int j = 4; j < 8; ++j) {
          targetAdj[make_pair(r + c + i, r + c + j)] = 1;
        }
        if (r == 0) {
          targetAdj[make_pair(r + c + i, r + c + i + 16)] = 1;
        }
        if (c == 0) {
          targetAdj[make_pair(r + c + i + 4, r + c + i + 12)] = 1;
        }
      }
    }
  }
  CompressedMatrix<int> targetGraph(32, 32, targetAdj);

  vector<vector<int> > embedding(9);
  embedding[0].push_back(0);
  embedding[0].push_back(16);
  embedding[0].push_back(21);
  embedding[1].push_back(9);
  embedding[1].push_back(4);
  embedding[1].push_back(12);
  embedding[2].push_back(19);
  embedding[2].push_back(3);
  embedding[2].push_back(22);
  embedding[3].push_back(1);
  embedding[3].push_back(17);
  embedding[4].push_back(2);
  embedding[4].push_back(5);
  embedding[4].push_back(13);
  embedding[5].push_back(14);
  embedding[5].push_back(6);
  embedding[6].push_back(27);
  embedding[6].push_back(31);
  embedding[6].push_back(11);
  embedding[6].push_back(23);
  embedding[7].push_back(10);
  embedding[7].push_back(15);
  embedding[7].push_back(7);
  embedding[8].push_back(8);
  embedding[8].push_back(24);
  embedding[8].push_back(20);
  embedding[8].push_back(28);

  EXPECT_TRUE(test::AssertValidFindEmbeddingResult(sourceGraph, targetGraph, embedding));
}


void f_NonemptySourceEmbeddingSizeMismatchFailure() {
  map<pair<int, int>, int> sourceAdj;
  sourceAdj[make_pair(0, 1)] = 1;
  sourceAdj[make_pair(0, 2)] = 1;
  sourceAdj[make_pair(1, 2)] = 1;
  CompressedMatrix<int> sourceGraph(3, 3, sourceAdj);

  map<pair<int, int>, int> targetAdj;
  targetAdj[make_pair(0, 1)] = 1;
  CompressedMatrix<int> targetGraph(2, 2, targetAdj);

  vector<vector<int> > embedding(2);
  embedding[0].push_back(1);
  embedding[1].push_back(0);

  EXPECT_TRUE(test::AssertValidFindEmbeddingResult(sourceGraph, targetGraph, embedding));
}

TEST(EmbeddingValidationTest, NonemptySourceEmbeddingSizeMismatchFailure) {
  EXPECT_NONFATAL_FAILURE(f_NonemptySourceEmbeddingSizeMismatchFailure(),
    "source size (3) does not match embeddings size (2)");
}


void f_EmptyVertexTargetFailure() {
  map<pair<int, int>, int> sourceAdj;
  sourceAdj[make_pair(0, 1)] = 1;
  sourceAdj[make_pair(1, 2)] = 1;
  CompressedMatrix<int> sourceGraph(3, 3, sourceAdj);

  map<pair<int, int>, int> targetAdj;
  targetAdj[make_pair(0, 1)] = 1;
  targetAdj[make_pair(1, 2)] = 1;
  targetAdj[make_pair(2, 0)] = 1;
  CompressedMatrix<int> targetGraph(3, 3, targetAdj);

  vector<vector<int> > embedding(3);
  embedding[0].push_back(0);
  embedding[2].push_back(1);

  EXPECT_TRUE(test::AssertValidFindEmbeddingResult(sourceGraph, targetGraph, embedding));
}

TEST(EmbeddingValidationTest, EmptyVertexTargetFailure) {
  EXPECT_NONFATAL_FAILURE(f_EmptyVertexTargetFailure(), "source vertex 1 has empty embedding");
}


void f0_InvalidTargetVertexFailure() {
  map<pair<int, int>, int> sourceAdj;
  sourceAdj[make_pair(0, 1)] = 1;
  CompressedMatrix<int> sourceGraph(2, 2, sourceAdj);

  map<pair<int, int>, int> targetAdj;
  targetAdj[make_pair(0, 1)] = 1;
  targetAdj[make_pair(1, 2)] = 1;
  targetAdj[make_pair(2, 3)] = 1;
  CompressedMatrix<int> targetGraph(4, 4, targetAdj);

  vector<vector<int> > embedding(2);
  embedding[0].push_back(1);
  embedding[1].push_back(-1);

  EXPECT_TRUE(test::AssertValidFindEmbeddingResult(sourceGraph, targetGraph, embedding));
}

void f1_InvalidTargetVertexFailure() {
  map<pair<int, int>, int> sourceAdj;
  sourceAdj[make_pair(0, 1)] = 1;
  CompressedMatrix<int> sourceGraph(2, 2, sourceAdj);

  map<pair<int, int>, int> targetAdj;
  targetAdj[make_pair(0, 1)] = 1;
  targetAdj[make_pair(1, 2)] = 1;
  targetAdj[make_pair(2, 3)] = 1;
  CompressedMatrix<int> targetGraph(4, 4, targetAdj);

  vector<vector<int> > embedding(2);
  embedding[0].push_back(4);
  embedding[1].push_back(2);

  EXPECT_TRUE(test::AssertValidFindEmbeddingResult(sourceGraph, targetGraph, embedding));
}

TEST(EmbeddingValidationTest, InvalidTargetVertexFailure) {
  EXPECT_NONFATAL_FAILURE(f0_InvalidTargetVertexFailure(), "source vertex 1 maps to invalid target vertex -1");
  EXPECT_NONFATAL_FAILURE(f1_InvalidTargetVertexFailure(), "source vertex 0 maps to invalid target vertex 4");
}


void f_DuplicateTargetVertexFailure() {
  map<pair<int, int>, int> sourceAdj;
  sourceAdj[make_pair(0, 1)] = 1;
  sourceAdj[make_pair(1, 2)] = 1;
  CompressedMatrix<int> sourceGraph(3, 3, sourceAdj);

  map<pair<int, int>, int> targetAdj;
  targetAdj[make_pair(0, 1)] = 1;
  targetAdj[make_pair(1, 2)] = 1;
  targetAdj[make_pair(2, 0)] = 1;
  CompressedMatrix<int> targetGraph(3, 3, targetAdj);

  vector<vector<int> > embedding(3);
  embedding[0].push_back(0);
  embedding[1].push_back(2);
  embedding[1].push_back(2);
  embedding[2].push_back(1);

  EXPECT_TRUE(test::AssertValidFindEmbeddingResult(sourceGraph, targetGraph, embedding));
}

TEST(EmbeddingValidationTest, DuplicateTargetVertexFailure) {
  EXPECT_NONFATAL_FAILURE(f_DuplicateTargetVertexFailure(),
    "target vertex 2 appears multiple times for source vertex 1");
}


void f_OverlappingEmbeddingsFailure() {
  map<pair<int, int>, int> sourceAdj;
  sourceAdj[make_pair(0, 1)] = 1;
  CompressedMatrix<int> sourceGraph(2, 2, sourceAdj);

  map<pair<int, int>, int> targetAdj;
  targetAdj[make_pair(0, 1)] = 1;
  targetAdj[make_pair(1, 2)] = 1;
  targetAdj[make_pair(2, 0)] = 1;
  CompressedMatrix<int> targetGraph(3, 3, targetAdj);

  vector<vector<int> > embedding(2);
  embedding[0].push_back(0);
  embedding[0].push_back(1);
  embedding[1].push_back(1);
  embedding[1].push_back(2);

  EXPECT_TRUE(test::AssertValidFindEmbeddingResult(sourceGraph, targetGraph, embedding));
}

TEST(EmbeddingValidationTest, OverlappingEmbeddingsFailure) {
  EXPECT_NONFATAL_FAILURE(f_OverlappingEmbeddingsFailure(),
    "target vertex 1 used by source vertices 0 and 1");
}


void f_DisconnectedInducedGraphFailure() {
  map<pair<int, int>, int> sourceAdj;
  sourceAdj[make_pair(0, 0)] = 1;
  CompressedMatrix<int> sourceGraph(1, 1, sourceAdj);

  map<pair<int, int>, int> targetAdj;
  targetAdj[make_pair(0, 1)] = 1;
  targetAdj[make_pair(1, 2)] = 1;
  CompressedMatrix<int> targetGraph(3, 3, targetAdj);

  vector<vector<int> > embedding(1);
  embedding[0].push_back(0);
  embedding[0].push_back(2);

  EXPECT_TRUE(test::AssertValidFindEmbeddingResult(sourceGraph, targetGraph, embedding));
}

TEST(EmbeddingValidationTest, DisconnectedInducedGraphFailure) {
  EXPECT_NONFATAL_FAILURE(f_DisconnectedInducedGraphFailure(),
    "embedding of source vertex 0 does not induce a connected graph");
}


void f_NonadjacentTargetsFailure() {
  map<pair<int, int>, int> sourceAdj;
  sourceAdj[make_pair(0, 1)] = 1;
  sourceAdj[make_pair(1, 2)] = 1;
  sourceAdj[make_pair(0, 2)] = 1;
  CompressedMatrix<int> sourceGraph(3, 3, sourceAdj);

  map<pair<int, int>, int> targetAdj;
  targetAdj[make_pair(0, 1)] = 1;
  targetAdj[make_pair(1, 2)] = 1;
  targetAdj[make_pair(2, 3)] = 1;
  targetAdj[make_pair(3, 4)] = 1;
  targetAdj[make_pair(4, 0)] = 1;
  CompressedMatrix<int> targetGraph(5, 5, targetAdj);

  vector<vector<int> > embedding(3);
  embedding[0].push_back(4);
  embedding[0].push_back(3);
  embedding[1].push_back(0);
  embedding[2].push_back(1);

  EXPECT_TRUE(test::AssertValidFindEmbeddingResult(sourceGraph, targetGraph, embedding));
}

TEST(EmbeddingValidationTest, NonadjacentTargetsFailure) {
  EXPECT_NONFATAL_FAILURE(f_NonadjacentTargetsFailure(),
    "adjacent source vertices 0 and 2 are not adjacent after embedding");
}

} // namespace {anonymous}
