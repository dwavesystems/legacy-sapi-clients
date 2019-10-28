% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testOrangSample %#ok<STOUT,*DEFNU>
initTestSuite
end

function p = params(varargin)
p.num_vars = 0;
p.active_vars = [];
p.active_var_pairs = [];
p.var_order = [];
p.num_reads = 1;
p.max_answers = 1;
p.answer_histogram = true;
p.beta = 1.0;
p.random_seed = [];

for setp=reshape(varargin, 2, [])
  p.(setp{1}) = setp{2};
end
end

function testTrivial
assertEqual(sapilocal_orangSample('ising', [], params), ...
  struct('energies', [0], 'solutions', zeros(0, 1), 'num_occurrences', [1]))
assertEqual(sapilocal_orangSample('qubo', [], params), ...
  struct('energies', [0], 'solutions', zeros(0, 1), 'num_occurrences', [1]))
end

function testRngChange
p = params('num_vars', 10, 'active_vars', 0:9, 'var_order', 0:9, ...
           'num_reads', 100, 'max_answers', 100, ...
           'answer_histogram', false, 'beta', 0.01, 'random_seed', []);
[r c] = find(triu(ones(10), 1));
p.active_var_pairs = [r-1 c-1]';
problem = triu(ones(10), 1);

r1 = sapilocal_orangSample('ising', problem, p);
r2 = sapilocal_orangSample('ising', problem, p);
assertFalse(isequal(r1, r2))
end

function testExactResult1
p.num_vars = 8;
p.active_vars = [0 2 4 6];
p.active_var_pairs = [0 2; 2 4; 4 6]';
p.var_order = [4 2 6 0];
p.num_reads = 10000;
p.max_answers = 3;
p.answer_histogram = true;
p.beta = 1.5;
p.random_seed = 11235;

problem = sparse([1 1 3 5], [1 3 5 7], [0.25 -1 1 -0.5]);
result = sapilocal_orangSample('ising', problem, p);

assertEqual(result, struct( ...
  'energies', [-2.75 -2.25 -1.75], ...
  'solutions', [-1 3 -1 3 +1 3 +1 3
                +1 3 +1 3 -1 3 -1 3
                -1 3 -1 3 +1 3 -1 3]', ...
  'num_occurrences', [5137 2354 1097]))
end

function testExactResult2
p.num_vars = 4;
p.active_vars = 0:3;
p.active_var_pairs = [0 1; 0 2; 0 3; 1 2; 1 3; 2 3]';
p.var_order = 0:3;
p.num_reads = 7;
p.max_answers = 7;
p.answer_histogram = false;
p.beta = 0.25;
p.random_seed = 10101;

problem = [-1  0.25  0.25  0.25
            0 -1     0.5   0.5
            0  0    -1    -1
            0  0     0    -1];

result = sapilocal_orangSample('qubo', problem, p);
assertEqual(result, struct( ...
  'energies', [-3 -1.75 -1 -3 -1 -1.75 -1.75], ...
  'solutions', [0 1 1 0 1 1 1
                0 0 0 0 0 0 1
                1 1 0 1 0 0 0
                1 0 0 1 0 1 0]))
end

function testBadProblemType
assertExceptionThrown(@() sapilocal_orangSample('blarg', [], params), ...
  'sapilocal:BadProblemType')
end

function testBadProblem
assertExceptionThrown(@() sapilocal_orangSample('ising', 'blah', params), ...
  'sapilocal:BadArgType')
assertExceptionThrown(@() sapilocal_orangSample('ising', [1], params), ...
  'sapilocal:SolveFailed')
end

function assertMissing(name)
p = rmfield(params, name);
assertExceptionThrown(@() sapilocal_orangSample('ising', [], p), ...
  'sapilocal:MissingField')
end

function assertBadArgType(name, value)
p = params(name, value);
assertExceptionThrown(@() sapilocal_orangSample('ising', [], p), ...
  'sapilocal:BadArgType')
end

function testBadParams
assertMissing('num_vars')
assertMissing('active_vars')
assertMissing('active_var_pairs')
assertMissing('var_order')
assertMissing('num_reads')
assertMissing('max_answers')
assertMissing('answer_histogram')
assertMissing('beta')

assertBadArgType('num_vars', 'four')
assertBadArgType('active_vars', {1 2 3})
assertBadArgType('active_var_pairs', zeros(3))
assertBadArgType('var_order', 'backwards')
assertBadArgType('num_reads', 1:10)
assertBadArgType('max_answers', 'all')
assertBadArgType('answer_histogram', 'yes')
assertBadArgType('beta', {1})
assertBadArgType('random_seed', magic(5))
end
