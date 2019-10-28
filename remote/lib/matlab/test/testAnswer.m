% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testAnswer %#ok<STOUT,*DEFNU>
initTestSuite
end

function testSuccess
solvers = sapiremote_solvers(sapiremote_connection('', '', []));
type = 'type';
data = -4;
params.answer = true;
h = sapiremote_submit(solvers(1), type, data, params);
answer = sapiremote_answer(h);
assertEqual(answer{1}, type)
assertEqual(answer{2}, ...
  struct('type', type, 'data', data, 'params', params))
end

function testBadHandle
assertExceptionThrown(@() sapiremote_answer('nope'), ...
  'sapiremote:InvalidHandle')
end

function testNotDone
assertExceptionThrown(@() sapiremote_answer(answerHandle('incomplete')), ...
  'sapiremote:AsyncNotDone')
end
