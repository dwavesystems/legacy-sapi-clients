% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testParameterValidation %#ok<STOUT,*DEFNU>
initTestSuite
end



function testNoParameters
solver = struct( ...
    'property', struct(), ...
    'solve', @(a, b, c) 'the answer', ...
    'submit', @(a, b, c) 'submitted thing');

warning('off', 'test:dummy')
warning('off', 'sapi:ParameterValidation')

warning('test:dummy', 'ignore')
sapiSolveIsing(solver, [], []);
[~, wid] = lastwarn;
assertEqual(wid, 'sapi:ParameterValidation')

warning('test:dummy', 'ignore')
sapiSolveQubo(solver, []);
[~, wid] = lastwarn;
assertEqual(wid, 'sapi:ParameterValidation')

warning('test:dummy', 'ignore')
sapiAsyncSolveIsing(solver, [], []);
[~, wid] = lastwarn;
assertEqual(wid, 'sapi:ParameterValidation')

warning('test:dummy', 'ignore')
sapiAsyncSolveQubo(solver, []);
[~, wid] = lastwarn;
assertEqual(wid, 'sapi:ParameterValidation')
end



function testInvalidParameters
solver = struct( ...
    'property', struct('parameters', struct('valid_param', 'yes')), ...
    'solve', @(a, b, c) 'the answer', ...
    'submit', @(a, b, c) 'submitted thing');

assertExceptionThrown(@() sapiSolveIsing(solver, [], [], 'bad', 123), ...
    'sapi:InvalidParameter')
assertExceptionThrown(@() sapiSolveQubo(solver, [], 'bad', 123), ...
    'sapi:InvalidParameter')
assertExceptionThrown(@() sapiAsyncSolveIsing(solver, [], [], 'bad', 123), ...
    'sapi:InvalidParameter')
assertExceptionThrown(@() sapiAsyncSolveQubo(solver, [], 'bad', 123), ...
    'sapi:InvalidParameter')
end



function testValidParameters
solver = struct( ...
    'property', struct('parameters', struct('valid_param', 'yes')), ...
    'solve', @(a, b, c) 'the answer', ...
    'submit', @(a, b, c) 'submitted thing');

assertEqual(sapiSolveIsing(solver, [], [], 'valid_param', 1), 'the answer')
assertEqual(sapiSolveQubo(solver, [], 'valid_param', 1), 'the answer')
assertEqual(sapiAsyncSolveIsing(solver, [], [], 'valid_param', 1), ...
    'submitted thing')
assertEqual(sapiAsyncSolveQubo(solver, [], 'valid_param', 1), ...
    'submitted thing')
end



function testIgnoreXParameters
solver = struct( ...
    'property', struct('parameters', struct('valid_param', 'yes')), ...
    'solve', @(a, b, c) 'the answer', ...
    'submit', @(a, b, c) 'submitted thing');

assertEqual(sapiSolveIsing(solver, [], [], 'valid_param', 1, 'x_blah', 234), ...
    'the answer')
assertEqual(sapiSolveQubo(solver, [], 'x_aaa', 'hello'), 'the answer')
assertEqual(sapiAsyncSolveIsing(solver, [], [], 'x_wooooo', 'something', ...
    'valid_param', 1), 'submitted thing')
assertEqual(sapiAsyncSolveQubo(solver, [], 'x_a', 1, 'x_b', 2, ...
    'x_c', 3, 'x_d', 4', 'x_x_x_x_x_x_x', 999), 'submitted thing')
end
