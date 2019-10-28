//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <exceptions.hpp>
#include <coding.hpp>
#include <solver.hpp>

#include "test.hpp"
#include "json-builder.hpp"

using std::make_shared;
using std::pair;
using std::map;
using std::vector;

using sapiremote::Solver;
using sapiremote::SubmittedProblemPtr;
using sapiremote::encodeQpProblem;
using sapiremote::QpProblem;
using sapiremote::EncodingException;

namespace {

auto a = jsonArray();
auto o = jsonObject();

class NonSolver : public Solver {
private:
  virtual SubmittedProblemPtr submitProblemImpl(std::string&, json::Value&, json::Object&) const {
    throw std::runtime_error("not implemented");
  }
public:
  NonSolver(json::Object props) : Solver("nonsolver", std::move(props)) {}
};

} // namespace {anonymous}


TEST(QpInfoTest, Trivial) {
  auto solver = make_shared<NonSolver>((o, "qubits", a, "couplers", a ));
  EXPECT_TRUE(solver->qpInfo()->qubits.empty());
  EXPECT_TRUE(solver->qpInfo()->couplers.empty());
  EXPECT_TRUE(solver->qpInfo()->qubitIndices.empty());
}

TEST(QpInfoTest, Typical) {
  auto solver = make_shared<NonSolver>( (o, "qubits", (a, 0, 2, 3), "couplers", (a, (a, 0, 2), (a, 2, 3))) );

  auto expectedQubits = vector<int>{0, 2, 3};
  EXPECT_EQ(expectedQubits, solver->qpInfo()->qubits);

  auto expectedCouplers = vector<pair<int, int>>{{0, 2}, {2, 3}};
  EXPECT_EQ(expectedCouplers, solver->qpInfo()->couplers);

  // convert to map since gtest can't compare unordered_map
  const auto& qubitIndices = solver->qpInfo()->qubitIndices;
  auto qubitIndicesMap = map<int, int>(qubitIndices.begin(), qubitIndices.end());
  auto expectedQubitIndices = map<int, int>{{0, 0}, {2, 1}, {3, 2}};
  EXPECT_EQ(expectedQubitIndices, qubitIndicesMap);
}

TEST(QpInfoTest, BadQubits) {
  EXPECT_FALSE(make_shared<NonSolver>((o, "couplers", a ))->qpInfo());
  EXPECT_FALSE(make_shared<NonSolver>((o, "qubits", 4, "couplers", a))->qpInfo());
  EXPECT_FALSE(make_shared<NonSolver>((o, "qubits", (a, -1), "couplers", a))->qpInfo());
}

TEST(QpInfoTest, BadCouplers) {
  EXPECT_FALSE(make_shared<NonSolver>((o, "qubits", a))->qpInfo());
  EXPECT_FALSE(make_shared<NonSolver>((o, "qubits", (a, 0, 1), "couplers", 2))->qpInfo());
  EXPECT_FALSE(make_shared<NonSolver>((o, "qubits", (a, 0, 1), "couplers", (a, 0, 1)))->qpInfo());
  EXPECT_FALSE(make_shared<NonSolver>((o, "qubits", (a, 0, 1), "couplers", (a, (a, 0, 2))))->qpInfo());
}

TEST(QpEncoderTest, TrivialSolver) {
  auto solver = make_shared<NonSolver>( (o, "qubits", a, "couplers", a).object() );
  auto problem = encodeQpProblem(solver, QpProblem{});

  auto expected = (o, "format", "qp", "lin", "", "quad", "").value();
  EXPECT_EQ(expected, problem);
}

TEST(QpEncoderTest, TrivialProblem) {
  auto solver = make_shared<NonSolver>( (o,
      "qubits", (a, 0, 2, 3),
      "couplers", (a, (a, 0, 2), (a, 2, 3))
      ).object() );
  auto problem = encodeQpProblem(solver, QpProblem{});

  auto expected = (o, "format", "qp", "lin", "AAAAAAAA+H8AAAAAAAD4fwAAAAAAAPh/", "quad", "").value();
  EXPECT_EQ(expected, problem);
}

TEST(QpEncoderTest, FullLinear) {
  auto solver = make_shared<NonSolver>( (o,
      "qubits", (a, 0, 2, 3),
      "couplers", (a, (a, 0, 2), (a, 2, 3))
      ).object() );
  auto problem = encodeQpProblem(solver, QpProblem{{0, 0, 0}, {2, 2, -2}, {3, 3, 3}});

  auto expected = (o, "format", "qp", "lin", "AAAAAAAAAAAAAAAAAAAAwAAAAAAAAAhA",
      "quad", "AAAAAAAAAAAAAAAAAAAAAA==").value();
  EXPECT_EQ(expected, problem);
}

TEST(QpEncoderTest, AddLinear) {
  auto solver = make_shared<NonSolver>( (o,
      "qubits", (a, 0, 2, 3),
      "couplers", (a, (a, 0, 2), (a, 2, 3))
      ).object() );
  auto problem = encodeQpProblem(solver, QpProblem{{0, 0, 0}, {2, 2, 1}, {3, 3, 3}, {2, 2, -3}});

  auto expected = (o, "format", "qp", "lin", "AAAAAAAAAAAAAAAAAAAAwAAAAAAAAAhA",
      "quad", "AAAAAAAAAAAAAAAAAAAAAA==").value();
  EXPECT_EQ(expected, problem);
}

TEST(QpEncoderTest, FullQuad) {
  auto solver = make_shared<NonSolver>( (o,
      "qubits", (a, 0, 1, 2, 3, 5),
      "couplers", (a, (a, 0, 2), (a, 2, 3))
      ).object() );
  auto problem = encodeQpProblem(solver, QpProblem{{0, 2, 10}, {2, 3, 20}});

  auto expected = (o, "format", "qp", "lin", "AAAAAAAAAAAAAAAAAAD4fwAAAAAAAAAAAAAAAAAAAAAAAAAAAAD4fw==",
      "quad", "AAAAAAAAJEAAAAAAAAA0QA==").value();
  EXPECT_EQ(expected, problem);
}

TEST(QpEncoderTest, AddFlipQuad) {
  auto solver = make_shared<NonSolver>( (o,
      "qubits", (a, 0, 1, 2, 3, 5),
      "couplers", (a, (a, 0, 2), (a, 2, 3))
      ).object() );
  auto problem = encodeQpProblem(solver, QpProblem{{0, 2, 100}, {2, 3, 2}, {3, 2, 18}, {2, 0, 0}, {2, 0, -90}});

  auto expected = (o, "format", "qp", "lin", "AAAAAAAAAAAAAAAAAAD4fwAAAAAAAAAAAAAAAAAAAAAAAAAAAAD4fw==",
      "quad", "AAAAAAAAJEAAAAAAAAA0QA==").value();
  EXPECT_EQ(expected, problem);
}

TEST(QpEncoderTest, Typical) {
  auto solver = make_shared<NonSolver>( (o,
      "qubits", (a, 1, 2, 4, 5, 7),
      "couplers", (a, (a, 2, 4), (a, 2, 5), (a, 2, 7), (a, 1, 7), (a, 1, 5), (a, 1, 4))
      ).object() );
  auto problem = encodeQpProblem(solver, QpProblem{
    {7, 7, -1}, {1, 4, 4}, {4, 2, -6}, {1, 7, 2.5}, {1, 1, -3}, {7, 2, 3}, {7, 1, -7.5}, {2, 2, 2}, {7, 7, 2}
  });

  auto expected = (o, "format", "qp", "lin", "AAAAAAAACMAAAAAAAAAAQAAAAAAAAAAAAAAAAAAA+H8AAAAAAADwPw==",
      "quad", "AAAAAAAAGMAAAAAAAAAIQAAAAAAAABTAAAAAAAAAEEA=").value();
  EXPECT_EQ(expected, problem);
}

TEST(QpEncoderTest, BadProblem) {
  auto solver = make_shared<NonSolver>( (o,
    "qubits", (a, 0, 2, 3),
    "couplers", (a, (a, 0, 2), (a, 2, 3))
    ).object() );

  EXPECT_THROW(encodeQpProblem(solver, QpProblem{{1, 1, 1}}), EncodingException);
  EXPECT_THROW(encodeQpProblem(solver, QpProblem{{0, 1, 1}}), EncodingException);
  EXPECT_THROW(encodeQpProblem(solver, QpProblem{{0, 3, 1}}), EncodingException);
}
