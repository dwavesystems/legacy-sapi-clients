//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef TEST_HPP_INCLUDED
#define TEST_HPP_INCLUDED

#include <vector>

#include <gtest/gtest.h>

#include <compressed_matrix.hpp>


namespace test {

testing::AssertionResult AssertValidFindEmbeddingResult(
  const compressed_matrix::CompressedMatrix<int>& sourceGraph,
  const compressed_matrix::CompressedMatrix<int>& targetGraph,
  const std::vector<std::vector<int> >& embedding);

} // namespace test

#endif
