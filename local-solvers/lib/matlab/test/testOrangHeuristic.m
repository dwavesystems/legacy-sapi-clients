% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testOrangHeuristic %#ok<STOUT,*DEFNU>
initTestSuite
end

function p = params(varargin)
p.iteration_limit = 10;
p.max_bit_flip_prob = 1.0 / 8.0;
p.max_local_complexity = 9;
p.min_bit_flip_prob = 1.0 / 32.0;
p.local_stuck_limit = 8;
p.num_perturbed_copies = 4;
p.num_variables = 0;
p.random_seed = [];
p.time_limit_seconds = 5.0;

for setp=reshape(varargin, 2, [])
  p.(setp{1}) = setp{2};
end
end

function testTrivial
assertEqual(sapilocal_orangHeuristic('ising', [], params), ...
  struct('energies', [0], 'solutions', zeros(0, 1)))
assertEqual(sapilocal_orangHeuristic('qubo', [], params), ...
  struct('energies', [0], 'solutions', zeros(0, 1)))
end

function testBadProblemType
assertExceptionThrown(@() sapilocal_orangHeuristic('blarg', [], params), ...
  'sapilocal:BadProblemType')
end

function testBadProblem
assertExceptionThrown(@() sapilocal_orangHeuristic('ising', 'blah', params), ...
  'sapilocal:BadArgType')
end

function assertMissing(name)
p = rmfield(params, name);
assertExceptionThrown(@() sapilocal_orangHeuristic('ising', [], p), ...
  'sapilocal:MissingField')
end

function assertBadArgType(name, value)
p = params(name, value);
assertExceptionThrown(@() sapilocal_orangHeuristic('ising', [], p), ...
  'sapilocal:BadArgType')
end

function testBadParams
assertMissing('iteration_limit')
assertMissing('max_bit_flip_prob')
assertMissing('max_local_complexity')
assertMissing('min_bit_flip_prob')
assertMissing('local_stuck_limit')
assertMissing('num_perturbed_copies')
assertMissing('num_variables')
assertMissing('time_limit_seconds')

assertBadArgType('iteration_limit', 'a')
assertBadArgType('max_bit_flip_prob', 'a')
assertBadArgType('max_local_complexity', 'a')
assertBadArgType('min_bit_flip_prob', 'backwards')
assertBadArgType('local_stuck_limit', 'a')
assertBadArgType('num_perturbed_copies', 'a')
assertBadArgType('num_variables', 'a')
assertBadArgType('random_seed', 'a')
assertBadArgType('time_limit_seconds', 'a')
end
