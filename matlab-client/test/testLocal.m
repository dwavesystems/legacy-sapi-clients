% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testLocal %#ok<STOUT,*DEFNU>
initTestSuite
end



function testSolverNames

expected = {'c4-sw_optimize'; 'c4-sw_sample'; 'ising-heuristic'};
solvers = sapiListSolvers(sapiLocalConnection);
assertEqual(sort(solvers(:)), sort(expected(:)));

end



function testParamProp

optSolver = sapiSolver(sapiLocalConnection, 'c4-sw_optimize');
assertTrue(isfield(sapiSolverProperties(optSolver), 'parameters'))

sampSolver = sapiSolver(sapiLocalConnection, 'c4-sw_sample');
assertTrue(isfield(sapiSolverProperties(sampSolver), 'parameters'))

heurSolver = sapiSolver(sapiLocalConnection, 'ising-heuristic');
assertTrue(isfield(sapiSolverProperties(heurSolver), 'parameters'))

end



function testOptimizeParams

solver = sapiSolver(sapiLocalConnection, 'c4-sw_optimize');
assertTrue(isfield(sapiSolverProperties(solver), 'parameters'))

sapiSolveQubo(solver, -1, 'num_reads', 10, 'answer_mode', 'raw', ...
  'max_answers', 5);

end



function testSampleParams

solver = sapiSolver(sapiLocalConnection, 'c4-sw_sample');
assertTrue(isfield(sapiSolverProperties(solver), 'parameters'))

sapiSolveQubo(solver, -1, 'num_reads', 10, 'answer_mode', 'raw', ...
  'max_answers', 5, 'beta', 1.5, 'random_seed', 555);

end



function testHeuristicParams

solver = sapiSolver(sapiLocalConnection, 'ising-heuristic');
assertTrue(isfield(sapiSolverProperties(solver), 'parameters'))

sapiSolveQubo(solver, -1,  'iteration_limit', 200, ...
  'time_limit_seconds', 0.1, 'random_seed', 1234, 'num_variables', 15, ...
  'max_local_complexity', 8, 'local_stuck_limit', 10, ...
  'num_perturbed_copies', 9, 'min_bit_flip_prob', 0.1, ...
  'max_bit_flip_prob', 0.8);

end
