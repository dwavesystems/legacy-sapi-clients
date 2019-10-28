//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <coding.hpp>
#include <solver.hpp>
#include <json.hpp>

#include <remote.hpp>

#include "test.hpp"
#include "json-builder.hpp"

using std::make_shared;
using std::string;
using std::vector;

using testing::StrictMock;

using sapiremote::QpProblem;

using sapi::RemoteSolver;

namespace {

class MockRemoteSolver : public sapiremote::Solver {
private:
  json::Value encodedProblem_;
  QpProblem lastProblem_;

public:
  json::Value encodedProblem() const { return encodedProblem_; }
  void encodedProblem(json::Value p) { encodedProblem_ = std::move(p); }
  const QpProblem& lastProblem() const { return lastProblem_; }
  void lastProblem(QpProblem p) { lastProblem_ = std::move(p); }

  MOCK_CONST_METHOD3(submitProblemImpl, sapiremote::SubmittedProblemPtr(
      string& type, json::Value& problem, json::Object& params));
  MockRemoteSolver(string id, json::Object properties) :
      sapiremote::Solver(std::move(id), std::move(properties)) {}
};

auto o = jsonObject();
auto a = jsonArray();

} // namespace {anonymous}

TEST(RemotePropertiesTest, NoProps) {
  auto mockSolver = make_shared<StrictMock<MockRemoteSolver>>("", json::Object());
  auto solver = RemoteSolver(mockSolver);
  auto props = solver.properties();
  EXPECT_FALSE(!!props->supported_problem_types);
  EXPECT_FALSE(!!props->quantum_solver);
  EXPECT_FALSE(!!props->ising_ranges);
  EXPECT_FALSE(!!props->anneal_offset);
  EXPECT_FALSE(!!props->anneal_schedule);
  EXPECT_FALSE(!!props->parameters);
  EXPECT_FALSE(!!props->virtual_graph);
}


TEST(RemotePropertiesTest, SupportedProblemTypesProp) {
  auto rprops = (o, "supported_problem_types", (a, json::Null(), "abc", "qaz", 4)).object(); // garbage ignored
  auto mockSolver = make_shared<StrictMock<MockRemoteSolver>>("", rprops);
  auto solver = RemoteSolver(mockSolver);
  auto props = solver.properties();
  ASSERT_TRUE(!!props->supported_problem_types);
  EXPECT_EQ(2, props->supported_problem_types->len);
  ASSERT_TRUE(!!props->supported_problem_types->elements);
  ASSERT_TRUE(!!props->supported_problem_types->elements[0]);
  EXPECT_EQ(string("abc"), props->supported_problem_types->elements[0]);
  ASSERT_TRUE(!!props->supported_problem_types->elements[1]);
  EXPECT_EQ(string("qaz"), props->supported_problem_types->elements[1]);
  EXPECT_FALSE(!!props->quantum_solver);
  EXPECT_FALSE(!!props->ising_ranges);
  EXPECT_FALSE(!!props->anneal_offset);
  EXPECT_FALSE(!!props->parameters);
}


TEST(RemotePropertiesTest, SupportedProblemTypesPropBadType) {
  auto rprops = (o, "supported_problem_types", json::Null()).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->supported_problem_types);
}


TEST(RemotePropertiesTest, QuantumSolverPropsTrivial) {
  auto rprops = (o, "num_qubits", 0, "qubits", a, "couplers", a).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto qs = solver.properties()->quantum_solver;
  ASSERT_TRUE(!!qs);
  EXPECT_EQ(0, qs->num_qubits);
  EXPECT_EQ(0, qs->qubits_len);
  EXPECT_EQ(0, qs->couplers_len);
}


TEST(RemotePropertiesTest, QuantumSolverProps) {
  auto rprops = json::Object();
  rprops["num_qubits"] = 100;
  rprops["qubits"] = (a, 0, -1, 3, 6, 88, 3, 101); // accept whatever garbage server provides!
  rprops["couplers"] = (a, (a, -1, -1), (a, 200, 30));
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto qs = solver.properties()->quantum_solver;
  ASSERT_TRUE(!!qs);
  EXPECT_EQ(rprops["num_qubits"].getInteger(), qs->num_qubits);
  EXPECT_EQ(rprops["qubits"].getArray().size(), qs->qubits_len);
  EXPECT_EQ(rprops["couplers"].getArray().size(), qs->couplers_len);
  ASSERT_TRUE(!!qs->qubits);
  auto expectedQubits = vector<int>{0, -1, 3, 6, 88, 3, 101};
  auto qubits = vector<int>(qs->qubits, qs->qubits + qs->qubits_len);
  EXPECT_EQ(expectedQubits, qubits);
  auto expectedCouplers = vector<sapi_Coupler>{{-1, -1}, {200, 30}};
  auto couplers = vector<sapi_Coupler>(qs->couplers, qs->couplers + qs->couplers_len);
  EXPECT_EQ(expectedCouplers, couplers);
}


TEST(RemotePropertiesTest, QuantumSolverPropsMissingNumQubits) {
  auto rprops = (o, "qubits", a, "couplers", a).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto qs = solver.properties()->quantum_solver;
  ASSERT_FALSE(!!qs);
}


TEST(RemotePropertiesTest, QuantumSolverPropsBadNumQubitsType) {
  auto rprops = (o, "num_qubits", "hello", "qubits", a, "couplers", a).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto qs = solver.properties()->quantum_solver;
  ASSERT_FALSE(!!qs);
}


TEST(RemotePropertiesTest, QuantumSolverPropsMissingQubits) {
  auto rprops = (o, "num_qubits", 0, "couplers", a).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto qs = solver.properties()->quantum_solver;
  ASSERT_FALSE(!!qs);
}


TEST(RemotePropertiesTest, QuantumSolverPropsBadQubitsType) {
  auto rprops = (o, "num_qubits", 0, "qubits", o, "couplers", a).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto qs = solver.properties()->quantum_solver;
  ASSERT_FALSE(!!qs);
}


TEST(RemotePropertiesTest, QuantumSolverPropsBadQubitsValue) {
  auto rprops = (o, "num_qubits", 0, "qubits", (a, json::Null()), "couplers", a).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto qs = solver.properties()->quantum_solver;
  ASSERT_FALSE(!!qs);
}


TEST(RemotePropertiesTest, QuantumSolverPropsMissingCouplers) {
  auto rprops = (o, "num_qubits", 0, "qubits", a).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto qs = solver.properties()->quantum_solver;
  ASSERT_FALSE(!!qs);
}


TEST(RemotePropertiesTest, QuantumSolverPropsBadCouplersType) {
  auto rprops = (o, "num_qubits", 0, "qubits", a, "couplers", 2).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto qs = solver.properties()->quantum_solver;
  ASSERT_FALSE(!!qs);
}


TEST(RemotePropertiesTest, QuantumSolverPropsBadCouplersValueType) {
  auto rprops = (o, "num_qubits", 0, "qubits", a, "couplers", (a, "5")).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto qs = solver.properties()->quantum_solver;
  ASSERT_FALSE(!!qs);
}


TEST(RemotePropertiesTest, QuantumSolverPropsBadCouplersValueLong) {
  auto rprops = (o, "num_qubits", 0, "qubits", a, "couplers", (a, 1, 2, 3)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto qs = solver.properties()->quantum_solver;
  ASSERT_FALSE(!!qs);
}


TEST(RemotePropertiesTest, QuantumSolverPropsBadCouplersValueShort) {
  auto rprops = (o, "num_qubits", 0, "qubits", a, "couplers", (a, 0)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto qs = solver.properties()->quantum_solver;
  ASSERT_FALSE(!!qs);
}


TEST(RemotePropertiesTest, IsingRangeProps) {
  auto rprops = (o, "h_range", (a, -1, 1), "j_range", (a, 2, -3)).object(); // accept server garbage
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto ir = solver.properties()->ising_ranges;
  ASSERT_TRUE(!!ir);
  EXPECT_EQ(-1.0, ir->h_min);
  EXPECT_EQ(1.0, ir->h_max);
  EXPECT_EQ(2.0, ir->j_min);
  EXPECT_EQ(-3.0, ir->j_max);
}


TEST(RemotePropertiesTest, IsingRangePropsMissingHRange) {
  auto rprops = (o, "j_range", (a, -1, 1)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->ising_ranges);
}


TEST(RemotePropertiesTest, IsingRangePropsBadHRangeType) {
  auto rprops = (o, "h_range", 2.5, "j_range", (a, -1, 1)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->ising_ranges);
}


TEST(RemotePropertiesTest, IsingRangePropsBadHRangeLong) {
  auto rprops = (o, "h_range", (a, -1, 1, 10), "j_range", (a, -1, 1)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->ising_ranges);
}


TEST(RemotePropertiesTest, IsingRangePropsBadHRangeShort) {
  auto rprops = (o, "h_range", (a, 0), "j_range", (a, -1, 1)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->ising_ranges);
}


TEST(RemotePropertiesTest, IsingRangePropsBadHRangeValue) {
  auto rprops = (o, "h_range", (a, 0, "six"), "j_range", (a, -1, 1)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->ising_ranges);
}


TEST(RemotePropertiesTest, IsingRangePropsMissingJRange) {
  auto rprops = (o, "h_range", (a, -1, 1)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->ising_ranges);
}


TEST(RemotePropertiesTest, IsingRangePropsBadJRangeType) {
  auto rprops = (o, "h_range", (a, -1, 1), "j_range", 2.5).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->ising_ranges);
}


TEST(RemotePropertiesTest, IsingRangePropsBadJRangeLong) {
  auto rprops = (o, "h_range", (a, -1, 1), "j_range", (a, -1, 1, 10)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->ising_ranges);
}


TEST(RemotePropertiesTest, IsingRangePropsBadJRangeShort) {
  auto rprops = (o, "h_range", (a, -1, 1), "j_range", (a, 0)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->ising_ranges);
}


TEST(RemotePropertiesTest, IsingRangePropsBadJRangeValue) {
  auto rprops = (o, "h_range", (a, -1, 1), "j_range", (a, 0, "six")).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->ising_ranges);
}


TEST(RemotePropertiesTest, AnnealOffsetProps) {
  auto rprops = (o,
      "anneal_offset_ranges", (a, (a, -1, 1), (a, -0.5, 0.25), (a, 0, 0.5)),
      "anneal_offset_step", 1.0 / 32.0,
      "anneal_offset_step_phi0", 0.5).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto ao = solver.properties()->anneal_offset;
  ASSERT_TRUE(!!ao);
  EXPECT_EQ(1.0 / 32.0, ao->step);
  EXPECT_EQ(0.5, ao->step_phi0);
  EXPECT_EQ(3, ao->ranges_len);
  EXPECT_EQ(-1.0, ao->ranges[0].min);
  EXPECT_EQ(1.0, ao->ranges[0].max);
  EXPECT_EQ(-0.5, ao->ranges[1].min);
  EXPECT_EQ(0.25, ao->ranges[1].max);
  EXPECT_EQ(0.0, ao->ranges[2].min);
  EXPECT_EQ(0.5, ao->ranges[2].max);
}


TEST(RemotePropertiesTest, AnnealOffsetPropsTrivial) {
  auto rprops = (o, "anneal_offset_ranges", a, "anneal_offset_step", 1.0, "anneal_offset_step_phi0", 2.0).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto ao = solver.properties()->anneal_offset;
  ASSERT_TRUE(!!ao);
  EXPECT_EQ(1.0, ao->step);
  EXPECT_EQ(2.0, ao->step_phi0);
  EXPECT_EQ(0, ao->ranges_len);
}


TEST(RemotePropertiesTest, AnnealOffsetPropsMissingRanges) {
  auto rprops = (o, "anneal_offset_step", 1.0, "anneal_offset_step_phi0", 2.0).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->anneal_offset);
}


TEST(RemotePropertiesTest, AnnealOffsetPropsMissingStep) {
  auto rprops = (o, "anneal_offset_ranges", a, "anneal_offset_step_phi0", 2.0).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto ao = solver.properties()->anneal_offset;
  ASSERT_TRUE(!!ao);
  EXPECT_EQ(-1.0, ao->step);
  EXPECT_EQ(2.0, ao->step_phi0);
  EXPECT_EQ(0, ao->ranges_len);
}


TEST(RemotePropertiesTest, AnnealOffsetPropsMissingPhi0) {
  auto rprops = (o, "anneal_offset_ranges", a, "anneal_offset_step", 1.0).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto ao = solver.properties()->anneal_offset;
  ASSERT_TRUE(!!ao);
  EXPECT_EQ(1.0, ao->step);
  EXPECT_EQ(-1.0, ao->step_phi0);
  EXPECT_EQ(0, ao->ranges_len);
}


TEST(RemotePropertiesTest, AnnealScheduleProps) {
  auto rprops = (o, "max_anneal_schedule_points", 123, "annealing_time_range", (a, 12, 34.5)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto as = solver.properties()->anneal_schedule;
  ASSERT_TRUE(!!as);
  EXPECT_EQ(123, as->max_points);
  EXPECT_EQ(12, as->min_annealing_time);
  EXPECT_EQ(34.5, as->max_annealing_time);
}


TEST(RemotePropertiesTest, AnnealSchedulePropsMissingMaxPoints) {
  auto rprops = (o, "annealing_time_range", (a, 1, 2)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto as = solver.properties()->anneal_schedule;
  ASSERT_TRUE(!!as);
  EXPECT_EQ(-1, as->max_points);
  EXPECT_EQ(1, as->min_annealing_time);
  EXPECT_EQ(2, as->max_annealing_time);
}


TEST(RemotePropertiesTest, AnnealSchedulePropsMissingRange) {
  auto rprops = (o, "max_anneal_schedule_points", 456).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto as = solver.properties()->anneal_schedule;
  ASSERT_TRUE(!!as);
  EXPECT_EQ(456, as->max_points);
  EXPECT_EQ(-1, as->min_annealing_time);
  EXPECT_EQ(-1, as->max_annealing_time);
}


TEST(RemotePropertiesTest, AnnealSchedulePropsBadTypes) {
  auto rprops = (o, "max_anneal_schedule_points", "lots", "annealing_time_range", (a, 1, 2, 3)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  EXPECT_FALSE(!!solver.properties()->anneal_schedule);
}


TEST(RemotePropertiesTest, ParametersProp) {
  auto rprops = (o, "parameters", (o, "xyz", json::Null(), "abc", "something", "def", 3)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto pp = solver.properties()->parameters;
  ASSERT_TRUE(!!pp);
  ASSERT_EQ(3, pp->len);

  auto expectedParams = vector<string>{"abc", "def", "xyz"};
  auto params = vector<string>(pp->elements, pp->elements + pp->len);
  EXPECT_EQ(expectedParams, params);
}


TEST(RemotePropertiesTest, ParametersPropEmpty) {
  auto rprops = (o, "parameters", o).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto pp = solver.properties()->parameters;
  ASSERT_TRUE(!!pp);
  EXPECT_EQ(0, pp->len);
}


TEST(RemotePropertiesTest, ParametersPropWrongType) {
  auto rprops = (o, "parameters", (a, "abc", "xyz", "def")).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->parameters);
}


TEST(RemotePropertiesTest, VirtualGraphProps) {
  auto rprops = (o, "extended_j_range", (a, -1, 1), "per_qubit_coupling_range", (a, 2, -3)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  auto vg = solver.properties()->virtual_graph;
  ASSERT_TRUE(!!vg);
  EXPECT_EQ(-1.0, vg->extended_j_min);
  EXPECT_EQ(1.0, vg->extended_j_max);
  EXPECT_EQ(2.0, vg->per_qubit_coupling_min);
  EXPECT_EQ(-3.0, vg->per_qubit_coupling_max);
}


TEST(RemotePropertiesTest, VirtualGraphPropsMissingExtendedJRange) {
  auto rprops = (o, "per_qubit_coupling_range", (a, -1, 1)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->virtual_graph);
}


TEST(RemotePropertiesTest, VirtualGraphPropsBadExtendedJRangeType) {
  auto rprops = (o, "extended_j_range", 2.5, "per_qubit_coupling_range", (a, -1, 1)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->virtual_graph);
}


TEST(RemotePropertiesTest, VirtualGraphPropsBadExtendedRangeLong) {
  auto rprops = (o, "extended_j_range", (a, -1, 1, 10), "per_qubit_coupling_range", (a, -1, 1)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->virtual_graph);
}


TEST(RemotePropertiesTest, VirtualGraphPropsBadExtendedJRangeShort) {
  auto rprops = (o, "extended_j_range", (a, 0), "per_qubit_coupling_range", (a, -1, 1)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->virtual_graph);
}


TEST(RemotePropertiesTest, VirtualGraphPropsBadExtendedJRangeValue) {
  auto rprops = (o, "extended_j_range", (a, 0, "six"), "per_qubit_coupling_range", (a, -1, 1)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->virtual_graph);
}


TEST(RemotePropertiesTest, VirtualGraphPropsMissingPerQubitCouplingRange) {
  auto rprops = (o, "extended_j_range", (a, -1, 1)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->virtual_graph);
}


TEST(RemotePropertiesTest, VirtualGraphPropsBadPerQubitCouplingRangeType) {
  auto rprops = (o, "extended_j_range", (a, -1, 1), "per_qubit_coupling_range", 2.5).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->virtual_graph);
}


TEST(RemotePropertiesTest, VirtualGraphPropsBadPerQubitCouplingRangeLong) {
  auto rprops = (o, "extended_j_range", (a, -1, 1), "per_qubit_coupling_range", (a, -1, 1, 10)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->virtual_graph);
}


TEST(RemotePropertiesTest, VirtualGraphPropsBadPerQubitCouplingRangeShort) {
  auto rprops = (o, "extended_j_range", (a, -1, 1), "per_qubit_coupling_range", (a, 0)).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->virtual_graph);
}


TEST(RemotePropertiesTest, VirtualGraphPropsBadPerQubitCouplingRangeValue) {
  auto rprops = (o, "extended_j_range", (a, -1, 1), "per_qubit_coupling_range", (a, 0, "six")).object();
  auto solver = RemoteSolver(make_shared<StrictMock<MockRemoteSolver>>("", rprops));
  ASSERT_FALSE(!!solver.properties()->virtual_graph);
}
