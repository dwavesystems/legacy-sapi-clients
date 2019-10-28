//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <map>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <dwave_sapi.h>

using std::size_t;
using std::make_pair;
using std::map;
using std::pair;
using std::vector;


TEST(FixVariablesTest, standard) {
  auto problemData = vector<sapi_ProblemEntry>{{0, 1, -1.0}, {2, 2, -1.0}, {3, 3, -1.0}, {2, 3, 2.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};
  sapi_FixVariablesResult *r;

  ASSERT_EQ(SAPI_OK, sapi_fixVariables(&problem, SAPI_FIX_VARIABLES_METHOD_STANDARD, &r, 0));
  EXPECT_EQ(-1.0, r->offset);

  auto expectedFixedVars = map<int, int>{{0, 1}, {1, 1}};
  auto fixedVars = map<int, int>{};
  for (size_t i = 0; i < r->fixed_variables_len; ++i) {
    fixedVars[r->fixed_variables[i].var] = r->fixed_variables[i].value;
  }
  EXPECT_EQ(expectedFixedVars, fixedVars);

  auto expectedNewProblem = map<pair<int, int>, double>{
    {make_pair(2, 2), -1.0},
    {make_pair(2, 3), 2.0},
    {make_pair(3, 3), -1.0}
  };
  auto newProblem = map<pair<int, int>, double>{};
  for (size_t i = 0; i < r->new_problem.len; ++i) {
    const auto& e = r->new_problem.elements[i];
    newProblem[make_pair(e.i, e.j)] += e.value;
  }
  EXPECT_EQ(expectedNewProblem, newProblem);

  sapi_freeFixVariablesResult(r);
}


TEST(FixVariablesTest, optimized) {
  auto problemData = vector<sapi_ProblemEntry>{{0, 1, -1.0}, {2, 2, -1.0}, {3, 3, -1.0}, {2, 3, 2.0}};
  auto problem = sapi_Problem{problemData.data(), problemData.size()};
  sapi_FixVariablesResult *r;

  ASSERT_EQ(SAPI_OK, sapi_fixVariables(&problem, SAPI_FIX_VARIABLES_METHOD_OPTIMIZED, &r, 0));
  EXPECT_EQ(-2.0, r->offset);
  EXPECT_EQ(0, r->new_problem.len);
  EXPECT_EQ(4, r->fixed_variables_len);

  auto fixedVars = map<int, int>{};
  for (size_t i = 0; i < r->fixed_variables_len; ++i) {
    fixedVars[r->fixed_variables[i].var] = r->fixed_variables[i].value;
  }
  EXPECT_NE(fixedVars.find(0), fixedVars.end());
  EXPECT_NE(fixedVars.find(1), fixedVars.end());
  EXPECT_NE(fixedVars.find(2), fixedVars.end());
  EXPECT_NE(fixedVars.find(3), fixedVars.end());
  EXPECT_EQ(1, fixedVars[0]);
  EXPECT_EQ(1, fixedVars[1]);
  EXPECT_NE(fixedVars[2], fixedVars[3]);

  sapi_freeFixVariablesResult(r);
}

